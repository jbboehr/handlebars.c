
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>

#include "handlebars.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"

#include "handlebars_cache.h"
#include "handlebars_map.h"
#include "handlebars_string.h"
#include "handlebars_value.h"

static inline bool should_gc(struct handlebars_cache * cache)
{
    return (
        (cache->max_size > 0 && cache->current_size > cache->max_size) ||
        (cache->max_entries > 0 && cache->current_entries > cache->max_entries)
    );
}

static inline bool should_gc_entry(struct handlebars_cache * cache, struct handlebars_cache_entry * cache_entry, time_t now)
{
    if( cache->max_age > 0 && difftime(now, cache_entry->last_used) > cache->max_age ) {
        return true;
    }
    return should_gc(cache);
}

static int cache_entry_compare(const void * ptr1, const void * ptr2)
{
    double delta;
    struct handlebars_map_entry * map_entry1 = talloc_get_type(*(struct handlebars_map_entry **) ptr1, struct handlebars_map_entry);
    struct handlebars_map_entry * map_entry2 = talloc_get_type(*(struct handlebars_map_entry **) ptr2, struct handlebars_map_entry);
    assert(map_entry1 != NULL);
    assert(map_entry2 != NULL);

    struct handlebars_cache_entry * entry1 = talloc_get_type(map_entry1->value->v.ptr, struct handlebars_cache_entry);
    struct handlebars_cache_entry * entry2 = talloc_get_type(map_entry2->value->v.ptr, struct handlebars_cache_entry);
    assert(entry1 != NULL);
    assert(entry2 != NULL);

    delta = difftime(entry1->last_used, entry2->last_used);
    return (delta > 0) - (delta < 0);
}


struct handlebars_cache * handlebars_cache_ctor(struct handlebars_context * context)
{
    struct handlebars_cache * cache = MC(handlebars_talloc_zero(context, struct handlebars_cache));
    cache->map = talloc_steal(cache, handlebars_map_ctor(context));
    cache->map->ctx = HBSCTX(cache);
    return cache;
}

void handlebars_cache_dtor(struct handlebars_cache * cache)
{
    handlebars_talloc_free(cache);
}

int handlebars_cache_gc(struct handlebars_cache * cache)
{
    int removed = 0;
    struct handlebars_map_entry * arr[cache->map->i];
    struct handlebars_map_entry * item;
    struct handlebars_map_entry * tmp;
    size_t i = 0;
    time_t now;
    time(&now);

    handlebars_map_foreach(cache->map, item, tmp) {
        arr[i++] = item;
    }
    assert(i == cache->map->i);

    qsort(arr, cache->map->i, sizeof(struct handlebars_map_entry *), &cache_entry_compare);

    for( i = 0; i < cache->map->i; i++ ) {
        struct handlebars_map_entry * map_entry = arr[i];
        struct handlebars_cache_entry * entry = talloc_get_type(map_entry->value->v.ptr, struct handlebars_cache_entry);
        if( should_gc_entry(cache, entry, now) ) {
            size_t oldsize = entry->size;
            // Remove
            handlebars_map_remove(cache->map, map_entry->key);
#ifdef HANDLEBARS_NO_REFCOUNT
            // Delref should handle it if refcounting enabled
            handlebars_value_dtor(map_entry->value);
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

struct handlebars_cache_entry * handlebars_cache_find(struct handlebars_cache * cache, struct handlebars_string * tmpl)
{
    struct handlebars_value * value = handlebars_map_find(cache->map, tmpl);
    struct handlebars_cache_entry * entry = NULL;
    if( value ) {
        entry = (struct handlebars_cache_entry *) value->v.ptr;
        assert(value->type == HANDLEBARS_VALUE_TYPE_PTR);
        assert(talloc_get_type(entry, struct handlebars_cache_entry) != NULL);
        time(&entry->last_used);
        cache->hits++;
    } else {
        cache->misses++;
    }
    return entry;
}

struct handlebars_cache_entry * handlebars_cache_add(struct handlebars_cache * cache, struct handlebars_string * tmpl, struct handlebars_compiler * compiler)
{
    struct handlebars_value * value;
    struct handlebars_cache_entry * entry;
    size_t size = talloc_total_size(compiler); // this might take a while

    // Check if it would exceed the size
    if( should_gc(cache) ) {
        handlebars_cache_gc(cache);
    }

    entry = handlebars_talloc_zero(HBSCTX(cache), struct handlebars_cache_entry);
    entry->compiler = talloc_steal(entry, compiler);
    entry->size = size;
    time(&entry->last_used);

    value = handlebars_value_ctor(HBSCTX(cache));
    value->type = HANDLEBARS_VALUE_TYPE_PTR;
    value->v.ptr = talloc_steal(value, entry);

    handlebars_map_add(cache->map, tmpl, value);

    // Update master
    cache->current_entries++;
    cache->current_size += size;

    return entry;
}
