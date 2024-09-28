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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <limits.h>
#include <math.h>
#include <string.h>
#include <stdint.h>

#ifdef HANDLEBARS_HAVE_VALGRIND
#include <valgrind/memcheck.h>
#define HT_BOUNDARY_SIZE sizeof(void *)
#else
#define HT_BOUNDARY_SIZE 0
#endif

#include "sort_r.h"

#include "handlebars.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"
#include "handlebars_value_private.h"

#include "handlebars_map.h"
#include "handlebars_string.h"
#include "handlebars_value.h"

#ifndef HANDLEBARS_NO_REFCOUNT
#include "handlebars_rc.h"
#endif



struct handlebars_map {
    struct handlebars_context * ctx;
#ifndef HANDLEBARS_NO_REFCOUNT
    struct handlebars_rc rc;
#endif

    uint32_t i;
    uint32_t table_capacity;
    uint32_t vec_offset;
    uint32_t vec_capacity;

    bool is_in_iteration;

    char data[];
};

struct handlebars_map_entry {
    struct handlebars_string * key;
    struct handlebars_value value;
    uint32_t table_offset;
};

struct ht_find_result {
    bool empty_found;
    bool entry_found;
    uint32_t empty_offset;
    uint32_t entry_offset;
    struct handlebars_map_entry * entry;
    int tombstones;
};

struct map_sort_r_arg {
    handlebars_map_kv_compare_r_func compare;
    const void * arg;
};

static short HANDLEBARS_MAP_MIN_LOAD_FACTOR = 10;
static short HANDLEBARS_MAP_MAX_LOAD_FACTOR = 60;
static uint32_t HANDLEBARS_MAP_CAPACITY_TABLE[] = {
    5ul,
    11ul,
    23ul,
    53ul,
    97ul,
    193ul,
    389ul,
    769ul,
    1543ul,
    3079ul,
    6151ul,
    12289ul,
    24593ul,
    49157ul,
    98317ul,
    196613ul,
    393241ul,
    786433ul,
    1572869ul,
    3145739ul,
    6291469ul,
    12582917ul,
    25165843ul,
    50331653ul,
    100663319ul,
    201326611ul,
    402653189ul,
    805306457ul,
    1610612741ul
};
static struct handlebars_map_entry HANDLEBARS_MAP_TOMBSTONE_V = {0};
static struct handlebars_map_entry * HANDLEBARS_MAP_TOMBSTONE = &HANDLEBARS_MAP_TOMBSTONE_V;



HBS_ATTR_PURE
static inline uint8_t map_choose_vec_capacity_log2(size_t capacity) {
    // we actually want this function to stay size_t
#if defined(HAVE___BUILTIN_CLZLL) && SIZEOF_SIZE_T == SIZEOF_UNSIGNED_LONG_LONG
    return (sizeof(unsigned long long) * 8) - __builtin_clzll((unsigned long long) capacity | 3);
#elif defined(HAVE___BUILTIN_CLZL) && SIZEOF_SIZE_T == SIZEOF_UNSIGNED_LONG
    return (sizeof(unsigned long) * 8) - __builtin_clzl((unsigned long) capacity | 3);
#elif defined(HAVE___BUILTIN_CLZ) && SIZEOF_SIZE_T == SIZEOF_UNSIGNED
    return (sizeof(unsigned) * 8) - __builtin_clz((unsigned) capacity | 3);
#else
    size_t i = capacity | 3;
    uint8_t cap_log2 = 0;
    while (i > 0) {
        i >>= 1;
        cap_log2++;
    }
    assert(pow(2.0f, (double) cap_log2) >= (double) capacity);
    return cap_log2;
#endif
}

