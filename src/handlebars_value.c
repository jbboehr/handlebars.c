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
#include <talloc.h>

#ifdef HANDLEBARS_HAVE_VALGRIND
#include <valgrind/memcheck.h>
#endif

#include "handlebars.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"
#include "handlebars_value_private.h"

#include "handlebars_closure.h"
#include "handlebars_helpers.h"
#include "handlebars_map.h"
#include "handlebars_ptr.h"
#include "handlebars_stack.h"
#include "handlebars_string.h"
#include "handlebars_value.h"
#include "handlebars_value_handlers.h"

#ifndef HANDLEBARS_NO_REFCOUNT
#include "handlebars_rc.h"
#endif



// {{{ Prototypes & Variables

struct handlebars_value_traversal {
    struct handlebars_context * context;
    const void * active[HANDLEBARS_VALUE_MAX_DEPTH];
    size_t active_count;
    char * output;
};

static bool handlebars_value_iterator_init_ex(
    struct handlebars_value_iterator * it,
    struct handlebars_value * current,
    struct handlebars_value * value,
    struct handlebars_context * unwind_context
);

static const void * handlebars_value_traversal_identity(struct handlebars_value * value)
{
    switch( value->type ) {
        case HANDLEBARS_VALUE_TYPE_ARRAY:
            return value->v.stack;
        case HANDLEBARS_VALUE_TYPE_MAP:
            return value->v.map;
        case HANDLEBARS_VALUE_TYPE_USER:
            return value->v.user;
        default:
            return NULL;
    }
}

static void handlebars_value_traversal_enter(
    struct handlebars_value_traversal * state,
    struct handlebars_value * value
) {
    const void * identity = handlebars_value_traversal_identity(value);

    assert(identity != NULL);
    for( size_t i = 0; i < state->active_count; i++ ) {
        if( unlikely(state->active[i] == identity) ) {
            handlebars_throw(state->context, HANDLEBARS_ERROR, "Cyclic value reference");
        }
    }
    if( unlikely(state->active_count >= HANDLEBARS_VALUE_MAX_DEPTH) ) {
        handlebars_throw(
            state->context,
            HANDLEBARS_ERROR,
            "Value nesting exceeds the maximum depth of %d",
            HANDLEBARS_VALUE_MAX_DEPTH
        );
    }
    state->active[state->active_count++] = identity;
}

static void handlebars_value_traversal_leave(struct handlebars_value_traversal * state)
{
    assert(state->active_count > 0);
    state->active_count--;
}

static HBS_ATTR_NORETURN void handlebars_value_rethrow(
    struct handlebars_context * context,
    jmp_buf * previous
) {
    if( previous != NULL ) {
        handlebars_longjmp(context, previous, context->e->num);
    }
    fprintf(stderr, "Throw with invalid jmp_buf: %s\n", handlebars_error_msg(context));
    abort();
}

#undef HANDLEBARS_VALUE_SIZE
#undef HANDLEBARS_VALUE_INTERNALS_SIZE
#undef HANDLEBARS_VALUE_ITERATOR_SIZE
const size_t HANDLEBARS_VALUE_SIZE = sizeof(struct handlebars_value);
const size_t HANDLEBARS_VALUE_INTERNALS_SIZE = sizeof(union handlebars_value_internals);
const size_t HANDLEBARS_VALUE_ITERATOR_SIZE = sizeof(struct handlebars_value_iterator);
#define HANDLEBARS_VALUE_SIZE sizeof(struct handlebars_value)
#define HANDLEBARS_VALUE_INTERNALS_SIZE sizeof(union handlebars_value_internals)
#define HANDLEBARS_VALUE_ITERATOR_SIZE sizeof(struct handlebars_value_iterator)

// }}} Prototypes & Variables

// {{{ Constructors and Destructors

struct handlebars_value * handlebars_value_ctor(struct handlebars_context * ctx)
{
    struct handlebars_value * value = handlebars_talloc_zero(ctx, struct handlebars_value);
    HANDLEBARS_MEMCHECK(value, ctx);
    return value;
}

void handlebars_value_dtor(struct handlebars_value * value)
{
    // Release old value
    switch( value->type ) {
        case HANDLEBARS_VALUE_TYPE_ARRAY:
            handlebars_stack_delref(value->v.stack);
            break;
        case HANDLEBARS_VALUE_TYPE_MAP:
            handlebars_map_delref(value->v.map);
            break;
        case HANDLEBARS_VALUE_TYPE_STRING:
            handlebars_string_delref(value->v.string);
            break;
        case HANDLEBARS_VALUE_TYPE_USER:
            handlebars_user_delref(value->v.user);
            break;
        case HANDLEBARS_VALUE_TYPE_PTR:
            handlebars_ptr_delref(value->v.ptr);
            break;
        case HANDLEBARS_VALUE_TYPE_CLOSURE:
            handlebars_closure_delref(value->v.closure);
            break;
        default:
            // do nothing
            break;
    }

    // Initialize to null
    value->type = HANDLEBARS_VALUE_TYPE_NULL;
    memset(&value->v, 0, sizeof(value->v));

#ifdef HANDLEBARS_HAVE_VALGRIND
   VALGRIND_MAKE_MEM_UNDEFINED(&value->v, HANDLEBARS_VALUE_INTERNALS_SIZE);
#endif
}

struct handlebars_value * handlebars_value_init(struct handlebars_value * value)
{
    memset(value, 0, sizeof(struct handlebars_value));
    return value;
}

void handlebars_value_cleanup(struct handlebars_value * const * value_pp)
{
    assert(value_pp != NULL);
    assert(*value_pp != NULL);

    struct handlebars_value * value = *value_pp;

    // Release values that remain live when a scope exits early.
    if (value->type != HANDLEBARS_VALUE_TYPE_NULL) {
        handlebars_value_dtor(value);
    }

#ifdef HANDLEBARS_HAVE_VALGRIND
   VALGRIND_MAKE_MEM_UNDEFINED(value, HANDLEBARS_VALUE_SIZE);
#endif
}

// }}} Constructors and Destructors

