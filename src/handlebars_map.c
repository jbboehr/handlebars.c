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
#include <string.h>

#define HANDLEBARS_MAP_PRIVATE
#define HANDLEBARS_STRING_PRIVATE

#include "handlebars.h"
#include "handlebars_map.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"
#include "handlebars_string.h"
#include "handlebars_value.h"



struct ht_find_result {
    bool empty_found;
    bool entry_found;
    unsigned long empty_offset;
    unsigned long entry_offset;
    struct handlebars_map_entry * entry;
    int tombstones;
    int collisions;
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



static inline size_t ht_choose_capacity(size_t elements) {
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
        } else if( handlebars_string_eq_ex(table[pos]->key->val, table[pos]->key->len, table[pos]->key->hash, hbs_str_val(key), hbs_str_len(key), hbs_str_hash(key)) ) {
            ret.entry_found = true;
            ret.entry_offset = pos;
            ret.entry = table[pos];
            break;
        } else {
            ret.collisions++;
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

    // Add to linked list
    if( !map->first ) {
        map->first = map->last = entry;
    } else {
        map->last->next = entry;
        entry->prev = map->last;
        map->last = entry;
    }

    // Add to table
    map->table[offset] = entry;
    map->i++;
    map->vec_offset++;
}

static void map_rehash(struct handlebars_map * map)
{
    size_t vec_capacity = map->i >= 8 ? map->i * 2 : 8;
    size_t table_capacity = ht_choose_capacity(vec_capacity);
    struct handlebars_map_entry * entry;
    struct handlebars_map_entry * tmp;
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

    // Reset collisions
    map->collisions = 0;

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
    struct handlebars_map_entry * first = NULL;
    struct handlebars_map_entry * last = NULL;
    for (i = 0; i < map->vec_offset; i++) {
        if (0 != memcmp(&map->vec[i], &HANDLEBARS_MAP_TOMBSTONE_V, sizeof(HANDLEBARS_MAP_TOMBSTONE_V))) {
            struct handlebars_map_entry * entry = &vec[vec_offset];

            *entry = map->vec[i]; // copy entry
            ht_add_entry(HBSCTX(map), table, table_capacity, entry);
            vec_offset++;

            if( !first ) {
                first = last = entry;
            } else {
                last->next = entry;
                entry->prev = last;
                last = entry;
            }
        }
    }

    // Swap with old table
    handlebars_talloc_free(map->table);
    handlebars_talloc_free(map->vec);
    map->first = first;
    map->last = last;
    map->table = table;
    map->table_capacity = table_capacity;
    map->vec_offset = vec_offset;
    map->vec_capacity = vec_capacity;
    map->vec = vec;
    map->vec_tombstones = 0;
}



size_t handlebars_map_size() {
    return sizeof(struct handlebars_map);
}

struct handlebars_map * handlebars_map_ctor(struct handlebars_context * ctx, size_t capacity)
{
    struct handlebars_map * map = handlebars_talloc_zero(ctx, struct handlebars_map);
    HANDLEBARS_MEMCHECK(map, ctx);
    handlebars_context_bind(ctx, HBSCTX(map));

    if (capacity <= 0) {
        capacity = 4;
    }

    map->table_capacity = ht_choose_capacity(capacity);
    map->table = handlebars_talloc_array(ctx, struct handlebars_map_entry *, map->table_capacity);
    HANDLEBARS_MEMCHECK(map->table, ctx);
    map->table = talloc_steal(map, map->table);
    memset(map->table, 0, sizeof(struct handlebars_map_entry *) * map->table_capacity);

    map->vec_capacity = capacity;
    map->vec =  handlebars_talloc_array(ctx, struct handlebars_map_entry, map->vec_capacity);
    HANDLEBARS_MEMCHECK(map->vec, ctx);
    memset(map->vec, 0, sizeof(struct handlebars_map_entry) * map->vec_capacity);

    return map;
}



#undef CONTEXT
#define CONTEXT HBSCTX(map)

void handlebars_map_dtor(struct handlebars_map * map)
{
#ifndef HANDLEBARS_NO_REFCOUNT
    struct handlebars_map_entry * entry;
    struct handlebars_map_entry * tmp;

    handlebars_map_foreach(map, entry, tmp) {
        handlebars_value_delref(entry->value);
    } handlebars_map_foreach_end();
#endif

    handlebars_talloc_free(map);
}

void handlebars_map_add(struct handlebars_map * map, struct handlebars_string * key, struct handlebars_value * value)
{
    // Rehash
    if (map->vec_offset == map->vec_capacity || handlebars_map_load_factor(map) > HANDLEBARS_MAP_MAX_LOAD_FACTOR) {
        map_rehash(map);
    }

    // Add
    struct ht_find_result o = ht_find_entry(HBSCTX(map), map->table, map->table_capacity, key);
    if (o.entry_found || !o.empty_found) {
        // this should never happen
        handlebars_throw(HBSCTX(map), HANDLEBARS_ERROR, "Failed to add to hash table");
    }

    map_add_at_table_offset(map, key, value, o.empty_offset);

    if (o.collisions > 0) {
        map->collisions++;
    }
}

bool handlebars_map_remove(struct handlebars_map * map, struct handlebars_string * key)
{
    // Rehash
    if (handlebars_map_load_factor(map) < HANDLEBARS_MAP_MIN_LOAD_FACTOR) {
        map_rehash(map);
    }

    // Remove
    struct ht_find_result o = ht_find_entry(HBSCTX(map), map->table, map->table_capacity, key);
    struct handlebars_map_entry * entry = o.entry;
    if (!entry) {
        return 0;
    }

    struct handlebars_value * value = entry->value;

    // Remove from linked list
    if( map->first == entry ) {
        map->first = entry->next;
    }
    if( map->last == entry ) {
        map->last = entry->prev;
    }
    if( entry->next ) {
        entry->next->prev = entry->prev;
    }
    if( entry->prev ) {
        entry->prev->next = entry->next;
    }

    // Remove from hash table
    map->table[o.entry_offset] = HANDLEBARS_MAP_TOMBSTONE;

    // Remove from vector
    *entry = HANDLEBARS_MAP_TOMBSTONE_V;
    map->vec_tombstones++;

    // Free
    map->i--;
    handlebars_value_delref(value);
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
    if (map->vec_offset == map->vec_capacity || handlebars_map_load_factor(map) > HANDLEBARS_MAP_MAX_LOAD_FACTOR) {
        map_rehash(map);
    }

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

    if (o.collisions > 0) {
        map->collisions++;
    }
}

extern inline size_t handlebars_map_count(struct handlebars_map * map) {
    return map->i;
}

extern inline short handlebars_map_load_factor(struct handlebars_map * map) {
    return (map->i * 100) / map->table_capacity;
}

struct handlebars_string * handlebars_map_get_key_at_index(struct handlebars_map * map, size_t index)
{
    struct handlebars_map_entry * entry;
    struct handlebars_map_entry * tmp;

    if (index >= map->i) {
        return NULL;
    }

    // If the backing vector is not sparse, we can just get the entry out of it
    if (map->vec_tombstones <= 0 && index < map->vec_offset) {
        return map->vec[index].key;
    }

    // Otherwise we have to iterator over it to get the proper key
    size_t i = 0;
    handlebars_map_foreach(map, entry, tmp) {
        if (i == index) {
            return entry->key;
        }
        i++;
    } handlebars_map_foreach_end();

    return NULL;
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
