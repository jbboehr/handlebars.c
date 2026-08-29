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
#include <string.h>
#include <sys/stat.h>
#include <talloc.h>

#include "handlebars.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"
#include "handlebars_value_private.h"

#include "handlebars_map.h"
#define HANDLEBARS_PARTIAL_LOADER_PRIVATE
#include "handlebars_partial_loader.h"
#include "handlebars_string.h"
#include "handlebars_value.h"
#include "handlebars_value_handlers.h"

#ifndef HANDLEBARS_NO_REFCOUNT
#include "handlebars_rc.h"
#endif

#define GET_INTERN_V(value) GET_INTERN(handlebars_value_get_user(value))
#define GET_INTERN(user) ((struct handlebars_partial_loader *) talloc_get_type_abort(user, struct handlebars_partial_loader))



struct handlebars_partial_loader {
    struct handlebars_user user;
    struct handlebars_string *base_path;
    struct handlebars_string *extension;
    struct handlebars_map * map;
};

struct handlebars_partial_loader_load_state {
    FILE * file;
    struct handlebars_value value;
    struct handlebars_string * key;
};

struct handlebars_partial_loader_init_state {
    struct handlebars_string * base_path;
    struct handlebars_string * extension;
    struct handlebars_partial_loader * loader;
};

struct handlebars_partial_loader_find_state {
    struct handlebars_value * loader;
    struct handlebars_string * name;
    struct handlebars_value value;
    bool found;
};

static bool partial_name_is_safe(struct handlebars_string * key)
{
    const char * name = hbs_str_val(key);
    size_t len = hbs_str_len(key);
    size_t component_start = 0;

    if( len == 0 || name[0] == '/' || name[0] == '\\' ) {
        return false;
    }

    for( size_t i = 0; i <= len; i++ ) {
        if( i < len && name[i] == '\0' ) {
            return false;
        }
        if( i < len && name[i] == '\\' ) {
            return false;
        }
        if( i == len || name[i] == '/' ) {
            size_t component_len = i - component_start;
            if( component_len == 2 && name[component_start] == '.' && name[component_start + 1] == '.' ) {
                return false;
            }
            component_start = i + 1;
        }
    }

    return true;
}

static int partial_loader_dtor(struct handlebars_partial_loader * intern)
{
    // When this gets run, the map has been already freed by talloc it appears
    // if (intern->map) {
    //     handlebars_map_delref(intern->map);
    //     intern->map = NULL;
    // }

    return 0;
}

static int partial_loader_load_state_dtor(struct handlebars_partial_loader_load_state * state)
{
    if( state->file != NULL ) {
        fclose(state->file);
        state->file = NULL;
    }
    handlebars_value_dtor(&state->value);
    return 0;
}

static HBS_ATTR_NORETURN void partial_loader_rethrow(
    struct handlebars_context * context,
    jmp_buf * previous
) {
    if( previous != NULL ) {
        handlebars_longjmp(context, previous, context->e->num);
    }
    const char * message = handlebars_error_msg(context);
    fprintf(stderr, "Throw with invalid jmp_buf: %s\n", message != NULL ? message : "(no error message)");
    abort();
}

static struct handlebars_value * hbs_partial_loader_copy(struct handlebars_value * value)
{
    return NULL;
}

static void hbs_partial_loader_dtor(struct handlebars_user * user)
{
    struct handlebars_partial_loader * intern = GET_INTERN(user);
    if (intern->map) {
        handlebars_map_delref(intern->map);
        intern->map = NULL;
    }
}

static enum handlebars_value_type hbs_partial_loader_type(struct handlebars_value * value)
{
    return HANDLEBARS_VALUE_TYPE_MAP;
}

