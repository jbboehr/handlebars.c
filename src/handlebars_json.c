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

// json-c undeprecated json_object_object_get, but the version in xenial
// is too old, so let's silence deprecated warnings for json-c
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <json.h>
#include <json_object.h>
#include <json_tokener.h>
#pragma GCC diagnostic pop

#include "handlebars.h"
#include "handlebars_json.h"
#include "handlebars_private.h"
#include "handlebars_memory.h"
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


static int handlebars_json_dtor(struct handlebars_json * obj)
{
    if( obj && obj->object ) {
        json_object_put(obj->object);
        obj->object = NULL;
    }

    return 0;
}



#undef CONTEXT
#define CONTEXT HBSCTX(handlebars_value_get_ctx(value))

static struct handlebars_value * hbs_json_copy(struct handlebars_value * value)
{
    struct json_object * object = HANDLEBARS_JSON_OBJ(value);
    const char * str = json_object_to_json_string(object);
    return handlebars_value_from_json_string(CONTEXT, str);
}

static void hbs_json_dtor(struct handlebars_user * user)
{
    struct handlebars_json * json = GET_INTERN(user);
    if( json && json->object ) {
        json_object_put(json->object);
        json->object = NULL;
    }
}

static void hbs_json_convert(struct handlebars_value * value, bool recurse)
{
    struct json_object * intern = HANDLEBARS_JSON_OBJ(value);
    struct handlebars_value * new_value;

    switch( json_object_get_type(intern) ) {
        case json_type_object: {
            struct handlebars_map * map = handlebars_map_ctor(CONTEXT, json_object_object_length(intern));
            json_object_object_foreach(intern, k, v) {
                new_value = handlebars_value_from_json_object(CONTEXT, v);
                handlebars_map_str_update(map, k, strlen(k), new_value);
                if( recurse && handlebars_value_get_real_type(new_value) == HANDLEBARS_VALUE_TYPE_USER ) {
                    hbs_json_convert(new_value, recurse);
                }
            }
            handlebars_value_map(value, map);
            break;
        }

        case json_type_array: {
            size_t i;
            size_t l = json_object_array_length(intern);
            struct handlebars_stack * stack = handlebars_stack_ctor(CONTEXT, l);
            for( i = 0; i < l; i++ ) {
                new_value = handlebars_value_from_json_object(CONTEXT, json_object_array_get_idx(intern, i));
                // @TODO check index?
                stack = handlebars_stack_push(stack, new_value);
                if( recurse && handlebars_value_get_real_type(new_value) == HANDLEBARS_VALUE_TYPE_USER ) {
                    hbs_json_convert(new_value, recurse);
                }
            }
            handlebars_value_array(value, stack);
            break;
        }

        default: break; // LCOV_EXCL_LINE
    }
}

static enum handlebars_value_type hbs_json_type(struct handlebars_value * value)
{
    struct json_object * intern = HANDLEBARS_JSON_OBJ(value);

    switch( json_object_get_type(intern) ) {
        case json_type_object: return HANDLEBARS_VALUE_TYPE_MAP;
        case json_type_array: return HANDLEBARS_VALUE_TYPE_ARRAY;
        default: assert(0); break; // LCOV_EXCL_LINE
    }

    return HANDLEBARS_VALUE_TYPE_NULL;
}

static struct handlebars_value * hbs_json_map_find(struct handlebars_value * value, struct handlebars_string * key)
{
    struct json_object * intern = HANDLEBARS_JSON_OBJ(value);
    struct json_object * item = json_object_object_get(intern, hbs_str_val(key));
    if( item == NULL ) {
        return NULL;
    }
    return handlebars_value_from_json_object(CONTEXT, item);
}

static struct handlebars_value * hbs_json_array_find(struct handlebars_value * value, size_t index)
{
    struct json_object * intern = HANDLEBARS_JSON_OBJ(value);
    struct json_object * item = json_object_array_get_idx(intern, (int) index);
    if( item == NULL ) {
        return NULL;
    }
    return handlebars_value_from_json_object(CONTEXT, item);
}

static bool hbs_json_iterator_next_void(struct handlebars_value_iterator * it)
{
    return false;
}

static bool hbs_json_iterator_next_object(struct handlebars_value_iterator * it)
{
    struct handlebars_value * value = it->value;
    struct lh_entry * entry;
    char * tmp;

    assert(value != NULL);
    assert(handlebars_value_get_real_type(value) == HANDLEBARS_VALUE_TYPE_USER);
    assert(it->current != NULL);
    assert(it->key != NULL);

    handlebars_talloc_free(it->key);

    handlebars_value_delref(it->current);
    it->current = NULL;

    entry = (struct lh_entry *) it->usr;
    if( !entry || !entry->next ) {
        return false;
    }

    it->usr = (void *) (entry = entry->next);
    tmp = (char *) entry->k;
    it->key = handlebars_string_ctor(CONTEXT, tmp, strlen(tmp));
    it->current = handlebars_value_from_json_object(CONTEXT, (struct json_object *) entry->v);
    handlebars_value_addref(it->current);
    return true;
}

