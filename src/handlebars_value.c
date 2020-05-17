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
#include <string.h>
#include <talloc.h>

#include "handlebars.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"

#include "handlebars_map.h"
#include "handlebars_stack.h"
#include "handlebars_value.h"
#include "handlebars_value_handlers.h"

#ifndef HANDLEBARS_NO_REFCOUNT
#include "handlebars_rc.h"
#endif

#undef CONTEXT
#define CONTEXT HBSCTX(value->ctx)



// {{{ Prototypes & Variables

//! Internal value union
union handlebars_value_internals
{
    long lval;
    double dval;
    struct handlebars_string * string;
    struct handlebars_map * map;
    struct handlebars_stack * stack;
    struct handlebars_user * user;
    void * ptr;
    handlebars_helper_func helper;
    struct handlebars_options * options;
};

//! Main value struct
struct handlebars_value
{
    //! The type of value from enum #handlebars_value_type
	enum handlebars_value_type type;

#ifndef HANDLEBARS_NO_REFCOUNT
    struct handlebars_rc rc;
#endif

    //! Bitwise value flags from enum #handlebars_value_flags
    unsigned long flags;

    //! Internal value union
    union handlebars_value_internals v;

    //! Handlebars context, used for error handling and memory allocation
	struct handlebars_context * ctx;
};

const size_t HANDLEBARS_VALUE_SIZE = sizeof(struct handlebars_value);
const size_t HANDLEBARS_VALUE_INTERNALS_SIZE = sizeof(union handlebars_value_internals);

// }}} Prototypes & Variables

// {{{ Reference Counting

#ifndef HANDLEBARS_NO_REFCOUNT
static void value_rc_dtor(struct handlebars_rc * rc)
{
    struct handlebars_value * value = talloc_get_type_abort(hbs_container_of(rc, struct handlebars_value, rc), struct handlebars_value);
    handlebars_value_dtor(value);
    handlebars_talloc_free(value);
}
#endif

void handlebars_value_addref(struct handlebars_value * value)
{
#ifndef HANDLEBARS_NO_REFCOUNT
    handlebars_rc_addref(&value->rc);
#endif
}

void handlebars_value_delref(struct handlebars_value * value)
{
#ifndef HANDLEBARS_NO_REFCOUNT
    handlebars_rc_delref(&value->rc);
#endif
}

// }}} Reference Counting

// {{{ Constructors and Destructors

struct handlebars_value * handlebars_value_ctor(struct handlebars_context * ctx)
{
    struct handlebars_value * value = handlebars_talloc_zero(ctx, struct handlebars_value);
    HANDLEBARS_MEMCHECK(value, ctx);
    value->ctx = ctx;
    value->flags = HANDLEBARS_VALUE_FLAG_HEAP_ALLOCATED;
#ifndef HANDLEBARS_NO_REFCOUNT
    handlebars_rc_init(&value->rc, value_rc_dtor);
#endif
    return value;
}

void handlebars_value_dtor(struct handlebars_value * value)
{
    unsigned long restore_flags = 0;

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
            handlebars_talloc_free(value->v.ptr);
            break;
        default:
            // do nothing
            break;
    }

    if( value->flags & HANDLEBARS_VALUE_FLAG_HEAP_ALLOCATED ) {
        talloc_free_children(value);
        restore_flags = HANDLEBARS_VALUE_FLAG_HEAP_ALLOCATED;
    }

    // Initialize to null
    value->type = HANDLEBARS_VALUE_TYPE_NULL;
    memset(&value->v, 0, sizeof(value->v));
    value->flags = restore_flags;
}

struct handlebars_value * handlebars_value_copy(struct handlebars_value * value)
{
    struct handlebars_value * new_value = NULL;

    assert(value != NULL);

