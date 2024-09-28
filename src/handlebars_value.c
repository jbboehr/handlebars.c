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

    // Check refcount
    if (value->type != HANDLEBARS_VALUE_TYPE_NULL) {
        fprintf(stderr, "value %p was not destructed\n", value);
        abort();
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

void handlebars_value_convert_ex(struct handlebars_value * value, bool recurse)
{
    switch( value->type ) {
        case HANDLEBARS_VALUE_TYPE_USER:
            if (handlebars_value_get_handlers(value)->convert) {
                handlebars_value_get_handlers(value)->convert(value, recurse);
            }
            break;
        case HANDLEBARS_VALUE_TYPE_MAP:
        case HANDLEBARS_VALUE_TYPE_ARRAY:
            HANDLEBARS_VALUE_FOREACH(value, child) {
                handlebars_value_convert_ex(child, recurse);
            } HANDLEBARS_VALUE_FOREACH_END();
            break;
        default:
            // do nothing
            break;
    }
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

struct handlebars_string * handlebars_value_expression(
    struct handlebars_context * context,
    struct handlebars_value * value,
    bool escape
) {
    return handlebars_value_expression_append(context, value, handlebars_string_init(context, 0), escape);
}

struct handlebars_string * handlebars_value_expression_append(
    struct handlebars_context * context,
    struct handlebars_value * value,
    struct handlebars_string * string,
    bool escape
) {
    bool first;

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
            // fallthrough
        case HANDLEBARS_VALUE_TYPE_ARRAY:
            first = true;
            HANDLEBARS_VALUE_FOREACH(value, child) {
                if( !first ) {
                    string = handlebars_string_append(context, string, HBS_STRL(","));
                }
                string = handlebars_value_expression_append(context, child, string, escape);
                first = false;
            } HANDLEBARS_VALUE_FOREACH_END();
            break;

        default:
            // nothing
            break;
    }

    return string;
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

void handlebars_value_array_set(struct handlebars_value * value, size_t index, struct handlebars_value * child)
{
    assert(value->type == HANDLEBARS_VALUE_TYPE_ARRAY);
    value->v.stack = handlebars_stack_set(value->v.stack, index, child);
}

void handlebars_value_array_push(struct handlebars_value * value, struct handlebars_value * child)
{
    assert(value->type == HANDLEBARS_VALUE_TYPE_ARRAY);
    value->v.stack = handlebars_stack_push(value->v.stack, child);
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
            handlebars_string_delref(str);
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
    if( value->type != HANDLEBARS_VALUE_TYPE_MAP ) {
        // We don't have a context here... so just abort
        fprintf(stderr, "Unable to update map for value of type %d", value->type);
        abort();
    }

    value->v.map = handlebars_map_update(value->v.map, key, child);
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

char * handlebars_value_dump(struct handlebars_value * value, struct handlebars_context * context, size_t depth)
{
    char * buf = handlebars_talloc_strdup(context, "");
    char indent[0x80];
    char indent2[0x80];
    HANDLEBARS_MEMCHECK(buf, context);

    assert((depth + 1) * 4 < 0x80);

    size_t indent_len = (depth * 4) & 0x7F;
    memset(indent, ' ', indent_len);
    indent[indent_len] = 0;

    size_t indent2_len = ((depth + 1) * 4) & 0x7F;
    memset(indent2, ' ', indent2_len);
    indent2[indent2_len] = 0;

    switch( handlebars_value_get_type(value) ) {
        case HANDLEBARS_VALUE_TYPE_NULL:
            buf = handlebars_talloc_asprintf_append_buffer(buf, "NULL");
            break;
        case HANDLEBARS_VALUE_TYPE_TRUE:
            buf = handlebars_talloc_strndup_append_buffer(buf, HBS_STRL("boolean(true)"));
            break;
        case HANDLEBARS_VALUE_TYPE_FALSE:
            buf = handlebars_talloc_strndup_append_buffer(buf, HBS_STRL("boolean(false)"));
            break;
        case HANDLEBARS_VALUE_TYPE_FLOAT:
            buf = handlebars_talloc_asprintf_append_buffer(buf, "float(%g)", value->v.dval);
            break;
        case HANDLEBARS_VALUE_TYPE_INTEGER:
            buf = handlebars_talloc_asprintf_append_buffer(buf, "integer(%ld)", value->v.lval);
            break;
        case HANDLEBARS_VALUE_TYPE_STRING:
            buf = handlebars_talloc_asprintf_append_buffer(buf, "string(%.*s)", (int) hbs_str_len(value->v.string), hbs_str_val(value->v.string));
            break;
        case HANDLEBARS_VALUE_TYPE_ARRAY:
            buf = handlebars_talloc_asprintf_append_buffer(buf, "[%s", handlebars_value_count(value) ? "\n" : "");
            HANDLEBARS_VALUE_FOREACH_IDX(value, index, child) {
                char * tmp = handlebars_value_dump(child, context, depth + 1);
                buf = handlebars_talloc_asprintf_append_buffer(buf, "%s%zd => %s\n", indent2, index, tmp);
                handlebars_talloc_free(tmp);
            } HANDLEBARS_VALUE_FOREACH_END();
            buf = handlebars_talloc_asprintf_append_buffer(buf, "%s]", handlebars_value_count(value) ? indent : "");
            break;
        case HANDLEBARS_VALUE_TYPE_MAP:
            buf = handlebars_talloc_asprintf_append_buffer(buf, "{%s", handlebars_value_count(value) ? "\n" : "");
            HANDLEBARS_VALUE_FOREACH_KV(value, key, child) {
                char * tmp = handlebars_value_dump(child, context, depth + 1);
                buf = handlebars_talloc_asprintf_append_buffer(buf, "%s%.*s => %s\n", indent2, (int) hbs_str_len(key), hbs_str_val(key), tmp);
                handlebars_talloc_free(tmp);
            } HANDLEBARS_VALUE_FOREACH_END();
            buf = handlebars_talloc_asprintf_append_buffer(buf, "%s}", handlebars_value_count(value) ? indent : "");
            break;
        case HANDLEBARS_VALUE_TYPE_HELPER:
            buf = handlebars_talloc_asprintf_append_buffer(buf, "(function, real type %d)", value->type);
            break;
        default:
            buf = handlebars_talloc_asprintf_append_buffer(buf, "unknown type %d", value->type);
            break;
    }

    HANDLEBARS_MEMCHECK(buf, context);

    return buf;
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
    return false;
}

static bool handlebars_value_iterator_next_stack(struct handlebars_value_iterator * it)
{
    struct handlebars_value * value = it->value;

    assert(value != NULL);
    assert(value->type == HANDLEBARS_VALUE_TYPE_ARRAY);

    if( it->index >= handlebars_stack_count(value->v.stack) - 1 ) {
        handlebars_value_dtor(it->cur);
        return false;
    }

    it->index++;
    handlebars_value_value(it->cur, handlebars_stack_get(value->v.stack, it->index));
    return true;
}

static bool handlebars_value_iterator_next_map(struct handlebars_value_iterator * it)
{
    struct handlebars_value * tmp;

    assert(it->value != NULL);
    assert(it->value->type == HANDLEBARS_VALUE_TYPE_MAP);


    if( it->index >= handlebars_map_count(it->value->v.map) - 1 ) {
        handlebars_value_dtor(it->cur);
        handlebars_map_set_is_in_iteration(it->value->v.map, false);
        return false;
    }

    it->index++;
    handlebars_map_get_kv_at_index(it->value->v.map, it->index, &it->key, &tmp);
    handlebars_value_value(it->cur, tmp);
    return true;
}

bool handlebars_value_iterator_init(struct handlebars_value_iterator * it, struct handlebars_value * value)
{
    struct handlebars_value * tmp;

    memset(it, 0, HANDLEBARS_VALUE_ITERATOR_SIZE + HANDLEBARS_VALUE_SIZE);
    it->cur = (struct handlebars_value *) (void *) (((char *) it) + HANDLEBARS_VALUE_ITERATOR_SIZE);

    switch( value->type ) {
        case HANDLEBARS_VALUE_TYPE_ARRAY:
            if (handlebars_stack_count(value->v.stack) <= 0) {
                it->next = &handlebars_value_iterator_next_void;
                return false;
            }
            it->value = value;
            it->index = 0;
            handlebars_value_value(it->cur, handlebars_stack_get(value->v.stack, it->index));
            it->next = &handlebars_value_iterator_next_stack;
            return true;

        case HANDLEBARS_VALUE_TYPE_MAP:
            if (handlebars_map_count(value->v.map) <= 0) {
                it->next = &handlebars_value_iterator_next_void;
                return false;
            }
            handlebars_map_sparse_array_compact(value->v.map); // meh
            it->value = value;
            it->index = 0;
            handlebars_map_get_kv_at_index(value->v.map, it->index, &it->key, &tmp);
            handlebars_value_value(it->cur, tmp);
            it->next = &handlebars_value_iterator_next_map;
            if (handlebars_map_set_is_in_iteration(value->v.map, true)) {
                fprintf(stderr, "Nested map iteration is not currently supported [%s:%d]", __FILE__, __LINE__);
                abort();
            }
            return true;

        case HANDLEBARS_VALUE_TYPE_USER:
            return handlebars_value_get_handlers(value)->iterator(it, value);

        default:
            it->next = &handlebars_value_iterator_next_void;
            break;
    }

    return false;
}

bool handlebars_value_iterator_next(
    struct handlebars_value_iterator * it
) {
    assert(it->next != NULL);
    return it->next(it);
};

// }}} Iteration

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: fdm=marker
 * vim: et sw=4 ts=4
 */