static struct handlebars_value * hbs_partial_loader_map_find(struct handlebars_value * value, struct handlebars_string * key, struct handlebars_value * rv)
{
    struct handlebars_partial_loader * intern = GET_INTERN_V(value);
    struct handlebars_value *retval = handlebars_map_find(intern->map, key);
    struct handlebars_partial_loader_load_state * state;
    struct handlebars_error * error;
    struct handlebars_string * filename;
    struct stat file_stat;
    char * buf;
    long size;
    size_t read_size;
    bool read_failed;
    jmp_buf * volatile previous;
    volatile bool caught = false;
    struct handlebars_value * volatile result = NULL;
    jmp_buf jump;

    if (retval) {
        handlebars_value_value(rv, retval);
        return rv;
    }

    if( !partial_name_is_safe(key) ) {
        handlebars_throw(intern->user.ctx, HANDLEBARS_ERROR, "Invalid partial name");
    }

    state = handlebars_talloc_zero(intern->user.ctx, struct handlebars_partial_loader_load_state);
    HANDLEBARS_MEMCHECK(state, intern->user.ctx);
    talloc_set_destructor(state, partial_loader_load_state_dtor);

    error = intern->user.ctx->e;
    previous = error->jmp;
    if( handlebars_setjmp_ex(intern->user.ctx, &jump) ) {
        caught = true;
        goto done;
    }

    filename = talloc_steal(state, handlebars_string_copy_ctor(intern->user.ctx, intern->base_path));
    filename = talloc_steal(state, handlebars_string_append(intern->user.ctx, filename, HBS_STRL("/")));
    filename = talloc_steal(state, handlebars_string_append_str(intern->user.ctx, filename, key));
    if (intern->extension) {
        filename = talloc_steal(state, handlebars_string_append_str(intern->user.ctx, filename, intern->extension));
    }

    state->file = fopen(hbs_str_val(filename), "rb");
    if( !state->file ) {
        handlebars_throw(intern->user.ctx, HANDLEBARS_ERROR, "File to open partial: %.*s", (int) hbs_str_len(filename), hbs_str_val(filename));
    }

    if( fstat(fileno(state->file), &file_stat) != 0 || !S_ISREG(file_stat.st_mode) ) {
        handlebars_throw(intern->user.ctx, HANDLEBARS_ERROR, "Partial is not a regular file: %.*s", (int) hbs_str_len(filename), hbs_str_val(filename));
    }

    if( fseek(state->file, 0, SEEK_END) != 0 ) {
        handlebars_throw(intern->user.ctx, HANDLEBARS_ERROR, "Failed to seek partial: %.*s", (int) hbs_str_len(filename), hbs_str_val(filename));
    }
    size = ftell(state->file);
    if( size < 0 || fseek(state->file, 0, SEEK_SET) != 0 ) {
        handlebars_throw(intern->user.ctx, HANDLEBARS_ERROR, "Failed to determine partial size: %.*s", (int) hbs_str_len(filename), hbs_str_val(filename));
    }

    buf = handlebars_talloc_size(state, (size_t) size + 1);
    HANDLEBARS_MEMCHECK(buf, intern->user.ctx);
    read_size = fread(buf, 1, (size_t) size, state->file);
    read_failed = read_size != (size_t) size || ferror(state->file);
    fclose(state->file);
    state->file = NULL;

    if (read_failed) {
        handlebars_throw(intern->user.ctx, HANDLEBARS_ERROR, "Failed to read partial: %.*s", (int) hbs_str_len(filename), hbs_str_val(filename));
    }

    buf[size] = 0;

    // Need to duplicate the key because it may be owned by a child VM
    state->key = talloc_steal(state, handlebars_string_copy_ctor(intern->user.ctx, key));

    handlebars_value_str(
        &state->value,
        talloc_steal(state, handlebars_string_ctor(intern->user.ctx, buf, (size_t) size))
    );

    intern->map = handlebars_map_add(intern->map, state->key, &state->value);
    talloc_steal(intern->user.ctx, state->key);
    talloc_steal(intern->user.ctx, handlebars_value_get_string(&state->value));
    handlebars_value_value(rv, &state->value);
    result = rv;

done:
    error->jmp = previous;
    handlebars_talloc_free(state);
    if( caught ) {
        partial_loader_rethrow(intern->user.ctx, previous);
    }
    return (struct handlebars_value *) result;
}

