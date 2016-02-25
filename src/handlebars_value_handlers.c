
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <string.h>
#include <talloc.h>

#if defined(HAVE_JSON_C_JSON_H)
#include <json-c/json.h>
#include <json-c/json_object.h>
#include <json-c/json_tokener.h>
#elif defined(HAVE_JSON_JSON_H)
#include <json/json.h>
#include <json/json_object.h>
#include <json/json_tokener.h>
#endif

#include "handlebars_memory.h"
#include "handlebars_value.h"
#include "handlebars_value_handlers.h"



#undef CONTEXT
#define CONTEXT HBSCTX(value->ctx)

static struct handlebars_value * std_json_copy(struct handlebars_value * value)
{
    const char * str = json_object_to_json_string(value->v.usr.ptr);
    return handlebars_value_from_json_string(CONTEXT, str);
}

static void std_json_dtor(struct handlebars_value * value)
{
    struct json_object * result = (struct json_object *) value->v.usr.ptr;

    assert(value->type == HANDLEBARS_VALUE_TYPE_USER);
    assert(result != NULL);

    if( result != NULL ) {
        json_object_put(result);
        value->v.usr.ptr = NULL;
        value->v.usr.handlers = NULL;
    }
}

static void std_json_convert(struct handlebars_value * value, bool recurse)
{
    struct json_object * intern = (struct json_object *) value->v.usr.ptr;
    char * key;
    struct handlebars_value * new_value;

    switch( json_object_get_type(intern) ) {
        case json_type_object: {
            handlebars_value_map_init(value);
            json_object_object_foreach(intern, k, v) {
                new_value = handlebars_value_from_json_object(CONTEXT, v);
                handlebars_map_str_update(value->v.map, k, strlen(k), new_value);
                if( recurse && new_value->type == HANDLEBARS_VALUE_TYPE_USER ) {
                    std_json_convert(new_value, recurse);
                }
                handlebars_value_delref(new_value);
            }
            break;
        }
        case json_type_array: {
            handlebars_value_array_init(value);
            size_t i, l;

            for( i = 0, l = json_object_array_length(intern); i < l; i++ ) {
                new_value = handlebars_value_from_json_object(CONTEXT, json_object_array_get_idx(intern, i));
                handlebars_stack_set(value->v.stack, i, new_value);
                if( recurse && new_value->type == HANDLEBARS_VALUE_TYPE_USER ) {
                    std_json_convert(new_value, recurse);
                }
                handlebars_value_delref(new_value);
            }
            break;
        }
    }

    // Remove talloc destructor?
    talloc_set_destructor(value, NULL);
    value->flags &= ~HANDLEBARS_VALUE_FLAG_TALLOC_DTOR;
}

static enum handlebars_value_type std_json_type(struct handlebars_value * value)
{
    struct json_object * intern = (struct json_object *) value->v.usr.ptr;
    switch( json_object_get_type(intern) ) {
        case json_type_object: return HANDLEBARS_VALUE_TYPE_MAP;
        case json_type_array: return HANDLEBARS_VALUE_TYPE_ARRAY;

        default:
        case json_type_null:
        case json_type_boolean:
        case json_type_double:
        case json_type_int:
        case json_type_string:
            assert(0);
            break;
    }
}

static struct handlebars_value * std_json_map_find(struct handlebars_value * value, struct handlebars_string * key)
{
    struct json_object * intern = (struct json_object *) value->v.usr.ptr;
    struct json_object * item = json_object_object_get(intern, key->val);
    if( item == NULL ) {
        return NULL;
    }
    return handlebars_value_from_json_object(CONTEXT, item);
}

static struct handlebars_value * std_json_array_find(struct handlebars_value * value, size_t index)
{
    struct json_object * intern = (struct json_object *) value->v.usr.ptr;
    struct json_object * item = json_object_array_get_idx(intern, (int) index);
    if( item == NULL ) {
        return NULL;
    }
    return handlebars_value_from_json_object(CONTEXT, item);
}

bool std_json_iterator_init(struct handlebars_value_iterator * it, struct handlebars_value * value)
{
    struct json_object * intern = (struct json_object *) value->v.usr.ptr;
    struct lh_entry * entry;

    it->value = value;

    switch( json_object_get_type(intern) ) {
        case json_type_object:
            entry = json_object_get_object(intern)->head;
            if( entry ) {
                char * tmp = (char *) entry->k;
                it->usr = (void *) entry;
                it->key = handlebars_string_ctor(value->ctx, tmp, strlen(tmp));
                it->current = handlebars_value_from_json_object(CONTEXT, (json_object *) entry->v);
                it->length = (size_t) json_object_object_length(intern);
                return true;
            }
            break;
        case json_type_array:
            it->index = 0;
            it->current = handlebars_value_from_json_object(CONTEXT, json_object_array_get_idx(intern, (int) it->index));
            it->length = (size_t) json_object_array_length(intern);
            return true;
        default:
            // do nothing
            break;
    }

    return false;
}

bool std_json_iterator_next(struct handlebars_value_iterator * it)
{
    struct handlebars_value * value = it->value;
    struct json_object * intern = (struct json_object *) value->v.usr.ptr;
    struct lh_entry * entry;
    bool ret = false;

    if( it->key ) {
        handlebars_talloc_free(it->key);
        it->key = NULL;
    }

    switch( json_object_get_type(intern) ) {
        case json_type_object:
            entry = (struct lh_entry *) it->usr;
            if( entry && entry->next ) {
                char * tmp;
                ret = true;
                it->usr = (void *) (entry = entry->next);
                tmp = (char *) entry->k;
                it->key = handlebars_string_ctor(value->ctx, tmp, strlen(tmp));
                it->current = handlebars_value_from_json_object(CONTEXT, entry->v);
            }
            break;
        case json_type_array:
            if( it->index < json_object_array_length(intern) - 1 ) {
                ret = true;
                it->index++;
                it->current = handlebars_value_from_json_object(CONTEXT, json_object_array_get_idx(intern, it->index));
            }
            break;
        default:
            // @todo throw
            break;
    }

    return ret;
}

long std_json_count(struct handlebars_value * value)
{
    struct json_object * intern = (struct json_object *) value->v.usr.ptr;
    switch( json_object_get_type(intern) ) {
        case json_type_object:
            return json_object_object_length(intern);
        case json_type_array:
            return json_object_array_length(intern);
        default:
            return -1;
    }

}


static struct handlebars_value_handlers handlebars_value_std_json_handlers = {
        &std_json_copy,
        &std_json_dtor,
        &std_json_convert,
        &std_json_type,
        &std_json_map_find,
        &std_json_array_find,
        &std_json_iterator_init,
        &std_json_iterator_next,
        NULL, // call
        &std_json_count
};

struct handlebars_value_handlers * handlebars_value_get_std_json_handlers()
{
    return &handlebars_value_std_json_handlers;
}
