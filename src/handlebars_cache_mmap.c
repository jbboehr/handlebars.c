
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "handlebars.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"

#include "handlebars_cache.h"
#include "handlebars_map.h"
#include "handlebars_string.h"
#include "handlebars_value.h"
#include "handlebars_opcode_serializer.h"


static const size_t DEFAULT_SHM_SIZE = 1024 * 1024 * 64;
static const size_t DEFAULT_TABLE_SIZE = 1024 * 1024 * 16;
static const char head[] = "handlebars shared opcode cache";

enum handlebars_cache_mmap_table_entry_state {
    STATE_EMPTY = 0,
    STATE_INIT = 1,
    STATE_READY = 2
};

struct cache_header {
    //! Header
    char head[32];

    //! The size of the memory block
    size_t size;

    //! The version of handlebars this block was initialized with
    int version;

    //! The offset in bytes from the beginning of the block at which the hash table exists
    size_t table_offset;

    //! The size in bytes of the hash table
    size_t table_size;

    //! The number of slots in the hash table
    size_t table_count;

    //! The number of used slots in the hash table
    size_t table_entries;

    //! The offset in bytes from the beginning of the block at which the data segment exists
    size_t data_offset;

    //! The size in bytes of the data segment
    size_t data_size;

    //! The length in bytes of the currently used part of the data segment
    size_t data_length;

    bool in_reset;
    pthread_spinlock_t write_lock;
    long refcount;
};

struct table_entry {
    //! The state of the entry
    int state;

    //! The offset from the beginning of the memory block at which the key for the entry resides
    size_t key_offset;

    //! The offset from the beginning of the memory block at which the data for this entry resides
    size_t data_offset;
};

struct handlebars_cache_mmap {
    //! The mmap'd memory address
    void * addr;

    //! Information about the cache
    struct cache_header * h;
};



#undef CONTEXT
#define CONTEXT HBSCTX(cache)

static size_t append(struct handlebars_cache_mmap * intern, void * source, size_t size)
{
    size_t orig_offset = intern->h->data_offset;
    intern->h->data_offset += size;
    memcpy(intern->addr + orig_offset, source, size);
    return orig_offset;
}

static void cache_dtor(struct handlebars_cache * cache)
{
    // This may be unnecessary with MAP_ANONYMOUS
    struct handlebars_cache_mmap * intern = (struct handlebars_cache_mmap *) cache->internal;
    if( intern->addr ) {
        munmap(intern->addr, intern->h->size ?: DEFAULT_SHM_SIZE);
    }
    // @todo do we need to release the mutexes?
}

static void cache_reset(struct handlebars_cache * cache)
{
    struct handlebars_cache_mmap * intern = (struct handlebars_cache_mmap *) cache->internal;
    struct cache_header * head = intern->h;

    // Lock
    pthread_spin_lock(&head->write_lock);
    head->in_reset = true;

    // Try to wait for refcount to empty
    int counter = 0;
    while( head->refcount > 0 && ++counter <= 100 ) {
        usleep(5000);
    }
    if( head->refcount > 0 ) {
        goto error;
    }

    // Initialize header
    intern->h->version = handlebars_version();
    intern->h->table_offset = sizeof(struct cache_header);
    intern->h->data_offset = intern->h->table_offset + intern->h->table_size;
    intern->h->data_size = intern->h->size - intern->h->data_offset;
    intern->h->table_count = (intern->h->data_size / sizeof(struct table_entry));

    // Zero out the hash table
    memset(intern->addr + intern->h->table_offset, 0, intern->h->table_size);

error:
    // Unlock
    head->in_reset = false;
    pthread_spin_unlock(&head->write_lock);
}

static int cache_gc(struct handlebars_cache * cache)
{
    // @todo
    return 0;
}


static struct handlebars_module * cache_find(struct handlebars_cache * cache, struct handlebars_string * tmpl)
{
    struct handlebars_cache_mmap * intern = (struct handlebars_cache_mmap *) cache->internal;
    struct cache_header * head = intern->h;
    struct handlebars_module * module = NULL;
    time_t now;

    // Currently resetting
    if( head->in_reset ) {
        cache->misses++;
        goto error;
    }

    struct table_entry * table = (intern->addr + head->table_offset);
    struct table_entry * entry = table + (HBS_STR_HASH(tmpl) % head->table_count);

    if( entry->state != STATE_READY ) {
        // Not found, or not ready
        cache->misses++;
        goto error;
    }