    switch( value->type ) {
        case HANDLEBARS_VALUE_TYPE_STRING:
            new_value = handlebars_value_ctor(CONTEXT);
            handlebars_value_str(new_value, value->v.string);
            new_value->flags = (value->flags & HANDLEBARS_VALUE_FLAG_SAFE_STRING);
            break;
        case HANDLEBARS_VALUE_TYPE_ARRAY:
            new_value = handlebars_value_ctor(CONTEXT);
            handlebars_value_array_init(new_value, handlebars_value_count(value));
            HANDLEBARS_VALUE_FOREACH(value, child) {
                handlebars_value_array_push(new_value, handlebars_value_copy(child));
            } HANDLEBARS_VALUE_FOREACH_END();
            break;
        case HANDLEBARS_VALUE_TYPE_MAP: {
            struct handlebars_map * new_map;
            new_value = handlebars_value_ctor(CONTEXT);
            new_map = handlebars_map_ctor(CONTEXT, handlebars_value_count(value));
            // handlebars_value_map_init(new_value, handlebars_value_count(value));
            HANDLEBARS_VALUE_FOREACH_KV(value, key, child) {
                new_map = handlebars_map_update(new_map, key, handlebars_value_copy(child));
                // handlebars_map_update(new_value->v.map, key, handlebars_value_copy(child));
            } HANDLEBARS_VALUE_FOREACH_END();
            handlebars_value_map(new_value, new_map);
            break;
        }
        case HANDLEBARS_VALUE_TYPE_USER:
            new_value = handlebars_value_get_handlers(value)->copy(value);
            break;
        default:
            new_value = handlebars_value_ctor(CONTEXT);
            memcpy(&new_value->v, &value->v, sizeof(value->v));
            break;
    }

