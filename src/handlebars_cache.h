
#ifndef HANDLEBARS_CACHE_H
#define HANDLEBARS_CACHE_H

#include <time.h>
#include "handlebars.h"

#ifdef	__cplusplus
extern "C" {
#endif

struct handlebars_compiler;
struct handlebars_map;
struct handlebars_program;

/**
 * @brief In-memory opcode cache.
 * @todo Use shared memory
 */
struct handlebars_cache {
    //! Common header
    struct handlebars_context ctx;

    //! The hashtable for entry storage and lookup
    struct handlebars_map * map;

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

struct handlebars_cache_entry {
    //! The size of the entry, including the program
    size_t size;

    //! The last time this entry was accessed
    time_t last_used;

    //! The handlebars context for this program
    struct handlebars_context * context;

    //! The cached program
    struct handlebars_program * program;
};

/**
 * @brief Construct a new cache
 *
 * @param[in] context The handlebars context
 * @return The cache
 */
struct handlebars_cache * handlebars_cache_ctor(
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
 * @brief Garbage collect the cache
 *
 * @param[in] cache The cache
 * @return The number of entries removed
 */
int handlebars_cache_gc(struct handlebars_cache * cache) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Add a program to the cache. Adding the same key twice is an error.
 *
 * @param[in] cache The cache
 * @param[in] tmpl The template. Can be a filename, actual template, or arbitrary string
 * @param[in] program The program
 * @return The cache entry
 */
struct handlebars_cache_entry * handlebars_cache_add(
    struct handlebars_cache * cache,
    struct handlebars_string * tmpl,
    struct handlebars_program * program
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL;

/**
 * @brief Lookup a program from the cache.
 *
 * @param[in] cache The cache
 * @param[in] tmpl The template, Can be a filename, actual template, or arbitrary string
 * @return The cache entry, or NULL
 */
struct handlebars_cache_entry * handlebars_cache_find(
    struct handlebars_cache * cache,
    struct handlebars_string * tmpl
) HBS_ATTR_NONNULL_ALL;

#ifdef	__cplusplus
}
#endif

#endif