HBS_ATTR_PURE
static inline size_t ht_choose_table_capacity(size_t elements) {
    size_t target_capacity = elements * 100 / HANDLEBARS_MAP_MAX_LOAD_FACTOR;
    size_t i = 0;
    for (i = 0; i < sizeof(HANDLEBARS_MAP_CAPACITY_TABLE) - 1; i++) {
        if (HANDLEBARS_MAP_CAPACITY_TABLE[i] >= target_capacity) {
            return HANDLEBARS_MAP_CAPACITY_TABLE[i];
        }
    }
    // LCOV_EXCL_START
    fprintf(stderr, "Failed to obtain hash table capacity for minimum elements %zu (target capacity %zu)\n", elements, target_capacity);
    abort();
    // LCOV_EXCL_STOP
}

HBS_ATTR_PURE
static inline struct handlebars_map_entry * map_vec(struct handlebars_map * map)
{
    // Is it worth doing this to save 8 bytes off the map structure?
    return (struct handlebars_map_entry *) (void *) (map->data + HT_BOUNDARY_SIZE);
}

HBS_ATTR_PURE
static inline struct handlebars_map_entry ** map_table(struct handlebars_map * map)
{
    // Is it worth doing this to save 8 bytes off the map structure?
    size_t vec_size = map->vec_capacity * sizeof(struct handlebars_map_entry);
    return (struct handlebars_map_entry **) (void *) (map->data + HT_BOUNDARY_SIZE * 2 + vec_size);
}

static inline struct ht_find_result map_find_entry(
    struct handlebars_map * map,
    struct handlebars_string * key
) {
    struct handlebars_map_entry ** table = map_table(map);
    size_t table_capacity = map->table_capacity;
    size_t start = (uint32_t) hbs_str_hash(key) % (uint32_t) table_capacity;
    size_t i;
    size_t pos;
    struct ht_find_result ret = {0};

    for (i = 0; i < table_capacity; i++) {
        pos = (start + i) % table_capacity;
        if (!table[pos]) {
            ret.empty_found = true;
            ret.empty_offset = pos;
            break;
        } else if (table[pos] == HANDLEBARS_MAP_TOMBSTONE) {
            ret.tombstones++;
            if (!ret.empty_found) {
                ret.empty_offset = true;
                ret.empty_offset = pos;
            }
        } else if( handlebars_string_eq(table[pos]->key, key) ) {
            ret.entry_found = true;
            ret.entry_offset = pos;
            ret.entry = table[pos];
            break;
        }
    }

    return ret;
}

static inline void map_add_at_table_offset(
    struct handlebars_map * map,
    struct handlebars_string * key,
    struct handlebars_value * value,
    size_t offset
) {
    struct handlebars_map_entry * vec = map_vec(map);
    struct handlebars_map_entry ** table = map_table(map);
    struct handlebars_map_entry * entry = &vec[map->vec_offset];

    assert(map->vec_offset < map->vec_capacity);
    assert(table[offset] == NULL || table[offset] == HANDLEBARS_MAP_TOMBSTONE);

#ifndef HANDLEBARS_NO_REFCOUNT
    entry->key = key;
    handlebars_string_addref(entry->key);
#else
    entry->key = handlebars_string_copy_ctor(map->ctx, (const struct handlebars_string *) key);
    HANDLEBARS_MEMCHECK(entry->key, map->ctx);
#endif

    handlebars_value_init(&entry->value);
    handlebars_value_value(&entry->value, value);

    entry->table_offset = offset;

    // Add to table
    table[offset] = entry;
    map->i++;
    map->vec_offset++;
}

static void map_rebuild_references(struct handlebars_map * map)
{
    size_t i;
    struct handlebars_map_entry * vec = map_vec(map);
    struct handlebars_map_entry ** table = map_table(map);

    assert(map->vec_offset == map->i);

    for (i = 0; i < map->vec_offset; i++ ) {
        table[vec[i].table_offset] = &vec[i];
    }
}



// {{{ Reference Counting

