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



#undef CONTEXT
#define CONTEXT HBSCTX(ctx)

size_t HANDLEBARS_MAP_SIZE = sizeof(struct handlebars_map);


struct handlebars_map * handlebars_map_ctor(struct handlebars_context * ctx)
{
    struct handlebars_map * map = MC(handlebars_talloc_zero(ctx, struct handlebars_map));
    handlebars_context_bind(ctx, HBSCTX(map));
    map->table_size = 32;
    map->table = talloc_steal(map, MC(handlebars_talloc_array(ctx, struct handlebars_map_entry *, map->table_size)));
    memset(map->table, 0, sizeof(struct handlebars_map_entry *) * map->table_size);
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
    }
#endif

    handlebars_talloc_free(map);
}

static inline bool handlebars_map_entry_eq(struct handlebars_map_entry * entry1, struct handlebars_map_entry * entry2)
{
    return handlebars_string_eq(entry1->key, entry2->key);
}

static inline int _ht_add(struct handlebars_map_entry ** table, size_t table_size, struct handlebars_map_entry * entry)
{
    unsigned long index = HBS_STR_HASH(entry->key) % (unsigned long) table_size;
    struct handlebars_map_entry * parent = table[index];
    if( parent ) {
        assert(!handlebars_map_entry_eq(parent, entry));

        // Append to end of list
        while( parent->child ) {
            parent = parent->child;
        }
        parent->child = entry;
        entry->parent = parent;

#if 0
        fprintf(stderr, "Collision %s (%ld) %s (%ld)\n", parent->key->val, index, entry->key->val, index);
#endif
        return 1;
    } else {
        table[index] = entry;
        return 0;
    }
}

static inline void _rehash(struct handlebars_map * map)
{
    size_t table_size = map->table_size * 2;
    struct handlebars_map_entry * entry;
    struct handlebars_map_entry * tmp;
    struct handlebars_map_entry ** table;

#if 0
    fprintf(stderr, "Rehash: entries: %d, collisions: %d, old size: %d, new size: %d\n", map->i, map->collisions, map->table_size, table_size);
#endif

    // Reset collisions
    map->collisions = 0;

    // Create new table
    table = talloc_steal(map, MC(handlebars_talloc_array(CONTEXT, struct handlebars_map_entry *, table_size)));
    memset(table, 0, sizeof(struct handlebars_map_entry *) * table_size);

    // Reimport entries
    handlebars_map_foreach(map, entry, tmp) {
        entry->child = NULL;
        entry->parent = NULL;
        _ht_add(table, table_size, entry);
    }

    // Swap with old table
    handlebars_talloc_free(map->table);
    map->table = table;
    map->table_size = table_size;
}

static inline struct handlebars_map_entry * _entry_find(struct handlebars_map * map, const char * key, size_t length, unsigned long hash)
{
    struct handlebars_map_entry * found = map->table[hash  % map->table_size];
    while( found ) {
        if( handlebars_string_eq_ex(found->key->val, found->key->len, found->key->hash, key, length, hash) ) {
            return found;
        } else {
            found = found->child;
        }
    }
    return NULL;

}

static inline void _entry_add(struct handlebars_map * map, const char * key, size_t len, unsigned long hash, struct handlebars_value * value)
{
    struct handlebars_map_entry * entry = MC(handlebars_talloc_zero(map, struct handlebars_map_entry));
    entry->key = talloc_steal(entry, handlebars_string_ctor_ex(CONTEXT, key, len, hash));
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

    // Add to hash table
    map->collisions += _ht_add(map->table, map->table_size, entry);

    // Rehash
    if( map->i * 1000 / map->table_size > 500 ) { // @todo adjust
        _rehash(map);
    }

    map->i++;
}

static inline void _entry_remove(struct handlebars_map * map, struct handlebars_map_entry * entry)
{
    unsigned long hash = HBS_STR_HASH(entry->key);
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
    if( entry->parent ) {
        entry->parent->child = entry->child;
    }
    if( entry->child ) {
        entry->child->parent = entry->parent;
    }

    // Remove from hash table
    if( entry->child ) {
        entry->child->parent = entry->parent;
    }
    if( entry->parent ) {
        entry->parent->child = entry->child;
    } else {
        // It's the head
        map->table[hash % map->table_size] = entry->child; // possibly null
    }

    // Free
    handlebars_value_delref(value);
    handlebars_talloc_free(entry);
    map->i--;
}













void handlebars_map_add(struct handlebars_map * map, struct handlebars_string * string, struct handlebars_value * value)
{
    _entry_add(map, string->val, string->len, HBS_STR_HASH(string), value);
}

bool handlebars_map_remove(struct handlebars_map * map, struct handlebars_string * key)
{
    struct handlebars_map_entry * entry = _entry_find(map, key->val, key->len, HBS_STR_HASH(key));
    if( entry ) {
        _entry_remove(map, entry);
        return 1;
    }
    return 0;
}

struct handlebars_value * handlebars_map_find(struct handlebars_map * map, struct handlebars_string * key)
{
    struct handlebars_value * value = NULL;
    struct handlebars_map_entry * entry = _entry_find(map, key->val, key->len, HBS_STR_HASH(key));

    if( entry ) {
        value = entry->value;
        handlebars_value_addref(value);
    }

    return value;
}

void handlebars_map_update(struct handlebars_map * map, struct handlebars_string * key, struct handlebars_value * value)
{
    struct handlebars_map_entry * entry = _entry_find(map, key->val, key->len, HBS_STR_HASH(key));
    if( entry ) {
        handlebars_value_delref(entry->value);
        entry->value = value;
        handlebars_value_addref(entry->value);
    } else {
        _entry_add(map, key->val, key->len, HBS_STR_HASH(key), value);
    }
}

size_t handlebars_map_count(struct handlebars_map * map) {
    return map->i;
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
