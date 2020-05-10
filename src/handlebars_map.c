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

#define _GNU_SOURCE
#include <stdlib.h>

#include <assert.h>
#include <string.h>
#include <math.h>

#define HANDLEBARS_STRING_PRIVATE

#include "handlebars.h"
#include "handlebars_map.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"
#include "handlebars_string.h"
#include "handlebars_value.h"



struct handlebars_map {
    struct handlebars_context ctx;
    size_t i;

    size_t table_capacity;
    struct handlebars_map_entry ** table;

    size_t vec_offset;
    size_t vec_capacity;
    struct handlebars_map_entry * vec;

    bool is_in_iteration;

#ifndef NDEBUG
    size_t collisions;
#endif
};

struct handlebars_map_entry {
    struct handlebars_string * key;
    struct handlebars_value * value;
    size_t table_offset;
};

struct ht_find_result {
    bool empty_found;
    bool entry_found;
    unsigned long empty_offset;
    unsigned long entry_offset;
    struct handlebars_map_entry * entry;
    int tombstones;

#ifndef NDEBUG
    int collisions;
#endif
};

struct map_sort_r_arg {
    handlebars_map_kv_compare_r_func compare;
    const void * arg;
};

static short HANDLEBARS_MAP_MIN_LOAD_FACTOR = 10;
static short HANDLEBARS_MAP_MAX_LOAD_FACTOR = 60;
static size_t HANDLEBARS_MAP_CAPACITY_TABLE[] = {
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
    50331653ul
};
static struct handlebars_map_entry HANDLEBARS_MAP_TOMBSTONE_V = {0};
static struct handlebars_map_entry * HANDLEBARS_MAP_TOMBSTONE = &HANDLEBARS_MAP_TOMBSTONE_V;



static inline uint8_t ht_choose_vec_capacity_log2(size_t capacity) {
#if defined(HAVE___BUILTIN_CLZLL)
    return (sizeof(unsigned long long) * 8) - __builtin_clzll((unsigned long long) capacity | 3);
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

static inline size_t ht_choose_table_capacity(size_t elements) {
    size_t target_capacity = elements * 100 / HANDLEBARS_MAP_MAX_LOAD_FACTOR;
    size_t i = 0;
    for (i = 0; i < sizeof(HANDLEBARS_MAP_CAPACITY_TABLE) - 1; i++) {
        if (HANDLEBARS_MAP_CAPACITY_TABLE[i] >= target_capacity) {
            return HANDLEBARS_MAP_CAPACITY_TABLE[i];
        }
    }
    fprintf(stderr, "Failed to obtain hash table capacity for minimum elements %lu (target capacity %lu)\n", elements, target_capacity);
    abort();
}

static inline struct ht_find_result ht_find_entry(
    struct handlebars_context * ctx,
    struct handlebars_map_entry ** table,
    size_t table_capacity,
    struct handlebars_string * key
) {
    unsigned long start = HBS_STR_HASH(key) % (unsigned long) table_capacity;
    unsigned long i;
    unsigned long pos;
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
        } else {
#ifndef NDEBUG
            ret.collisions++;
#endif
        }
    }

    return ret;
}

static inline int ht_add_entry(
    struct handlebars_context * ctx,
    struct handlebars_map_entry ** table,
    size_t table_capacity,
    struct handlebars_map_entry * entry
) {
    struct ht_find_result o = ht_find_entry(ctx, table, table_capacity, entry->key);
    if (!o.empty_found) {
        // We should never get here (unless we implemented max_lookahead and forgot)
        handlebars_throw(ctx, HANDLEBARS_ERROR, "Failure to add hash element");
    } else if (o.entry_found) {
        // We should really never get here
        handlebars_throw(ctx, HANDLEBARS_ERROR, "Failure to add hash element");
    }
    table[o.empty_offset] = entry;
    entry->table_offset = o.empty_offset;
    return o.tombstones;
}