#ifndef HANDLEBARS_NO_REFCOUNT
static void map_rc_dtor(struct handlebars_rc * rc)
{
#ifdef HANDLEBARS_ENABLE_DEBUG
    if (getenv("HANDLEBARS_RC_DEBUG")) {
        fprintf(stderr, "MAP DTOR %p\n", hbs_container_of(rc, struct handlebars_map, rc));
    }
#endif
    struct handlebars_map * map = talloc_get_type_abort(hbs_container_of(rc, struct handlebars_map, rc), struct handlebars_map);
    handlebars_map_dtor(map);
}
#endif

#undef handlebars_map_addref
#undef handlebars_map_delref

void handlebars_map_addref(struct handlebars_map * map)
{
#ifndef HANDLEBARS_NO_REFCOUNT
    handlebars_rc_addref(&map->rc);
#endif
}

void handlebars_map_delref(struct handlebars_map * map)
{
#ifndef HANDLEBARS_NO_REFCOUNT
    handlebars_rc_delref(&map->rc, map_rc_dtor);
#endif
}

void handlebars_map_addref_ex(struct handlebars_map * map, const char * expr, const char * loc)
{
#ifndef HANDLEBARS_NO_REFCOUNT
    if (getenv("HANDLEBARS_RC_DEBUG")) { // LCOV_EXCL_START
        size_t rc = handlebars_rc_refcount(&map->rc);
        fprintf(stderr, "MAP ADDREF %p (%zu -> %zu) %s %s\n", map, rc, rc + 1, expr, loc);
    } // LCOV_EXCL_STOP
    handlebars_map_addref(map);
#endif
}

void handlebars_map_delref_ex(struct handlebars_map * map, const char * expr, const char * loc)
{
#ifndef HANDLEBARS_NO_REFCOUNT
    if (getenv("HANDLEBARS_RC_DEBUG")) { // LCOV_EXCL_START
        size_t rc = handlebars_rc_refcount(&map->rc);
        fprintf(stderr, "MAP DELREF %p (%zu -> %zu) %s %s\n", map, rc, rc - 1, expr, loc);
    } // LCOV_EXCL_STOP
    handlebars_map_delref(map);
#endif
}

#ifdef HANDLEBARS_ENABLE_DEBUG
#define handlebars_map_addref(map) handlebars_map_addref_ex(map, #map, HBS_LOC)
#define handlebars_map_delref(map) handlebars_map_delref_ex(map, #map, HBS_LOC)
#endif

// }}} Reference Counting



const size_t HANDLEBARS_MAP_SIZE = sizeof(struct handlebars_map);

#define HT_SIZES(capacity) \
    size_t vec_capacity = capacity; \
    size_t table_capacity = ht_choose_table_capacity(vec_capacity); \
    size_t size = sizeof(struct handlebars_map); \
    size_t vec_size = vec_capacity * sizeof(struct handlebars_map_entry); \
    size_t table_size = table_capacity * sizeof(struct handlebars_map_entry *); \
    size += HT_BOUNDARY_SIZE * 3 + vec_size + table_size

size_t handlebars_map_size_of(size_t capacity) {
    HT_SIZES(capacity);
    return size;
}

struct handlebars_map * handlebars_map_ctor(struct handlebars_context * ctx, size_t capacity)
{
    HT_SIZES(capacity);

    // Allocate map
    struct handlebars_map * map = handlebars_talloc_size(ctx, size);
    HANDLEBARS_MEMCHECK(map, ctx);
    talloc_set_type(map, struct handlebars_map);
    memset(map, 0, sizeof(struct handlebars_map));
    map->ctx = ctx;

    // The layout for the memory is: [map] [boundary] [vec] [boundary] [table] [boundary]
    // bounary size is 0 when compiled without valgrind

    // Allocate vector
    map->vec_capacity = vec_capacity;

