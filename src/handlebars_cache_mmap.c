/**
 * Copyright (C) 2016 John Boehr
 *
 * This file is part of handlebars.c.
 *
 * handlebars.c is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * handlebars.c is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with handlebars.c.  If not, see <http://www.gnu.org/licenses/>.
 */

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

static const char head[] = "handlebars shared opcode cache";
static const size_t PADDING = 1;
static size_t page_size;

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
    struct table_entry ** table;

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

    size_t collisions;

    bool in_reset;

    pthread_spinlock_t write_lock;

    long refcount;
};

struct table_entry {
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

static inline void * append(struct handlebars_cache_mmap * intern, void * source, size_t size)
{
    void * addr = intern->data + intern->data_length;
    if( intern->data_length + size + PADDING >= intern->data_size ) {
        return NULL;
    }
    intern->data_length += size + PADDING;
    memcpy(addr, source, size);
    memset(addr + size, 0, PADDING);
    return addr;
}

static inline void protect(struct handlebars_cache * cache, bool on)
{
    struct handlebars_cache_mmap * intern = (struct handlebars_cache_mmap *) cache->internal;
    int prot = on ? PROT_READ : PROT_READ | PROT_WRITE;
    int rc = mprotect(intern->table, intern->table_size + intern->data_size, prot);
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

static inline bool table_exists(struct handlebars_cache_mmap * intern, struct handlebars_string * string)
{
    size_t offset = HBS_STR_HASH(string) % intern->table_count;
    return NULL != intern->table[offset];
}

static inline struct table_entry * table_find(struct handlebars_cache_mmap * intern, struct handlebars_string * string)
{
    size_t offset = HBS_STR_HASH(string) % intern->table_count;
    return intern->table[offset];
}

static inline void table_set(struct handlebars_cache_mmap * intern, struct table_entry * entry)
{
    size_t offset = HBS_STR_HASH(entry->key) % intern->table_count;
    if( entry == NULL ) {
        intern->table[offset] = NULL;
    } else {
        intern->table[offset] = append(intern, entry, sizeof(struct table_entry));
    }
}

static inline void table_unset(struct handlebars_cache_mmap * intern, struct handlebars_string * string)
{
    size_t offset = HBS_STR_HASH(string) % intern->table_count;
    intern->table[offset] = NULL;
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


static struct handlebars_module * cache_find(struct handlebars_cache * cache, struct handlebars_string * key)
{
    struct handlebars_cache_mmap * intern = (struct handlebars_cache_mmap *) cache->internal;
    struct handlebars_module * module = NULL;
    time_t now;

    // Currently resetting
    if( intern->in_reset ) {
        return NULL;
    }

    // Find entry
    struct table_entry * entry = table_find(intern, key);

    if( !entry ) {
        // Not found, or not ready
        INCR(intern->misses);
        goto error;
    }

    // Compare key
    if( !handlebars_string_eq(key, entry->key) ) {
        INCR(intern->misses);
        //INCR(intern->collisions);
        goto error;
    }

    // Get data
    module = entry->data;

    // Check if it's too old or wrong version
    time(&now);
    if( module->version != handlebars_version() || (cache->max_age >= 0 && difftime(now, module->ts) >= cache->max_age) ) {
        protect(cache, false);
        lock(cache);
        table_unset(intern, key);
        intern->misses++;
        intern->table_entries--;
        unlock(cache);
        protect(cache, true);
        goto error;
    }

    // Check for pointer mismatch
    if( unlikely((void *) module != module->addr) ) {
        handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "Shared memory pointer mismatch: %p != %p", module, module->addr);
    }

    INCR(intern->hits);
    INCR(intern->refcount);

error:
    return module;
}

static void cache_add(
    struct handlebars_cache * cache,
    struct handlebars_string * key,
    struct handlebars_module * module
) {
    struct handlebars_cache_mmap * intern = (struct handlebars_cache_mmap *) cache->internal;
    struct table_entry entry;

    // Currently resetting
    if( intern->in_reset ) {
        return;
    }

    // Lock
    protect(cache, false);
    lock(cache);

    assert(module == module->addr);

    // Collision
    struct table_entry * found = table_find(intern, key);
    if( found ) {
        if( !handlebars_string_eq(found->key, key) ) {
            INCR(intern->collisions);
        }
        goto error;
    }

    // Copy key
    entry.key = append(intern, (void *) key, HBS_STR_SIZE(key->len));

    // Copy data
    entry.data = append(intern, (void *) module, module->size);

    // Check for failure
    if( unlikely(!entry.key || !entry.data) ) {
        cache_reset(cache);
        goto error;
    }

    // Pre-patch pointers
    handlebars_module_patch_pointers(entry.data);

    // Finish;
    table_set(intern, &entry);
    intern->table_entries++;

error:
    // Unlock
    unlock(cache);
    protect(cache, true);
}

static void cache_release(struct handlebars_cache * cache, struct handlebars_string * tmpl, struct handlebars_module * module)
{
    struct handlebars_cache_mmap * intern = (struct handlebars_cache_mmap *) cache->internal;
    DECR(intern->refcount);
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
    stat.collisions = intern->collisions;
    return stat;
}

#undef CONTEXT
#define CONTEXT context

struct handlebars_cache * handlebars_cache_mmap_ctor(
    struct handlebars_context * context,
    size_t size,
    size_t entries
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
    size_t shm_size = align_size(size);
    size_t table_size = align_size(entries * sizeof(struct table_entry *));
    size_t data_size = shm_size - table_size - intern_size;

    if( table_size >= shm_size ) {
        handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "Table size must not be greater than segment size");
    }

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
    intern->table_count = entries; //table_size / sizeof(struct table_entry *);

    intern->table = ((void *) intern) + intern_size;
    intern->data = ((void *) intern) + intern_size + table_size;

    cache_reset(cache);

    int rc = pthread_spin_init(&intern->write_lock, PTHREAD_PROCESS_SHARED);
    if( rc != 0 ) {
        handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "Failed to init spin lock: %s (%d)", strerror(rc), rc);
    }


    return cache;
}
