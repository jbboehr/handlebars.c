
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <errno.h>
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


#define HANDLE_FD_ERROR(fd) if( fd == -1 ) { handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "Failed to open shm: %s", strerror(errno)); }
#define DEFAULT_SHM_SIZE 1024 * 1024 * 100
#define DEFAULT_TABLE_SIZE 1024 * 1024 * 10

const char head[] = "handlebars shared opcode cache";

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
    //! The file descriptor for the memory block - used for flock
    int fd;

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
    struct handlebars_cache_mmap * intern = (struct handlebars_cache_mmap *) cache->internal;
    if( intern->addr ) {
        munmap(intern->addr, intern->h->size ?: DEFAULT_SHM_SIZE);
    }
    if( intern->fd > -1 ) {
        close(intern->fd);
    }
}

static void cache_reset(struct handlebars_cache * cache)
{
    struct handlebars_cache_mmap * intern = (struct handlebars_cache_mmap *) cache->internal;

    // Lock
    flock(intern->fd, LOCK_EX);

    // Initialize header
    intern->h->version = handlebars_version();
    intern->h->table_offset = sizeof(struct cache_header);
    intern->h->data_offset = intern->h->table_offset + intern->h->table_size;
    intern->h->data_size = intern->h->size - intern->h->data_offset;
    intern->h->table_count = (intern->h->data_size / sizeof(struct table_entry));

    // Zero out the hash table
    memset(intern->addr + intern->h->table_offset, 0, intern->h->table_size);

    // Unlock
    flock(intern->fd, LOCK_UN);
}

static int cache_gc(struct handlebars_cache * cache)
{
    // @todo
    return 0;
}


static struct handlebars_module * cache_find(struct handlebars_cache * cache, struct handlebars_string * tmpl)
{
    struct handlebars_cache_mmap * intern = (struct handlebars_cache_mmap *) cache->internal;
    struct handlebars_module * module = NULL;
    time_t now;

    time(&now);

    flock(intern->fd, LOCK_SH);

    struct table_entry * table = (intern->addr + intern->h->table_offset);
    struct table_entry * entry = table + (HBS_STR_HASH(tmpl) % intern->h->table_count);

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
    if( shm_module->version != handlebars_version() || (cache->max_age && difftime(now, shm_module->ts) > cache->max_age) ) {
        cache->misses++;

        // Flush
        flock(intern->fd, LOCK_EX);
        entry->state = STATE_EMPTY;
        cache->current_entries = intern->h->table_entries--;
        msync(intern->addr, sizeof(struct cache_header), MS_SYNC);
        msync(entry, sizeof(struct table_entry), MS_SYNC);

        goto error;
    }

    // Copy
    cache->hits++;
    module = MC(handlebars_talloc_size(cache, shm_module->size));
    memcpy(module, shm_module, shm_module->size);

    // Convert pointers
    handlebars_module_patch_pointers(module);

error:
    flock(intern->fd, LOCK_UN);
    return module;
}

static void cache_add(
        struct handlebars_cache * cache,
        struct handlebars_string * tmpl,
        struct handlebars_module * module
) {
    struct handlebars_cache_mmap * intern = (struct handlebars_cache_mmap *) cache->internal;

    flock(intern->fd, LOCK_SH);

    struct table_entry * table = (intern->addr + intern->h->table_offset);
    struct table_entry * entry = table + (HBS_STR_HASH(tmpl) % intern->h->table_count);

    if( entry->state == STATE_INIT ) {
        // Currently being written by another process
        goto error;
    } else if( entry->state == STATE_READY ) {
        // This key already exists
        goto error;
    }

    // Check if we have available memory
    if( module->size + intern->h->data_length > intern->h->data_size ) {
        // We ran out of memory - reset cache
        cache_reset(cache);
    }

    // Otherwise write the key
    flock(intern->fd, LOCK_EX);
    entry->state = STATE_INIT;

    // Copy key
    entry->key_offset = append(intern, tmpl, HANDLEBARS_STRING_SIZE(tmpl->len));

    // Copy data
    entry->data_offset = append(intern, module, module->size);

    // Finish
    entry->state = STATE_READY;
    cache->current_entries = intern->h->table_entries++;

    // Sync
    msync(intern->addr, sizeof(struct cache_header), MS_SYNC);
    msync(intern->addr + entry->data_offset, module->size, MS_SYNC);

error:
    flock(intern->fd, LOCK_UN);
}

#undef CONTEXT
#define CONTEXT context

struct handlebars_cache * handlebars_cache_mmap_ctor(
    struct handlebars_context * context,
    const char * name
) {
    struct handlebars_cache * cache = MC(handlebars_talloc_zero(context, struct handlebars_cache));
    cache->add = &cache_add;
    cache->find = &cache_find;
    cache->gc = &cache_gc;

    struct handlebars_cache_mmap * intern = MC(handlebars_talloc_zero(cache, struct handlebars_cache_mmap));
    cache->internal = intern;

    talloc_set_destructor(cache, cache_dtor);

    bool create = false;
    errno = 0;
    intern->fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0644);

    if( errno == EEXIST ) {
        // Use existing
        intern->fd = shm_open(name, O_RDWR, 0644);
        HANDLE_FD_ERROR(intern->fd);
    } else {
        // Create
        HANDLE_FD_ERROR(intern->fd);
        ftruncate(intern->fd, DEFAULT_SHM_SIZE);
        create = true;
    }

    intern->addr = mmap(NULL, DEFAULT_SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, intern->fd, 0);
    intern->h = intern->addr;

    if( create ) {
        memset(intern->h->head, 0, sizeof(struct cache_header));
        memcpy(intern->h->head, head, sizeof(head));
        intern->h->size = DEFAULT_SHM_SIZE;
        intern->h->table_size = DEFAULT_TABLE_SIZE;
        cache_reset(cache);
    }

    return cache;
}