    // Allocate table
    map->table_capacity = table_capacity;
    memset(map_table(map), 0, table_size);

#ifdef HANDLEBARS_HAVE_VALGRIND
   VALGRIND_MAKE_MEM_NOACCESS(map->data, HT_BOUNDARY_SIZE);
   VALGRIND_MAKE_MEM_NOACCESS(map->data + HT_BOUNDARY_SIZE + vec_size, HT_BOUNDARY_SIZE);
   VALGRIND_MAKE_MEM_NOACCESS(map->data + HT_BOUNDARY_SIZE + vec_size + HT_BOUNDARY_SIZE + table_size, HT_BOUNDARY_SIZE);
#endif

#ifndef HANDLEBARS_NO_REFCOUNT
    handlebars_rc_init(&map->rc);
#endif

    return map;
}

struct handlebars_map * handlebars_map_copy_ctor(struct handlebars_map * prev_map, size_t new_capacity)
{
    if (new_capacity < prev_map->vec_capacity) {
        new_capacity = prev_map->vec_capacity;
    }

    struct handlebars_map * map = handlebars_map_ctor(prev_map->ctx, new_capacity);

    handlebars_map_foreach(prev_map, index, key, value) {
        map = handlebars_map_add(map, key, value);
    } handlebars_map_foreach_end(prev_map);

    return map;
}



#undef CONTEXT
#define CONTEXT (map->ctx)

void handlebars_map_dtor(struct handlebars_map * map)
{
    handlebars_map_foreach(map, index, key, value) {
        handlebars_string_delref(key);
        handlebars_value_dtor(value);
    } handlebars_map_foreach_end(map);

    handlebars_talloc_free(map);
}

struct handlebars_map * handlebars_map_add(struct handlebars_map * map, struct handlebars_string * key, struct handlebars_value * value)
{
    // Rehash
    map = handlebars_map_rehash(map, false);

    // Add
    struct ht_find_result o = map_find_entry(map, key);
    if (o.entry_found || !o.empty_found) {
        // this should never happen - unless rehash locked due to iteration
        handlebars_throw(map->ctx, HANDLEBARS_ERROR, "Failed to add to hash table");
    }

    map_add_at_table_offset(map, key, value, o.empty_offset);

    return map;
}

struct handlebars_map * handlebars_map_remove(struct handlebars_map * map, struct handlebars_string * key)
{
    // Rehash
    map = handlebars_map_rehash(map, handlebars_map_load_factor(map) < HANDLEBARS_MAP_MIN_LOAD_FACTOR);

    // Remove
    struct ht_find_result o = map_find_entry(map, key);
    struct handlebars_map_entry * entry = o.entry;
    if (!entry) {
        return map;
    }

    handlebars_string_delref(entry->key);
    handlebars_value_null(&entry->value);

    // Remove from hash table
    struct handlebars_map_entry ** table = map_table(map);
    table[o.entry_offset] = HANDLEBARS_MAP_TOMBSTONE;

    // Remove from vector
    *entry = HANDLEBARS_MAP_TOMBSTONE_V;

    // Free
    map->i--;

    return map;
}

struct handlebars_value * handlebars_map_find(struct handlebars_map * map, struct handlebars_string * key)
{
    struct ht_find_result o = map_find_entry(map, key);
    if (o.entry) {
        return &o.entry->value;
    } else {
        return NULL;
    }
}

struct handlebars_map * handlebars_map_update(struct handlebars_map * map, struct handlebars_string * key, struct handlebars_value * value)
{
    // Rehash
    map = handlebars_map_rehash(map, false);

    // Update
    struct ht_find_result o = map_find_entry(map, key);
    struct handlebars_map_entry * entry = o.entry;
    if (entry) {
        handlebars_value_value(&entry->value, value);
        return map;
    }

    if (!o.empty_found) {
        // This should never happen
        handlebars_throw(map->ctx, HANDLEBARS_ERROR, "Failed to update to hash table");
    }

    map_add_at_table_offset(map, key, value, o.empty_offset);

    return map;
}