static bool hbs_partial_loader_iterator_next_void(struct handlebars_value_iterator * it)
{
    (void) it;
    return false;
}

static void hbs_partial_loader_iterator_close_map(struct handlebars_value_iterator * it)
{
    handlebars_value_dtor(it->cur);
    if( it->usr != NULL ) {
        handlebars_map_iterator_release((struct handlebars_map *) it->usr);
        it->usr = NULL;
    }
    it->key = NULL;
}

static bool hbs_partial_loader_iterator_next_map(struct handlebars_value_iterator * it)
{
    struct handlebars_map * map = (struct handlebars_map *) it->usr;
    struct handlebars_value * tmp;
    size_t count;

    assert(map != NULL);

    count = handlebars_map_sparse_array_count(map);
    for( size_t position = it->position + 1; position < count; position++ ) {
        handlebars_map_get_kv_at_index(map, position, &it->key, &tmp);
        if( it->key == NULL ) {
            continue;
        }

        it->position = position;
        it->index++;
        handlebars_value_value(it->cur, tmp);
        return true;
    }

    return false;
}

static bool hbs_partial_loader_iterator_init(struct handlebars_value_iterator * it, struct handlebars_value * value)
{
    struct handlebars_partial_loader * intern = GET_INTERN_V(value);
    struct handlebars_map * map = intern->map;
    struct handlebars_value * tmp;

    if (handlebars_map_count(map) <= 0) {
        it->next = &hbs_partial_loader_iterator_next_void;
        return false;
    }

    it->value = value;
    it->index = 0;
    it->usr = map;
    it->next = &hbs_partial_loader_iterator_next_map;
    it->close = &hbs_partial_loader_iterator_close_map;
    handlebars_map_iterator_acquire(map);

    for( it->position = 0; it->position < handlebars_map_sparse_array_count(map); it->position++ ) {
        handlebars_map_get_kv_at_index(map, it->position, &it->key, &tmp);
        if( it->key != NULL ) {
            handlebars_value_value(it->cur, tmp);
            return true;
        }
    }

    handlebars_value_iterator_close(it);
    return false;
}

static long hbs_partial_loader_count(struct handlebars_value * value)
{
    struct handlebars_partial_loader * intern = GET_INTERN_V(value);
    return handlebars_map_count(intern->map);
}

static const struct handlebars_value_handlers handlebars_value_hbs_partial_loader_handlers = {
    "json",
    &hbs_partial_loader_copy,
    &hbs_partial_loader_dtor,
    NULL, // convert
    &hbs_partial_loader_type,
    &hbs_partial_loader_map_find,
    NULL, // array_find
    &hbs_partial_loader_iterator_init,
    NULL, // call
    &hbs_partial_loader_count
};

HBS_LOCAL bool handlebars_value_is_partial_loader(struct handlebars_value * value)
{
    return handlebars_value_get_real_type(value) == HANDLEBARS_VALUE_TYPE_USER
        && handlebars_value_get_handlers(value) == &handlebars_value_hbs_partial_loader_handlers;
}

HBS_ATTR_NOINLINE
static enum handlebars_error_type partial_loader_init_guarded(
    struct handlebars_context * context,
    struct handlebars_partial_loader_init_state * state
)
{
    struct handlebars_error * error = context->e;
    jmp_buf * volatile previous = error->jmp;
    enum handlebars_error_type volatile caught = HANDLEBARS_SUCCESS;
    jmp_buf jump;

    if( handlebars_setjmp_ex(context, &jump) ) {
        caught = error->num;
    } else {
        state->loader = handlebars_talloc_zero(
            context,
            struct handlebars_partial_loader
        );
        HANDLEBARS_MEMCHECK(state->loader, context);
        handlebars_user_init(
            &state->loader->user,
            context,
            &handlebars_value_hbs_partial_loader_handlers
        );
        state->loader->base_path = talloc_steal(
            state->loader,
            handlebars_string_copy_ctor(context, state->base_path)
        );
        state->loader->extension = talloc_steal(
            state->loader,
            handlebars_string_copy_ctor(context, state->extension)
        );
        state->loader->map = talloc_steal(
            state->loader,
            handlebars_map_ctor(context, 32)
        );
        /* The iterator-retention API requires its owner to hold the initial ref. */
        handlebars_map_addref(state->loader->map);
        talloc_set_destructor(state->loader, partial_loader_dtor);
    }

    error->jmp = previous;
    return caught;
}