// {{{ Getters

enum handlebars_value_type handlebars_value_get_type(struct handlebars_value * value)
{
	if( value->type == HANDLEBARS_VALUE_TYPE_USER ) {
		return handlebars_value_get_handlers(value)->type(value);
	} else {
		return value->type;
	}
}

enum handlebars_value_type handlebars_value_get_real_type(struct handlebars_value * value)
{
    return value->type;
}

unsigned char handlebars_value_get_flags(struct handlebars_value * value)
{
    return value->flags;
}

const struct handlebars_value_handlers * handlebars_value_get_handlers(struct handlebars_value * value)
{
    assert(value->type == HANDLEBARS_VALUE_TYPE_USER);
    return value->v.user->handlers;
}

struct handlebars_map * handlebars_value_get_map(struct handlebars_value * value)
{
    if (value->type == HANDLEBARS_VALUE_TYPE_MAP) {
        return value->v.map;
    } else {
        return NULL;
    }
}

bool handlebars_value_ptr_try_get(
    struct handlebars_value * value,
    const char * typ,
    void ** result
)
{
    *result = NULL;
    if( unlikely(value->type != HANDLEBARS_VALUE_TYPE_PTR) ) {
        return false;
    }
    return handlebars_ptr_try_get(value->v.ptr, typ, result);
}

void * handlebars_value_get_ptr_ex(struct handlebars_value * value, const char * typ)
{
    if (value->type == HANDLEBARS_VALUE_TYPE_PTR) {
        return handlebars_ptr_get_ptr_ex(value->v.ptr, typ);
    } else {
        fprintf(stderr, "Failed to retrieve ptr from type: %s\n", handlebars_value_type_readable(value->type));
        abort();
    }
}

struct handlebars_stack * handlebars_value_get_stack(struct handlebars_value * value)
{
    if (value->type == HANDLEBARS_VALUE_TYPE_ARRAY) {
        return value->v.stack;
    } else {
        return NULL;
    }
}

struct handlebars_string * handlebars_value_get_string(struct handlebars_value * value)
{
    if (value->type == HANDLEBARS_VALUE_TYPE_STRING) {
        return value->v.string;
    } else {
        return NULL;
    }
}

struct handlebars_user * handlebars_value_get_user(struct handlebars_value * value)
{
    if (value->type == HANDLEBARS_VALUE_TYPE_USER) {
        return value->v.user;
    } else {
        return NULL;
    }
}

struct handlebars_closure * handlebars_value_get_closure(struct handlebars_value * value)
{
    if (value->type == HANDLEBARS_VALUE_TYPE_CLOSURE) {
        return value->v.closure;
    } else {
        return NULL;
    }
}

const char * handlebars_value_get_strval(struct handlebars_value * value)
{
    if( value->type == HANDLEBARS_VALUE_TYPE_STRING ) {
        return hbs_str_val(value->v.string);
    } else {
        return NULL;
    }
}

size_t handlebars_value_get_strlen(struct handlebars_value * value)
{
	if( value->type == HANDLEBARS_VALUE_TYPE_STRING ) {
		return hbs_str_len(value->v.string);
	}

	return 0;
}

bool handlebars_value_get_boolval(struct handlebars_value * value)
{
    switch( value->type ) {
        case HANDLEBARS_VALUE_TYPE_NULL:
            return false;
        case HANDLEBARS_VALUE_TYPE_TRUE:
            return true;
        case HANDLEBARS_VALUE_TYPE_FALSE:
            return false;
        case HANDLEBARS_VALUE_TYPE_FLOAT:
            return value->v.dval != 0;
        case HANDLEBARS_VALUE_TYPE_INTEGER:
            return value->v.lval != 0;
        case HANDLEBARS_VALUE_TYPE_STRING:
            return hbs_str_len(value->v.string) != 0 && strcmp(hbs_str_val(value->v.string), "0") != 0;
        case HANDLEBARS_VALUE_TYPE_ARRAY:
            return handlebars_stack_count(value->v.stack) != 0;
        case HANDLEBARS_VALUE_TYPE_MAP:
            return handlebars_map_count(value->v.map) > 0;
        case HANDLEBARS_VALUE_TYPE_USER:
            return handlebars_value_count(value) != 0;
        default:
            return false;
    }
}

long handlebars_value_get_intval(struct handlebars_value * value)
{
	if( value->type == HANDLEBARS_VALUE_TYPE_INTEGER ) {
        return value->v.lval;
	}

	return 0;
}

double handlebars_value_get_floatval(struct handlebars_value * value)
{
	if( value->type == HANDLEBARS_VALUE_TYPE_FLOAT ) {
        return value->v.dval;
	}

	return 0;
}

// }}} Getters

// {{{ Conversion

struct handlebars_string * handlebars_value_to_string(
    struct handlebars_value * value,
    struct handlebars_context * context
) {
    switch( value->type ) {
        case HANDLEBARS_VALUE_TYPE_STRING:
            handlebars_string_addref(value->v.string);
            return value->v.string;
        case HANDLEBARS_VALUE_TYPE_INTEGER:
            return handlebars_string_asprintf(context, "%ld", value->v.lval);
        case HANDLEBARS_VALUE_TYPE_FLOAT:
            return handlebars_string_asprintf(context, "%g", value->v.dval);
        case HANDLEBARS_VALUE_TYPE_TRUE:
            return handlebars_string_ctor(context, HBS_STRL("true"));
        case HANDLEBARS_VALUE_TYPE_FALSE:
            return handlebars_string_ctor(context, HBS_STRL("false"));
        default:
            return handlebars_string_init(context, 0);
    }
}