extern inline size_t handlebars_map_count(struct handlebars_map * map) {
    return map->i;
}

extern inline short handlebars_map_load_factor(struct handlebars_map * map) {
    return (map->i * 100) / map->table_capacity;
}

struct handlebars_string * handlebars_map_get_key_at_index(struct handlebars_map * map, size_t index)
{
    if (index >= map->vec_offset) {
        handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "Out of bounds");
    }

    struct handlebars_map_entry * vec = map_vec(map);
    return vec[index].key;
}

void handlebars_map_get_kv_at_index(struct handlebars_map * map, size_t index, struct handlebars_string ** key, struct handlebars_value ** value)
{
    if (index >= map->vec_offset) {
        handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "Out of bounds");
    }

    struct handlebars_map_entry * vec = map_vec(map);
    *key = vec[index].key;
    *value = &vec[index].value;
}

bool handlebars_map_is_sparse(struct handlebars_map * map)
{
    return map->vec_offset != map->i;
}

struct handlebars_map * handlebars_map_rehash(struct handlebars_map * map, bool force)
{
    if (map->is_in_iteration) { // this go go really wrong
        return map;
    }

#ifndef HANDLEBARS_NO_REFCOUNT
    if (handlebars_rc_refcount(&map->rc) > 1) {
        force = true;
    }
#endif

    if (force || map->vec_offset == map->vec_capacity || handlebars_map_load_factor(map) > HANDLEBARS_MAP_MAX_LOAD_FACTOR) {
        size_t vec_capacity = 1 << map_choose_vec_capacity_log2(map->i + 1);
        struct handlebars_map * prev_map = map;
        map = handlebars_map_copy_ctor(prev_map, vec_capacity);
#ifndef HANDLEBARS_NO_REFCOUNT
        if (handlebars_rc_refcount(&prev_map->rc) >= 1) { // ugh
            handlebars_map_addref(map);
        }
#endif
        handlebars_map_delref(prev_map);
    }

    return map;
}

void handlebars_map_sparse_array_compact(struct handlebars_map * map)
{
    // nothing to do
    if (map->i == map->vec_offset) {
        return;
    }

    uint32_t i = 0;
    uint32_t vec_offset = 0;
    struct handlebars_map_entry * vec = map_vec(map);
    struct handlebars_map_entry ** table = map_table(map);

    // Scan until the first tombstone
    for (; i < map->vec_offset; i++) {
        if (0 == memcmp(&vec[i], &HANDLEBARS_MAP_TOMBSTONE_V, sizeof(HANDLEBARS_MAP_TOMBSTONE_V))) {
            i++;
            vec_offset++;
            break;
        }
    }

    // Now patch everything
    for (; i < map->vec_offset; i++) {
        if (0 != memcmp(&vec[i], &HANDLEBARS_MAP_TOMBSTONE_V, sizeof(HANDLEBARS_MAP_TOMBSTONE_V))) {
            vec[vec_offset] = vec[i];
            table[vec[vec_offset].table_offset] = &vec[vec_offset];
            vec_offset++;
        }
    }

    map->vec_offset = vec_offset;
}

size_t handlebars_map_sparse_array_count(struct handlebars_map * map)
{
    return map->vec_offset;
}

bool handlebars_map_set_is_in_iteration(struct handlebars_map * map, bool is_in_iteration)
{
    bool old = map->is_in_iteration;
    map->is_in_iteration = is_in_iteration;
    return old;
}

static int map_entry_compare(const void * ptr1, const void * ptr2, void * arg)
{
    assert(ptr1 != NULL);
    assert(ptr2 != NULL);
    assert(arg != NULL);

    handlebars_map_kv_compare_func compare = (handlebars_map_kv_compare_func) arg;
    struct handlebars_map_entry * map_entry1 = (struct handlebars_map_entry *) ptr1;
    struct handlebars_map_entry * map_entry2 = (struct handlebars_map_entry *) ptr2;

    assert(map_entry1->key != NULL);
    // assert(map_entry1->value != NULL);
    assert(map_entry2->key != NULL);
    // assert(map_entry2->value != NULL);

    struct handlebars_map_kv_pair kv1 = {map_entry1->key, &map_entry1->value};
    struct handlebars_map_kv_pair kv2 = {map_entry2->key, &map_entry2->value};
    return compare(&kv1, &kv2);
}

