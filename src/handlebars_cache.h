
/**
 * @file
 * @brief Opcode cache
 */

#ifndef HANDLEBARS_CACHE_H
#define HANDLEBARS_CACHE_H

#include <time.h>
#include "handlebars.h"

#ifdef	__cplusplus
extern "C" {
#endif

struct handlebars_cache;
struct handlebars_compiler;
struct handlebars_map;
struct handlebars_module;
struct MDB_env;

typedef void (*handlebars_cache_add_func)(
    struct handlebars_cache * cache,
    struct handlebars_string * tmpl,
    struct handlebars_module * module
);

typedef struct handlebars_module * (*handlebars_cache_find_func)(
    struct handlebars_cache * cache,
    struct handlebars_string * tmpl
);

typedef int (*handlebars_cache_gc_func)(
    struct handlebars_cache * cache
);

typedef void (*handlebars_cache_release_func)(
        struct handlebars_cache * cache,
        struct handlebars_string * tmpl,
        struct handlebars_module * module
);

typedef struct handlebars_cache_stat (*handlebars_cache_stat_func)(
    struct handlebars_cache * cache
);

struct handlebars_cache_stat {
    const char * name;

    //! The total size of the cache in bytes
    size_t total_size;

    //! The total size of the cache in bytes
    size_t current_size;

    //! The total size of the table in bytes, or zero for dynamic allocation
    size_t total_table_size;

    //! The current size of the cache table in bytes
    size_t current_table_size;

    //! The total size of the data segment in bytes, or zero for dynamic allocation
    size_t total_data_size;

    //! The current size of the data segment in bytes, or zero for dynamic allocation
    size_t current_data_size;

    //! The total number of entries in the cache, or zero for dynamic allocation
    size_t total_entries;

    //! The current number of entries in the cache
    size_t current_entries;

    //! The number of cache hits
    size_t hits;

    //! The number of cache misses
    size_t misses;

    //! The number of cache entries currently being executed
    size_t refcount;

    //! The number of hash table collisions
    size_t collisions;
};

/**
 * @brief In-memory opcode cache.
 */
struct handlebars_cache {
    //! Common header
    struct handlebars_context ctx;

    //! Opaque pointer for implementation use
    void * internal;

    handlebars_cache_add_func add;

    handlebars_cache_find_func find;

    handlebars_cache_gc_func gc;

    handlebars_cache_release_func release;

    handlebars_cache_stat_func stat;

    //! The max amount of time to keep an entry, in seconds, or zero to disable
    double max_age;

    //! The max number of entries to keep, or zero to disable
    size_t max_entries;

    //! The max size of all entries, or zero to disable
    size_t max_size;
};

/**
 * @brief Construct a new simple cache
 * @param[in] context The handlebars context
 * @return The cache
 */
struct handlebars_cache * handlebars_cache_simple_ctor(
    struct handlebars_context * context
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL;

/**
 * @brief Construct a new LMDB cache. The file specified by path does not have
 *        to exist, but must be writeable.
 * @param[in] context The handlebars context
 * @param[in] path The database file
 * @return The cache
 */
struct handlebars_cache * handlebars_cache_lmdb_ctor(
    struct handlebars_context * context,
    const char * path
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL;

/**
 * @brief Construct a new mmap cache
 * @param[in] context The handlebars context
 * @param[in] size The size of the mmap block, in bytes
 * @param[in] entries The fixed number of entries in the hash table
 * @return The cache
 */
struct handlebars_cache * handlebars_cache_mmap_ctor(
    struct handlebars_context * context,
    size_t size,
    size_t entries
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL;

/**
 * @brief Construct a new cache
 * @param[in] context The handlebars context
 * @return The cache
 */
#define handlebars_cache_ctor handlebars_cache_simple_ctor

/**
 * @brief Lookup a program from the cache.
 * @param[in] cache The cache
 * @param[in] key The cache key, Can be a filename, actual template, or arbitrary string
 * @return The cache entry, or NULL
 */
#define handlebars_cache_find(cache, key) (cache->find(cache, key))

/**
 * @brief Add a program to the cache. Adding the same key twice is an error.
 * @param[in] cache The cache
 * @param[in] key The cache key. Can be a filename, actual template, or arbitrary string
 * @param[in] program The program
 * @return The cache entry
 */
#define handlebars_cache_add(cache, key, module) (cache->add(cache, key, module))

/**
 * @brief Garbage collect the cache
 * @param[in] cache The cache
 * @return The number of entries removed
 */
#define handlebars_cache_gc(cache) (cache->gc(cache))

/**
 * @brief Fetch cache statistics
 * @param[in] cache The cache
 * @return The cache statistics
 */
#define handlebars_cache_stat(cache) (cache->stat(cache))

/**
 * @brief Destruct a cache
 * @param[in] cache The cache
 * @return void
 */
#define handlebars_cache_dtor(cache) handlebars_talloc_free(cache)

#ifdef	__cplusplus
}
#endif

#endif
