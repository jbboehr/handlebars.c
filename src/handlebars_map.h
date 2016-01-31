
#ifndef HANDLEBARS_MAP_H
#define HANDLEBARS_MAP_H

#include <stddef.h>

struct handlebars_value;

struct handlebars_map_entry {
    char * key;
    struct handlebars_value * value;
    struct handlebars_map_entry * next;
    struct handlebars_map_entry * prev;
};

struct handlebars_map_iterator {
    struct handlebars_map_entry * current;
};

struct handlebars_map {
    void * ctx;
    size_t i;
    struct handlebars_map_entry * first;
    struct handlebars_map_entry * last;
};

#define handlebars_map_foreach(list, el, tmp) \
    for( (el) = (list->first); (el) && (tmp = (el)->next, 1); (el) = tmp)

struct handlebars_map * handlebars_map_ctor(void * ctx);
void handlebars_map_dtor(struct handlebars_map * map);
short handlebars_map_add(struct handlebars_map * map, const char * key, struct handlebars_value * value);
short handlebars_map_remove(struct handlebars_map * map, const char * key);
struct handlebars_value * handlebars_map_find(struct handlebars_map * map, const char * key);

static inline short handlebars_map_update(struct handlebars_map * map, const char * key, struct handlebars_value * value)
{
    handlebars_map_remove(map, key);
    return handlebars_map_add(map, key, value);
}

#endif
