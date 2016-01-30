
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

static struct handlebars_value * std_json_map_find(struct handlebars_value * value, const char * key, size_t len) {
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
            it->usr = (void *) entry;
            // @todo
            //it->key = entry->k;
            it->current = handlebars_value_from_json_object(value->ctx, entry->v);
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

short std_json_iterator_next(struct handlebars_value_iterator * it)
{
    struct handlebars_value * value = it->value;
    struct json_object * intern = (struct json_object *) value->v.usr;
    struct lh_entry * entry;
    short ret = 0;

//    if( it->current ) {
//        handlebars_value_delref(it->current);
//        it->current = NULL;
//    }

    switch( json_object_get_type(intern) ) {
        case json_type_object:
            entry = (struct lh_entry *) it->usr;
            if( entry && entry->next ) {
                ret = 1;
                it->usr = (void *) entry->next;
                // @todo
                //it->key = entry->k;
                it->current = handlebars_value_from_json_object(value->ctx, entry->v);
                // Need to increment refcount?
            }
            break;
        case json_type_array:
            fprintf("QWEJIKQWJEQWEQWE %d %d\n", it->index, json_object_array_length(intern));
            if( it->index < json_object_array_length(intern) ) {
                ret = 1;
                it->index++;
                it->current = handlebars_value_from_json_object(value->ctx, json_object_array_get_idx(intern, it->index));
            }
            break;
    }

    return ret;
}


static struct handlebars_value_handlers handlebars_value_std_json_handlers = {
        &std_json_dtor,
        &std_json_type,
        &std_json_map_find,
        &std_json_array_find,
        &std_json_iterator_ctor,
        &std_json_iterator_next
};

struct handlebars_value_handlers * handlebars_value_get_std_json_handlers()
{
    return &handlebars_value_std_json_handlers;
}
