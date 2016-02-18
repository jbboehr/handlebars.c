
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include "handlebars.h"
#include "handlebars_map.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"
#include "handlebars_value.h"



#undef CONTEXT
#define CONTEXT HBSCTX(ctx)

struct handlebars_map * handlebars_map_ctor(struct handlebars_context * ctx)
{
    struct handlebars_map * map = MC(handlebars_talloc_zero(ctx, struct handlebars_map));
    map->ctx = CONTEXT;
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

bool handlebars_map_add(struct handlebars_map * map, const char * key, struct handlebars_value * value)
{
    struct handlebars_map_entry * entry = MC(handlebars_talloc_zero(map, struct handlebars_map_entry));

    entry->key = MC(handlebars_talloc_strdup(entry, key));
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

bool handlebars_map_remove(struct handlebars_map * map, const char * key)
{
    struct handlebars_map_entry * entry;
    struct handlebars_map_entry * tmp;
    struct handlebars_value * value;
    short removed = 0;

    handlebars_map_foreach(map, entry, tmp) {
        if( 0 != strcmp(entry->key, key) ) {
            continue;
        }

        value = entry->value;
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
        handlebars_value_delref(value);
        handlebars_talloc_free(entry);
        removed++;
    }

    map->i -= removed;

    return (bool) removed;
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