static void handlebars_value_convert_walk(
    struct handlebars_value * value,
    bool recurse,
    struct handlebars_value_traversal * state
)
{
    switch( value->type ) {
        case HANDLEBARS_VALUE_TYPE_USER:
            if (handlebars_value_get_handlers(value)->convert) {
                handlebars_value_traversal_enter(state, value);
                handlebars_value_get_handlers(value)->convert(value, recurse);
                handlebars_value_traversal_leave(state);
            }
            break;
        case HANDLEBARS_VALUE_TYPE_MAP:
        case HANDLEBARS_VALUE_TYPE_ARRAY: {
            HANDLEBARS_VALUE_ITERATOR_DECL(iter);

            handlebars_value_traversal_enter(state, value);
            if( handlebars_value_iterator_init_ex(
                iter,
                HBS_VALUE_ITERATOR_CURRENT(iter),
                value,
                state->context
            ) ) {
                do {
                    handlebars_value_convert_walk(iter->cur, recurse, state);
                } while( handlebars_value_iterator_next(iter) );
            }
            handlebars_value_iterator_close(iter);
            handlebars_value_traversal_leave(state);
            break;
        }
        default:
            // do nothing
            break;
    }
}

void handlebars_value_convert_ex(struct handlebars_value * value, bool recurse)
{
    struct handlebars_value_traversal state;

    state.active_count = 0;
    state.output = NULL;

    switch( value->type ) {
        case HANDLEBARS_VALUE_TYPE_ARRAY:
            state.context = handlebars_stack_get_context(value->v.stack);
            break;
        case HANDLEBARS_VALUE_TYPE_MAP:
            state.context = handlebars_map_get_context(value->v.map);
            break;
        case HANDLEBARS_VALUE_TYPE_USER:
            state.context = value->v.user->ctx;
            break;
        default:
            return;
    }

    handlebars_value_convert_walk(value, recurse, &state);
}

bool handlebars_value_eq(
    struct handlebars_value * value,
    struct handlebars_value * value2
) {
    if (value->type != value2->type) {
        return false;
    }

    switch( value->type ) {
        case HANDLEBARS_VALUE_TYPE_NULL:
        case HANDLEBARS_VALUE_TYPE_TRUE:
        case HANDLEBARS_VALUE_TYPE_FALSE:
            return true;

        case HANDLEBARS_VALUE_TYPE_FLOAT:
            return value->v.dval == value2->v.dval;

        case HANDLEBARS_VALUE_TYPE_INTEGER:
            return value->v.lval == value2->v.lval;

        case HANDLEBARS_VALUE_TYPE_STRING:
            return value->v.string == value2->v.string || handlebars_string_eq(value->v.string, value2->v.string);

        // these only test pointer equality
        case HANDLEBARS_VALUE_TYPE_ARRAY:
            return value->v.stack == value2->v.stack;
        case HANDLEBARS_VALUE_TYPE_MAP:
            return value->v.map == value2->v.map;
        case HANDLEBARS_VALUE_TYPE_USER:
            return value->v.user == value2->v.user;
        case HANDLEBARS_VALUE_TYPE_HELPER:
            return value->v.helper == value2->v.helper;
        case HANDLEBARS_VALUE_TYPE_PTR:
            return value->v.ptr == value2->v.ptr;
        case HANDLEBARS_VALUE_TYPE_CLOSURE:
            return value->v.closure == value2->v.closure;

        default:
            fprintf(stderr, "Unsupported value comparison of type %s (%d)", handlebars_value_type_readable(value->type), value->type);
            abort();
    }
}

static struct handlebars_string * handlebars_value_expression_append_walk(
    struct handlebars_context * context,
    struct handlebars_value * value,
    struct handlebars_string * string,
    bool escape,
    struct handlebars_value_traversal * state
);

static struct handlebars_string * handlebars_value_expression_append_array_walk(
    struct handlebars_context * context,
    struct handlebars_value * value,
    struct handlebars_string * string,
    bool escape,
    struct handlebars_value_traversal * state
) {
    HANDLEBARS_VALUE_ITERATOR_DECL(iter);
    bool first = true;

    handlebars_value_traversal_enter(state, value);
    if( handlebars_value_iterator_init_ex(
        iter,
        HBS_VALUE_ITERATOR_CURRENT(iter),
        value,
        state->context
    ) ) {
        do {
            if( !first ) {
                string = handlebars_string_append(context, string, HBS_STRL(","));
            }
            string = handlebars_value_expression_append_walk(context, iter->cur, string, escape, state);
            first = false;
        } while( handlebars_value_iterator_next(iter) );
    }
    handlebars_value_iterator_close(iter);
    handlebars_value_traversal_leave(state);
    return string;
}

static struct handlebars_string * handlebars_value_expression_append_walk(
    struct handlebars_context * context,
    struct handlebars_value * value,
    struct handlebars_string * string,
    bool escape,
    struct handlebars_value_traversal * state
) {
    switch( value->type ) {
        case HANDLEBARS_VALUE_TYPE_TRUE:
            string = handlebars_string_append(context, string, HBS_STRL("true"));
            break;

        case HANDLEBARS_VALUE_TYPE_FALSE:
            string = handlebars_string_append(context, string, HBS_STRL("false"));
            break;

        case HANDLEBARS_VALUE_TYPE_FLOAT:
            string = handlebars_string_asprintf_append(context, string, "%g", value->v.dval);
            break;

        case HANDLEBARS_VALUE_TYPE_INTEGER:
            string = handlebars_string_asprintf_append(context, string, "%ld", value->v.lval);
            break;

        case HANDLEBARS_VALUE_TYPE_STRING:
            if( escape && !(value->flags & HANDLEBARS_VALUE_FLAG_SAFE_STRING) ) {
                string = handlebars_string_htmlspecialchars_append(context, string, HBS_STR_STRL(value->v.string));
            } else {
                string = handlebars_string_append_str(context, string, value->v.string);
            }
            break;

        case HANDLEBARS_VALUE_TYPE_USER:
            if( handlebars_value_get_type(value) != HANDLEBARS_VALUE_TYPE_ARRAY ) {
                break;
            }
            return handlebars_value_expression_append_array_walk(context, value, string, escape, state);

        case HANDLEBARS_VALUE_TYPE_ARRAY:
            return handlebars_value_expression_append_array_walk(context, value, string, escape, state);

        default:
            // nothing
            break;
    }

    return string;
}