static bool hbs_json_iterator_next_array(struct handlebars_value_iterator * it)
{
    struct handlebars_value * value = it->value;
    struct json_object * intern = HANDLEBARS_JSON_OBJ(value);

    assert(value != NULL);
    assert(handlebars_value_get_real_type(value) == HANDLEBARS_VALUE_TYPE_USER);
    assert(it->current != NULL);

    handlebars_value_delref(it->current);
    it->current = NULL;

    it->index++;
    if( it->index >= (size_t) json_object_array_length(intern) ) {
        return false;
    }

    it->current = handlebars_value_from_json_object(CONTEXT, json_object_array_get_idx(intern, it->index));
    handlebars_value_addref(it->current);
    return true;
}

static bool hbs_json_iterator_init(struct handlebars_value_iterator * it, struct handlebars_value * value)
{
    struct json_object * intern = HANDLEBARS_JSON_OBJ(value);
    struct lh_entry * entry;

    it->value = value;

    switch( json_object_get_type(intern) ) {
        case json_type_object:
            entry = json_object_get_object(intern)->head;
            if( entry ) {
                char * tmp = (char *) entry->k;
                it->usr = (void *) entry;
                it->key = handlebars_string_ctor(CONTEXT, tmp, strlen(tmp));
                it->current = handlebars_value_from_json_object(CONTEXT, (json_object *) entry->v);
                it->length = (size_t) json_object_object_length(intern);
                it->next = &hbs_json_iterator_next_object;
                handlebars_value_addref(it->current);
                return true;
            } else {
                it->next = &hbs_json_iterator_next_void;
            }
            break;
        case json_type_array:
            it->index = 0;
            it->current = handlebars_value_from_json_object(CONTEXT, json_object_array_get_idx(intern, (int) it->index));
            it->length = (size_t) json_object_array_length(intern);
            it->next = &hbs_json_iterator_next_array;
            handlebars_value_addref(it->current);
            return true;
        default:
            it->next = &hbs_json_iterator_next_void;
            break;
    }

    return false;
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

#undef CONTEXT
#define CONTEXT ctx

void handlebars_value_init_json_object(struct handlebars_context * ctx, struct handlebars_value * value, struct json_object *json)
{
    struct handlebars_json * obj;

    switch( json_object_get_type(json) ) {
        case json_type_null:
            // do nothing
            break;
        case json_type_boolean:
            handlebars_value_boolean(value, json_object_get_boolean(json));
            break;
        case json_type_double:
            handlebars_value_float(value, json_object_get_double(json));
            break;
        case json_type_int:
            // @todo make sure sizing is correct
            handlebars_value_integer(value, json_object_get_int64(json));
            break;
        case json_type_string:
            handlebars_value_stringl(value, json_object_get_string(json), json_object_get_string_len(json));
            break;

        case json_type_object:
        case json_type_array:
            // Increment refcount
            json_object_get(json);

            obj = MC(handlebars_talloc(ctx, struct handlebars_json));
            handlebars_user_init((struct handlebars_user *) obj, &handlebars_value_hbs_json_handlers);
            obj->object = json;
            talloc_set_destructor(obj, handlebars_json_dtor);
            handlebars_value_user(value, (struct handlebars_user *) obj);
            break;

        default: assert(0); break; // LCOV_EXCL_LINE
    }
}

void handlebars_value_init_json_string(struct handlebars_context *ctx, struct handlebars_value * value, const char * json)
{
    enum json_tokener_error parse_err = json_tokener_success;
    struct json_object * result = json_tokener_parse_verbose(json, &parse_err);
    // @todo test parse error
    if( parse_err == json_tokener_success ) {
        handlebars_value_init_json_object(ctx, value, result);
    } else {
        handlebars_throw(ctx, HANDLEBARS_ERROR, "JSON Parse error: %s", json_tokener_error_desc(parse_err));
    }
}

struct handlebars_value * handlebars_value_from_json_object(struct handlebars_context *ctx, struct json_object * json)
{
    struct handlebars_value * value = handlebars_value_ctor(ctx);
    handlebars_value_init_json_object(ctx, value, json);
    // handlebars_value_convert_ex(value, true);
    return value;
}

struct handlebars_value * handlebars_value_from_json_string(struct handlebars_context *ctx, const char * json)
{
    struct handlebars_value * value = handlebars_value_ctor(ctx);
    handlebars_value_init_json_string(ctx, value, json);
    // handlebars_value_convert_ex(value, true);
    return value;
}