static inline void map_add_at_table_offset(
    struct handlebars_map * map,
    struct handlebars_string * key,
    struct handlebars_value * value,
    unsigned long offset
) {
    assert(map->vec_offset < map->vec_capacity);
    assert(map->table[offset] == NULL || map->table[offset] == HANDLEBARS_MAP_TOMBSTONE);

    struct handlebars_map_entry * entry = &map->vec[map->vec_offset];
    entry->key = talloc_steal(map, handlebars_string_copy_ctor(HBSCTX(map), (const struct handlebars_string *) key));
    HANDLEBARS_MEMCHECK(entry->key, HBSCTX(map));
    entry->value = value;
    handlebars_value_addref(value);
    entry->table_offset = offset;

    // Add to table
    map->table[offset] = entry;
    map->i++;
    map->vec_offset++;
}

static void map_rebuild_references(struct handlebars_map * map)
{
    size_t i;

    assert(map->vec_offset == map->i);

    for (i = 0; i < map->vec_offset; i++ ) {
        map->table[map->vec[i].table_offset] = &map->vec[i];
    }
}

static void map_rehash(struct handlebars_map * map)
{
    size_t vec_capacity = 1 << ht_choose_vec_capacity_log2(map->i + 1);
    size_t table_capacity = ht_choose_table_capacity(vec_capacity);
    struct handlebars_map_entry ** table;
    struct handlebars_map_entry * vec;

#if 0
    fprintf(stderr, "REHASH: "
        "entries: %lu, "
        "collisions: %lu, "
        "load factor: %d, "
        "table size (old/new): %lu / %lu, "
        "capacity (old/new): %lu / %lu\n",
        map->i,
        map->collisions,
        handlebars_map_load_factor(map),
        map->table_capacity,
        table_capacity,
        map->vec_capacity,
        vec_capacity
    );
#endif

#ifndef NDEBUG
    // Reset collisions
    map->collisions = 0;
#endif

    // Allocate new entries
    vec = handlebars_talloc_array(HBSCTX(map), struct handlebars_map_entry, vec_capacity);
    HANDLEBARS_MEMCHECK(vec, HBSCTX(map));
    vec = talloc_steal(map, vec);
    memset(vec, 0, sizeof(struct handlebars_map_entry) * vec_capacity);

    // Create new table
    table = handlebars_talloc_array(HBSCTX(map), struct handlebars_map_entry *, table_capacity);
    HANDLEBARS_MEMCHECK(table, HBSCTX(map));
    table = talloc_steal(map, table);
    memset(table, 0, sizeof(struct handlebars_map_entry *) * table_capacity);

    // Reimport entries
    size_t i;
    size_t vec_offset = 0;
    for (i = 0; i < map->vec_offset; i++) {
        if (0 != memcmp(&map->vec[i], &HANDLEBARS_MAP_TOMBSTONE_V, sizeof(HANDLEBARS_MAP_TOMBSTONE_V))) {
            struct handlebars_map_entry * entry = &vec[vec_offset];

            entry->key = map->vec[i].key;
            entry->value = map->vec[i].value;
            ht_add_entry(HBSCTX(map), table, table_capacity, entry);
            vec_offset++;
        }
    }

    // Swap with old table
    handlebars_talloc_free(map->table);
    handlebars_talloc_free(map->vec);
    map->table = table;
    map->table_capacity = table_capacity;
    map->vec_offset = vec_offset;
    map->vec_capacity = vec_capacity;
    map->vec = vec;
}



size_t handlebars_map_size() {
    return sizeof(struct handlebars_map);
}

struct handlebars_map * handlebars_map_ctor(struct handlebars_context * ctx, size_t capacity)
{
    // Allocate map
    struct handlebars_map * map = handlebars_talloc_zero(ctx, struct handlebars_map);
    HANDLEBARS_MEMCHECK(map, ctx);
    handlebars_context_bind(ctx, HBSCTX(map));

    // Allocate vector
    size_t vec_capacity = capacity;
    map->vec_capacity = vec_capacity;
    map->vec = handlebars_talloc_array(ctx, struct handlebars_map_entry, vec_capacity);
    HANDLEBARS_MEMCHECK(map->vec, ctx);
    memset(map->vec, 0, sizeof(struct handlebars_map_entry) * vec_capacity);

