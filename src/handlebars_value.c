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

#if defined(HAVE_LIBYAML)
#include <yaml.h>
#endif

#if defined(HAVE_JSON_C_JSON_H) || defined(JSONC_INCLUDE_WITH_C)
#include <json-c/json.h>
#include <json-c/json_object.h>
#include <json-c/json_tokener.h>
#elif defined(HAVE_JSON_JSON_H) || defined(HAVE_LIBJSONC)
#include <json/json.h>
#include <json/json_object.h>
#include <json/json_tokener.h>
#endif

#define HANDLEBARS_MAP_PRIVATE
#define HANDLEBARS_STRING_PRIVATE
#define HANDLEBARS_VALUE_PRIVATE
#define HANDLEBARS_VALUE_HANDLERS_PRIVATE

#include "handlebars.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"

#include "handlebars_map.h"
#include "handlebars_stack.h"
#include "handlebars_utils.h"
#include "handlebars_value.h"
#include "handlebars_value_handlers.h"



size_t HANDLEBARS_VALUE_SIZE = sizeof(struct handlebars_value);



extern inline bool handlebars_value_is_callable(struct handlebars_value * value);
extern inline bool handlebars_value_is_empty(struct handlebars_value * value);
extern inline bool handlebars_value_is_scalar(struct handlebars_value * value);
extern inline long handlebars_value_count(struct handlebars_value * value);

extern inline struct handlebars_value_handlers * handlebars_value_get_handlers(struct handlebars_value * value);
extern inline struct handlebars_map * handlebars_value_get_map(struct handlebars_value * value);
extern inline void * handlebars_value_get_ptr(struct handlebars_value * value);
extern inline struct handlebars_stack * handlebars_value_get_stack(struct handlebars_value * value);
extern inline struct handlebars_string * handlebars_value_get_string(struct handlebars_value * value);
extern inline void * handlebars_value_get_usr(struct handlebars_value * value);
extern inline enum handlebars_value_type handlebars_value_get_type(struct handlebars_value * value);

extern inline void handlebars_value_null(struct handlebars_value * value);
extern inline void handlebars_value_boolean(struct handlebars_value * value, bool bval);
extern inline void handlebars_value_integer(struct handlebars_value * value, long lval);
extern inline void handlebars_value_float(struct handlebars_value * value, double dval);
extern inline void handlebars_value_str(struct handlebars_value * value, struct handlebars_string * string);
extern inline void handlebars_value_str_steal(struct handlebars_value * value, struct handlebars_string * string);
extern inline void handlebars_value_string(struct handlebars_value * value, const char * strval);
extern inline void handlebars_value_string_steal(struct handlebars_value * value, char * strval);
extern inline void handlebars_value_stringl(struct handlebars_value * value, const char * strval, size_t strlen);
extern inline void handlebars_value_map_init(struct handlebars_value * value, size_t capacity);
extern inline void handlebars_value_array_init(struct handlebars_value * value, size_t capacity);
extern inline void handlebars_value_ptr(struct handlebars_value * value, void * ptr);



#undef CONTEXT
#define CONTEXT HBSCTX(ctx)

struct handlebars_value * handlebars_value_ctor(struct handlebars_context * ctx)
{
    struct handlebars_value * value = MC(handlebars_talloc_zero(ctx, struct handlebars_value));
    value->ctx = CONTEXT;
    value->flags = HANDLEBARS_VALUE_FLAG_HEAP_ALLOCATED;
    return value;
}

#undef CONTEXT
#define CONTEXT HBSCTX(value->ctx)

struct handlebars_value * handlebars_value_map_find(struct handlebars_value * value, struct handlebars_string * key)
{
    if( value->type == HANDLEBARS_VALUE_TYPE_USER ) {
        if( handlebars_value_get_type(value) == HANDLEBARS_VALUE_TYPE_MAP ) {
            return handlebars_value_get_handlers(value)->map_find(value, key);
        }
    } else if( value->type == HANDLEBARS_VALUE_TYPE_MAP ) {
        return handlebars_map_find(value->v.map, key);
    }