struct handlebars_string * handlebars_value_expression(
    struct handlebars_context * context,
    struct handlebars_value * value,
    bool escape
) {
    struct handlebars_error * error = context->e;
    jmp_buf * volatile previous = error->jmp;
    struct handlebars_value_traversal state;
    void * owner = handlebars_talloc_zero_size(context, 1);
    struct handlebars_string * output;
    jmp_buf buf;

    state.context = context;
    state.active_count = 0;
    state.output = NULL;
    HANDLEBARS_MEMCHECK(owner, context);
    if( handlebars_setjmp_ex(context, &buf) ) {
        error->jmp = previous;
        handlebars_talloc_free(owner);
        handlebars_value_rethrow(context, previous);
    }

    output = talloc_steal(owner, handlebars_string_init(context, 0));
    output = handlebars_value_expression_append_walk(context, value, output, escape, &state);
    output = talloc_steal(context, output);
    error->jmp = previous;
    handlebars_talloc_free(owner);
    return output;
}

static struct handlebars_string * handlebars_value_expression_append_array(
    struct handlebars_context * context,
    struct handlebars_value * value,
    struct handlebars_string * string,
    bool escape
) {
    struct handlebars_error * error = context->e;
    jmp_buf * volatile previous = error->jmp;
    struct handlebars_value_traversal state;
    void * owner = handlebars_talloc_zero_size(context, 1);
    struct handlebars_string * suffix;
    jmp_buf buf;

    state.context = context;
    state.active_count = 0;
    state.output = NULL;
    HANDLEBARS_MEMCHECK(owner, context);
    if( handlebars_setjmp_ex(context, &buf) ) {
        error->jmp = previous;
        handlebars_talloc_free(owner);
        handlebars_value_rethrow(context, previous);
    }

    suffix = talloc_steal(owner, handlebars_string_init(context, 0));
    suffix = handlebars_value_expression_append_array_walk(context, value, suffix, escape, &state);
    string = handlebars_string_append_str(context, string, suffix);
    error->jmp = previous;
    handlebars_talloc_free(owner);
    return string;
}

struct handlebars_string * handlebars_value_expression_append(
    struct handlebars_context * context,
    struct handlebars_value * value,
    struct handlebars_string * string,
    bool escape
) {
    if( value->type == HANDLEBARS_VALUE_TYPE_ARRAY ) {
        return handlebars_value_expression_append_array(context, value, string, escape);
    }
    if( value->type == HANDLEBARS_VALUE_TYPE_USER ) {
        if( handlebars_value_get_type(value) == HANDLEBARS_VALUE_TYPE_ARRAY ) {
            return handlebars_value_expression_append_array(context, value, string, escape);
        }
        return string;
    }
    return handlebars_value_expression_append_walk(context, value, string, escape, NULL);
}

// }}} Conversion

// {{{ Mutators

void handlebars_value_null(struct handlebars_value * value)
{
    if( value->type != HANDLEBARS_VALUE_TYPE_NULL ) {
        handlebars_value_dtor(value);
    }
}

void handlebars_value_boolean(struct handlebars_value * value, bool bval)
{
    handlebars_value_null(value);
    value->type = bval ? HANDLEBARS_VALUE_TYPE_TRUE : HANDLEBARS_VALUE_TYPE_FALSE;
}

void handlebars_value_integer(struct handlebars_value * value, long lval)
{
    handlebars_value_null(value);
    value->type = HANDLEBARS_VALUE_TYPE_INTEGER;
    value->v.lval = lval;
}

void handlebars_value_float(struct handlebars_value * value, double dval)
{
    handlebars_value_null(value);
    value->type = HANDLEBARS_VALUE_TYPE_FLOAT;
    value->v.dval = dval;
}

void handlebars_value_str(struct handlebars_value * value, struct handlebars_string * string)
{
    handlebars_string_addref(string);
    handlebars_value_null(value);
    value->type = HANDLEBARS_VALUE_TYPE_STRING;
    value->v.string = string;
}

void handlebars_value_ptr(struct handlebars_value * value, struct handlebars_ptr * ptr)
{
    handlebars_ptr_addref(ptr);
    handlebars_value_null(value);
    value->type = HANDLEBARS_VALUE_TYPE_PTR;
    value->v.ptr = ptr;
}

void handlebars_value_user(struct handlebars_value * value, struct handlebars_user * user)
{
    handlebars_user_addref(user);
    handlebars_value_null(value);
    value->type = HANDLEBARS_VALUE_TYPE_USER;
    value->v.user = user;
}

void handlebars_value_map(struct handlebars_value * value, struct handlebars_map * map)
{
    handlebars_map_addref(map);
    handlebars_value_null(value);
    value->type = HANDLEBARS_VALUE_TYPE_MAP;
    value->v.map = map;
}

void handlebars_value_array(struct handlebars_value * value, struct handlebars_stack * stack)
{
    handlebars_stack_addref(stack);
    handlebars_value_null(value);
    value->type = HANDLEBARS_VALUE_TYPE_ARRAY;
    value->v.stack = stack;
}

void handlebars_value_helper(struct handlebars_value * value, handlebars_helper_func helper)
{
    handlebars_value_null(value);
    value->type = HANDLEBARS_VALUE_TYPE_HELPER;
    value->v.helper = helper;
}

void handlebars_value_closure(struct handlebars_value * value, struct handlebars_closure * closure)
{
    handlebars_closure_addref(closure);
    handlebars_value_null(value);
    value->type = HANDLEBARS_VALUE_TYPE_CLOSURE;
    value->v.closure = closure;
}

void handlebars_value_value(struct handlebars_value * dest, struct handlebars_value * src)
{
    if( dest == src ) {
        return;
    }

    handlebars_value_null(dest);
    *dest = *src;
    switch( dest->type ) {
        case HANDLEBARS_VALUE_TYPE_ARRAY:
            handlebars_stack_addref(dest->v.stack);
            break;
        case HANDLEBARS_VALUE_TYPE_MAP:
            handlebars_map_addref(dest->v.map);
            break;
        case HANDLEBARS_VALUE_TYPE_STRING:
            handlebars_string_addref(dest->v.string);
            break;
        case HANDLEBARS_VALUE_TYPE_USER:
            handlebars_user_addref(dest->v.user);
            break;
        case HANDLEBARS_VALUE_TYPE_PTR:
            handlebars_ptr_addref(dest->v.ptr);
            break;
        case HANDLEBARS_VALUE_TYPE_CLOSURE:
            handlebars_closure_addref(dest->v.closure);
            break;
        default:
            // do nothing
            break;
    }
}