static enum handlebars_error_type partial_loader_init_transaction(
    struct handlebars_context * context,
    struct handlebars_string * base_path,
    struct handlebars_string * extension,
    struct handlebars_value * result,
    bool clear_error
)
{
    struct handlebars_partial_loader_init_state state = {
        .base_path = base_path,
        .extension = extension
    };
    enum handlebars_error_type error;

    if( clear_error ) {
        handlebars_error_clear(context);
    }
    error = partial_loader_init_guarded(context, &state);
    if( error != HANDLEBARS_SUCCESS ) {
        if( state.loader != NULL ) {
            handlebars_talloc_free(state.loader);
        }
        return error;
    }
    if( unlikely(state.loader == NULL) ) {
        handlebars_error_set(
            context,
            HANDLEBARS_ERROR,
            "Partial loader initialization produced no result"
        );
        return handlebars_error_num(context);
    }
    handlebars_value_user(result, &state.loader->user);
    return HANDLEBARS_SUCCESS;
}

struct handlebars_value * handlebars_value_partial_loader_init(
    struct handlebars_context * context,
    struct handlebars_string * base_path,
    struct handlebars_string * extension,
    struct handlebars_value * rv
) {
    enum handlebars_error_type error = partial_loader_init_transaction(
        context,
        base_path,
        extension,
        rv,
        false
    );

    if( unlikely(error != HANDLEBARS_SUCCESS) ) {
        partial_loader_rethrow(context, context->e->jmp);
    }
    return rv;
}

enum handlebars_error_type handlebars_value_partial_loader_init_try(
    struct handlebars_context * context,
    struct handlebars_string * base_path,
    struct handlebars_string * extension,
    struct handlebars_value * result
) {
    return partial_loader_init_transaction(
        context,
        base_path,
        extension,
        result,
        true
    );
}

HBS_ATTR_NOINLINE
static enum handlebars_error_type partial_loader_find_guarded(
    struct handlebars_context * context,
    struct handlebars_partial_loader_find_state * state
)
{
    struct handlebars_error * error = context->e;
    jmp_buf * volatile previous = error->jmp;
    enum handlebars_error_type volatile caught = HANDLEBARS_SUCCESS;
    jmp_buf jump;

    if( handlebars_setjmp_ex(context, &jump) ) {
        caught = error->num;
    } else {
        state->found = hbs_partial_loader_map_find(
            state->loader,
            state->name,
            &state->value
        ) != NULL;
    }

    error->jmp = previous;
    return caught;
}

enum handlebars_error_type handlebars_value_partial_loader_find_try(
    struct handlebars_value * loader,
    struct handlebars_string * name,
    struct handlebars_value * result
) {
    struct handlebars_partial_loader * intern;
    struct handlebars_partial_loader_find_state state = {
        .loader = loader,
        .name = name
    };
    enum handlebars_error_type error;

    assert(handlebars_value_is_partial_loader(loader));
    intern = GET_INTERN_V(loader);
    handlebars_value_init(&state.value);
    handlebars_error_clear(intern->user.ctx);
    error = partial_loader_find_guarded(intern->user.ctx, &state);
    if( error == HANDLEBARS_SUCCESS && state.found ) {
        handlebars_value_value(result, &state.value);
    }
    handlebars_value_dtor(&state.value);
    return error;
}
