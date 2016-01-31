
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

struct handlebars_value * handlebars_value_map_find(struct handlebars_value * value, const char * key)
{
	if( value->type == HANDLEBARS_VALUE_TYPE_USER ) {
		if( handlebars_value_get_type(value) == HANDLEBARS_VALUE_TYPE_MAP ) {
			return value->handlers->map_find(value, key);
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
    switch( value->type ) {
        case HANDLEBARS_VALUE_TYPE_STRING:
            return value->v.strval;
        case HANDLEBARS_VALUE_TYPE_INTEGER:
            return handlebars_talloc_asprintf(value, "%ld", value->v.lval);
        case HANDLEBARS_VALUE_TYPE_FLOAT:
            return handlebars_talloc_asprintf(value, "%f", value->v.lval);
        case HANDLEBARS_VALUE_TYPE_BOOLEAN:
            return handlebars_talloc_strdup(value, value->v.bval ? "true" : "false");
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

void handlebars_value_convert_ex(struct handlebars_value * value, short recurse)
{
    struct handlebars_value_iterator * it;

    switch( value->type ) {
        case HANDLEBARS_VALUE_TYPE_USER:
            value->handlers->convert(value, recurse);
            break;
        case HANDLEBARS_VALUE_TYPE_MAP:
        case HANDLEBARS_VALUE_TYPE_ARRAY:
            it = handlebars_value_iterator_ctor(value);
            for( ; it->current != NULL; handlebars_value_iterator_next(it) ) {
                handlebars_value_convert_ex(it->current, recurse);
            }
            break;
    }
}

struct handlebars_value_iterator * handlebars_value_iterator_ctor(struct handlebars_value * value)
{
    struct handlebars_value_iterator * it;
    struct handlebars_map_entry * entry;

    switch( value->type ) {
        case HANDLEBARS_VALUE_TYPE_ARRAY:
            it = handlebars_talloc_zero(value, struct handlebars_value_iterator);
            it->value = value;
            it->current = handlebars_stack_get(value->v.stack, 0);
            break;
        case HANDLEBARS_VALUE_TYPE_MAP:
            it = handlebars_talloc_zero(value, struct handlebars_value_iterator);
            entry = value->v.map->first;
            if( entry ) {
                it->value = value;
                it->usr = (void *) entry;
                it->key = entry->key;
                it->current = entry->value;
                handlebars_value_addref(it->current);
            }
            break;
        case HANDLEBARS_VALUE_TYPE_USER:
            it = value->handlers->iterator(value);
            break;
        default:
            it->current = 1;
            assert(0);
            it = handlebars_talloc_zero(value, struct handlebars_value_iterator);
            break;
    }

    return it;
}

short handlebars_value_iterator_next(struct handlebars_value_iterator * it)
{
    assert(it != NULL);
    assert(it->value != NULL);

    struct handlebars_value * value = it->value;
    struct handlebars_map_entry * entry;
    struct handlebars_value * current = it->current;
    short ret = 0;

    if( it->current != NULL ) {
        handlebars_value_delref(it->current);
        it->current = NULL;
    }

    switch( value->type ) {
        case HANDLEBARS_VALUE_TYPE_ARRAY:
            if( it->index < handlebars_stack_length(value->v.stack) - 1 ) {
                ret = 1;
                it->index++;
                it->current = handlebars_stack_get(value->v.stack, it->index);
            }
            break;
        case HANDLEBARS_VALUE_TYPE_MAP:
            entry = talloc_get_type(it->usr, struct handlebars_map_entry);
            if( entry && entry->next ) {
                ret = 1;
                it->usr = (void *) (entry = entry->next);
                it->key = entry->key;
                it->current = entry->value;
                handlebars_value_addref(it->current);
            }
            break;
        case HANDLEBARS_VALUE_TYPE_USER:
            ret = value->handlers->next(it);
            break;
    }

    it->eof = !ret;

    return ret;
}

char * handlebars_value_dump(struct handlebars_value * value, size_t depth)
{
    char * buf = handlebars_talloc_strdup(value->ctx, "");
    struct handlebars_value_iterator * it;
    char indent[64];
    char indent2[64];

    memset(indent, 0, sizeof(indent));
    memset(indent, ' ', depth * 4);

    memset(indent2, 0, sizeof(indent2));
    memset(indent2, ' ', (depth + 1) * 4);

    switch( handlebars_value_get_type(value) ) {
        case HANDLEBARS_VALUE_TYPE_BOOLEAN:
            buf = handlebars_talloc_asprintf_append_buffer(buf, "boolean(%s)", value->v.bval ? "true" : "false");
            break;
        case HANDLEBARS_VALUE_TYPE_FLOAT:
            buf = handlebars_talloc_asprintf_append_buffer(buf, "float(%f)", value->v.dval);
            break;
        case HANDLEBARS_VALUE_TYPE_INTEGER:
            buf = handlebars_talloc_asprintf_append_buffer(buf, "integer(%ld)", value->v.lval);
            break;
        case HANDLEBARS_VALUE_TYPE_NULL:
            buf = handlebars_talloc_asprintf_append_buffer(buf, "NULL");
            break;
        case HANDLEBARS_VALUE_TYPE_STRING:
            buf = handlebars_talloc_asprintf_append_buffer(buf, "string(%s)", value->v.strval);
            break;
        case HANDLEBARS_VALUE_TYPE_ARRAY:
            buf = handlebars_talloc_asprintf_append_buffer(buf, "%s\n", "[");
            it = handlebars_value_iterator_ctor(value);
            for( ; it->current != NULL; handlebars_value_iterator_next(it) ) {
                char * tmp = handlebars_value_dump(it->current, depth + 1);
                buf = handlebars_talloc_asprintf_append_buffer(buf, "%s%d => %s\n", indent2, it->index, tmp);
                handlebars_talloc_free(tmp);
            }
            buf = handlebars_talloc_asprintf_append_buffer(buf, "%s%s", indent, "]");
            break;
        case HANDLEBARS_VALUE_TYPE_MAP:
            buf = handlebars_talloc_asprintf_append_buffer(buf, "%s\n", "{");
            it = handlebars_value_iterator_ctor(value);
            for( ; it->current != NULL; handlebars_value_iterator_next(it) ) {
                char * tmp = handlebars_value_dump(it->current, depth + 1);
                buf = handlebars_talloc_asprintf_append_buffer(buf, "%s%s => %s\n", indent2, it->key, tmp);
                handlebars_talloc_free(tmp);
            }
            buf = handlebars_talloc_asprintf_append_buffer(buf, "%s%s", indent, "}");
            break;
        default:
            buf = handlebars_talloc_asprintf_append_buffer(buf, "unknown type %d", value->type);
            break;
    }

    return buf;
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

    if( escape && !(value->flags & HANDLEBARS_VALUE_FLAG_SAFE_STRING) ) {
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
        value->refcount = 1;
	}
	return value;
}

void handlebars_value_dtor(struct handlebars_value * value)
{
    // Release old value
    switch( value->type ) {
        case HANDLEBARS_VALUE_TYPE_ARRAY:
            handlebars_stack_dtor(value->v.stack);
            break;
        case HANDLEBARS_VALUE_TYPE_MAP:
            handlebars_map_dtor(value->v.map);
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
    value->type = HANDLEBARS_VALUE_TYPE_NULL;
    value->handlers = NULL;
    memset(&value->v, 0, sizeof(value->v));
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
                // Increment refcount?
                json_object_get(json);
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