void handlebars_value_set_flag(
    struct handlebars_value * value,
    enum handlebars_value_flags flag
) {
    value->flags |= flag;
}

// }}} Mutators

// {{{ Misc

bool handlebars_value_is_callable(struct handlebars_value * value)
{
    return handlebars_value_get_type(value) == HANDLEBARS_VALUE_TYPE_HELPER || value->type == HANDLEBARS_VALUE_TYPE_CLOSURE;
}

bool handlebars_value_is_empty(struct handlebars_value * value)
{
    return !handlebars_value_get_boolval(value);
}

bool handlebars_value_is_scalar(struct handlebars_value * value)
{
    switch( value->type ) {
        case HANDLEBARS_VALUE_TYPE_NULL:
        case HANDLEBARS_VALUE_TYPE_TRUE:
        case HANDLEBARS_VALUE_TYPE_FALSE:
        case HANDLEBARS_VALUE_TYPE_FLOAT:
        case HANDLEBARS_VALUE_TYPE_INTEGER:
        case HANDLEBARS_VALUE_TYPE_STRING:
            return true;
        default:
            return false;
    }
}

long handlebars_value_count(struct handlebars_value * value)
{
    switch( value->type ) {
        case HANDLEBARS_VALUE_TYPE_ARRAY:
            return handlebars_stack_count(value->v.stack);
        case HANDLEBARS_VALUE_TYPE_MAP:
            return handlebars_map_count(value->v.map);
        case HANDLEBARS_VALUE_TYPE_USER:
            return handlebars_value_get_handlers(value)->count(value);
        default:
            return -1;
    }
}

// }}} Misc

// {{{ Array

struct handlebars_value_mutation_state {
    struct handlebars_value * value;
    struct handlebars_value * child;
    struct handlebars_string * key;
    size_t index;
};

typedef void (*handlebars_value_mutation_func)(
    struct handlebars_value_mutation_state * state
);

static HBS_ATTR_NORETURN void handlebars_value_mutation_type_abort(
    struct handlebars_value * value,
    enum handlebars_value_type expected
) {
    fprintf(
        stderr,
        "Unable to mutate value of native type %s as %s\n",
        handlebars_value_type_readable(value->type),
        handlebars_value_type_readable(expected)
    );
    abort();
}

static void handlebars_value_array_set_native(
    struct handlebars_value_mutation_state * state
) {
    state->value->v.stack = handlebars_stack_set(
        state->value->v.stack,
        state->index,
        state->child
    );
}

static void handlebars_value_array_push_native(
    struct handlebars_value_mutation_state * state
) {
    state->value->v.stack = handlebars_stack_push(
        state->value->v.stack,
        state->child
    );
}

static void handlebars_value_map_update_native(
    struct handlebars_value_mutation_state * state
) {
    state->value->v.map = handlebars_map_update(
        state->value->v.map,
        state->key,
        state->child
    );
}

static HBS_ATTR_NOINLINE enum handlebars_error_type
handlebars_value_mutation_try_guarded(
    struct handlebars_context * context,
    handlebars_value_mutation_func mutate,
    struct handlebars_value_mutation_state * state
) {
    struct handlebars_error * error = context->e;
    jmp_buf * volatile previous = error->jmp;
    enum handlebars_error_type volatile caught = HANDLEBARS_SUCCESS;
    jmp_buf buf;

    if( handlebars_setjmp_ex(context, &buf) ) {
        caught = error->num;
    } else {
        mutate(state);
    }

    error->jmp = previous;
    return caught;
}

static enum handlebars_error_type handlebars_value_mutation_try(
    struct handlebars_value_mutation_state * state,
    enum handlebars_value_type expected,
    handlebars_value_mutation_func mutate
) {
    struct handlebars_context * context;

    if( unlikely(state->value->type != expected) ) {
        return HANDLEBARS_TYPE_ERROR;
    }

    if( expected == HANDLEBARS_VALUE_TYPE_ARRAY ) {
        context = handlebars_stack_get_context(state->value->v.stack);
    } else {
        assert(expected == HANDLEBARS_VALUE_TYPE_MAP);
        context = handlebars_map_get_context(state->value->v.map);
    }

    handlebars_error_clear(context);
    return handlebars_value_mutation_try_guarded(context, mutate, state);
}

void handlebars_value_array_set(struct handlebars_value * value, size_t index, struct handlebars_value * child)
{
    if( unlikely(value->type != HANDLEBARS_VALUE_TYPE_ARRAY) ) {
        handlebars_value_mutation_type_abort(value, HANDLEBARS_VALUE_TYPE_ARRAY);
    }
    value->v.stack = handlebars_stack_set(value->v.stack, index, child);
}

enum handlebars_error_type handlebars_value_array_set_try(
    struct handlebars_value * value,
    size_t index,
    struct handlebars_value * child
) {
    struct handlebars_value_mutation_state state = {
        .value = value,
        .child = child,
        .index = index
    };

    return handlebars_value_mutation_try(
        &state,
        HANDLEBARS_VALUE_TYPE_ARRAY,
        handlebars_value_array_set_native
    );
}

void handlebars_value_array_push(struct handlebars_value * value, struct handlebars_value * child)
{
    if( unlikely(value->type != HANDLEBARS_VALUE_TYPE_ARRAY) ) {
        handlebars_value_mutation_type_abort(value, HANDLEBARS_VALUE_TYPE_ARRAY);
    }
    value->v.stack = handlebars_stack_push(value->v.stack, child);
}

enum handlebars_error_type handlebars_value_array_push_try(
    struct handlebars_value * value,
    struct handlebars_value * child
) {
    struct handlebars_value_mutation_state state = {
        .value = value,
        .child = child
    };

    return handlebars_value_mutation_try(
        &state,
        HANDLEBARS_VALUE_TYPE_ARRAY,
        handlebars_value_array_push_native
    );
}

