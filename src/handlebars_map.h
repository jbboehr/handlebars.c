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
#include "handlebars_value.h"

#ifndef HANDLEBARS_MAP_H
#define HANDLEBARS_MAP_H

HBS_EXTERN_C_START

struct handlebars_context;
struct handlebars_map;
struct handlebars_map_iterator;
struct handlebars_string;
struct handlebars_value;

struct handlebars_map_kv_pair {
    struct handlebars_string * key;
    struct handlebars_value * value;
};

/**
 * @brief Closeable iterator over a map's stable backing vector.
 *
 * Declare with #HANDLEBARS_MAP_ITERATOR_DECL and initialize with
 * #handlebars_map_iterator_init. Values returned by
 * #handlebars_map_iterator_next are borrowed from the map and remain valid
 * until the iterator is advanced, closed, or the caller mutates that entry.
 * See #handlebars_map_iterator_init for the map-lifetime requirements.
 */
struct handlebars_map_iterator {
    /** Internal iterator state. Callers must not modify this member. */
    struct handlebars_value_iterator iterator;

    /** Internal conditional map-reference ownership. */
    bool retains_map;
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
 *
 * The copy uses the source map's talloc context and shares referenced payloads
 * with the source entries. It is not an independent cross-context deep copy.
 *
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

/**
 * @brief Retain the map backing storage for a value iterator. In refcounted
 *        builds, the caller must already own a reference to the map.
 * @param[in] map
 */
void handlebars_map_iterator_acquire(struct handlebars_map * map)
    HBS_ATTR_NONNULL_ALL;

/**
 * @brief Release map backing storage retained by a value iterator.
 * @param[in] map
 */
void handlebars_map_iterator_release(struct handlebars_map * map)
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

/**
 * @brief Compact the map's sparse backing vector when no iterator is active.
 *
 * If any map or value iterator is active, compaction is deferred and this
 * function leaves the map unchanged.
 *
 * @param[in] map
 */
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

// {{{ Iteration

/**
 * @brief Initialize a zero-initialized map iterator.
 *
 * The iterator keeps the backing vector it traverses stable. It must be
 * advanced to completion or closed with #handlebars_map_iterator_close before
 * it is reinitialized. In refcounted builds it retains a map that already has
 * an owner; raw zero-refcount maps, and all maps in no-refcount builds, must
 * otherwise outlive the iterator.
 *
 * @param[in,out] iterator A zero-initialized iterator
 * @param[in] map The map to iterate
 * @return true if the map contains an entry, otherwise false
 *
 * @code{.c}
 * HANDLEBARS_MAP_ITERATOR_DECL(iterator);
 * struct handlebars_string *key;
 * struct handlebars_value *value;
 *
 * if (handlebars_map_iterator_init(iterator, map)) {
 *     while (handlebars_map_iterator_next(iterator, &key, &value)) {
 *         // use key and value
 *     }
 * }
 * handlebars_map_iterator_close(iterator);
 * @endcode
 */
bool handlebars_map_iterator_init(
    struct handlebars_map_iterator * iterator,
    struct handlebars_map * map
) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Yield the next map entry.
 *
 * Both output pointers are cleared when the iterator is exhausted. Advancing
 * an iterator to exhaustion closes it automatically.
 *
 * @param[in,out] iterator An initialized iterator
 * @param[out] key The borrowed key
 * @param[out] value The borrowed value
 * @return true if an entry was yielded, otherwise false
 */
bool handlebars_map_iterator_next(
    struct handlebars_map_iterator * iterator,
    struct handlebars_string ** key,
    struct handlebars_value ** value
) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Release iterator-owned state.
 *
 * Safe to call more than once on an iterator that was zero initialized or
 * initialized with #handlebars_map_iterator_init.
 *
 * @param[in,out] iterator The iterator to close
 */
void handlebars_map_iterator_close(
    struct handlebars_map_iterator * iterator
) HBS_ATTR_NONNULL_ALL;

/** @internal Scope-cleanup backend for #HANDLEBARS_MAP_ITERATOR_DECL. */
void handlebars_map_iterator_cleanup(
    struct handlebars_map_iterator * const * iterator
);

#if defined(HBS_HAVE_ATTR_CLEANUP)
#define HANDLEBARS_MAP_ITERATOR_DECL_CLEANUP HBS_ATTR_CLEANUP(handlebars_map_iterator_cleanup)
#else
#define HANDLEBARS_MAP_ITERATOR_DECL_CLEANUP
#endif

/**
 * @brief Declare zero-initialized automatic storage for a map iterator.
 *
 * Supported compilers close the iterator automatically on ordinary lexical
 * exits. Library errors unwinding through the map's context also close active
 * iterators. Portable callers should still close manually when leaving an
 * iteration early.
 *
 * @param name A bare C identifier
 */
#define HANDLEBARS_MAP_ITERATOR_DECL(name) \
    struct handlebars_map_iterator mem_ ## name; \
    memset(&mem_ ## name, 0, sizeof(mem_ ## name)); \
    struct handlebars_map_iterator * const name \
        HANDLEBARS_MAP_ITERATOR_DECL_CLEANUP = &mem_ ## name

// }}} Iteration

/**
 * @brief Sort the map's backing vector using a specified compare function. Will rehash
 *        if the backing vector is sparse. In the future, may reallocate the map itself
 *        if the backing vector is sparse. This function WILL ABORT if qsort_r was not
 *        available at compile-time. Calling any map functions on the map being sorted
 *        IS PROBABLY going to EXPLODE. Sorting while any iterator is active raises a
 *        #HANDLEBARS_ERROR instead of invalidating borrowed iterator entries.
 * @param[in] map
 * @param[in] compare
 * @return The sorted map, which may be a replacement allocation.
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

/**
 * @brief Iterate over map entries in sparse-array order.
 *
 * Normal completion, `continue`, and `break` always close the hidden iterator.
 * On compilers with cleanup attributes, `return` and outward `goto` do too;
 * library errors unwinding through the map's context are also handled. Use the
 * explicit map iterator API when portable early lexical exits are required.
 * Removal and value replacement through the iterated map happen in place. A
 * mutation is rejected if a value iterator snapshot of the same map is also
 * active, because the direct and snapshot iteration contracts conflict.
 */
#define handlebars_map_foreach(map, index_var, key, value) \
    do { \
        struct handlebars_map_iterator old_is_in_iteration \
            HBS_ATTR_CLEANUP(handlebars_map_iterator_close); \
        memset(&old_is_in_iteration, 0, sizeof(old_is_in_iteration)); \
        struct handlebars_string * key; \
        struct handlebars_value * value; \
        if( handlebars_map_iterator_init(&old_is_in_iteration, map) ) { \
            while( handlebars_map_iterator_next(&old_is_in_iteration, &key, &value) ) { \
                size_t index_var HBS_ATTR_UNUSED = old_is_in_iteration.iterator.index;

#define handlebars_map_foreach_end(map) \
            } \
        } \
        handlebars_map_iterator_close(&old_is_in_iteration); \
    } while(0)

HBS_EXTERN_C_END

#endif /* HANDLEBARS_MAP_H */