    // Allocate table
    map->table_capacity = ht_choose_table_capacity(vec_capacity);
    map->table = handlebars_talloc_array(ctx, struct handlebars_map_entry *, map->table_capacity);
    HANDLEBARS_MEMCHECK(map->table, ctx);
    map->table = talloc_steal(map, map->table);
    memset(map->table, 0, sizeof(struct handlebars_map_entry *) * map->table_capacity);

    return map;
}



#undef CONTEXT
#define CONTEXT HBSCTX(map)

void handlebars_map_dtor(struct handlebars_map * map)
{
    handlebars_map_foreach(map, index, key, value) {
        handlebars_value_delref(value);
    } handlebars_map_foreach_end();

    handlebars_talloc_free(map);
}

void handlebars_map_add(struct handlebars_map * map, struct handlebars_string * key, struct handlebars_value * value)
{
    // Rehash
    map = handlebars_map_rehash(map, false);

    // Add
    struct ht_find_result o = ht_find_entry(HBSCTX(map), map->table, map->table_capacity, key);
    if (o.entry_found || !o.empty_found) {
        // this should never happen - unless rehash locked due to iteration
        handlebars_throw(HBSCTX(map), HANDLEBARS_ERROR, "Failed to add to hash table");
    }

    map_add_at_table_offset(map, key, value, o.empty_offset);

#ifndef NDEBUG
    if (o.collisions > 0) {
        map->collisions++;
    }
#endif
}

bool handlebars_map_remove(struct handlebars_map * map, struct handlebars_string * key)
{
    // Rehash
    map = handlebars_map_rehash(map, handlebars_map_load_factor(map) < HANDLEBARS_MAP_MIN_LOAD_FACTOR);

    // Remove
    struct ht_find_result o = ht_find_entry(HBSCTX(map), map->table, map->table_capacity, key);
    struct handlebars_map_entry * entry = o.entry;
    if (!entry) {
        return 0;
    }

    struct handlebars_value * value = entry->value;

    // Remove from hash table
    map->table[o.entry_offset] = HANDLEBARS_MAP_TOMBSTONE;

    // Remove from vector
    *entry = HANDLEBARS_MAP_TOMBSTONE_V;

    // Free
    map->i--;
    handlebars_value_delref(value);

    return 1;
}

struct handlebars_value * handlebars_map_find(struct handlebars_map * map, struct handlebars_string * key)
{
    struct ht_find_result o = ht_find_entry(CONTEXT, map->table, map->table_capacity, key);
    if (o.entry) {
        return o.entry->value;
    } else {
        return NULL;
    }
}

void handlebars_map_update(struct handlebars_map * map, struct handlebars_string * key, struct handlebars_value * value)
{
    // Rehash
    map = handlebars_map_rehash(map, false);

    // Update
    struct ht_find_result o = ht_find_entry(CONTEXT, map->table, map->table_capacity, key);
    struct handlebars_map_entry * entry = o.entry;
    if (entry) {
        handlebars_value_delref(entry->value);
        entry->value = value;
        handlebars_value_addref(entry->value);
        return;
    }

    if (!o.empty_found) {
        // This should never happen
        handlebars_throw(HBSCTX(map), HANDLEBARS_ERROR, "Failed to update to hash table");
    }

    map_add_at_table_offset(map, key, value, o.empty_offset);

#ifndef NDEBUG
    if (o.collisions > 0) {
        map->collisions++;
    }
#endif
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

    return map->vec[index].key;
}

void handlebars_map_get_kv_at_index(struct handlebars_map * map, size_t index, struct handlebars_string ** key, struct handlebars_value ** value)
{
    if (index >= map->vec_offset) {
        handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "Out of bounds");
    }

    *key = map->vec[index].key;
    *value = map->vec[index].value;
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

    short load_factor = handlebars_map_load_factor(map);

    if (force || map->vec_offset == map->vec_capacity || load_factor > HANDLEBARS_MAP_MAX_LOAD_FACTOR) {
        map_rehash(map);
    }

    return map;
}