struct handlebars_value * handlebars_value_array_find(
    struct handlebars_value * value,
    size_t index,
    struct handlebars_value * rv
) {
    struct handlebars_value * result = NULL;

    if( value->type == HANDLEBARS_VALUE_TYPE_USER ) {
        if( handlebars_value_get_type(value) == HANDLEBARS_VALUE_TYPE_ARRAY ) {
            result = handlebars_value_get_handlers(value)->array_find(value, index, rv);
        }
    } else if( value->type == HANDLEBARS_VALUE_TYPE_ARRAY ) {
        struct handlebars_value * tmp = handlebars_stack_get(value->v.stack, index);
        if (tmp) {
            handlebars_value_value(rv, tmp);
            result = rv;
        }
    }

    return result;
}

// }}} Array

// {{{ Map

struct handlebars_value * handlebars_value_map_find(struct handlebars_value * value, struct handlebars_string * key, struct handlebars_value * rv)
{
    struct handlebars_value * result = NULL;

    if( value->type == HANDLEBARS_VALUE_TYPE_USER ) {
        if( handlebars_value_get_type(value) == HANDLEBARS_VALUE_TYPE_MAP ) {
            result = handlebars_value_get_handlers(value)->map_find(value, key, rv);
        }
    } else if( value->type == HANDLEBARS_VALUE_TYPE_MAP ) {
        struct handlebars_value * tmp = handlebars_map_find(value->v.map, key);
        if (tmp) {
            result = rv;
            handlebars_value_value(result, tmp);
        }
    }

    return result;
}

struct handlebars_value * handlebars_value_map_str_find(struct handlebars_value * value, const char * key, size_t len, struct handlebars_value * rv)
{
    struct handlebars_value * result = NULL;

	if( value->type == HANDLEBARS_VALUE_TYPE_USER ) {
		if( handlebars_value_get_type(value) == HANDLEBARS_VALUE_TYPE_MAP ) {
            struct handlebars_string * str = handlebars_string_ctor(value->v.user->ctx, key, len);
            handlebars_string_addref(str);
			result = handlebars_value_get_handlers(value)->map_find(value, str, rv);
#ifdef HANDLEBARS_NO_REFCOUNT
            handlebars_talloc_free(str);
#else
            handlebars_string_delref(str);
#endif
		}
	} else if( value->type == HANDLEBARS_VALUE_TYPE_MAP ) {
        struct handlebars_value * tmp = handlebars_map_str_find(value->v.map, key, len);
        if (tmp) {
            result = rv;
            handlebars_value_value(result, tmp);
        }
    }

	return result;
}

void handlebars_value_map_update(struct handlebars_value * value, struct handlebars_string * key, struct handlebars_value * child)
{
    if( unlikely(value->type != HANDLEBARS_VALUE_TYPE_MAP) ) {
        handlebars_value_mutation_type_abort(value, HANDLEBARS_VALUE_TYPE_MAP);
    }
    value->v.map = handlebars_map_update(value->v.map, key, child);
}

enum handlebars_error_type handlebars_value_map_update_try(
    struct handlebars_value * value,
    struct handlebars_string * key,
    struct handlebars_value * child
) {
    struct handlebars_value_mutation_state state = {
        .value = value,
        .child = child,
        .key = key
    };

    return handlebars_value_mutation_try(
        &state,
        HANDLEBARS_VALUE_TYPE_MAP,
        handlebars_value_map_update_native
    );
}

// }}} Map

// {{{ Misc

struct handlebars_value * handlebars_value_call(struct handlebars_value * value, HANDLEBARS_HELPER_ARGS)
{
    assert(rv != NULL);

    switch (value->type) {
        case HANDLEBARS_VALUE_TYPE_HELPER:
            rv = value->v.helper(HANDLEBARS_HELPER_ARGS_PASSTHRU);
            break;

        case HANDLEBARS_VALUE_TYPE_CLOSURE:
            rv = handlebars_closure_call(value->v.closure, HANDLEBARS_HELPER_ARGS_PASSTHRU);
            break;

        case HANDLEBARS_VALUE_TYPE_USER:
            if (handlebars_value_get_handlers(value)->call) {
                rv = handlebars_value_get_handlers(value)->call(value, HANDLEBARS_HELPER_ARGS_PASSTHRU);
                break;
            }
            // fallthrough

        default:
            handlebars_throw(HBSCTX(vm), HANDLEBARS_ERROR, "Unable to call value of type: %s", handlebars_value_type_readable(value->type));
            break;
    }

    assert(rv != NULL);

    return rv;
}

