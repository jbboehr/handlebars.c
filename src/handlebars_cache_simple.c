
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <lmdb.h>

#include "handlebars.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"

#include "handlebars_cache.h"
#include "handlebars_map.h"
#include "handlebars_string.h"
#include "handlebars_value.h"
#include "handlebars_opcode_serializer.h"

static inline bool should_gc(struct handlebars_cache * cache)
{
    return (
            (cache->max_size > 0 && cache->current_size > cache->max_size) ||
            (cache->max_entries > 0 && cache->current_entries > cache->max_entries)
    );
}

static inline bool should_gc_entry(struct handlebars_cache * cache, struct handlebars_module * module, time_t now)
{
    if( cache->max_age > 0 && difftime(now, module->ts) > cache->max_age ) {
        return true;
    }
    return should_gc(cache);
}

static int cache_compare(const void * ptr1, const void * ptr2)
{
    double delta;
    struct handlebars_map_entry * map_entry1 = talloc_get_type(*(struct handlebars_map_entry **) ptr1, struct handlebars_map_entry);
    struct handlebars_map_entry * map_entry2 = talloc_get_type(*(struct handlebars_map_entry **) ptr2, struct handlebars_map_entry);
    assert(map_entry1 != NULL);
    assert(map_entry2 != NULL);

    struct handlebars_module * entry1 = talloc_get_type(map_entry1->value->v.ptr, struct handlebars_module);
    struct handlebars_module * entry2 = talloc_get_type(map_entry2->value->v.ptr, struct handlebars_module);
    assert(entry1 != NULL);
    assert(entry2 != NULL);

    delta = difftime(entry1->ts, entry2->ts);
    return (delta > 0) - (delta < 0);
}

int cache_gc(struct handlebars_cache * cache)
{
    int removed = 0;
    struct handlebars_map_entry * arr[cache->u.map->i];
    struct handlebars_map_entry * item;
    struct handlebars_map_entry * tmp;
    size_t i = 0;
    time_t now;
    time(&now);

    handlebars_map_foreach(cache->u.map, item, tmp) {
        arr[i++] = item;
    }
    assert(i == cache->u.map->i);

    qsort(arr, cache->u.map->i, sizeof(struct handlebars_map_entry *), &cache_compare);

    for( i = 0; i < cache->u.map->i; i++ ) {
        struct handlebars_map_entry * map_entry = arr[i];
        struct handlebars_module * module = talloc_get_type(map_entry->value->v.ptr, struct handlebars_module);
        if( should_gc_entry(cache, module, now) ) {
            size_t oldsize = module->size;
            // Remove
            handlebars_map_remove(cache->u.map, map_entry->key);
#ifdef HANDLEBARS_NO_REFCOUNT
            // Delref should handle it if refcounting enabled - maybe?
            //handlebars_value_dtor(map_entry->value);
#endif
            cache->current_entries--;
            cache->current_size -= oldsize;
            removed++;
        } else {
            break;
        }
    }

    return removed;
}

static struct handlebars_module * cache_find(struct handlebars_cache * cache, struct handlebars_string * tmpl)
{
    struct handlebars_value * value = handlebars_map_find(cache->u.map, tmpl);
    struct handlebars_module * module = NULL;
    if( value ) {
        module = (struct handlebars_module *) value->v.ptr;
        assert(value->type == HANDLEBARS_VALUE_TYPE_PTR);
        assert(talloc_get_type(module, struct handlebars_module) != NULL);
        time(&module->ts);
        cache->hits++;
    } else {
        cache->misses++;
    }
    return module;
}

static void cache_add(struct handlebars_cache * cache, struct handlebars_string * tmpl, struct handlebars_module * module)
{
    struct handlebars_value * value;

    // Check if it would exceed the size
    if( should_gc(cache) ) {
        handlebars_cache_gc(cache);
    }

    time(&module->ts);

    value = handlebars_value_ctor(HBSCTX(cache));
    value->type = HANDLEBARS_VALUE_TYPE_PTR;
    value->v.ptr = talloc_steal(cache, module);

    handlebars_map_add(cache->u.map, tmpl, value);

    // Update master
    cache->current_entries++;
    cache->current_size += module->size;
}

struct handlebars_cache * handlebars_cache_simple_ctor(
    struct handlebars_context * context
) {
    struct handlebars_cache * cache = MC(handlebars_talloc_zero(context, struct handlebars_cache));
    cache->u.map = talloc_steal(cache, handlebars_map_ctor(context));
    cache->u.map->ctx = HBSCTX(cache);
    cache->add = &cache_add;
    cache->find = &cache_find;
    cache->gc = &cache_gc;
    return cache;
}
