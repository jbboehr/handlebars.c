
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include "handlebars_context.h"
#include "handlebars_map.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"
#include "handlebars_value.h"

#define __S1(x) #x
#define __S2(x) __S1(x)
#define __MEMCHECK(ptr) \
    do { \
        if( unlikely(ptr == NULL) ) { \
            handlebars_context_throw(CONTEXT, HANDLEBARS_NOMEM, "Out of memory  [" __S2(__FILE__) ":" __S2(__LINE__) "]"); \
        } \
    } while(0)



#define CONTEXT ctx

struct handlebars_map * handlebars_map_ctor(struct handlebars_context * ctx)
{
    struct handlebars_map * map = handlebars_talloc_zero(ctx, struct handlebars_map);
    __MEMCHECK(map);
    map->ctx = ctx;
    return map;
}

#undef CONTEXT
#define CONTEXT map->ctx

void handlebars_map_dtor(struct handlebars_map * map)
{
    struct handlebars_map_entry * entry;
    struct handlebars_map_entry * tmp;

    handlebars_map_foreach(map, entry, tmp) {
        handlebars_value_delref(entry->value);
    }

    handlebars_talloc_free(map);
}

bool handlebars_map_add(struct handlebars_map * map, const char * key, struct handlebars_value * value)
{
    struct handlebars_map_entry * entry = handlebars_talloc_zero(map, struct handlebars_map_entry);
    __MEMCHECK(entry);

    entry->key = handlebars_talloc_strdup(entry, key);
    __MEMCHECK(entry->key);
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