static void handlebars_value_dump_append(
    struct handlebars_value * value,
    struct handlebars_value_traversal * state,
    size_t depth
)
{
    enum handlebars_value_type type = handlebars_value_get_type(value);
    long count;

#define HANDLEBARS_VALUE_DUMP_APPEND(...) \
    do { \
        state->output = handlebars_talloc_asprintf_append_buffer(state->output, __VA_ARGS__); \
        HANDLEBARS_MEMCHECK(state->output, state->context); \
    } while(0)

    switch( type ) {
        case HANDLEBARS_VALUE_TYPE_NULL:
            HANDLEBARS_VALUE_DUMP_APPEND("NULL");
            break;
        case HANDLEBARS_VALUE_TYPE_TRUE:
            HANDLEBARS_VALUE_DUMP_APPEND("boolean(true)");
            break;
        case HANDLEBARS_VALUE_TYPE_FALSE:
            HANDLEBARS_VALUE_DUMP_APPEND("boolean(false)");
            break;
        case HANDLEBARS_VALUE_TYPE_FLOAT:
            HANDLEBARS_VALUE_DUMP_APPEND("float(%g)", value->v.dval);
            break;
        case HANDLEBARS_VALUE_TYPE_INTEGER:
            HANDLEBARS_VALUE_DUMP_APPEND("integer(%ld)", value->v.lval);
            break;
        case HANDLEBARS_VALUE_TYPE_STRING:
            HANDLEBARS_VALUE_DUMP_APPEND("string(%.*s)", (int) hbs_str_len(value->v.string), hbs_str_val(value->v.string));
            break;
        case HANDLEBARS_VALUE_TYPE_ARRAY: {
            HANDLEBARS_VALUE_ITERATOR_DECL(iter);

            if( unlikely(depth >= HANDLEBARS_VALUE_MAX_DEPTH) ) {
                handlebars_throw(
                    state->context,
                    HANDLEBARS_ERROR,
                    "Value nesting exceeds the maximum depth of %d",
                    HANDLEBARS_VALUE_MAX_DEPTH
                );
            }
            handlebars_value_traversal_enter(state, value);
            count = handlebars_value_count(value);
            HANDLEBARS_VALUE_DUMP_APPEND("[%s", count ? "\n" : "");
            if( handlebars_value_iterator_init_ex(
                iter,
                HBS_VALUE_ITERATOR_CURRENT(iter),
                value,
                state->context
            ) ) {
                do {
                    HANDLEBARS_VALUE_DUMP_APPEND("%*s%zu => ", (int) ((depth + 1) * 4), "", iter->index);
                    handlebars_value_dump_append(iter->cur, state, depth + 1);
                    HANDLEBARS_VALUE_DUMP_APPEND("\n");
                } while( handlebars_value_iterator_next(iter) );
            }
            handlebars_value_iterator_close(iter);
            if( count ) {
                HANDLEBARS_VALUE_DUMP_APPEND("%*s", (int) (depth * 4), "");
            }
            HANDLEBARS_VALUE_DUMP_APPEND("]");
            handlebars_value_traversal_leave(state);
            break;
        }
        case HANDLEBARS_VALUE_TYPE_MAP: {
            HANDLEBARS_VALUE_ITERATOR_DECL(iter);

            if( unlikely(depth >= HANDLEBARS_VALUE_MAX_DEPTH) ) {
                handlebars_throw(
                    state->context,
                    HANDLEBARS_ERROR,
                    "Value nesting exceeds the maximum depth of %d",
                    HANDLEBARS_VALUE_MAX_DEPTH
                );
            }
            handlebars_value_traversal_enter(state, value);
            count = handlebars_value_count(value);
            HANDLEBARS_VALUE_DUMP_APPEND("{%s", count ? "\n" : "");
            if( handlebars_value_iterator_init_ex(
                iter,
                HBS_VALUE_ITERATOR_CURRENT(iter),
                value,
                state->context
            ) ) {
                do {
                    HANDLEBARS_VALUE_DUMP_APPEND(
                        "%*s%.*s => ",
                        (int) ((depth + 1) * 4),
                        "",
                        (int) hbs_str_len(iter->key),
                        hbs_str_val(iter->key)
                    );
                    handlebars_value_dump_append(iter->cur, state, depth + 1);
                    HANDLEBARS_VALUE_DUMP_APPEND("\n");
                } while( handlebars_value_iterator_next(iter) );
            }
            handlebars_value_iterator_close(iter);
            if( count ) {
                HANDLEBARS_VALUE_DUMP_APPEND("%*s", (int) (depth * 4), "");
            }
            HANDLEBARS_VALUE_DUMP_APPEND("}");
            handlebars_value_traversal_leave(state);
            break;
        }
        case HANDLEBARS_VALUE_TYPE_HELPER:
            HANDLEBARS_VALUE_DUMP_APPEND("(function, real type %d)", value->type);
            break;
        default:
            HANDLEBARS_VALUE_DUMP_APPEND("unknown type %d", value->type);
            break;
    }

#undef HANDLEBARS_VALUE_DUMP_APPEND
}

char * handlebars_value_dump(struct handlebars_value * value, struct handlebars_context * context, size_t depth)
{
    struct handlebars_error * error = context->e;
    jmp_buf * volatile previous = error->jmp;
    struct handlebars_value_traversal state;
    void * owner = handlebars_talloc_zero_size(context, 1);
    char * output;
    jmp_buf buf;

    state.context = context;
    state.active_count = 0;
    state.output = NULL;
    HANDLEBARS_MEMCHECK(owner, context);
    if( handlebars_setjmp_ex(context, &buf) ) {
        error->jmp = previous;
        handlebars_talloc_free(owner);
        handlebars_value_rethrow(context, previous);
    }

    state.output = talloc_steal(owner, handlebars_talloc_strdup(context, ""));
    HANDLEBARS_MEMCHECK(state.output, context);
    handlebars_value_dump_append(value, &state, depth);
    output = talloc_steal(context, state.output);
    error->jmp = previous;
    handlebars_talloc_free(owner);
    return output;
}

const char * handlebars_value_type_readable(enum handlebars_value_type type)
{
    switch (type) {
        case HANDLEBARS_VALUE_TYPE_NULL: return "null";
        case HANDLEBARS_VALUE_TYPE_TRUE: return "true";
        case HANDLEBARS_VALUE_TYPE_FALSE: return "false";
        case HANDLEBARS_VALUE_TYPE_INTEGER: return "integer";
        case HANDLEBARS_VALUE_TYPE_FLOAT: return "float";
        case HANDLEBARS_VALUE_TYPE_STRING: return "string";
        case HANDLEBARS_VALUE_TYPE_ARRAY: return "array";
        case HANDLEBARS_VALUE_TYPE_MAP: return "map";
        case HANDLEBARS_VALUE_TYPE_USER: return "user";
        case HANDLEBARS_VALUE_TYPE_PTR: return "ptr";
        case HANDLEBARS_VALUE_TYPE_HELPER: return "helper";
        case HANDLEBARS_VALUE_TYPE_CLOSURE: return "closure";
        default:
#ifdef HANDLEBARS_ENABLE_DEBUG
            fprintf(stderr, "Unknown value type %d", type);
            abort();
#else
            return "unknown";
#endif
    }
}

// }}} Misc

// {{{ Iteration

static bool handlebars_value_iterator_next_void(struct handlebars_value_iterator * it)
{
    (void) it;
    return false;
}

