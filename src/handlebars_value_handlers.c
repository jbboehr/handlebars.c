
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
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



static struct handlebars_value * std_json_copy(struct handlebars_value * value)
{
    const char * str = json_object_to_json_string(value->v.usr);
    return handlebars_value_from_json_string(value->ctx, str);
}

static void std_json_dtor(struct handlebars_value * value)
{
    struct json_object * result = (struct json_object *) value->v.usr;

    assert(value->type == HANDLEBARS_VALUE_TYPE_USER);
    assert(result != NULL);

    if( result != NULL ) {
        json_object_put(result);
        value->v.usr = NULL;
    }
}

static void std_json_convert(struct handlebars_value * value, bool recurse)
{
    struct json_object * intern = (struct json_object *) value->v.usr;
    char * key;
    struct handlebars_value * new_value;

    switch( json_object_get_type(intern) ) {
        case json_type_object: {
            handlebars_value_map_init(value);
            json_object_object_foreach(intern, k, v) {
                new_value = handlebars_value_from_json_object(value->ctx, v);
                handlebars_map_add(value->v.map, k, new_value);
                handlebars_value_delref(new_value);
                if( recurse && new_value->type == HANDLEBARS_VALUE_TYPE_USER ) {
                    std_json_convert(new_value, recurse);
                }
            }
            break;
        }
        case json_type_array: {
            handlebars_value_array_init(value);
            size_t i, l;

            for( i = 0, l = json_object_array_length(intern); i < l; i++ ) {
                new_value = handlebars_value_from_json_object(value->ctx, json_object_array_get_idx(intern, i));
                handlebars_stack_set(value->v.map, i, new_value);
                handlebars_value_delref(new_value);
                if( recurse && new_value->type == HANDLEBARS_VALUE_TYPE_USER ) {
                    std_json_convert(new_value, recurse);
                }
            }
            break;
        }
    }

    // Remove talloc destructor?
    talloc_set_destructor(value, NULL);
    value->flags &= ~HANDLEBARS_VALUE_FLAG_TALLOC_DTOR;
}

static enum handlebars_value_type std_json_type(struct handlebars_value * value) {
    struct json_object * intern = (struct json_object *) value->v.usr;
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

static struct handlebars_value * std_json_map_find(struct handlebars_value * value, const char * key) {
    struct json_object * intern = (struct json_object *) value->v.usr;
    struct json_object * item = json_object_object_get(intern, key);
    if( item == NULL ) {
        return NULL;
    }
    return handlebars_value_from_json_object(value->ctx, item);
}

static struct handlebars_value * std_json_array_find(struct handlebars_value * value, size_t index) {
    struct json_object * intern = (struct json_object *) value->v.usr;
    struct json_object * item = json_object_array_get_idx(intern, (int) index);
    if( item == NULL ) {
        return NULL;
    }
    return handlebars_value_from_json_object(value->ctx, item);
}

struct handlebars_value_iterator * std_json_iterator_ctor(struct handlebars_value * value)
{
    struct json_object * intern = (struct json_object *) value->v.usr;
    struct handlebars_value_iterator * it = handlebars_talloc(value, struct handlebars_value_iterator);
    struct lh_entry * entry;

    it->value = value;

    switch( json_object_get_type(intern) ) {
        case json_type_object:
            entry = json_object_get_object(intern)->head;
            if( entry ) {
                it->usr = (void *) entry;
                it->key = (char *) entry->k;
                it->current = handlebars_value_from_json_object(value->ctx, entry->v);
            }
            break;
        case json_type_array:
            it->index = 0;
            it->current = handlebars_value_from_json_object(value->ctx, json_object_array_get_idx(intern, it->index));
            break;
        default:
            handlebars_talloc_free(it);
            it = NULL;
            break;
    }

    return it;
}

bool std_json_iterator_next(struct handlebars_value_iterator * it)
{
    struct handlebars_value * value = it->value;
    struct json_object * intern = (struct json_object *) value->v.usr;
    struct lh_entry * entry;
    bool ret = false;

    switch( json_object_get_type(intern) ) {
        case json_type_object:
            entry = (struct lh_entry *) it->usr;
            if( entry && entry->next ) {
                ret = true;
                it->usr = (void *) (entry = entry->next);
                it->key = (char *) entry->k;
                it->current = handlebars_value_from_json_object(value->ctx, entry->v);
            }
            break;
        case json_type_array:
            if( it->index < json_object_array_length(intern) - 1 ) {
                ret = true;
                it->index++;
                it->current = handlebars_value_from_json_object(value->ctx, json_object_array_get_idx(intern, it->index));
            }
            break;
    }

    return ret;
}


static struct handlebars_value_handlers handlebars_value_std_json_handlers = {
        &std_json_copy,
        &std_json_dtor,
        &std_json_convert,
        &std_json_type,
        &std_json_map_find,
        &std_json_array_find,
        &std_json_iterator_ctor,
        &std_json_iterator_next,
        NULL // call
};

struct handlebars_value_handlers * handlebars_value_get_std_json_handlers()
{
    return &handlebars_value_std_json_handlers;
}