    return MC(new_value);
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

unsigned long handlebars_value_get_flags(struct handlebars_value * value)
{
    return value->flags;
}

const struct handlebars_value_handlers * handlebars_value_get_handlers(struct handlebars_value * value)
{
    if (value->type != HANDLEBARS_VALUE_TYPE_USER) {
        handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "Invalid type");
    }
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

void * handlebars_value_get_ptr(struct handlebars_value * value)
{
    if (value->type == HANDLEBARS_VALUE_TYPE_PTR) {
        return value->v.ptr;
    } else {
        return NULL;
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

struct handlebars_context * handlebars_value_get_ctx(struct handlebars_value * value)
{
    return value->ctx;
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

struct handlebars_string * handlebars_value_to_string(struct handlebars_value * value)
{
    switch( value->type ) {
        case HANDLEBARS_VALUE_TYPE_STRING:
            handlebars_string_addref(value->v.string);
            return value->v.string;
        case HANDLEBARS_VALUE_TYPE_INTEGER:
            return handlebars_string_asprintf(CONTEXT, "%ld", value->v.lval);
        case HANDLEBARS_VALUE_TYPE_FLOAT:
            return handlebars_string_asprintf(CONTEXT, "%g", value->v.dval);
        case HANDLEBARS_VALUE_TYPE_TRUE:
            return handlebars_string_ctor(CONTEXT, HBS_STRL("true"));
        case HANDLEBARS_VALUE_TYPE_FALSE:
            return handlebars_string_ctor(CONTEXT, HBS_STRL("false"));
        default:
            return handlebars_string_init(CONTEXT, 0);
    }
}

void handlebars_value_convert_ex(struct handlebars_value * value, bool recurse)
{
    switch( value->type ) {
        case HANDLEBARS_VALUE_TYPE_USER:
            handlebars_value_get_handlers(value)->convert(value, recurse);
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

struct handlebars_string * handlebars_value_expression(struct handlebars_value * value, bool escape)
{
    return handlebars_value_expression_append(handlebars_string_init(value->ctx, 0), value, escape);
}

struct handlebars_string * handlebars_value_expression_append(
    struct handlebars_string * string,
    struct handlebars_value * value,
    bool escape
) {
    bool first;

    switch( value->type ) {
        case HANDLEBARS_VALUE_TYPE_TRUE:
            string = handlebars_string_append(value->ctx, string, HBS_STRL("true"));
            break;

        case HANDLEBARS_VALUE_TYPE_FALSE:
            string = handlebars_string_append(value->ctx, string, HBS_STRL("false"));
            break;

        case HANDLEBARS_VALUE_TYPE_FLOAT:
            string = handlebars_string_asprintf_append(value->ctx, string, "%g", value->v.dval);
            break;

        case HANDLEBARS_VALUE_TYPE_INTEGER:
            string = handlebars_string_asprintf_append(value->ctx, string, "%ld", value->v.lval);
            break;

        case HANDLEBARS_VALUE_TYPE_STRING:
            if( escape && !(value->flags & HANDLEBARS_VALUE_FLAG_SAFE_STRING) ) {
                string = handlebars_string_htmlspecialchars_append(value->ctx, string, HBS_STR_STRL(value->v.string));
            } else {
                string = handlebars_string_append_str(value->ctx, string, value->v.string);
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
                    string = handlebars_string_append(value->ctx, string, HBS_STRL(","));
                }
                string = handlebars_value_expression_append(string, child, escape);
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

void handlebars_value_cstr_steal(struct handlebars_value * value, char * strval)
{
    handlebars_value_null(value);
    value->type = HANDLEBARS_VALUE_TYPE_STRING;
    value->v.string = /*talloc_steal(value,*/ handlebars_string_ctor(value->ctx, strval, strlen(strval))/*)*/;
    handlebars_talloc_free(strval);
    handlebars_string_addref(value->v.string);
}

void handlebars_value_cstrl(struct handlebars_value * value, const char * strval, size_t strlen)
{
    handlebars_value_null(value);
    value->type = HANDLEBARS_VALUE_TYPE_STRING;
    value->v.string = /*talloc_steal(value,*/ handlebars_string_ctor(value->ctx, strval, strlen)/*)*/;
    handlebars_string_addref(value->v.string);
}

void handlebars_value_ptr(struct handlebars_value * value, void * ptr)
{
    handlebars_value_null(value);
    value->type = HANDLEBARS_VALUE_TYPE_PTR;
    value->v.ptr = ptr;
}

void handlebars_value_user(struct handlebars_value * value, struct handlebars_user * user)
{
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

void handlebars_value_set_flag(
    struct handlebars_value * value,
    unsigned long flag
) {
    value->flags |= flag;
}

// }}} Mutators

// {{{ Misc

bool handlebars_value_is_callable(struct handlebars_value * value)
{
    return handlebars_value_get_type(value) == HANDLEBARS_VALUE_TYPE_HELPER;
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
            return (handlebars_value_handlers_get_count_fn(handlebars_value_get_handlers(value)))(value);
        default:
            return -1;
    }
}

// }}} Misc

// {{{ Array

void handlebars_value_array_init(struct handlebars_value * value, size_t capacity)
{
    handlebars_value_null(value);
    value->type = HANDLEBARS_VALUE_TYPE_ARRAY;
    value->v.stack = /*talloc_steal(value,*/ handlebars_stack_ctor(value->ctx, capacity)/*)*/;
    handlebars_stack_addref(value->v.stack);
}

void handlebars_value_array_set(struct handlebars_value * value, size_t index, struct handlebars_value * child)
{
    if (value->type != HANDLEBARS_VALUE_TYPE_ARRAY) {
        handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "Invalid type");
    }
    value->v.stack = handlebars_stack_set(value->v.stack, index, child);
}

void handlebars_value_array_push(struct handlebars_value * value, struct handlebars_value * child)
{
    if (value->type != HANDLEBARS_VALUE_TYPE_ARRAY) {
        handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "Invalid type");
    }
    value->v.stack = handlebars_stack_push(value->v.stack, child);
}

struct handlebars_value * handlebars_value_array_find(struct handlebars_value * value, size_t index)
{
    struct handlebars_value * child = NULL;

	if( value->type == HANDLEBARS_VALUE_TYPE_USER ) {
		if( handlebars_value_get_type(value) == HANDLEBARS_VALUE_TYPE_ARRAY ) {
			child = handlebars_value_get_handlers(value)->array_find(value, index);
		}
	} else if( value->type == HANDLEBARS_VALUE_TYPE_ARRAY ) {
        child = handlebars_stack_get(value->v.stack, index);
    }

    if (child) {
        handlebars_value_addref(child);
    }

	return child;
}

// }}} Array

// {{{ Map

void handlebars_value_map_init(struct handlebars_value * value, size_t capacity)
{
    handlebars_value_null(value);
    value->type = HANDLEBARS_VALUE_TYPE_MAP;
    value->v.map = /*talloc_steal(value,*/ handlebars_map_ctor(value->ctx, capacity)/*)*/;
    handlebars_map_addref(value->v.map);
}

struct handlebars_value * handlebars_value_map_find(struct handlebars_value * value, struct handlebars_string * key)
{
    struct handlebars_value * child = NULL;

    if( value->type == HANDLEBARS_VALUE_TYPE_USER ) {
        if( handlebars_value_get_type(value) == HANDLEBARS_VALUE_TYPE_MAP ) {
            child = handlebars_value_get_handlers(value)->map_find(value, key);
        }
    } else if( value->type == HANDLEBARS_VALUE_TYPE_MAP ) {
        child = handlebars_map_find(value->v.map, key);
    }

    if (child) {
        handlebars_value_addref(child);
    }

    return child;
}

struct handlebars_value * handlebars_value_map_str_find(struct handlebars_value * value, const char * key, size_t len)
{
    struct handlebars_value * ret = NULL;
    struct handlebars_string * str = handlebars_string_ctor(value->ctx, key, len);
    handlebars_string_addref(str);

	if( value->type == HANDLEBARS_VALUE_TYPE_USER ) {
		if( handlebars_value_get_type(value) == HANDLEBARS_VALUE_TYPE_MAP ) {
			ret = handlebars_value_get_handlers(value)->map_find(value, str);
		}
	} else if( value->type == HANDLEBARS_VALUE_TYPE_MAP ) {
        ret = handlebars_map_find(value->v.map, str);
    }

    if (ret) {
        handlebars_value_addref(ret);
    }

    handlebars_string_delref(str);
	return ret;
}

// }}} Map

// {{{ Misc

struct handlebars_value * handlebars_value_call(struct handlebars_value * value, HANDLEBARS_HELPER_ARGS)
{
    if( value->type == HANDLEBARS_VALUE_TYPE_HELPER ) {
        return value->v.helper(argc, argv, options);
    } else if( value->type == HANDLEBARS_VALUE_TYPE_USER && handlebars_value_get_handlers(value)->call ) {
        return handlebars_value_get_handlers(value)->call(value, argc, argv, options);
    } else {
        handlebars_throw(value->ctx, HANDLEBARS_ERROR, "Unable to call value of type %d", value->type);
    }
}

char * handlebars_value_dump(struct handlebars_value * value, size_t depth)
{
    char * buf = MC(handlebars_talloc_strdup(CONTEXT, ""));
    char indent[64];
    char indent2[64];

    memset(indent, 0, sizeof(indent));
    memset(indent, ' ', depth * 4); // @todo fix

    memset(indent2, 0, sizeof(indent2));
    memset(indent2, ' ', (depth + 1) * 4); // @todo fix

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
            buf = handlebars_talloc_asprintf_append_buffer(buf, "float(%f)", value->v.dval);
            break;
        case HANDLEBARS_VALUE_TYPE_INTEGER:
            buf = handlebars_talloc_asprintf_append_buffer(buf, "integer(%ld)", value->v.lval);
            break;
        case HANDLEBARS_VALUE_TYPE_STRING:
            buf = handlebars_talloc_asprintf_append_buffer(buf, "string(%.*s)", (int) hbs_str_len(value->v.string), hbs_str_val(value->v.string));
            break;
        case HANDLEBARS_VALUE_TYPE_ARRAY:
            buf = handlebars_talloc_asprintf_append_buffer(buf, "%s\n", "[");
            HANDLEBARS_VALUE_FOREACH_IDX(value, index, child) {
                char * tmp = handlebars_value_dump(child, depth + 1);
                buf = handlebars_talloc_asprintf_append_buffer(buf, "%s%zd => %s\n", indent2, index, tmp);
                handlebars_talloc_free(tmp);
            } HANDLEBARS_VALUE_FOREACH_END();
            buf = handlebars_talloc_asprintf_append_buffer(buf, "%s%s", indent, "]");
            break;
        case HANDLEBARS_VALUE_TYPE_MAP:
            buf = handlebars_talloc_asprintf_append_buffer(buf, "%s\n", "{");
            HANDLEBARS_VALUE_FOREACH_KV(value, key, child) {
                char * tmp = handlebars_value_dump(child, depth + 1);
                buf = handlebars_talloc_asprintf_append_buffer(buf, "%s%.*s => %s\n", indent2, (int) hbs_str_len(key), hbs_str_val(key), tmp);
                handlebars_talloc_free(tmp);
            } HANDLEBARS_VALUE_FOREACH_END();
            buf = handlebars_talloc_asprintf_append_buffer(buf, "%s%s", indent, "}");
            break;
        default:
            buf = handlebars_talloc_asprintf_append_buffer(buf, "unknown type %d", value->type);
            break;
    }

    return MC(buf);
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
    assert(it->current != NULL);

    it->current = NULL;

    if( it->index >= handlebars_stack_count(value->v.stack) - 1 ) {
        return false;
    }

    it->index++;
    it->current = handlebars_stack_get(value->v.stack, it->index);
    return true;
}

static bool handlebars_value_iterator_next_map(struct handlebars_value_iterator * it)
{
    assert(it->value != NULL);
    assert(it->value->type == HANDLEBARS_VALUE_TYPE_MAP);
    assert(it->current != NULL);

    it->current = NULL;

    if( it->index >= handlebars_map_count(it->value->v.map) - 1 ) {
        handlebars_map_set_is_in_iteration(it->value->v.map, false); // @todo we should restore the previous flag?
        return false;
    }

    it->index++;
    handlebars_map_get_kv_at_index(it->value->v.map, it->index, &it->key, &it->current);
    return true;
}

bool handlebars_value_iterator_init(struct handlebars_value_iterator * it, struct handlebars_value * value)
{
    memset(it, 0, sizeof(struct handlebars_value_iterator));

    // @TODO make sure this works for type_user?
    if (handlebars_value_count(value) <= 0) {
        it->next = &handlebars_value_iterator_next_void;
        return false;
    }

    switch( value->type ) {
        case HANDLEBARS_VALUE_TYPE_ARRAY:
            it->value = value;
            it->index = 0;
            it->length = handlebars_stack_count(value->v.stack);
            it->current = handlebars_stack_get(value->v.stack, it->index);
            it->next = &handlebars_value_iterator_next_stack;
            return true;

        case HANDLEBARS_VALUE_TYPE_MAP:
            handlebars_map_sparse_array_compact(value->v.map); // meh
            it->value = value;
            it->index = 0;
            it->length = handlebars_map_count(value->v.map);
            handlebars_map_get_kv_at_index(value->v.map, it->index, &it->key, &it->current);
            it->next = &handlebars_value_iterator_next_map;
            handlebars_map_set_is_in_iteration(value->v.map, true); // @todo we should store the result
            return true;

        case HANDLEBARS_VALUE_TYPE_USER:
            return handlebars_value_get_handlers(value)->iterator(it, value);

        default:
            it->next = &handlebars_value_iterator_next_void;
            //handlebars_context_throw(value->ctx, HANDLEBARS_ERROR, "Cannot iterator over type %d", value->type);
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