    // Compare key
    if( !handlebars_string_eq(tmpl, (struct handlebars_string *) (intern->addr + entry->key_offset)) ) {
        cache->misses++;
        goto error;
    }

    // Got data
    struct handlebars_module * shm_module = intern->addr + entry->data_offset;

    // Check if it's too old or wrong version
    time(&now);
    if( shm_module->version != handlebars_version() || (cache->max_age >= 0 && difftime(now, shm_module->ts) >= cache->max_age) ) {
        cache->misses++;

        pthread_spin_lock(&head->write_lock);
        entry->state = STATE_EMPTY;
        cache->current_entries = intern->h->table_entries--;
        pthread_spin_unlock(&head->write_lock);

        goto error;
    }

    // Copy
    cache->hits++;

    if( (void *) shm_module != shm_module->addr ) {
        handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "Shared memory pointer mismatch: %p != %p", shm_module, shm_module->addr);
//        module = MC(handlebars_talloc_size(cache, shm_module->size));
//        memcpy(module, shm_module, shm_module->size);
//        handlebars_module_patch_pointers(module);
    }

    pthread_spin_lock(&head->write_lock);
    intern->h->refcount++;
    pthread_spin_unlock(&head->write_lock);

    module = shm_module; // danger

error:
    return module;
}

static void cache_add(
    struct handlebars_cache * cache,
    struct handlebars_string * tmpl,
    struct handlebars_module * module
) {
    struct handlebars_cache_mmap * intern = (struct handlebars_cache_mmap *) cache->internal;
    struct cache_header * head = intern->h;

    // Currently resetting
    if( head->in_reset ) {
        goto error;
    }

    struct table_entry * table = (intern->addr + head->table_offset);
    struct table_entry * entry = table + (HBS_STR_HASH(tmpl) % head->table_count);

    if( entry->state != STATE_EMPTY ) {
        // Currently being written by another process or already exists
        goto error;
    }

    // Check if we have available memory
    if( module->size + head->data_length > head->data_size ) {
        // We ran out of memory - reset cache
        cache_reset(cache);
    }

    // Otherwise write the key
    pthread_spin_lock(&head->write_lock);
    entry->state = STATE_INIT;

    // Copy key
    entry->key_offset = append(intern, tmpl, HANDLEBARS_STRING_SIZE(tmpl->len));

    // Copy data
    entry->data_offset = append(intern, module, module->size);

    // Pre-patch pointers
    handlebars_module_patch_pointers(intern->addr + entry->data_offset);

    // Finish
    entry->state = STATE_READY;
    cache->current_entries = intern->h->table_entries++;

    // Unlock
    pthread_spin_unlock(&head->write_lock);

error:
    ;
}

static void cache_release(struct handlebars_cache * cache, struct handlebars_string * tmpl, struct handlebars_module * module)
{
    struct handlebars_cache_mmap * intern = (struct handlebars_cache_mmap *) cache->internal;
    struct cache_header * head = intern->h;

    pthread_spin_lock(&head->write_lock);
    head->refcount--;
    pthread_spin_unlock(&head->write_lock);
}

#undef CONTEXT
#define CONTEXT context

struct handlebars_cache * handlebars_cache_mmap_ctor(
    struct handlebars_context * context
) {
    struct handlebars_cache * cache = MC(handlebars_talloc_zero(context, struct handlebars_cache));
    handlebars_context_bind(context, HBSCTX(cache));

    cache->max_age = -1;
    cache->add = &cache_add;
    cache->find = &cache_find;
    cache->gc = &cache_gc;
    cache->release = &cache_release;

    struct handlebars_cache_mmap * intern = MC(handlebars_talloc_zero(cache, struct handlebars_cache_mmap));
    cache->internal = intern;

    talloc_set_destructor(cache, cache_dtor);

    intern->addr = mmap(NULL, DEFAULT_SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if( intern->addr == MAP_FAILED ) {
        handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "Failed to mmap: %s", strerror(errno));
    }

    intern->h = intern->addr;
    memset(intern->h->head, 0, sizeof(struct cache_header));
    memcpy(intern->h->head, head, sizeof(head));
    intern->h->size = DEFAULT_SHM_SIZE;
    intern->h->table_size = DEFAULT_TABLE_SIZE;

    pthread_spin_init(&intern->h->write_lock, PTHREAD_PROCESS_SHARED);

    cache_reset(cache);

    return cache;
}
