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

#include "handlebars.h"
#include "handlebars_cache.h"
#include "handlebars_cache_private.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"

#include <stdlib.h>
#include <string.h>

#ifdef HANDLEBARS_HAVE_PTHREAD
#include <errno.h>
#include <pthread.h>
#endif


const size_t HANDLEBARS_CACHE_SIZE = sizeof(struct handlebars_cache);

#ifdef HANDLEBARS_HAVE_PTHREAD
struct handlebars_cache_try_lock_entry {
    struct handlebars_error * error;
    pthread_mutex_t mutex;
    struct handlebars_cache_try_lock_entry * next;
};

static pthread_mutex_t handlebars_cache_try_registry_mutex = PTHREAD_MUTEX_INITIALIZER;
static struct handlebars_cache_try_lock_entry * handlebars_cache_try_registry;

static void handlebars_cache_try_error_set(
    struct handlebars_error * error,
    enum handlebars_error_type type
)
{
    struct handlebars_context context = { .e = error };

    if( type == HANDLEBARS_NOMEM ) {
        handlebars_error_set(
            &context,
            HANDLEBARS_NOMEM,
            HANDLEBARS_MEMCHECK_MSG
        );
    } else {
        handlebars_error_set(
            &context,
            type,
            "Cache try synchronization error"
        );
    }
}

static int handlebars_cache_try_entry_init(
    struct handlebars_cache_try_lock_entry * entry
)
{
    pthread_mutexattr_t attr;
    bool attr_initialized = false;
    bool mutex_initialized = false;
    int rc;

    rc = pthread_mutexattr_init(&attr);
    if( rc == 0 ) {
        attr_initialized = true;
        rc = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    }
    if( rc == 0 ) {
        rc = pthread_mutex_init(&entry->mutex, &attr);
        mutex_initialized = rc == 0;
    }
    if( attr_initialized ) {
        int destroy_rc = pthread_mutexattr_destroy(&attr);
        if( rc == 0 ) {
            rc = destroy_rc;
        }
    }
    if( rc != 0 && mutex_initialized ) {
        pthread_mutex_destroy(&entry->mutex);
    }
    return rc;
}

static int handlebars_cache_try_entry_dtor(
    struct handlebars_cache_try_lock_entry * entry
)
{
    struct handlebars_cache_try_lock_entry ** cursor;
    int result = 0;

    if( pthread_mutex_lock(&handlebars_cache_try_registry_mutex) != 0 ) {
        return -1;
    }
    for(
        cursor = &handlebars_cache_try_registry;
        *cursor != entry;
        cursor = &(*cursor)->next
    ) {
        if( *cursor == NULL ) {
            result = -1;
            goto done;
        }
    }
    if( pthread_mutex_destroy(&entry->mutex) != 0 ) {
        result = -1;
        goto done;
    }
    *cursor = entry->next;

done:
    if( pthread_mutex_unlock(&handlebars_cache_try_registry_mutex) != 0 ) {
        return -1;
    }
    return result;
}

static enum handlebars_error_type handlebars_cache_try_entry_get(
    struct handlebars_error * error,
    struct handlebars_cache_try_guard * guard
)
{
    struct handlebars_cache_try_lock_entry * entry;
    enum handlebars_error_type result = HANDLEBARS_SUCCESS;
    int rc = pthread_mutex_lock(&handlebars_cache_try_registry_mutex);

    if( rc != 0 ) {
        return HANDLEBARS_ERROR;
    }

    for(
        entry = handlebars_cache_try_registry;
        entry != NULL;
        entry = entry->next
    ) {
        if( entry->error == error ) {
            break;
        }
    }

    if( entry == NULL ) {
        entry = handlebars_talloc_zero(
            error,
            struct handlebars_cache_try_lock_entry
        );
        if( entry == NULL ) {
            handlebars_cache_try_error_set(error, HANDLEBARS_NOMEM);
            result = HANDLEBARS_NOMEM;
            goto done;
        }
        entry->error = error;
        rc = handlebars_cache_try_entry_init(entry);
        if( rc != 0 ) {
            handlebars_talloc_free(entry);
            entry = NULL;
            handlebars_cache_try_error_set(error, HANDLEBARS_ERROR);
            result = HANDLEBARS_ERROR;
            goto done;
        }
        talloc_set_destructor(entry, handlebars_cache_try_entry_dtor);
        entry->next = handlebars_cache_try_registry;
        handlebars_cache_try_registry = entry;
    }

    guard->entry = entry;

done:
    if( pthread_mutex_unlock(&handlebars_cache_try_registry_mutex) != 0 ) {
        return HANDLEBARS_ERROR;
    }
    return result;
}
#endif

