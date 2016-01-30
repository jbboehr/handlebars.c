
#include <string.h>

#include "handlebars_map.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"
#include "handlebars_value.h"

struct handlebars_map * handlebars_map_ctor(void * ctx)
{
    struct handlebars_map * map = handlebars_talloc_zero(ctx, struct handlebars_map);
    if( likely(map != NULL) ) {
        map->ctx = ctx;
    }
    return map;
}

void handlebars_map_dtor(struct handlebars_map * map)
{
    struct handlebars_map_entry * entry;
    struct handlebars_map_entry * tmp;

    handlebars_map_foreach(map, entry, tmp) {
        handlebars_value_delref(entry->value);
    }

    handlebars_talloc_free(map);
}

short handlebars_map_add(struct handlebars_map * map, const char * key, struct handlebars_value * value)
{
    struct handlebars_map_entry * entry = handlebars_talloc_zero(map, struct handlebars_map_entry);

    if( !entry ) {
        return 0;
    }

    entry->key = handlebars_talloc_strdup(entry, key);
    entry->value = value;
    handlebars_value_addref(value);

    if( !map->first ) {
        map->first = map->last = entry;
    } else {
        map->last->next = entry;
        entry->prev = map->last;
        map->last = entry;
    }

    map->i++;

    return 1;
}

short handlebars_map_remove(struct handlebars_map * map, const char * key)
{
    struct handlebars_map_entry * entry;
    struct handlebars_map_entry * tmp;
    struct handlebars_value * value;
    short removed = 0;

    handlebars_map_foreach(map, entry, tmp) {
        if( 0 == strcmp(entry->key, key) ) {
            value = entry->value;
            if( entry == map->first ) {
                map->first = entry->next;
                map->first->prev = NULL;
            }
            if( entry == map->last ) {
                map->last = entry->prev;
                map->last->next = NULL;
            }
            handlebars_value_delref(value);
            handlebars_talloc_free(entry);
            removed++;
        }
    }

    map->i -= removed;

    return removed;
}

struct handlebars_value * handlebars_map_find(struct handlebars_map * map, const char * key)
{
    struct handlebars_map_entry * entry;
    struct handlebars_map_entry * tmp;
    struct handlebars_value * value = NULL;

    handlebars_map_foreach(map, entry, tmp) {
        if( 0 == strcmp(entry->key, key) ) {
            value = entry->value;
            break;
        }
    }

    if( value != NULL ) {
        handlebars_value_addref(value);
    }

    return value;
}
