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

#include "handlebars.h"

#ifndef HANDLEBARS_MAP_H
#define HANDLEBARS_MAP_H

HBS_EXTERN_C_START

struct handlebars_context;
struct handlebars_map;
struct handlebars_string;
struct handlebars_value;

struct handlebars_map_kv_pair {
    struct handlebars_string * key;
    struct handlebars_value * value;
};

typedef int (*handlebars_map_kv_compare_func)(
    const struct handlebars_map_kv_pair *,
    const struct handlebars_map_kv_pair *
);

typedef int (*handlebars_map_kv_compare_r_func)(
    const struct handlebars_map_kv_pair *,
    const struct handlebars_map_kv_pair *,
    const void *
);

extern const size_t HANDLEBARS_MAP_SIZE;

size_t handlebars_map_size_of(size_t capacity)
    HBS_ATTR_PURE;

// {{{ Constructors and Destructors

/**
 * @brief Construct a new map
 * @param[in] ctx The handlebars context
 * @param[in] capacity The desired number of values to be stored
 * @return The new map
 */
struct handlebars_map * handlebars_map_ctor(
    struct handlebars_context * ctx,
    size_t capacity
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

/**
 * @brief Construct a new map by copying the specified map. If capacity is
          less than the specified map's capacity, it will be the
          specified map's capacity.
 * @param[in] map The map to copy
 * @param[in] capacity The desired number of values to be stored
 * @return The new map
 */
struct handlebars_map * handlebars_map_copy_ctor(
    struct handlebars_map * map,
    size_t capacity
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

/**
 * @brief Destruct a map
 * @param[in] map The map to destruct
 * @return void
 */
void handlebars_map_dtor(
    struct handlebars_map * map
) HBS_ATTR_NONNULL_ALL;

// }}} Constructors and Destructors

// {{{ Reference Counting
void handlebars_map_addref(struct handlebars_map * map)
    HBS_ATTR_NONNULL_ALL;
void handlebars_map_delref(struct handlebars_map * map)
    HBS_ATTR_NONNULL_ALL;
void handlebars_map_addref_ex(struct handlebars_map * map, const char * expr, const char * loc)
    HBS_ATTR_NONNULL_ALL;
void handlebars_map_delref_ex(struct handlebars_map * map, const char * expr, const char * loc)
    HBS_ATTR_NONNULL_ALL;

#ifdef HANDLEBARS_ENABLE_DEBUG
#define handlebars_map_addref(map) handlebars_map_addref_ex(map, #map, HBS_LOC)
#define handlebars_map_delref(map) handlebars_map_delref_ex(map, #map, HBS_LOC)
#endif
// }}} Reference Counting

/**
 * @brief Find a value by key (#handlebars_string variant)
 * @param[in] map
 * @param[in] key
 * @return The found value, or NULL
 */
struct handlebars_value * handlebars_map_find(
    struct handlebars_map * map,
    struct handlebars_string * key
) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Find a value by key (const char[] with length variant)
 * @param[in] map
 * @param[in] key
 * @param[in] len
 * @return The found value, or NULL
 */
struct handlebars_value * handlebars_map_str_find(
    struct handlebars_map * map,
    const char * key,
    size_t len
) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Add a value to a map. Adding a key twice is an error, use #handlebars_map_update instead. (#handlebars_string variant)
 * @param[in] map
 * @param[in] key
 * @param[in] value
 * @return The original map, or if reallocated, a new map
 */
struct handlebars_map * handlebars_map_add(
    struct handlebars_map * map,
    struct handlebars_string * key,
    struct handlebars_value * value
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

/**
 * @brief Add a value to a map. Adding a key twice is an error, use #handlebars_map_str_update instead. (const char[] with length variant)
 * @param[in] map
 * @param[in] key
 * @param[in] len
 * @param[in] value
 * @return The original map, or if reallocated, a new map
 */
struct handlebars_map * handlebars_map_str_add(
    struct handlebars_map * map,
    const char * key,
    size_t len,
    struct handlebars_value * value
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

/**
 * @brief Update or add a value (#handlebars_string variant)
 * @param[in] map
 * @param[in] key
 * @param[in] value
 * @return The original map, or if reallocated, a new map
 */
struct handlebars_map * handlebars_map_update(
    struct handlebars_map * map,
    struct handlebars_string * key,
    struct handlebars_value * value
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

/**
 * @brief Update or add a value (const char[] with length variant)
 * @param[in] map
 * @param[in] key
 * @param[in] len
 * @param[in] value
 * @return The original map, or if reallocated, a new map
 */
struct handlebars_map * handlebars_map_str_update(
    struct handlebars_map * map,
    const char * key,
    size_t len,
    struct handlebars_value * value
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

/**
 * @brief Remove a value by key (#handlebars_string variant)
 * @param[in] map
 * @param[in] key
 * @return The original map, or if reallocated, a new map
 */
struct handlebars_map * handlebars_map_remove(
    struct handlebars_map * map,
    struct handlebars_string * key
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

/**
 * @brief Remove a value by key (const char[] with length variant)
 * @param[in] map
 * @param[in] key
 * @param[in] len
 * @return The original map, or if reallocated, a new map
 */
struct handlebars_map * handlebars_map_str_remove(
    struct handlebars_map * map,
    const char * key,
    size_t len
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

/**
 * @brief Returns the number of items in the nmap
 * @param[in] map
 * @return The number of items in the map
 */
size_t handlebars_map_count(
    struct handlebars_map * map
) HBS_ATTR_NONNULL_ALL;

struct handlebars_string * handlebars_map_get_key_at_index(
    struct handlebars_map * map,
    size_t index
) HBS_ATTR_NONNULL_ALL;

void handlebars_map_get_kv_at_index(
    struct handlebars_map * map,
    size_t index,
    struct handlebars_string ** key,
    struct handlebars_value ** value
) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Returns the load factor in percent of the map
 * @param[in] map
 * @return The load factor
 */
short handlebars_map_load_factor(
    struct handlebars_map * map
) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Rehashes the map if the table is not within the configured load factor. In the future,
 *        may reallocate the map itself.
 * @param[in] map
 * @param[in] force
 * @return The rehashed map
 */
struct handlebars_map * handlebars_map_rehash(
    struct handlebars_map * map,
    bool force
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

void handlebars_map_sparse_array_compact(
    struct handlebars_map * map
) HBS_ATTR_NONNULL_ALL;

size_t handlebars_map_sparse_array_count(
    struct handlebars_map * map
) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Checks if the map's backing vector is sparse.
 * @param[in] map
 * @return true if the backing vector is sparse.
 */
bool handlebars_map_is_sparse(
    struct handlebars_map * map
) HBS_ATTR_NONNULL_ALL;

bool handlebars_map_set_is_in_iteration(
    struct handlebars_map * map,
    bool is_in_iteration
) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Sort the map's backing vector using a specified compare function. Will rehash
 *        if the backing vector is sparse. In the future, may reallocate the map itself
 *        if the backing vector is sparse. This function WILL ABORT if qsort_r was not
 *        available at compile-time. Calling any map functions on the map being sorted
 *        IS PROBABLY going to EXPLODE.
 * @param[in] map
 * @param[in] compare
 * @return true if the backing vector is sparse.
 */
struct handlebars_map * handlebars_map_sort(
    struct handlebars_map * map,
    handlebars_map_kv_compare_func compare
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

/**
 * @brief Like #handlebars_map_sort, but with a context pointer.
 * @param[in] map
 * @param[in] compare
 * @param[in] arg
 * @return true if the backing vector is sparse.
 */
struct handlebars_map * handlebars_map_sort_r(
    struct handlebars_map * map,
    handlebars_map_kv_compare_r_func compare,
    const void * arg
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

#define handlebars_map_foreach(map, index, key, value) \
    do { \
        size_t index = 0; \
        struct handlebars_string * key; \
        struct handlebars_value * value; \
        bool old_is_in_iteration = handlebars_map_set_is_in_iteration(map, true); \
        for (; index < handlebars_map_sparse_array_count(map); index++) { \
            handlebars_map_get_kv_at_index(map, index, &key, &value); \
            if (key == NULL || value == NULL) continue;

#define handlebars_map_foreach_end(map) \
        } \
        handlebars_map_set_is_in_iteration(map, old_is_in_iteration); \
    } while(0)

HBS_EXTERN_C_END

#endif /* HANDLEBARS_MAP_H */
