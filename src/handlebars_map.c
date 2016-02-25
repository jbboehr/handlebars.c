
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <string.h>

#include "handlebars.h"
#include "handlebars_map.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"
#include "handlebars_string.h"
#include "handlebars_value.h"



#undef CONTEXT
#define CONTEXT HBSCTX(ctx)

struct handlebars_map * handlebars_map_ctor(struct handlebars_context * ctx)
{
    struct handlebars_map * map = MC(handlebars_talloc_zero(ctx, struct handlebars_map));
    map->ctx = CONTEXT;
    map->table_size = 32;
    map->table = talloc_steal(map, MC(handlebars_talloc_array(ctx, struct handlebars_map_entry *, map->table_size)));
    memset(map->table, 0, sizeof(struct handlebars_map_entry *) * map->table_size);
    return map;
}

#undef CONTEXT
#define CONTEXT HBSCTX(map->ctx)

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
    unsigned long index = entry->key->hash % (unsigned long) table_size;
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
    unsigned long hash = entry->key->hash;
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













bool handlebars_map_add(struct handlebars_map * map, struct handlebars_string * string, struct handlebars_value * value)
{
    _entry_add(map, string->val, string->len, string->hash, value);
    return true;
}

bool handlebars_map_str_add(struct handlebars_map * map, const char * key, size_t len, struct handlebars_value * value)
{
    _entry_add(map, key, len, handlebars_string_hash(key), value);
    return true;
}

bool handlebars_map_remove(struct handlebars_map * map, struct handlebars_string * key)
{

    struct handlebars_map_entry * entry = _entry_find(map, key->val, key->len, key->hash);
    if( entry ) {
        _entry_remove(map, entry);
        return 1;
    }
    return 0;
}

bool handlebars_map_str_remove(struct handlebars_map * map, const char * key, size_t len)
{
    unsigned long hash = handlebars_string_hash(key);
    struct handlebars_map_entry * entry = _entry_find(map, key, len, hash);
    if( entry ) {
        _entry_remove(map, entry);
        return 1;
    }
    return 0;
}


struct handlebars_value * handlebars_map_find(struct handlebars_map * map, struct handlebars_string * key)
{
    struct handlebars_value * value = NULL;
    struct handlebars_map_entry * entry = _entry_find(map, key->val, key->len, key->hash);

    if( entry ) {
        value = entry->value;
        handlebars_value_addref(value);
    }

    return value;
}

struct handlebars_value * handlebars_map_str_find(struct handlebars_map * map, const char * key, size_t len)
{
    struct handlebars_value * value = NULL;
    struct handlebars_map_entry * entry = _entry_find(map, key, len, handlebars_string_hash(key));

    if( entry ) {
        value = entry->value;
        handlebars_value_addref(value);
    }

    return value;
}


bool handlebars_map_update(struct handlebars_map * map, struct handlebars_string * string, struct handlebars_value * value)
{
    struct handlebars_map_entry * entry = _entry_find(map, string->val, string->len, string->hash);
    if( entry ) {
        handlebars_value_delref(entry->value);
        entry->value = value;
        handlebars_value_addref(entry->value);
        return true;
    } else {
        _entry_add(map, string->val, string->len, string->hash, value);
        return true;
    }
}

bool handlebars_map_str_update(struct handlebars_map * map, const char * key, size_t len, struct handlebars_value * value)
{
    struct handlebars_string * string = talloc_steal(map, handlebars_string_ctor(CONTEXT, key, len));
    struct handlebars_map_entry * entry = _entry_find(map, string->val, string->len, string->hash);
    if( entry ) {
        handlebars_value_delref(entry->value);
        entry->value = value;
        handlebars_value_addref(entry->value);
        handlebars_talloc_free(string);
        return true;
    } else {
        _entry_add(map, string->val, string->len, string->hash, value);
        return true;
    }
}
