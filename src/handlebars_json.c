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
#include <limits.h>
#include <string.h>
#include <talloc.h>

// json-c undeprecated json_object_object_get, but the version in xenial
// is too old, so let's silence deprecated warnings for json-c
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <json.h>
#include <json_object.h>
#include <json_tokener.h>
#pragma GCC diagnostic pop

#include "handlebars.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"
#include "handlebars_value_private.h"

#include "handlebars_json.h"
#include "handlebars_map.h"
#include "handlebars_stack.h"
#include "handlebars_string.h"
#include "handlebars_value.h"
#include "handlebars_value_handlers.h"

#ifndef HANDLEBARS_NO_REFCOUNT
#include "handlebars_rc.h"
#endif



#define GET_INTERN_V(value) GET_INTERN(handlebars_value_get_user(value))
#define GET_INTERN(user) ((struct handlebars_json *) talloc_get_type_abort(user, struct handlebars_json))
#define HANDLEBARS_JSON_OBJ(value) GET_INTERN_V(value)->object

struct handlebars_json {
    struct handlebars_user user;
    struct json_object * object;
};

struct handlebars_json_traversal {
    struct handlebars_context * context;
    struct json_object * active[HANDLEBARS_VALUE_MAX_DEPTH];
    size_t active_count;
};

enum handlebars_json_try_operation {
    handlebars_json_try_object,
    handlebars_json_try_string
};

struct handlebars_json_try_state {
    struct handlebars_value value;
    enum handlebars_json_try_operation operation;
    struct json_object * object;
    const char * json;
    size_t length;
};

static void hbs_json_validate_traversal(
    struct json_object * object,
    struct handlebars_json_traversal * state
) {
    enum json_type type = json_object_get_type(object);

    if( type != json_type_object && type != json_type_array ) {
        return;
    }
    for( size_t i = 0; i < state->active_count; i++ ) {
        if( unlikely(state->active[i] == object) ) {
            handlebars_throw(state->context, HANDLEBARS_ERROR, "Cyclic JSON value reference");
        }
    }
    if( unlikely(state->active_count >= HANDLEBARS_VALUE_MAX_DEPTH) ) {
        handlebars_throw(
            state->context,
            HANDLEBARS_ERROR,
            "JSON value nesting exceeds the maximum depth of %d",
            HANDLEBARS_VALUE_MAX_DEPTH
        );
    }

    state->active[state->active_count++] = object;
    if( type == json_type_object ) {
        json_object_object_foreach(object, key, child) {
            (void) key;
            hbs_json_validate_traversal(child, state);
        }
    } else {
        size_t count = json_object_array_length(object);

        for( size_t i = 0; i < count; i++ ) {
            hbs_json_validate_traversal(json_object_array_get_idx(object, i), state);
        }
    }
    state->active_count--;
}


static int handlebars_json_dtor(struct handlebars_json * obj)
{
    if( obj && obj->object ) {
        json_object_put(obj->object);
        obj->object = NULL;
    }

    return 0;
}

static struct handlebars_value * hbs_json_copy(struct handlebars_value * value)
{
    // struct handlebars_json * intern = GET_INTERN_V(value);
    // const char * str = json_object_to_json_string(intern->object);
    // return handlebars_value_from_json_string(intern->user.ctx, str);
    abort();
}

static void hbs_json_dtor(struct handlebars_user * user)
{
    struct handlebars_json * json = GET_INTERN(user);
    if( json && json->object ) {
        json_object_put(json->object);
        json->object = NULL;
    }
}