static void handlebars_value_iterator_close_stack(struct handlebars_value_iterator * it)
{
    handlebars_value_dtor(it->cur);
    if( it->usr != NULL ) {
        handlebars_stack_delref((struct handlebars_stack *) it->usr);
        it->usr = NULL;
    }
}

static bool handlebars_value_iterator_next_stack(struct handlebars_value_iterator * it)
{
    struct handlebars_stack * stack = (struct handlebars_stack *) it->usr;
    size_t count;

    assert(stack != NULL);

    count = handlebars_stack_count(stack);
    if( it->index + 1 >= count ) {
        return false;
    }

    it->index++;
    handlebars_value_value(it->cur, handlebars_stack_get(stack, it->index));
    return true;
}

static void handlebars_value_iterator_close_map(struct handlebars_value_iterator * it)
{
    handlebars_value_dtor(it->cur);
    if( it->usr != NULL ) {
        handlebars_map_iterator_release((struct handlebars_map *) it->usr);
        it->usr = NULL;
    }
    it->key = NULL;
}

static bool handlebars_value_iterator_next_map(struct handlebars_value_iterator * it)
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

static void handlebars_value_iterator_register(
    struct handlebars_value_iterator * it,
    struct handlebars_context * context
) {
    struct handlebars_error * error = context->e;

    /* There is nothing to unwind when the caller has no active boundary. */
    if( error->jmp == NULL ) {
        return;
    }

    assert(it->unwind_previous == NULL);
    it->unwind_target = error->jmp;
    it->unwind_next = error->iterator_cleanup;
    it->unwind_previous = &error->iterator_cleanup;
    if( it->unwind_next != NULL ) {
        it->unwind_next->unwind_previous = &it->unwind_next;
    }
    error->iterator_cleanup = it;
}

static void handlebars_value_iterator_unregister(struct handlebars_value_iterator * it)
{
    if( it->unwind_previous != NULL ) {
        *it->unwind_previous = it->unwind_next;
        if( it->unwind_next != NULL ) {
            it->unwind_next->unwind_previous = it->unwind_previous;
        }
    }

    it->unwind_next = NULL;
    it->unwind_previous = NULL;
    it->unwind_target = NULL;
}

bool handlebars_value_iterator_init_internal(
    struct handlebars_value_iterator * it,
    struct handlebars_value * current,
    struct handlebars_value * value
)
{
    struct handlebars_value * tmp;

    memset(it, 0, sizeof(*it));
    handlebars_value_init(current);
    it->cur = current;

    switch( value->type ) {
        case HANDLEBARS_VALUE_TYPE_ARRAY:
            if (handlebars_stack_count(value->v.stack) <= 0) {
                it->next = &handlebars_value_iterator_next_void;
                return false;
            }
            it->value = value;
            it->usr = value->v.stack;
            handlebars_stack_addref(value->v.stack);
            it->index = 0;
            handlebars_value_value(it->cur, handlebars_stack_get(value->v.stack, it->index));
            it->next = &handlebars_value_iterator_next_stack;
            it->close = &handlebars_value_iterator_close_stack;
            handlebars_value_iterator_register(it, handlebars_stack_get_context(value->v.stack));
            return true;

        case HANDLEBARS_VALUE_TYPE_MAP: {
            struct handlebars_map * map = value->v.map;

            if (handlebars_map_count(value->v.map) <= 0) {
                it->next = &handlebars_value_iterator_next_void;
                return false;
            }

            it->value = value;
            it->index = 0;
            it->usr = map;
            it->next = &handlebars_value_iterator_next_map;
            it->close = &handlebars_value_iterator_close_map;
            handlebars_map_iterator_acquire(map);
            handlebars_value_iterator_register(it, handlebars_map_get_context(map));

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

        case HANDLEBARS_VALUE_TYPE_USER: {
            bool result;

            it->user = value->v.user;
            handlebars_user_addref(it->user);
            handlebars_value_iterator_register(it, it->user->ctx);
            result = handlebars_value_get_handlers(value)->iterator(it, value);
            if( !result ) {
                handlebars_value_iterator_close(it);
            }
            return result;
        }

        default:
            it->next = &handlebars_value_iterator_next_void;
            break;
    }

    return false;
}

static bool handlebars_value_iterator_init_ex(
    struct handlebars_value_iterator * it,
    struct handlebars_value * current,
    struct handlebars_value * value,
    struct handlebars_context * unwind_context
) {
    bool result = handlebars_value_iterator_init_internal(it, current, value);

    if( result ) {
        handlebars_value_iterator_unregister(it);
        handlebars_value_iterator_register(it, unwind_context);
    }
    return result;
}

bool handlebars_value_iterator_next(
    struct handlebars_value_iterator * it
) {
    bool result;

    assert(it->next != NULL);
    result = it->next(it);
    if( !result ) {
        handlebars_value_iterator_close(it);
    }
    return result;
};

void handlebars_value_iterator_close(struct handlebars_value_iterator * it)
{
    void (*close)(struct handlebars_value_iterator * it) = it->close;
    struct handlebars_user * user;

    handlebars_value_iterator_unregister(it);
    it->close = NULL;
    if( close != NULL ) {
        close(it);
    }
    user = it->user;
    it->user = NULL;
    if( user != NULL ) {
        handlebars_user_delref(user);
    }
    it->next = &handlebars_value_iterator_next_void;
}

void handlebars_value_iterator_unwind(struct handlebars_error * error, jmp_buf * target)
{
    struct handlebars_value_iterator * it = error->iterator_cleanup;

    while( it != NULL ) {
        struct handlebars_value_iterator * next = it->unwind_next;

        if( it->unwind_target == target ) {
            handlebars_value_iterator_close(it);
        }
        it = next;
    }
}

void handlebars_value_iterator_cleanup(struct handlebars_value_iterator * const * it)
{
    if( it != NULL && *it != NULL ) {
        handlebars_value_iterator_close(*it);
    }
}

// }}} Iteration

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: fdm=marker
 * vim: et sw=4 ts=4
 */