HBS_LOCAL enum handlebars_error_type handlebars_cache_try_guard_begin(
    struct handlebars_error * error,
    struct handlebars_cache_try_guard * guard
)
{
    memset(guard, 0, sizeof(*guard));
    guard->error = error;
#ifdef HANDLEBARS_HAVE_PTHREAD
    enum handlebars_error_type result = handlebars_cache_try_entry_get(
        error,
        guard
    );
    struct handlebars_cache_try_lock_entry * entry = guard->entry;

    if( result != HANDLEBARS_SUCCESS ) {
        return result;
    }
    if( pthread_mutex_lock(&entry->mutex) != 0 ) {
        guard->entry = NULL;
        return HANDLEBARS_ERROR;
    }
#endif
    guard->locked = true;
    return HANDLEBARS_SUCCESS;
}

HBS_LOCAL enum handlebars_error_type handlebars_cache_try_guard_try_begin(
    struct handlebars_error * error,
    struct handlebars_cache_try_guard * guard,
    bool * acquired
)
{
    *acquired = false;
    memset(guard, 0, sizeof(*guard));
    guard->error = error;
#ifdef HANDLEBARS_HAVE_PTHREAD
    enum handlebars_error_type result = handlebars_cache_try_entry_get(
        error,
        guard
    );
    struct handlebars_cache_try_lock_entry * entry = guard->entry;
    int rc;

    if( result != HANDLEBARS_SUCCESS ) {
        return result;
    }
    rc = pthread_mutex_trylock(&entry->mutex);
    if( rc != 0 ) {
        guard->entry = NULL;
        if( rc == EBUSY ) {
            return HANDLEBARS_SUCCESS;
        }
        return HANDLEBARS_ERROR;
    }
#endif
    guard->locked = true;
    *acquired = true;
    return HANDLEBARS_SUCCESS;
}

HBS_LOCAL enum handlebars_error_type handlebars_cache_try_guard_end(
    struct handlebars_cache_try_guard * guard
)
{
    if( !guard->locked ) {
        return HANDLEBARS_ERROR;
    }
#ifdef HANDLEBARS_HAVE_PTHREAD
    struct handlebars_cache_try_lock_entry * entry = guard->entry;

    if( pthread_mutex_unlock(&entry->mutex) != 0 ) {
        return HANDLEBARS_ERROR;
    }
    guard->locked = false;
    guard->entry = NULL;
    return HANDLEBARS_SUCCESS;
#else
    guard->locked = false;
    return HANDLEBARS_SUCCESS;
#endif
}

enum handlebars_cache_try_operation {
    handlebars_cache_try_find,
    handlebars_cache_try_add,
    handlebars_cache_try_gc,
    handlebars_cache_try_reset,
    handlebars_cache_try_release,
    handlebars_cache_try_stat
};

struct handlebars_cache_try_state {
    enum handlebars_cache_try_operation operation;
    struct handlebars_string * key;
    struct handlebars_module * module;
    struct handlebars_module * found;
    int removed;
    struct handlebars_cache_stat stat;
};

void handlebars_cache_dtor(struct handlebars_cache * cache)
{
    handlebars_talloc_free(cache);
}

struct handlebars_cache_stat handlebars_cache_stat(struct handlebars_cache * cache)
{
    return cache->hnd->stat(cache);
}

struct handlebars_module * handlebars_cache_find(
    struct handlebars_cache * cache,
    struct handlebars_string * key
) {
    return cache->hnd->find(cache, key);
}

void handlebars_cache_add(
    struct handlebars_cache * cache,
    struct handlebars_string * tmpl,
    struct handlebars_module * module
) {
    cache->hnd->add(cache, tmpl, module);
}

int handlebars_cache_gc(struct handlebars_cache * cache)
{
    return cache->hnd->gc(cache);
}

void handlebars_cache_reset(struct handlebars_cache * cache)
{
    cache->hnd->reset(cache);
}

void handlebars_cache_release(
    struct handlebars_cache * cache,
    struct handlebars_string * key,
    struct handlebars_module * module
) {
    cache->hnd->release(cache, key, module);
}