    return NULL;
}

struct handlebars_value * handlebars_value_map_str_find(struct handlebars_value * value, const char * key, size_t len)
{
    struct handlebars_value * ret = NULL;
    struct handlebars_string * str = handlebars_string_ctor(value->ctx, key, len);

	if( value->type == HANDLEBARS_VALUE_TYPE_USER ) {
		if( handlebars_value_get_type(value) == HANDLEBARS_VALUE_TYPE_MAP ) {
			ret = handlebars_value_get_handlers(value)->map_find(value, str);
		}
	} else if( value->type == HANDLEBARS_VALUE_TYPE_MAP ) {
        ret = handlebars_map_find(value->v.map, str);
    }

    handlebars_talloc_free(str);
	return ret;
}

struct handlebars_value * handlebars_value_array_find(struct handlebars_value * value, size_t index)
{
	if( value->type == HANDLEBARS_VALUE_TYPE_USER ) {
		if( handlebars_value_get_type(value) == HANDLEBARS_VALUE_TYPE_ARRAY ) {
			return handlebars_value_get_handlers(value)->array_find(value, index);
		}
	} else if( value->type == HANDLEBARS_VALUE_TYPE_ARRAY ) {
        return handlebars_stack_get(value->v.stack, index);
    }

	return NULL;
}

struct handlebars_string * handlebars_value_to_string(struct handlebars_value * value)
{
    enum handlebars_value_type type = value ? value->type : HANDLEBARS_VALUE_TYPE_NULL;