static void hbs_json_convert_unchecked(struct handlebars_value * value, bool recurse)
{
    struct handlebars_json * intern = GET_INTERN_V(value);

    switch( json_object_get_type(intern->object) ) {
        case json_type_object: {
            struct handlebars_map * map = handlebars_map_ctor(intern->user.ctx, json_object_object_length(intern->object));
            json_object_object_foreach(intern->object, k, v) {
                HANDLEBARS_VALUE_DECL(new_value);
                handlebars_value_init_json_object(intern->user.ctx, new_value, v);
                if( recurse && handlebars_value_get_real_type(new_value) == HANDLEBARS_VALUE_TYPE_USER ) {
                    hbs_json_convert_unchecked(new_value, recurse);
                }
                map = handlebars_map_str_update(map, k, strlen(k), new_value);
                HANDLEBARS_VALUE_UNDECL(new_value);
            }
            handlebars_value_map(value, map);
            break;
        }

        case json_type_array: {
            size_t i;
            size_t l = json_object_array_length(intern->object);
            struct handlebars_stack * stack = handlebars_stack_ctor(intern->user.ctx, l);
            for( i = 0; i < l; i++ ) {
                HANDLEBARS_VALUE_DECL(new_value);
                handlebars_value_init_json_object(intern->user.ctx, new_value, json_object_array_get_idx(intern->object, i));
                if( recurse && handlebars_value_get_real_type(new_value) == HANDLEBARS_VALUE_TYPE_USER ) {
                    hbs_json_convert_unchecked(new_value, recurse);
                }
                stack = handlebars_stack_push(stack, new_value);
                HANDLEBARS_VALUE_UNDECL(new_value);
            }
            handlebars_value_array(value, stack);
            break;
        }

        default: break; // LCOV_EXCL_LINE
    }
}

static void hbs_json_convert(struct handlebars_value * value, bool recurse)
{
    struct handlebars_json * intern = GET_INTERN_V(value);

    if( recurse ) {
        struct handlebars_json_traversal state;

        /* Validate API-constructed graphs before conversion mutates the value. */
        state.context = intern->user.ctx;
        state.active_count = 0;
        hbs_json_validate_traversal(intern->object, &state);
    }
    hbs_json_convert_unchecked(value, recurse);
}

static enum handlebars_value_type hbs_json_type(struct handlebars_value * value)
{
    struct json_object * intern = HANDLEBARS_JSON_OBJ(value);

    switch( json_object_get_type(intern) ) {
        case json_type_object: return HANDLEBARS_VALUE_TYPE_MAP;
        case json_type_array: return HANDLEBARS_VALUE_TYPE_ARRAY;
        default: // LCOV_EXCL_START
            assert(0);
            return HANDLEBARS_VALUE_TYPE_NULL;
            // LCOV_EXCL_STOP
    }
}

static struct handlebars_value * hbs_json_map_find(struct handlebars_value * value, struct handlebars_string * key, struct handlebars_value * rv)
{
    struct handlebars_json * intern = GET_INTERN_V(value);
    struct json_object * item = json_object_object_get(intern->object, hbs_str_val(key));
    if( item == NULL ) {
        return NULL;
    }
    handlebars_value_init_json_object(intern->user.ctx, rv, item);
    return rv;
}

static struct handlebars_value * hbs_json_array_find(struct handlebars_value * value, size_t index, struct handlebars_value * rv)
{
    struct handlebars_json * intern = GET_INTERN_V(value);
    struct json_object * item = json_object_array_get_idx(intern->object, (int) index);
    if( item == NULL ) {
        return NULL;
    }
    handlebars_value_init_json_object(intern->user.ctx, rv, item);
    return rv;
}

static bool hbs_json_iterator_next_void(struct handlebars_value_iterator * it)
{
    (void) it;
    return false;
}

static void hbs_json_iterator_close_object(struct handlebars_value_iterator * it)
{
    if( it->key != NULL ) {
        handlebars_string_delref(it->key);
        it->key = NULL;
    }
    handlebars_value_dtor(it->cur);
}

static bool hbs_json_iterator_next_object(struct handlebars_value_iterator * it)
{
    struct handlebars_json * intern = GET_INTERN(it->user);
    struct lh_entry * entry;
    char * tmp;

    assert(it->user != NULL);
    assert(it->key != NULL);

    handlebars_string_delref(it->key);
    it->key = NULL;

    entry = (struct lh_entry *) it->usr;
    if( !entry || !entry->next ) {
        return false;
    }

    it->usr = (void *) (entry = entry->next);
    tmp = (char *) entry->k;
    it->key = handlebars_string_ctor(intern->user.ctx, tmp, strlen(tmp));
    handlebars_value_init_json_object(intern->user.ctx, it->cur, (struct json_object *) entry->v);
    handlebars_string_addref(it->key);
    return true;
}

static void hbs_json_iterator_close_array(struct handlebars_value_iterator * it)
{
    handlebars_value_dtor(it->cur);
}

static bool hbs_json_iterator_next_array(struct handlebars_value_iterator * it)
{
    struct handlebars_json * intern = GET_INTERN(it->user);

    assert(it->user != NULL);

    it->index++;
    if( it->index >= (size_t) json_object_array_length(intern->object) ) {
        return false;
    }

    handlebars_value_init_json_object(intern->user.ctx, it->cur, json_object_array_get_idx(intern->object, it->index));
    return true;
}