struct handlebars_map * handlebars_map_sort(struct handlebars_map * map, handlebars_map_kv_compare_func compare)
{
    map = handlebars_map_rehash(map, handlebars_map_is_sparse(map));

    struct handlebars_map_entry * vec = map_vec(map);

    sort_r(vec, map->i, sizeof(struct handlebars_map_entry), &map_entry_compare, (void *) compare);

    map_rebuild_references(map);

    return map;
}

static int map_entry_compare_r(const void * ptr1, const void * ptr2, void * arg)
{
    assert(ptr1 != NULL);
    assert(ptr2 != NULL);
    assert(arg != NULL);

    struct map_sort_r_arg * sort_r_arg = (struct map_sort_r_arg *) arg;
    struct handlebars_map_entry * map_entry1 = (struct handlebars_map_entry *) ptr1;
    struct handlebars_map_entry * map_entry2 = (struct handlebars_map_entry *) ptr2;

    assert(sort_r_arg->compare != NULL);
    assert(map_entry1->key != NULL);
    // assert(map_entry1->value != NULL);
    assert(map_entry2->key != NULL);
    // assert(map_entry2->value != NULL);

    struct handlebars_map_kv_pair kv1 = {map_entry1->key, &map_entry1->value};
    struct handlebars_map_kv_pair kv2 = {map_entry2->key, &map_entry2->value};
    return sort_r_arg->compare(&kv1, &kv2, sort_r_arg->arg);
}

struct handlebars_map * handlebars_map_sort_r(
    struct handlebars_map * map,
    handlebars_map_kv_compare_r_func compare,
    const void * arg
) {
    map = handlebars_map_rehash(map, handlebars_map_is_sparse(map));

    struct map_sort_r_arg sort_r_arg = {compare, arg};

    struct handlebars_map_entry * vec = map_vec(map);

    sort_r(vec, map->i, sizeof(struct handlebars_map_entry), &map_entry_compare_r, (void *) &sort_r_arg);

    map_rebuild_references(map);

    return map;
}


// compat

struct handlebars_map * handlebars_map_str_remove(struct handlebars_map * map, const char * key, size_t len)
{
    struct handlebars_string * string = handlebars_string_ctor(CONTEXT, key, len);
    handlebars_string_addref(string);
    map = handlebars_map_remove(map, string);
    handlebars_string_delref(string);
    return map;
}

struct handlebars_map * handlebars_map_str_add(struct handlebars_map * map, const char * key, size_t len, struct handlebars_value * value)
{
    struct handlebars_string * string = handlebars_string_ctor(CONTEXT, key, len);
    handlebars_string_addref(string);
    map = handlebars_map_add(map, string, value);
    handlebars_string_delref(string);
    return map;
}

struct handlebars_value * handlebars_map_str_find(struct handlebars_map * map, const char * key, size_t len)
{
    struct handlebars_string * string = handlebars_string_ctor(CONTEXT, key, len);
    handlebars_string_addref(string);
    struct handlebars_value * value = handlebars_map_find(map, string);
    handlebars_string_delref(string);
    return value;
}

struct handlebars_map * handlebars_map_str_update(struct handlebars_map * map, const char * key, size_t len, struct handlebars_value * value)
{
    struct handlebars_string * string = handlebars_string_ctor(CONTEXT, key, len);
    handlebars_string_addref(string);
    map = handlebars_map_update(map, string, value);
    handlebars_string_delref(string);
    return map;
}
