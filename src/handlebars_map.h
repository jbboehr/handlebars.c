
#ifndef HANDLEBARS_MAP_H
#define HANDLEBARS_MAP_H

#include "handlebars.h"

#ifdef	__cplusplus
extern "C" {
#endif

struct handlebars_context;
struct handlebars_value;

struct handlebars_map_entry {
    //char * key;
    struct handlebars_string * key;
    struct handlebars_value * value;
    struct handlebars_map_entry * next;
    struct handlebars_map_entry * prev;
};

struct handlebars_map {
    struct handlebars_context * ctx;
    size_t i;
    struct handlebars_map_entry * first;
    struct handlebars_map_entry * last;
};

#define handlebars_map_foreach(list, el, tmp) \
    for( (el) = (list->first); (el) && (tmp = (el)->next, 1); (el) = tmp)

struct handlebars_map * handlebars_map_ctor(struct handlebars_context * ctx) HBSARN;
void handlebars_map_dtor(struct handlebars_map * map);
bool handlebars_map_add(struct handlebars_map * map, const char * key, struct handlebars_value * value);
bool handlebars_map_remove(struct handlebars_map * map, const char * key);
struct handlebars_value * handlebars_map_find(struct handlebars_map * map, const char * key);

static inline bool handlebars_map_update(struct handlebars_map * map, const char * key, struct handlebars_value * value)
{
    handlebars_map_remove(map, key);
    return handlebars_map_add(map, key, value);
}

#ifdef	__cplusplus
}
#endif

#endif
