/**
 * Copyright (c) anno Domini nostri Jesu Christi MMXVI-MMXXIV John Boehr & contributors
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file
 * @brief Opcode cache
 */

#ifndef HANDLEBARS_CACHE_H
#define HANDLEBARS_CACHE_H

#include "handlebars.h"

HBS_EXTERN_C_START

struct handlebars_cache;
struct handlebars_cache_stat;
struct handlebars_compiler;
struct handlebars_map;
struct handlebars_module;
struct handlebars_string;

extern const size_t HANDLEBARS_CACHE_SIZE;

/**
 * @brief Construct a new simple cache
 * @param[in] context The handlebars context
 * @return The cache
 */
struct handlebars_cache * handlebars_cache_simple_ctor(
    struct handlebars_context * context
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

/**
 * @brief Construct a new simple cache without allowing library errors to
 *        longjmp into the caller.
 * @param[in] context The handlebars context
 * @param[out] result The cache on success, or NULL on failure
 * @return #HANDLEBARS_SUCCESS on success, otherwise the error code
 */
enum handlebars_error_type handlebars_cache_simple_ctor_try(
    struct handlebars_context * context,
    struct handlebars_cache ** result
) HBS_ATTR_NONNULL_ALL HBS_ATTR_WARN_UNUSED_RESULT;

#ifdef HANDLEBARS_HAVE_LMDB

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
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

/**
 * @brief Construct a new LMDB cache without allowing library errors to
 *        longjmp into the caller.
 * @param[in] context The handlebars context
 * @param[in] path The database file
 * @param[out] result The cache on success, or NULL on failure
 * @return #HANDLEBARS_SUCCESS on success, otherwise the error code
 */
enum handlebars_error_type handlebars_cache_lmdb_ctor_try(
    struct handlebars_context * context,
    const char * path,
    struct handlebars_cache ** result
) HBS_ATTR_NONNULL_ALL HBS_ATTR_WARN_UNUSED_RESULT;

#endif

#ifdef HANDLEBARS_HAVE_PTHREAD

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
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

/**
 * @brief Construct a new mmap cache without allowing library errors to
 *        longjmp into the caller.
 * @param[in] context The handlebars context
 * @param[in] size The size of the mmap block, in bytes
 * @param[in] entries The fixed number of entries in the hash table
 * @param[out] result The cache on success, or NULL on failure
 * @return #HANDLEBARS_SUCCESS on success, otherwise the error code
 */
enum handlebars_error_type handlebars_cache_mmap_ctor_try(
    struct handlebars_context * context,
    size_t size,
    size_t entries,
    struct handlebars_cache ** result
) HBS_ATTR_NONNULL_ALL HBS_ATTR_WARN_UNUSED_RESULT;

#endif

/**
 * @brief Destruct a cache
 * @param[in] cache The cache
 * @return void
 */
void handlebars_cache_dtor(
    struct handlebars_cache * cache
) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Lookup a program from the cache.
 * @param[in] cache The cache
 * @param[in] key The cache key, Can be a filename, actual template, or arbitrary string
 * @return The cache entry, or NULL. Every non-NULL result must be passed
 *         exactly once to handlebars_cache_release() with the same cache and
 *         key. The result must not be used after it is released.
 */
struct handlebars_module * handlebars_cache_find(
    struct handlebars_cache * cache,
    struct handlebars_string * key
) HBS_ATTR_NONNULL_ALL HBS_ATTR_WARN_UNUSED_RESULT;

/**
 * @brief Lookup a program without allowing library errors to longjmp into the
 *        caller. A cache miss is successful and produces NULL.
 * @param[in] cache The cache
 * @param[in] key The cache key
 * @param[out] result The cache entry on a hit, otherwise NULL. Every non-NULL
 *                    result must be passed exactly once to
 *                    handlebars_cache_release() or
 *                    handlebars_cache_release_try() with the same cache and
 *                    key. The result must not be used after it is released.
 * @return #HANDLEBARS_SUCCESS on a hit or miss, otherwise the error code
 */
enum handlebars_error_type handlebars_cache_find_try(
    struct handlebars_cache * cache,
    struct handlebars_string * key,
    struct handlebars_module ** result
) HBS_ATTR_NONNULL_ALL HBS_ATTR_WARN_UNUSED_RESULT;

/**
 * @brief Add a program to the cache. Adding the same key twice is an error.
 * @param[in] cache The cache
 * @param[in] key The cache key. Can be a filename, actual template, or arbitrary string
 * @param[in] program The program
 * @return The cache entry
 */
void handlebars_cache_add(
    struct handlebars_cache * cache,
    struct handlebars_string * key,
    struct handlebars_module * module
) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Add a program without allowing library errors to longjmp into the
 *        caller.
 * @return #HANDLEBARS_SUCCESS on success, otherwise the error code
 */
enum handlebars_error_type handlebars_cache_add_try(
    struct handlebars_cache * cache,
    struct handlebars_string * key,
    struct handlebars_module * module
) HBS_ATTR_NONNULL_ALL HBS_ATTR_WARN_UNUSED_RESULT;

/**
 * @brief Garbage collect the cache
 * @param[in] cache The cache
 * @return The number of entries removed
 */
int handlebars_cache_gc(
    struct handlebars_cache * cache
) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Garbage collect without allowing library errors to longjmp into the
 *        caller. The output is modified only on success.
 * @param[out] removed The number of entries removed
 * @return #HANDLEBARS_SUCCESS on success, otherwise the error code
 */
enum handlebars_error_type handlebars_cache_gc_try(
    struct handlebars_cache * cache,
    int * removed
) HBS_ATTR_NONNULL_ALL HBS_ATTR_WARN_UNUSED_RESULT;

/**
 * @brief Reset the cache.
 *
 * The mmap backend may leave the cache unchanged while lookup results remain
 * active. This condition is not reported to the caller.
 * @param[in] cache The cache
 * @return void
 */
void handlebars_cache_reset(
    struct handlebars_cache * cache
) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Reset the cache without allowing library errors to longjmp into the
 *        caller. The mmap backend may return #HANDLEBARS_SUCCESS without
 *        clearing the cache while lookup results remain active.
 * @return #HANDLEBARS_SUCCESS when no error was raised, otherwise the error
 *         code
 */
enum handlebars_error_type handlebars_cache_reset_try(
    struct handlebars_cache * cache
) HBS_ATTR_NONNULL_ALL HBS_ATTR_WARN_UNUSED_RESULT;

/**
 * @brief Release a cache entry returned by handlebars_cache_find() or
 *        handlebars_cache_find_try().
 * @param[in] cache The cache used for the lookup
 * @param[in] key The key used for the lookup
 * @param[in] module The non-NULL lookup result
 * @return void
 *
 * Call this exactly once for every cache hit, even when the selected backend
 * implements release as a no-op.
 */
void handlebars_cache_release(
    struct handlebars_cache * cache,
    struct handlebars_string * key,
    struct handlebars_module * module
) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Release a cache entry without allowing library errors to longjmp into
 *        the caller. Call this exactly once for every non-NULL result from
 *        handlebars_cache_find_try(), using the same cache and key.
 * @param[in] cache The cache used for the lookup
 * @param[in] key The key used for the lookup
 * @param[in] module The non-NULL lookup result
 * @return #HANDLEBARS_SUCCESS on success, otherwise the error code
 */
enum handlebars_error_type handlebars_cache_release_try(
    struct handlebars_cache * cache,
    struct handlebars_string * key,
    struct handlebars_module * module
) HBS_ATTR_NONNULL_ALL HBS_ATTR_WARN_UNUSED_RESULT;

struct handlebars_cache_stat handlebars_cache_stat(
    struct handlebars_cache * cache
) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Read cache statistics without allowing library errors to longjmp into
 *        the caller. The output is modified only on success.
 * @param[out] result The cache statistics
 * @return #HANDLEBARS_SUCCESS on success, otherwise the error code
 */
enum handlebars_error_type handlebars_cache_stat_try(
    struct handlebars_cache * cache,
    struct handlebars_cache_stat * result
) HBS_ATTR_NONNULL_ALL HBS_ATTR_WARN_UNUSED_RESULT;

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

HBS_EXTERN_C_END

#endif /* HANDLEBARS_CACHE_H */
