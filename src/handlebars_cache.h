
#ifndef HANDLEBARS_CACHE_H
#define HANDLEBARS_CACHE_H

#include <time.h>
#include "handlebars.h"

#ifdef	__cplusplus
extern "C" {
#endif

struct handlebars_compiler;
struct handlebars_map;


struct handlebars_cache {
    struct handlebars_context ctx;
    struct handlebars_map * map;
    double max_age;
    size_t current_entries;
    size_t max_entries;
    size_t current_size;
    size_t max_size;
    size_t hits;
    size_t misses;
};

struct handlebars_cache_entry {
    size_t size;
    struct handlebars_context * context;
    struct handlebars_compiler * compiler;
    time_t last_used;
};

struct handlebars_cache * handlebars_cache_ctor(struct handlebars_context * context);
void handlebars_cache_dtor(struct handlebars_cache * cache);
int handlebars_cache_gc(struct handlebars_cache * cache);
struct handlebars_cache_entry * handlebars_cache_add(struct handlebars_cache * cache, struct handlebars_string * tmpl, struct handlebars_compiler * compiler);
struct handlebars_cache_entry * handlebars_cache_find(struct handlebars_cache * cache, struct handlebars_string * tmpl);

#ifdef	__cplusplus
}
#endif

#endif