static bool hbs_json_iterator_init(struct handlebars_value_iterator * it, struct handlebars_value * value)
{
    struct handlebars_json * intern = GET_INTERN_V(value);
    struct lh_entry * entry;

    it->value = value;

    switch( json_object_get_type(intern->object) ) {
        case json_type_object: {
            entry = json_object_get_object(intern->object)->head;
            if (unlikely(entry == NULL)) {
                it->next = &hbs_json_iterator_next_void;
                return false;
            }
            char * tmp = (char *) entry->k;
            it->usr = (void *) entry;
            it->key = handlebars_string_ctor(intern->user.ctx, tmp, strlen(tmp));
            handlebars_value_init_json_object(intern->user.ctx, it->cur, (json_object *) entry->v);
            it->next = &hbs_json_iterator_next_object;
            it->close = &hbs_json_iterator_close_object;
            handlebars_string_addref(it->key);
            return true;
        }

        case json_type_array:
            if( unlikely(json_object_array_length(intern->object) == 0) ) {
                it->next = &hbs_json_iterator_next_void;
                return false;
            }
            it->index = 0;
            handlebars_value_init_json_object(intern->user.ctx, it->cur, json_object_array_get_idx(intern->object, (int) it->index));
            it->next = &hbs_json_iterator_next_array;
            it->close = &hbs_json_iterator_close_array;
            return true;

        default: // LCOV_EXCL_START
            assert(0);
            it->next = &hbs_json_iterator_next_void;
            return false;
            // LCOV_EXCL_STOP
    }
}

static long hbs_json_count(struct handlebars_value * value)
{
    struct json_object * intern = HANDLEBARS_JSON_OBJ(value);
    switch( json_object_get_type(intern) ) {
        case json_type_object:
            return json_object_object_length(intern);
        case json_type_array:
            return json_object_array_length(intern);
        default:
            return -1;
    }

}

static const struct handlebars_value_handlers handlebars_value_hbs_json_handlers = {
    "json",
    &hbs_json_copy,
    &hbs_json_dtor,
    &hbs_json_convert,
    &hbs_json_type,
    &hbs_json_map_find,
    &hbs_json_array_find,
    &hbs_json_iterator_init,
    NULL, // call
    &hbs_json_count
};

void handlebars_value_init_json_object(struct handlebars_context * ctx, struct handlebars_value * value, struct json_object *json)
{
    struct handlebars_json * obj;

    switch( json_object_get_type(json) ) {
        case json_type_null:
            handlebars_value_null(value);
            break;
        case json_type_boolean:
            handlebars_value_boolean(value, json_object_get_boolean(json));
            break;
        case json_type_double:
            handlebars_value_float(value, json_object_get_double(json));
            break;
        case json_type_int:
            handlebars_value_integer(value, json_object_get_int64(json));
            break;
        case json_type_string:
            handlebars_value_str(value, handlebars_string_ctor(ctx, json_object_get_string(json), json_object_get_string_len(json)));
            break;

        case json_type_object:
        case json_type_array:
            obj = handlebars_talloc(ctx, struct handlebars_json);
            HANDLEBARS_MEMCHECK(obj, ctx);
            handlebars_user_init((struct handlebars_user *) obj, ctx, &handlebars_value_hbs_json_handlers);
            // Increment only after the fallible allocation so a failed wrap
            // cannot leak a JSON reference.
            json_object_get(json);
            obj->object = json;
            talloc_set_destructor(obj, handlebars_json_dtor);
            handlebars_value_user(value, (struct handlebars_user *) obj);
            break;

        default: assert(0); break; // LCOV_EXCL_LINE
    }
}

static struct json_object *json_tokener_parse_verbose_length(
    struct handlebars_context * context,
    const char *str,
    size_t length,
    enum json_tokener_error *error
)
{
	struct json_tokener *tok;
	struct json_object *obj;

	tok = json_tokener_new();
	if (!tok) {
        handlebars_throw(context, HANDLEBARS_NOMEM, "Failed to initialize JSON parser");
    }

	obj = json_tokener_parse_ex(tok, str, length);
	*error = tok->err;
	if (tok->err != json_tokener_success) {
		if (obj != NULL) {
			json_object_put(obj);
        }
		obj = NULL;
	}

	json_tokener_free(tok);
	return obj;
}

