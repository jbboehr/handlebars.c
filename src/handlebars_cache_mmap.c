
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


#undef CONTEXT
#define CONTEXT HBSCTX(cache)

#if defined(__ATOMIC_SEQ_CST) && !defined(INTELLIJ)
#define HAVE_ATOMIC_BUILTINS 1
#endif

static const size_t DEFAULT_SHM_SIZE = 1024 * 1024 * 64;
static const size_t DEFAULT_TABLE_SIZE = 1024 * 1024 * 8;
static const char head[] = "handlebars shared opcode cache";
static const size_t PADDING = 16;
static size_t page_size;

enum handlebars_cache_mmap_table_entry_state {
    STATE_EMPTY = 0,
    STATE_INIT = 1,
    STATE_READY = 2
};

struct handlebars_cache_mmap {
    //! Header
    char head[32];

    //! The size of the memory block
    size_t size;

    //! The page-aligned size of this struct
    size_t intern_size;

    //! The version of handlebars this block was initialized with
    int version;

    //! The pointer to the table segment
    struct table_entry * table;

    //! The size in bytes of the hash table
    size_t table_size;

    //! The number of slots in the hash table
    size_t table_count;

    //! The number of used slots in the hash table
    size_t table_entries;

    //! The pointer to the data segment
    void * data;

    //! The size in bytes of the data segment
    size_t data_size;

    //! The length in bytes of the currently used part of the data segment
    size_t data_length;

    size_t hits;

    size_t misses;

    bool in_reset;

    pthread_spinlock_t write_lock;

    long refcount;
};

struct table_entry {
    //! The state of the entry
    int state;

    //! The offset from the beginning of the memory block at which the key for the entry resides
    struct handlebars_string * key;

    //! The offset from the beginning of the memory block at which the data for this entry resides
    void * data;
};

static inline size_t align_size(size_t size)
{
    size_t pages = (size / page_size);
    if( size % page_size != 0 ) {
        pages++;
    }
    return pages * page_size;
}

static inline struct table_entry * table_find(struct handlebars_cache_mmap * intern, struct handlebars_string * string)
{
    return intern->table + (HBS_STR_HASH(string) % intern->table_count);
}

static inline void * append(struct handlebars_cache_mmap * intern, void * source, size_t size)
{
    void * addr = intern->data + intern->data_length;
    intern->data_length += size + PADDING;
    memcpy(addr, source, size);
    memset(addr + size, 0, PADDING);
    return addr;
}

static inline void protect(struct handlebars_cache * cache, bool on)
{
    struct handlebars_cache_mmap * intern = (struct handlebars_cache_mmap *) cache->internal;
    int prot = on ? PROT_READ : PROT_READ | PROT_WRITE;
    int rc = mprotect(intern->data, intern->data_size, prot);
    if( rc != 0 ) {
        handlebars_throw(HBSCTX(cache), HANDLEBARS_ERROR, "mprotect error: %s (%d)", strerror(rc), rc);
    }
}

static inline void lock(struct handlebars_cache * cache)
{
    struct handlebars_cache_mmap * intern = (struct handlebars_cache_mmap *) cache->internal;
    int rc;

    // Lock
    rc = pthread_spin_lock(&intern->write_lock);
    if( rc != 0 ) {
        handlebars_throw(HBSCTX(cache), HANDLEBARS_ERROR, "pthread lock error: %s (%d)", strerror(rc), rc);
    }
}

static inline void unlock(struct handlebars_cache * cache)
{
    struct handlebars_cache_mmap * intern = (struct handlebars_cache_mmap *) cache->internal;
    int rc;

    // Unlock
    rc = pthread_spin_unlock(&intern->write_lock);
    if( rc != 0 ) {
        handlebars_throw(HBSCTX(cache), HANDLEBARS_ERROR, "pthread unlock error: %s (%d)", strerror(rc), rc);
    }
}

#if HAVE_ATOMIC_BUILTINS
#define INCR(var) __atomic_add_fetch(&var, 1, __ATOMIC_SEQ_CST)
#define DECR(var) __atomic_sub_fetch(&var, 1, __ATOMIC_SEQ_CST)
#else
#define INCR(var) lock(cache); var++; unlock(cache)
#define DECR(var) lock(cache); var--; unlock(cache)
#endif

static void cache_dtor(struct handlebars_cache * cache)
{
    // This may be unnecessary with MAP_ANONYMOUS
    struct handlebars_cache_mmap * intern = (struct handlebars_cache_mmap *) cache->internal;
    if( intern ) {
        munmap(intern, intern->size);
    }
    // @todo do we need to release the mutexes?
}