    switch( type ) {
        case HANDLEBARS_VALUE_TYPE_STRING:
            return handlebars_string_copy_ctor(CONTEXT, value->v.string);
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

const char * handlebars_value_get_strval(struct handlebars_value * value)
{
    if( value->type == HANDLEBARS_VALUE_TYPE_STRING ) {
        return value->v.string->val;
    } else {
        return NULL;
    }
}

size_t handlebars_value_get_strlen(struct handlebars_value * value)
{
	if( value->type == HANDLEBARS_VALUE_TYPE_STRING ) {
		return value->v.string->len;
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
            return value->v.string->len != 0 && strcmp(value->v.string->val, "0") != 0;
        case HANDLEBARS_VALUE_TYPE_ARRAY:
            return handlebars_stack_length(value->v.stack) != 0;
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
            buf = handlebars_talloc_asprintf_append_buffer(buf, "string(%.*s)", (int) value->v.string->len, value->v.string->val);
            break;
        case HANDLEBARS_VALUE_TYPE_ARRAY:
            buf = handlebars_talloc_asprintf_append_buffer(buf, "%s\n", "[");
            HANDLEBARS_VALUE_FOREACH_IDX(value, index, child) {
                char * tmp = handlebars_value_dump(child, depth + 1);
                buf = handlebars_talloc_asprintf_append_buffer(buf, "%s%ld => %s\n", indent2, index, tmp);
                handlebars_talloc_free(tmp);
            } HANDLEBARS_VALUE_FOREACH_END();
            buf = handlebars_talloc_asprintf_append_buffer(buf, "%s%s", indent, "]");
            break;
        case HANDLEBARS_VALUE_TYPE_MAP:
            buf = handlebars_talloc_asprintf_append_buffer(buf, "%s\n", "{");
            HANDLEBARS_VALUE_FOREACH_KV(value, key, child) {
                char * tmp = handlebars_value_dump(child, depth + 1);
                buf = handlebars_talloc_asprintf_append_buffer(buf, "%s%.*s => %s\n", indent2, (int) key->len, key->val, tmp);
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
                string = handlebars_string_append(value->ctx, string, value->v.string->val, value->v.string->len);
            }
            break;

        case HANDLEBARS_VALUE_TYPE_USER:
            if( handlebars_value_get_type(value) != HANDLEBARS_VALUE_TYPE_ARRAY ) {
                break;
            }
            // fall-through to array
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
                // @todo double-check index?
                new_value->v.stack = handlebars_stack_push(new_value->v.stack, handlebars_value_copy(child));
            } HANDLEBARS_VALUE_FOREACH_END();
            break;
        case HANDLEBARS_VALUE_TYPE_MAP:
            new_value = handlebars_value_ctor(CONTEXT);
            handlebars_value_map_init(new_value, handlebars_value_count(value));
            HANDLEBARS_VALUE_FOREACH_KV(value, key, child) {
                handlebars_map_update(new_value->v.map, key, handlebars_value_copy(child));
            } HANDLEBARS_VALUE_FOREACH_END();
            break;
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

void handlebars_value_dtor(struct handlebars_value * value)
{
    unsigned long restore_flags = 0;

    // Release old value
    switch( value->type ) {
        case HANDLEBARS_VALUE_TYPE_ARRAY:
            handlebars_stack_dtor(value->v.stack);
            break;
        case HANDLEBARS_VALUE_TYPE_MAP:
            handlebars_map_dtor(value->v.map);
            break;
        case HANDLEBARS_VALUE_TYPE_STRING:
            handlebars_talloc_free(value->v.string);
            break;
        case HANDLEBARS_VALUE_TYPE_USER:
            handlebars_value_get_handlers(value)->dtor(value);
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





// Iteration

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

    handlebars_value_delref(it->current);
    it->current = NULL;

    if( it->index >= handlebars_stack_length(value->v.stack) - 1 ) {
        return false;
    }

    it->index++;
    it->current = handlebars_stack_get(value->v.stack, it->index);
    return true;
}

static bool handlebars_value_iterator_next_map(struct handlebars_value_iterator * it)
{
    //struct handlebars_map_entry * entry = talloc_get_type(it->usr, struct handlebars_map_entry);
    struct handlebars_map_entry * entry = (struct handlebars_map_entry *) it->usr;

    assert(it->value != NULL);
    assert(it->value->type == HANDLEBARS_VALUE_TYPE_MAP);
    assert(it->current != NULL);
    assert(entry != NULL);

    handlebars_value_delref(it->current);
    it->current = NULL;

    if( !entry->next ) {
        handlebars_map_set_is_in_iteration(it->value->v.map, false); // @todo we should restore the previous flag?
        return false;
    }

    it->usr = (void *) (entry = entry->next);
    it->key = entry->key;
    it->current = entry->value;
    handlebars_value_addref(it->current);

    return true;
}

bool handlebars_value_iterator_init(struct handlebars_value_iterator * it, struct handlebars_value * value)
{
    struct handlebars_map_entry * entry;

    memset(it, 0, sizeof(struct handlebars_value_iterator));

    // @TODO make sure this works for type_user?
    if (handlebars_value_count(value) <= 0) {
        it->next = &handlebars_value_iterator_next_void;
        return false;
    }

    switch( value->type ) {
        case HANDLEBARS_VALUE_TYPE_ARRAY:
            it->value = value;
            it->current = handlebars_stack_get(value->v.stack, 0);
            it->length = handlebars_stack_length(value->v.stack);
            it->next = &handlebars_value_iterator_next_stack;
            return true;

        case HANDLEBARS_VALUE_TYPE_MAP:
            entry = value->v.map->first;
            if( entry ) {
                it->value = value;
                it->usr = (void *) entry;
                it->key = entry->key;
                it->current = entry->value;
                it->length = value->v.map->i;
                it->next = &handlebars_value_iterator_next_map;
                handlebars_value_addref(it->current);
                handlebars_map_set_is_in_iteration(value->v.map, true); // @todo we should store the result
                return true;
            } else {
                it->next = &handlebars_value_iterator_next_void;
            }
            break;

        case HANDLEBARS_VALUE_TYPE_USER:
            return handlebars_value_get_handlers(value)->iterator(it, value);

        default:
            it->next = &handlebars_value_iterator_next_void;
            //handlebars_context_throw(value->ctx, HANDLEBARS_ERROR, "Cannot iterator over type %d", value->type);
            break;
    }

    return false;
}
