
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

#include "handlebars_map.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"
#include "handlebars_stack.h"
#include "handlebars_utils.h"
#include "handlebars_value.h"
#include "handlebars_value_handlers.h"



enum handlebars_value_type handlebars_value_get_type(struct handlebars_value * value) {
	if( value->type == HANDLEBARS_VALUE_TYPE_USER ) {
		return value->handlers->type(value);
	} else {
		return value->type;
	}
}

struct handlebars_value * handlebars_value_map_find(struct handlebars_value * value, const char * key, size_t len)
{
	if( value->type == HANDLEBARS_VALUE_TYPE_USER ) {
		if( handlebars_value_get_type(value) == HANDLEBARS_VALUE_TYPE_MAP ) {
			return value->handlers->map_find(value, key, len);
		}
	} else if( value->type == HANDLEBARS_VALUE_TYPE_MAP ) {
        return handlebars_map_find(value->v.map, key);
    }

	return NULL;
}

struct handlebars_value * handlebars_value_array_find(struct handlebars_value * value, size_t index)
{
	if( value->type == HANDLEBARS_VALUE_TYPE_USER ) {
		if( handlebars_value_get_type(value) == HANDLEBARS_VALUE_TYPE_ARRAY ) {
			return value->handlers->array_find(value, index);
		}
	} else if( value->type == HANDLEBARS_VALUE_TYPE_ARRAY ) {
        return handlebars_stack_get(value->v.stack, index);
    }

	return NULL;
}

const char * handlebars_value_get_strval(struct handlebars_value * value)
{
	if( value->type == HANDLEBARS_VALUE_TYPE_STRING ) {
		return value->v.strval;
	}

	return NULL;
}

size_t handlebars_value_get_strlen(struct handlebars_value * value)
{
	if( value->type == HANDLEBARS_VALUE_TYPE_STRING ) {
		return strlen(value->v.strval);
	}

	return 0;
}

