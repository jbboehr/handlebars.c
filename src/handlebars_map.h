
#ifndef HANDLEBARS_MAP_H
#define HANDLEBARS_MAP_H

#include "handlebars.h"

#ifdef	__cplusplus
extern "C" {
#endif

struct handlebars_context;
struct handlebars_value;

struct handlebars_map_entry {
    struct handlebars_string * key;
    struct handlebars_value * value;
    struct handlebars_map_entry * next;
    struct handlebars_map_entry * prev;

    struct handlebars_map_entry * parent;
    struct handlebars_map_entry * child;
};

struct handlebars_map {
    struct handlebars_context * ctx;
    size_t i;
    struct handlebars_map_entry * first;
    struct handlebars_map_entry * last;
    size_t table_size;
    struct handlebars_map_entry ** table;
    size_t collisions;
};

#define handlebars_map_foreach(list, el, tmp) \
    for( (el) = (list->first); (el) && (tmp = (el)->next, 1); (el) = tmp)

struct handlebars_map * handlebars_map_ctor(struct handlebars_context * ctx) HBS_ATTR_RETURNS_NONNULL;
void handlebars_map_dtor(struct handlebars_map * map);

struct handlebars_value * handlebars_map_find(struct handlebars_map * map, struct handlebars_string * key);
struct handlebars_value * handlebars_map_str_find(struct handlebars_map * map, const char * key, size_t len);

bool handlebars_map_add(struct handlebars_map * map, struct handlebars_string * key, struct handlebars_value * value);
bool handlebars_map_str_add(struct handlebars_map * map, const char * key, size_t len, struct handlebars_value * value);

bool handlebars_map_str_update(struct handlebars_map * map, const char * key, size_t len, struct handlebars_value * value);
bool handlebars_map_update(struct handlebars_map * map, struct handlebars_string * string, struct handlebars_value * value);

bool handlebars_map_remove(struct handlebars_map * map, struct handlebars_string * key);
bool handlebars_map_str_remove(struct handlebars_map * map, const char * key, size_t len);

#ifdef	__cplusplus
}
#endif

#endif