static void cache_reset(struct handlebars_cache * cache)
{
    struct handlebars_cache_mmap * intern = (struct handlebars_cache_mmap *) cache->internal;

    // Lock
    intern->in_reset = true;

    // Try to wait for refcount to empty
    int counter = 0;
    while( intern->refcount > 0 && ++counter <= 100 ) {
        usleep(5000);
    }
    if( intern->refcount > 0 ) {
        goto error;
    }

    // Initialize header
    intern->table_entries = 0;
    intern->data_length = 0;

    // Zero out the hash table
    memset(intern->table, 0, intern->table_size);

error:
    // Unlock
    intern->in_reset = false;
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

    // Currently resetting
    if( intern->in_reset ) {
        return NULL;
    }

    // Find entry
    struct table_entry * entry = table_find(intern, tmpl);

    if( entry->state != STATE_READY ) {
        // Not found, or not ready
        INCR(intern->misses);
        goto error;
    }

    // Compare key
    if( !handlebars_string_eq(tmpl, entry->key) ) {
        INCR(intern->misses);
        goto error;
    }

    // Get data
    module = entry->data;

    // Check if it's too old or wrong version
    time(&now);
    if( module->version != handlebars_version() || (cache->max_age >= 0 && difftime(now, module->ts) >= cache->max_age) ) {
        lock(cache);
        entry->state = STATE_EMPTY;
        intern->misses++;
        intern->table_entries--;
        unlock(cache);
        goto error;
    }

    // Check for pointer mismatch
    if( ((void *) module) != module->addr ) {
        handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "Shared memory pointer mismatch: %p != %p", module, module->addr);
    }

    INCR(intern->hits);
    INCR(intern->refcount);

error:
    return module;
}

static void cache_add(
    struct handlebars_cache * cache,
    struct handlebars_string * tmpl,
    struct handlebars_module * module
) {
    struct handlebars_cache_mmap * intern = (struct handlebars_cache_mmap *) cache->internal;

    // Currently resetting
    if( intern->in_reset ) {
        return;
    }

    // Lock
    protect(cache, false);
    lock(cache);

    assert(module == module->addr);

    // Check if we have available memory
    if( module->size + intern->data_length + HANDLEBARS_STRING_SIZE(tmpl->len) + PADDING * 2 > intern->data_size ) {
        // We ran out of memory - reset cache
        cache_reset(cache);
    }

    struct table_entry * entry = table_find(intern, tmpl);

    if( entry->state == STATE_EMPTY ) {
        // Otherwise write the key
        entry->state = STATE_INIT;

        // Copy key
        entry->key = append(intern, (void *) tmpl, HANDLEBARS_STRING_SIZE(tmpl->len));

        // Copy data
        entry->data = append(intern, (void *) module, module->size);

        // Pre-patch pointers
        handlebars_module_patch_pointers(entry->data);

        // Finish
        entry->state = STATE_READY;
        intern->table_entries++;
    } else {
        // Currently being written by another process or already exists
    }

    // Unlock
    unlock(cache);
    protect(cache, true);
}

static void cache_release(struct handlebars_cache * cache, struct handlebars_string * tmpl, struct handlebars_module * module)
{
    struct handlebars_cache_mmap * intern = (struct handlebars_cache_mmap *) cache->internal;
    lock(cache);
    intern->refcount--;
    unlock(cache);
}

static struct handlebars_cache_stat cache_stat(struct handlebars_cache * cache)
{
    struct handlebars_cache_mmap * intern = (struct handlebars_cache_mmap *) cache->internal;
    struct handlebars_cache_stat stat = {0};
    stat.name = "mmap";
    stat.total_size = intern->size;
    stat.total_table_size = intern->table_size;
    stat.total_data_size = intern->data_size;
    stat.total_entries = intern->table_count;
    stat.current_entries = intern->table_entries;
    stat.current_table_size = intern->table_entries * sizeof(struct table_entry);
    stat.current_data_size = intern->data_length;
    stat.current_size = stat.current_table_size + stat.current_data_size;
    stat.hits = intern->hits;
    stat.misses = intern->misses;
    stat.refcount = intern->refcount;
    return stat;
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
    cache->stat = &cache_stat;

    talloc_set_destructor(cache, cache_dtor);

    // Get page size
#if defined(_SC_PAGESIZE) && _SC_PAGESIZE != -1
    page_size = (size_t) sysconf(_SC_PAGESIZE);
#elif defined(PAGE_SIZE)
    page_size = PAGE_SIZE;
#else
#error "Unable to query page size"
#endif

    // Calculate sizes
    size_t intern_size = align_size(sizeof(struct handlebars_cache_mmap));
    size_t shm_size = align_size(DEFAULT_SHM_SIZE);
    size_t table_size = align_size(DEFAULT_TABLE_SIZE);
    size_t data_size = shm_size - table_size - intern_size;

    struct handlebars_cache_mmap * intern = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if( intern == MAP_FAILED ) {
        handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "Failed to mmap: %s", strerror(errno));
    }
    cache->internal = intern;

    memset(intern, 0, intern_size);
    memcpy(intern, head, sizeof(head));
    intern->version = handlebars_version();
    intern->size = shm_size;
    intern->intern_size = intern_size;
    intern->table_size = table_size;
    intern->data_size = data_size;
    intern->table_count = table_size / sizeof(struct table_entry);

    intern->table = ((void *) intern) + intern_size;
    intern->data = ((void *) intern) + intern_size + table_size;

    cache_reset(cache);

    int rc = pthread_spin_init(&intern->write_lock, PTHREAD_PROCESS_SHARED);
    if( rc != 0 ) {
        handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "Failed to init spin lock: %s (%d)", strerror(rc), rc);
    }


    return cache;
}
