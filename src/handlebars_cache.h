
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


/**
 * @brief In-memory opcode cache.
 * @todo Use shared memory
 */
struct handlebars_cache {
    //! Common header
    struct handlebars_context ctx;

    void * internal;

    handlebars_cache_add_func add;

    handlebars_cache_find_func find;

    handlebars_cache_gc_func gc;

    handlebars_cache_release_func release;

    //! The max amount of time to keep an entry, in seconds, or zero to disable
    double max_age;

    //! The max number of entries to keep, or zero to disable
    size_t max_entries;

    //! The max size of all entries, or zero to disable
    size_t max_size;

    //! The current number of entries in the cache
    size_t current_entries;

    //! The current size of the cache
    size_t current_size;

    //! The number of cache hits
    size_t hits;

    //! The number of cache misses
    size_t misses;
};

/**
 * @brief Construct a new cache
 *
 * @param[in] context The handlebars context
 * @return The cache
 */
struct handlebars_cache * handlebars_cache_simple_ctor(
    struct handlebars_context * context
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL;

struct handlebars_cache * handlebars_cache_lmdb_ctor(
    struct handlebars_context * context,
    const char * path
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL;

struct handlebars_cache * handlebars_cache_mmap_ctor(
    struct handlebars_context * context
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL;

/**
 * @brief Destruct a cache
 *
 * @param[in] cache The cache
 * @return void
 */
void handlebars_cache_dtor(struct handlebars_cache * cache) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Construct a new cache
 *
 * @param[in] context The handlebars context
 * @return The cache
 */
#define handlebars_cache_ctor handlebars_cache_simple_ctor

/**
 * @brief Lookup a program from the cache.
 *
 * @param[in] cache The cache
 * @param[in] tmpl The template, Can be a filename, actual template, or arbitrary string
 * @return The cache entry, or NULL
 */
#define handlebars_cache_find(cache, key) (cache->find(cache, key))

/**
 * @brief Add a program to the cache. Adding the same key twice is an error.
 *
 * @param[in] cache The cache
 * @param[in] tmpl The template. Can be a filename, actual template, or arbitrary string
 * @param[in] program The program
 * @return The cache entry
 */
#define handlebars_cache_add(cache, key, module) (cache->add(cache, key, module))

/**
 * @brief Garbage collect the cache
 *
 * @param[in] cache The cache
 * @return The number of entries removed
 */
#define handlebars_cache_gc(cache) (cache->gc(cache))

#ifdef	__cplusplus
}
#endif

#endif
