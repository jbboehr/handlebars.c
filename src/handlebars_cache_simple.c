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

static inline bool exceeds_limits(
    struct handlebars_cache * cache,
    size_t reserve_size,
    size_t reserve_entries
)
{
    struct handlebars_cache_simple * intern = (struct handlebars_cache_simple *) cache->internal;
    struct handlebars_cache_stat * stat = &intern->stat;
    return (
        (cache->max_size > 0
            && (reserve_size > cache->max_size || stat->current_size > cache->max_size - reserve_size)) ||
        (cache->max_entries > 0
            && (reserve_entries > cache->max_entries || stat->current_entries > cache->max_entries - reserve_entries))
    );
}

static inline bool should_gc_entry(
    struct handlebars_cache * cache,
    struct handlebars_module * module,
    time_t now,
    size_t reserve_size,
    size_t reserve_entries
)
{
    if( cache->max_age >= 0 && difftime(now, module->ts) >= cache->max_age ) {
        return true;
    }
    return exceeds_limits(cache, reserve_size, reserve_entries);
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

static int cache_gc_for(struct handlebars_cache * cache, size_t reserve_size, size_t reserve_entries)
{
    struct handlebars_cache_simple * intern = (struct handlebars_cache_simple *) cache->internal;
    struct handlebars_map * map = intern->map;
    struct handlebars_cache_stat * stat = &intern->stat;
    int removed = 0;
    time_t now;
    time(&now);

    intern->map = map = handlebars_map_sort(map, cache_compare);

    while( handlebars_map_count(map) > 0 ) {
        struct handlebars_string * key;
#ifdef HANDLEBARS_NO_REFCOUNT
        struct handlebars_string * stable_key;
#endif
        struct handlebars_value * value;
        size_t module_size;

        handlebars_map_get_kv_at_index(map, 0, &key, &value);
        struct handlebars_module * module = handlebars_value_get_ptr(value, struct handlebars_module);
        if( !should_gc_entry(cache, module, now, reserve_size, reserve_entries) ) {
            break;
        }

        module_size = module->size;
#ifdef HANDLEBARS_NO_REFCOUNT
        /* Do not rely on an entry-owned key remaining usable if removal
         * replaces the backing map while reference counting is disabled. */
        stable_key = handlebars_string_copy_ctor(HBSCTX(cache), key);
        key = stable_key;
#endif
        map = handlebars_map_remove(map, key);
        intern->map = map;
#ifdef HANDLEBARS_NO_REFCOUNT
        handlebars_talloc_free(stable_key);
        /* Value destruction is intentionally a no-op in this build, so the
         * cache must release the module and its staged key/pointer wrapper. */
        handlebars_talloc_free(module);
#endif

        /* Keep the oldest remaining entry at index zero without allocating.
         * If a later remove needs to rehash and fails, intern->map and the
         * accounting still describe the entries already committed here. */
        handlebars_map_sparse_array_compact(map);

        assert(stat->current_entries > 0);
        assert(stat->current_size >= module_size);
        stat->current_entries--;
        stat->current_size -= module_size;
        removed++;
    }

    return removed;
}

static int cache_gc(struct handlebars_cache * cache)
{
    return cache_gc_for(cache, 0, 0);
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
    struct handlebars_string * key;
    struct handlebars_ptr * uptr;
    HANDLEBARS_VALUE_DECL(value);

    /* Reject duplicates before changing ownership or evicting another entry. */
    if( unlikely(handlebars_map_find(intern->map, tmpl)) ) {
        handlebars_throw(HBSCTX(cache), HANDLEBARS_ERROR, "Duplicate cache key");
    }

    /* A module larger than the entire cache can never be admitted. Leave it
     * owned by the caller, which may still be about to execute it. */
    if( cache->max_size > 0 && module->size > cache->max_size ) {
        return;
    }

    /* Do not evict synchronously while admitting an entry. An existing cache
     * entry may be the module executing the nested compilation that reached
     * this function, and the simple cache does not pin active modules. */
    if( exceeds_limits(cache, module->size, 1) ) {
        return;
    }

    /* Make any required map allocation before staging the entry. */
    intern->map = handlebars_map_rehash(intern->map, false);

    time(&module->ts);

    // Cache keys must outlive the caller's template. The map only takes a
    // reference to its key, which is not enough when the incoming string has
    // no independently retained reference or is a temporary preprocessing
    // result.
    key = handlebars_string_copy_ctor(HBSCTX(cache), tmpl);
    talloc_steal(module, key);
    uptr = handlebars_ptr_ctor(HBSCTX(cache), struct handlebars_module, module, false);
    talloc_steal(module, uptr);
    handlebars_value_ptr(value, uptr);

    intern->map = handlebars_map_add(intern->map, key, value);

    /* From here onward no operation can fail. Commit the ownership transfer
     * only after the map contains the entry. */
#ifndef HANDLEBARS_NO_REFCOUNT
    talloc_steal(cache, key);
    talloc_steal(cache, uptr);
#endif
    module = talloc_steal(cache, module);

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
    struct handlebars_map * old_map = intern->map;

    /* Allocate before changing the cache so a failed reset leaves the existing
     * entries and accounting intact. */
    intern->map = handlebars_map_ctor(HBSCTX(cache), 32);
    memset(&intern->stat, 0, sizeof(intern->stat));

#ifdef HANDLEBARS_NO_REFCOUNT
    handlebars_map_foreach(old_map, index, key, value) {
        struct handlebars_module * module = handlebars_value_get_ptr(value, struct handlebars_module);
        handlebars_talloc_free(module);
    } handlebars_map_foreach_end(old_map);
#endif
    handlebars_map_dtor(old_map);
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

struct handlebars_cache_simple_ctor_state {
    struct handlebars_cache * cache;
};

static void handlebars_cache_simple_ctor_init(
    struct handlebars_context * context,
    struct handlebars_cache_simple_ctor_state * state
) {
    struct handlebars_cache * cache = MC(handlebars_talloc_zero(context, struct handlebars_cache));

    state->cache = cache;
    handlebars_context_bind(context, HBSCTX(cache));
    cache->max_age = -1;
    cache->hnd = &hbs_cache_handlers_simple;

    struct handlebars_cache_simple * intern = MC(handlebars_talloc_zero(cache, struct handlebars_cache_simple));
    cache->internal = intern;

    intern->map = handlebars_map_ctor(HBSCTX(cache), 32);
}

struct handlebars_cache * handlebars_cache_simple_ctor(
    struct handlebars_context * context
) {
    struct handlebars_cache_simple_ctor_state state = {0};

    handlebars_cache_simple_ctor_init(context, &state);
    return state.cache;
}

HBS_ATTR_NOINLINE
static enum handlebars_error_type handlebars_cache_simple_ctor_try_guarded(
    struct handlebars_context * context,
    struct handlebars_cache_simple_ctor_state * state
) {
    struct handlebars_error * error = context->e;
    jmp_buf * volatile previous = error->jmp;
    enum handlebars_error_type volatile caught = HANDLEBARS_SUCCESS;
    jmp_buf buf;

    if( handlebars_setjmp_ex(context, &buf) ) {
        caught = error->num;
        if( state->cache != NULL ) {
            handlebars_cache_dtor(state->cache);
            state->cache = NULL;
        }
    } else {
        handlebars_cache_simple_ctor_init(context, state);
    }

    error->jmp = previous;
    return caught;
}

enum handlebars_error_type handlebars_cache_simple_ctor_try(
    struct handlebars_context * context,
    struct handlebars_cache ** result
) {
    struct handlebars_cache_simple_ctor_state state = {0};
    struct handlebars_cache_try_guard guard;
    enum handlebars_error_type error;
    enum handlebars_error_type guard_error;

    *result = NULL;
    error = handlebars_cache_try_guard_begin(context->e, &guard);
    if( error != HANDLEBARS_SUCCESS ) {
        return error;
    }
    handlebars_error_clear(context);
    error = handlebars_cache_simple_ctor_try_guarded(context, &state);
    guard_error = handlebars_cache_try_guard_end(&guard);
    if( error == HANDLEBARS_SUCCESS ) {
        error = guard_error;
    }
    if( error == HANDLEBARS_SUCCESS ) {
        *result = state.cache;
    } else if( state.cache != NULL ) {
        handlebars_cache_dtor(state.cache);
    }
    return error;
}