void handlebars_map_sparse_array_compact(struct handlebars_map * map)
{
    // nothing to do
    if (map->i == map->vec_offset) {
        return;
    }

    size_t i;
    size_t vec_offset = 0;

    // Scan until the first tombstone
    for (i = 0; i < map->vec_offset; i++) {
        if (0 == memcmp(&map->vec[i], &HANDLEBARS_MAP_TOMBSTONE_V, sizeof(HANDLEBARS_MAP_TOMBSTONE_V))) {
            i++;
            break;
        }
    }

    // Now patch everything
    for (; i < map->vec_offset; i++) {
        if (0 != memcmp(&map->vec[i], &HANDLEBARS_MAP_TOMBSTONE_V, sizeof(HANDLEBARS_MAP_TOMBSTONE_V))) {
            map->vec[vec_offset] = map->vec[i];
            map->table[map->vec[vec_offset].table_offset] = &map->vec[vec_offset];
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
    assert(map_entry1->value != NULL);
    assert(map_entry2->key != NULL);
    assert(map_entry2->value != NULL);

    struct handlebars_map_kv_pair kv1 = {map_entry1->key, map_entry1->value};
    struct handlebars_map_kv_pair kv2 = {map_entry2->key, map_entry2->value};
    return compare(&kv1, &kv2);
}

struct handlebars_map * handlebars_map_sort(struct handlebars_map * map, handlebars_map_kv_compare_func compare)
{
#ifndef HAVE_QSORT_R
    handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "qsort_r not available");
#else
    if (handlebars_map_is_sparse(map)) {
        map = handlebars_map_rehash(map, true);
    }

    qsort_r(map->vec, map->i, sizeof(struct handlebars_map_entry), &map_entry_compare, (void *) compare);

    map_rebuild_references(map);

    return map;
#endif
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
    assert(map_entry1->value != NULL);
    assert(map_entry2->key != NULL);
    assert(map_entry2->value != NULL);

    struct handlebars_map_kv_pair kv1 = {map_entry1->key, map_entry1->value};
    struct handlebars_map_kv_pair kv2 = {map_entry2->key, map_entry2->value};
    return sort_r_arg->compare(&kv1, &kv2, sort_r_arg->arg);
}

struct handlebars_map * handlebars_map_sort_r(
    struct handlebars_map * map,
    handlebars_map_kv_compare_r_func compare,
    const void * arg
) {
    // @TODO fix this for cmake
#ifndef HAVE_QSORT_R
    handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "qsort_r not available");
#else
    if (handlebars_map_is_sparse(map)) {
        map = handlebars_map_rehash(map, true);
    }

    struct map_sort_r_arg sort_r_arg = {compare, arg};

    qsort_r(map->vec, map->i, sizeof(struct handlebars_map_entry), &map_entry_compare_r, (void *) &sort_r_arg);

    map_rebuild_references(map);

    return map;
#endif
}


// compat

bool handlebars_map_str_remove(struct handlebars_map * map, const char * key, size_t len)
{
    struct handlebars_string * string = handlebars_string_ctor(CONTEXT, key, len);
    bool removed = handlebars_map_remove(map, string);
    handlebars_talloc_free(string);
    return removed;
}

void handlebars_map_str_add(struct handlebars_map * map, const char * key, size_t len, struct handlebars_value * value)
{
    struct handlebars_string * string = handlebars_string_ctor(CONTEXT, key, len);
    handlebars_map_add(map, string, value);
    handlebars_talloc_free(string);
}

struct handlebars_value * handlebars_map_str_find(struct handlebars_map * map, const char * key, size_t len)
{
    struct handlebars_string * string = handlebars_string_ctor(CONTEXT, key, len);
    struct handlebars_value * value = handlebars_map_find(map, string);
    handlebars_talloc_free(string);
    return value;
}

void handlebars_map_str_update(struct handlebars_map * map, const char * key, size_t len, struct handlebars_value * value)
{
    struct handlebars_string * string = handlebars_string_ctor(CONTEXT, key, len);
    handlebars_map_update(map, string, value);
    handlebars_talloc_free(string);
}