HBS_ATTR_NOINLINE
static enum handlebars_error_type handlebars_cache_try_guarded(
    struct handlebars_cache * cache,
    struct handlebars_cache_try_state * state
) {
    struct handlebars_error * error = HBSCTX(cache)->e;
    jmp_buf * volatile previous = error->jmp;
    enum handlebars_error_type volatile caught = HANDLEBARS_SUCCESS;
    jmp_buf buf;

    if( handlebars_setjmp_ex(cache, &buf) ) {
        caught = error->num;
    } else {
        switch( state->operation ) {
            case handlebars_cache_try_find:
                state->found = handlebars_cache_find(cache, state->key);
                break;
            case handlebars_cache_try_add:
                handlebars_cache_add(cache, state->key, state->module);
                break;
            case handlebars_cache_try_gc:
                state->removed = handlebars_cache_gc(cache);
                break;
            case handlebars_cache_try_reset:
                handlebars_cache_reset(cache);
                break;
            case handlebars_cache_try_release:
                handlebars_cache_release(cache, state->key, state->module);
                break;
            case handlebars_cache_try_stat:
                state->stat = handlebars_cache_stat(cache);
                break;
            default: abort(); // LCOV_EXCL_LINE
        }
    }

    error->jmp = previous;
    return caught;
}

static enum handlebars_error_type handlebars_cache_try(
    struct handlebars_cache * cache,
    struct handlebars_cache_try_state * state
) {
    struct handlebars_cache_try_guard guard;
    enum handlebars_error_type error;
    enum handlebars_error_type guard_error;

    error = handlebars_cache_try_guard_begin(HBSCTX(cache)->e, &guard);
    if( error != HANDLEBARS_SUCCESS ) {
        return error;
    }
    handlebars_error_clear(HBSCTX(cache));
    error = handlebars_cache_try_guarded(cache, state);
    guard_error = handlebars_cache_try_guard_end(&guard);
    if( error == HANDLEBARS_SUCCESS ) {
        error = guard_error;
    }
    return error;
}

enum handlebars_error_type handlebars_cache_find_try(
    struct handlebars_cache * cache,
    struct handlebars_string * key,
    struct handlebars_module ** result
) {
    struct handlebars_cache_try_state state = {
        .operation = handlebars_cache_try_find,
        .key = key
    };
    enum handlebars_error_type error;

    *result = NULL;
    error = handlebars_cache_try(cache, &state);
    if( error == HANDLEBARS_SUCCESS ) {
        *result = state.found;
    }
    return error;
}

enum handlebars_error_type handlebars_cache_add_try(
    struct handlebars_cache * cache,
    struct handlebars_string * key,
    struct handlebars_module * module
) {
    struct handlebars_cache_try_state state = {
        .operation = handlebars_cache_try_add,
        .key = key,
        .module = module
    };

    return handlebars_cache_try(cache, &state);
}

enum handlebars_error_type handlebars_cache_gc_try(
    struct handlebars_cache * cache,
    int * removed
) {
    struct handlebars_cache_try_state state = {
        .operation = handlebars_cache_try_gc
    };
    enum handlebars_error_type error = handlebars_cache_try(cache, &state);

    if( error == HANDLEBARS_SUCCESS ) {
        *removed = state.removed;
    }
    return error;
}

enum handlebars_error_type handlebars_cache_reset_try(
    struct handlebars_cache * cache
) {
    struct handlebars_cache_try_state state = {
        .operation = handlebars_cache_try_reset
    };

    return handlebars_cache_try(cache, &state);
}

enum handlebars_error_type handlebars_cache_release_try(
    struct handlebars_cache * cache,
    struct handlebars_string * key,
    struct handlebars_module * module
) {
    struct handlebars_cache_try_state state = {
        .operation = handlebars_cache_try_release,
        .key = key,
        .module = module
    };

    return handlebars_cache_try(cache, &state);
}

enum handlebars_error_type handlebars_cache_stat_try(
    struct handlebars_cache * cache,
    struct handlebars_cache_stat * result
) {
    struct handlebars_cache_try_state state = {
        .operation = handlebars_cache_try_stat
    };
    enum handlebars_error_type error = handlebars_cache_try(cache, &state);

    if( error == HANDLEBARS_SUCCESS ) {
        *result = state.stat;
    }
    return error;
}