short handlebars_value_get_boolval(struct handlebars_value * value)
{
	if( value->type == HANDLEBARS_VALUE_TYPE_BOOLEAN ) {
        return value->v.bval;
	}

	return 0;
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

struct handlebars_value_iterator * handlebars_value_iterator_ctor(struct handlebars_value * value)
{
    struct handlebars_value_iterator * it = NULL;

    switch( value->type ) {
        case HANDLEBARS_VALUE_TYPE_ARRAY:
            it = handlebars_talloc(value, struct handlebars_value_iterator);
            break;
        case HANDLEBARS_VALUE_TYPE_MAP:
            it = handlebars_talloc(value, struct handlebars_value_iterator);
            it->current = value->v.map->first;
            break;
        case HANDLEBARS_VALUE_TYPE_USER:
            it = value->handlers->iterator(value);
            break;
    }

    return it;
}

short handlebars_value_iterator_next(struct handlebars_value * value, struct handlebars_value_iterator * it)
{
    assert(it != NULL);

    switch( value->type ) {
        case HANDLEBARS_VALUE_TYPE_ARRAY:
            if( it->index >= handlebars_stack_length(value->v.stack) ) {
                return 0;
            }
            it->index++;
            return 1;
        case HANDLEBARS_VALUE_TYPE_MAP: {
            struct handlebars_map_entry * entry = talloc_get_type(it->current, struct handlebars_map_entry);
            if( !entry || !entry->next ) {
                return 0;
            }
            it->current = entry->next;
            return 1;
        }
        case HANDLEBARS_VALUE_TYPE_USER:
            return value->handlers->next(value, it);
        default:
            return 0;
    }
}

struct handlebars_value * handlebars_value_iterator_current(struct handlebars_value * value, void * iterator)
{
    switch( value->type ) {
        case HANDLEBARS_VALUE_TYPE_ARRAY: {
            struct handlebars_stack_iterator *it = talloc_get_type(iterator, struct handlebars_stack_iterator);
            if( !it ) {
                return NULL;
            } else {
                return handlebars_stack_get(value->v.stack, it->pos);
            }
        }
        case HANDLEBARS_VALUE_TYPE_MAP: {
            struct handlebars_map_iterator *it = talloc_get_type(iterator, struct handlebars_map_iterator);
            if( !it ) {
                return NULL;
            } else {
                return it->current;
            }
        }
        case HANDLEBARS_VALUE_TYPE_USER:
            return value->handlers->next(value, iterator);
        default:
            return 0;
    }
}

char * handlebars_value_expression(void * ctx, struct handlebars_value * value, short escape)
{
    char * ret = NULL;

    switch( handlebars_value_get_type(value) ) {
        case HANDLEBARS_VALUE_TYPE_BOOLEAN:
            if( handlebars_value_get_boolval(value) ) {
                ret = handlebars_talloc_strdup(ctx, "true");
            } else {
                ret = handlebars_talloc_strdup(ctx, "false");
            }
            break;

        case HANDLEBARS_VALUE_TYPE_FLOAT:
            ret = handlebars_talloc_asprintf(ctx, "%f", handlebars_value_get_floatval(value));
            break;

        case HANDLEBARS_VALUE_TYPE_INTEGER:
            ret = handlebars_talloc_asprintf(ctx, "%ld", handlebars_value_get_intval(value));
            break;

        case HANDLEBARS_VALUE_TYPE_NULL:
            ret = handlebars_talloc_strdup(ctx, "");
            break;

        case HANDLEBARS_VALUE_TYPE_STRING:
            ret = handlebars_talloc_strdup(ctx, handlebars_value_get_strval(value));
            break;

        case HANDLEBARS_VALUE_TYPE_ARRAY:
        case HANDLEBARS_VALUE_TYPE_MAP:
        case HANDLEBARS_VALUE_TYPE_USER:
            // assert(0);
            //return NULL;
            ret = handlebars_talloc_strdup(ctx, "");
            break;
    }

    if( escape ) {
        char * esc = handlebars_htmlspecialchars(ret);
        handlebars_talloc_free(ret);
        ret = esc;
    }

    return ret;
}



struct handlebars_value * handlebars_value_ctor(void * ctx)
{
	struct handlebars_value * value = handlebars_talloc_zero(ctx, struct handlebars_value);
	if( likely(value != NULL) ) {
        value->ctx = ctx;
		handlebars_value_addref(value);
	}
	return value;
}

void handlebars_value_dtor(struct handlebars_value * value)
{
    // Release old value
    switch( value->type ) {
        case HANDLEBARS_VALUE_TYPE_ARRAY:
            handlebars_talloc_free(value->v.stack);
            break;
        case HANDLEBARS_VALUE_TYPE_MAP:
            handlebars_talloc_free(value->v.map);
            break;
        case HANDLEBARS_VALUE_TYPE_STRING:
            handlebars_talloc_free(value->v.strval);
            break;
        case HANDLEBARS_VALUE_TYPE_USER:
            assert(value->handlers != NULL);
            value->handlers->dtor(value);
            break;
    }

    // Initialize to null
    memset(value, 0, sizeof(struct handlebars_value));
}

struct handlebars_value * handlebars_value_from_json_object(void *ctx, struct json_object *json)
{
    struct handlebars_value * value = handlebars_value_ctor(ctx);

    if( value ) {
        switch( json_object_get_type(json) ) {
            case json_type_null:
                // do nothing
                break;
            case json_type_boolean:
                value->type = HANDLEBARS_VALUE_TYPE_BOOLEAN;
                value->v.bval = json_object_get_boolean(json);
                break;
            case json_type_double:
                value->type = HANDLEBARS_VALUE_TYPE_FLOAT;
                value->v.dval = json_object_get_double(json);
                break;
            case json_type_int:
                value->type = HANDLEBARS_VALUE_TYPE_INTEGER;
                // @todo make sure sizing is correct
                value->v.lval = json_object_get_int64(json);
                break;
            case json_type_string:
                value->type = HANDLEBARS_VALUE_TYPE_STRING;
                value->v.strval = handlebars_talloc_strdup(value, json_object_get_string(json));
                break;

            case json_type_object:
            case json_type_array:
                value->type = HANDLEBARS_VALUE_TYPE_USER;
                value->handlers = handlebars_value_get_std_json_handlers();
                value->v.usr = (void *) json;
                break;
            default:
                // ruh roh
                assert(0);
                break;
        }
    }

    return value;
}

struct handlebars_value * handlebars_value_from_json_string(void *ctx, const char * json)
{
    struct handlebars_value * ret;
    struct json_object * result = json_tokener_parse(json);
    if( result ) {
        ret = handlebars_value_from_json_object(ctx, result);
        if( ret ) {
            talloc_set_destructor(ret, handlebars_value_dtor);
            ret->flags |= HANDLEBARS_VALUE_FLAG_TALLOC_DTOR;
        }
    }
    return ret;
}
