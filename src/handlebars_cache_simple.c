/**
 * Copyright (c) anno Domini nostri Jesu Christi MMXVI-MMXXIV John Boehr & contributors
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <time.h>

#define HANDLEBARS_OPCODE_SERIALIZER_PRIVATE

#include "handlebars.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"
#include "handlebars_cache.h"
#include "handlebars_cache_private.h"
#include "handlebars_map.h"
#include "handlebars_ptr.h"
#include "handlebars_string.h"
#include "handlebars_value.h"
#include "handlebars_value_handlers.h"
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

static int cache_compare(const struct handlebars_map_kv_pair * pair1, const struct handlebars_map_kv_pair * pair2)
{
    double delta;

    assert(pair1 != NULL);
    assert(pair2 != NULL);
    assert(pair1->value != NULL);
    assert(pair2->value != NULL);

    struct handlebars_module * entry1 = handlebars_value_get_ptr(pair1->value, struct handlebars_module);
    struct handlebars_module * entry2 = handlebars_value_get_ptr(pair2->value, struct handlebars_module);

    assert(entry1 != NULL);
    assert(entry2 != NULL);

    delta = difftime(entry1->ts, entry2->ts);
    return (delta > 0) - (delta < 0);
}

static int cache_gc(struct handlebars_cache * cache)
{
    struct handlebars_cache_simple * intern = (struct handlebars_cache_simple *) cache->internal;
    struct handlebars_map * map = intern->map;
    struct handlebars_cache_stat * stat = &intern->stat;
    int removed = 0;
    size_t i = 0;
    time_t now;
    time(&now);

    size_t remove_keys_i = 0;
    struct handlebars_string ** remove_keys = handlebars_talloc_array(HBSCTX(cache), struct handlebars_string *, handlebars_map_count(map) + 1);
    HANDLEBARS_MEMCHECK(remove_keys, HBSCTX(cache));

    intern->map = map = handlebars_map_sort(map, cache_compare);

    handlebars_map_foreach(map, index, key, value) {
        struct handlebars_module * module = handlebars_value_get_ptr(value, struct handlebars_module);
        if( should_gc_entry(cache, module, now) ) {
            handlebars_string_addref(key);
            remove_keys[remove_keys_i++] = key;
            stat->current_entries--;
            stat->current_size -= module->size;
            removed++;
        } else {
            break;
        }
    } handlebars_map_foreach_end(map);

    for( i = 0; i < remove_keys_i; i++ ) {
        struct handlebars_string * key = remove_keys[i];
        map = handlebars_map_remove(map, key);
        handlebars_string_delref(key);
    }

    map = handlebars_map_rehash(map, false);

    intern->map = map;

    handlebars_talloc_free(remove_keys);

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
        module = handlebars_value_get_ptr(value, struct handlebars_module);
        assert(handlebars_value_get_type(value) == HANDLEBARS_VALUE_TYPE_PTR);
        assert(talloc_get_type_abort(module, struct handlebars_module) != NULL);
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
    struct handlebars_cache_stat * stat = &intern->stat;
    HANDLEBARS_VALUE_DECL(value);

    // Check if it would exceed the size
    if( should_gc(cache) ) {
        handlebars_cache_gc(cache);
    }

    time(&module->ts);

    module = talloc_steal(cache, module);
    struct handlebars_ptr * uptr = handlebars_ptr_ctor(HBSCTX(cache), struct handlebars_module, module, false);
    handlebars_value_ptr(value, uptr);

    intern->map = handlebars_map_add(intern->map, tmpl, value);

    // Update master
    stat->current_entries++;
    stat->current_size += module->size;

    HANDLEBARS_VALUE_UNDECL(value);
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

static const struct handlebars_cache_handlers hbs_cache_handlers_simple = {
    &cache_add,
    &cache_find,
    &cache_gc,
    &cache_release,
    &cache_stat,
    &cache_reset
};

struct handlebars_cache * handlebars_cache_simple_ctor(
    struct handlebars_context * context
) {
    struct handlebars_cache * cache = MC(handlebars_talloc_zero(context, struct handlebars_cache));
    handlebars_context_bind(context, HBSCTX(cache));
    cache->max_age = -1;
    cache->hnd = &hbs_cache_handlers_simple;

    struct handlebars_cache_simple * intern = MC(handlebars_talloc_zero(cache, struct handlebars_cache_simple));
    cache->internal = intern;

    intern->map = handlebars_map_ctor(HBSCTX(cache), 32);

    return cache;
}
