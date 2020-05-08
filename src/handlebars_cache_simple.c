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
#include <time.h>

#define HANDLEBARS_CACHE_PRIVATE
#define HANDLEBARS_MAP_PRIVATE
#define HANDLEBARS_OPCODE_SERIALIZER_PRIVATE

#include "handlebars.h"
#include "handlebars_cache.h"
#include "handlebars_map.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"
#include "handlebars_string.h"
#include "handlebars_value.h"
#include "handlebars_opcode_serializer.h"



struct handlebars_cache_simple {
    struct handlebars_map * map;
    struct handlebars_cache_stat stat;
};

static inline bool should_gc(struct handlebars_cache * cache)
{
    struct handlebars_cache_simple * intern = (struct handlebars_cache_simple *) cache->internal;
    struct handlebars_cache_stat * stat = &intern->stat;
    return (
        (cache->max_size > 0 && stat->current_size > cache->max_size) ||
        (cache->max_entries > 0 && stat->current_entries > cache->max_entries)
    );
}

static inline bool should_gc_entry(struct handlebars_cache * cache, struct handlebars_module * module, time_t now)
{
    if( cache->max_age >= 0 && difftime(now, module->ts) >= cache->max_age ) {
        return true;
    }
    return should_gc(cache);
}

static int cache_compare(const void * ptr1, const void * ptr2)
{
    double delta;

    struct handlebars_map_kv_pair * map_entry1 = (struct handlebars_map_kv_pair *) ptr1;
    struct handlebars_map_kv_pair * map_entry2 = (struct handlebars_map_kv_pair *) ptr2;
    assert(map_entry1 != NULL);
    assert(map_entry2 != NULL);

    struct handlebars_module * entry1 = talloc_get_type(handlebars_value_get_ptr(map_entry1->value), struct handlebars_module);
    struct handlebars_module * entry2 = talloc_get_type(handlebars_value_get_ptr(map_entry2->value), struct handlebars_module);
    assert(entry1 != NULL);
    assert(entry2 != NULL);

    delta = difftime(entry1->ts, entry2->ts);
    return (delta > 0) - (delta < 0);
}

static int cache_compare2(const struct handlebars_map_kv_pair * pair1, const struct handlebars_map_kv_pair * pair2)
{
    double delta;

    assert(pair1 != NULL);
    assert(pair2 != NULL);
    assert(pair1->value != NULL);
    assert(pair2->value != NULL);

    struct handlebars_module * entry1 = talloc_get_type(handlebars_value_get_ptr(pair1->value), struct handlebars_module);
    struct handlebars_module * entry2 = talloc_get_type(handlebars_value_get_ptr(pair2->value), struct handlebars_module);

    assert(entry1 != NULL);
    assert(entry2 != NULL);

    delta = difftime(entry1->ts, entry2->ts);
    return (delta > 0) - (delta < 0);
}

int cache_gc(struct handlebars_cache * cache)
{
    struct handlebars_cache_simple * intern = (struct handlebars_cache_simple *) cache->internal;
    struct handlebars_map * map = intern->map;
    struct handlebars_cache_stat * stat = &intern->stat;
    int removed = 0;
    size_t i = 0;
    time_t now;
    time(&now);

    intern->map = map = handlebars_map_sort(map, cache_compare2);

    // @TODO reimplement this with handlebars_map_foreach when it uses indexes
    for( i = 0; i < handlebars_map_count(map); i++ ) {
        struct handlebars_string * key = handlebars_map_get_key_at_index(map, i);
        struct handlebars_value * value = handlebars_map_find(map, key);
        struct handlebars_module * module = talloc_get_type(handlebars_value_get_ptr(value), struct handlebars_module);
        if( should_gc_entry(cache, module, now) ) {
            size_t oldsize = module->size;
            // Remove
            handlebars_map_remove(map, key);
            i--; // ugh
#ifdef HANDLEBARS_NO_REFCOUNT
            // Delref should handle it if refcounting enabled - maybe?
            //handlebars_value_dtor(map_entry->value);
#endif
            stat->current_entries--;
            stat->current_size -= oldsize;
            removed++;
        } else {
            break;
        }
    }

    return removed;
}

static struct handlebars_module * cache_find(struct handlebars_cache * cache, struct handlebars_string * tmpl)
{
    struct handlebars_cache_simple * intern = (struct handlebars_cache_simple *) cache->internal;
    struct handlebars_map * map = intern->map;
    struct handlebars_value * value = handlebars_map_find(map, tmpl);
    struct handlebars_module * module = NULL;
    if( value ) {
        // module = (struct handlebars_module *) value->v.ptr;
        module = talloc_get_type(handlebars_value_get_ptr(value), struct handlebars_module);
        assert(handlebars_value_get_type(value) == HANDLEBARS_VALUE_TYPE_PTR);
        assert(talloc_get_type(module, struct handlebars_module) != NULL);
        time(&module->ts);
        intern->stat.hits++;
    } else {
        intern->stat.misses++;
    }
    return module;
}

static void cache_add(struct handlebars_cache * cache, struct handlebars_string * tmpl, struct handlebars_module * module)
{
    struct handlebars_cache_simple * intern = (struct handlebars_cache_simple *) cache->internal;
    struct handlebars_map * map = intern->map;
    struct handlebars_cache_stat * stat = &intern->stat;
    struct handlebars_value * value;

    // Check if it would exceed the size
    if( should_gc(cache) ) {
        handlebars_cache_gc(cache);
    }

    time(&module->ts);

    value = handlebars_value_ctor(HBSCTX(cache));
    handlebars_value_ptr(value, talloc_steal(cache, module));

    handlebars_map_add(map, tmpl, value);

    // Update master
    stat->current_entries++;
    stat->current_size += module->size;
}

static void cache_release(struct handlebars_cache * cache, struct handlebars_string * tmpl, struct handlebars_module * module)
{
    ;
}

static struct handlebars_cache_stat cache_stat(struct handlebars_cache * cache)
{
    struct handlebars_cache_simple * intern = (struct handlebars_cache_simple *) cache->internal;
    struct handlebars_cache_stat stat = intern->stat;
    stat.name = "simple";
    stat.total_size = talloc_total_size(cache); // meh
    return stat;
}

static void cache_reset(struct handlebars_cache * cache)
{
    struct handlebars_cache_simple * intern = (struct handlebars_cache_simple *) cache->internal;
    handlebars_talloc_free(intern->map);
    intern->map = handlebars_map_ctor(HBSCTX(cache), 32);

    memset(&intern->stat, 0, sizeof(intern->stat));
}

#undef CONTEXT
#define CONTEXT context

struct handlebars_cache * handlebars_cache_simple_ctor(
    struct handlebars_context * context
) {
    struct handlebars_cache * cache = MC(handlebars_talloc_zero(context, struct handlebars_cache));
    handlebars_context_bind(context, HBSCTX(cache));
    cache->max_age = -1;
    cache->add = &cache_add;
    cache->find = &cache_find;
    cache->gc = &cache_gc;
    cache->release = &cache_release;
    cache->stat = &cache_stat;
    cache->reset = &cache_reset;

    struct handlebars_cache_simple * intern = MC(handlebars_talloc_zero(cache, struct handlebars_cache_simple));
    cache->internal = intern;

    intern->map = handlebars_map_ctor(HBSCTX(cache), 32);

    return cache;
}