static HBS_ATTR_NORETURN void handlebars_json_rethrow(
    struct handlebars_context * context,
    jmp_buf * previous
)
{
    if( previous != NULL ) {
        handlebars_longjmp(context, previous, context->e->num);
    }
    abort();
}

void handlebars_value_init_json_stringl(struct handlebars_context *ctx, struct handlebars_value * value, const char * json, size_t length)
{
    struct handlebars_error * error = ctx->e;
    jmp_buf * volatile previous = error->jmp;
    enum json_tokener_error parse_err = json_tokener_success;
    struct json_object * result;
    jmp_buf buf;

    if( unlikely(length > INT_MAX) ) {
        handlebars_throw(ctx, HANDLEBARS_ERROR, "JSON input length exceeds parser limit");
    }

    result = json_tokener_parse_verbose_length(ctx, json, length, &parse_err);
    if( parse_err == json_tokener_success ) {
        if( handlebars_setjmp_ex(ctx, &buf) ) {
            json_object_put(result);
            error->jmp = previous;
            handlebars_json_rethrow(ctx, previous);
        }
        handlebars_value_init_json_object(ctx, value, result);
        json_object_put(result);
        error->jmp = previous;
    } else {
        enum handlebars_error_type error_type = HANDLEBARS_ERROR;

        // json_tokener_error_memory was added in json-c 0.17.
#if defined(JSON_C_VERSION_NUM) && JSON_C_VERSION_NUM >= 0x001100
        if( parse_err == json_tokener_error_memory ) {
            error_type = HANDLEBARS_NOMEM;
        }
#endif
        handlebars_throw(ctx, error_type, "JSON Parse error: %s", json_tokener_error_desc(parse_err));
    }
}

void handlebars_value_init_json_string(struct handlebars_context *ctx, struct handlebars_value * value, const char * json)
{
    handlebars_value_init_json_stringl(ctx, value, json, strlen(json) + 1);
}

HBS_ATTR_NOINLINE
static enum handlebars_error_type handlebars_json_try_guarded(
    struct handlebars_context * context,
    struct handlebars_json_try_state * state
)
{
    struct handlebars_error * error = context->e;
    jmp_buf * volatile previous = error->jmp;
    enum handlebars_error_type volatile caught = HANDLEBARS_SUCCESS;
    jmp_buf buf;

    if( handlebars_setjmp_ex(context, &buf) ) {
        caught = error->num;
    } else {
        switch( state->operation ) {
            case handlebars_json_try_object:
                handlebars_value_init_json_object(
                    context,
                    &state->value,
                    state->object
                );
                break;
            case handlebars_json_try_string:
                handlebars_value_init_json_stringl(
                    context,
                    &state->value,
                    state->json,
                    state->length
                );
                break;
            default: abort(); // LCOV_EXCL_LINE
        }
    }

    error->jmp = previous;
    return caught;
}

static enum handlebars_error_type handlebars_json_try(
    struct handlebars_context * context,
    struct handlebars_value * value,
    struct handlebars_json_try_state * state
)
{
    enum handlebars_error_type error;

    handlebars_value_init(&state->value);
    handlebars_error_clear(context);
    error = handlebars_json_try_guarded(context, state);
    if( error == HANDLEBARS_SUCCESS ) {
        handlebars_value_value(value, &state->value);
    }
    handlebars_value_dtor(&state->value);
    return error;
}

enum handlebars_error_type handlebars_value_init_json_object_try(
    struct handlebars_context * context,
    struct handlebars_value * value,
    struct json_object * json
)
{
    struct handlebars_json_try_state state = {
        .operation = handlebars_json_try_object,
        .object = json
    };

    return handlebars_json_try(context, value, &state);
}

enum handlebars_error_type handlebars_value_init_json_stringl_try(
    struct handlebars_context * context,
    struct handlebars_value * value,
    const char * json,
    size_t length
)
{
    struct handlebars_json_try_state state = {
        .operation = handlebars_json_try_string,
        .json = json,
        .length = length
    };

    return handlebars_json_try(context, value, &state);
}

enum handlebars_error_type handlebars_value_init_json_string_try(
    struct handlebars_context * context,
    struct handlebars_value * value,
    const char * json
)
{
    return handlebars_value_init_json_stringl_try(
        context,
        value,
        json,
        strlen(json) + 1
    );
}
