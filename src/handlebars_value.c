
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
	}

	return NULL;
}

struct handlebars_value * handlebars_value_array_find(struct handlebars_value * value, size_t index)
{
	if( value->type == HANDLEBARS_VALUE_TYPE_USER ) {
		if( handlebars_value_get_type(value) == HANDLEBARS_VALUE_TYPE_ARRAY ) {
			return value->handlers->array_find(value, index);
		}
	}

	return NULL;
}

const char * handlebars_value_get_strval(struct handlebars_value * value)
{
	if( value->type == HANDLEBARS_VALUE_TYPE_USER ) {
		if( handlebars_value_get_type(value) == HANDLEBARS_VALUE_TYPE_STRING ) {
			return value->handlers->strval(value);
		}
	} else if( value->type == HANDLEBARS_VALUE_TYPE_STRING ) {
		return value->v.strval;
	}

	return NULL;
}

size_t handlebars_value_get_strlen(struct handlebars_value * value)
{
	if( value->type == HANDLEBARS_VALUE_TYPE_USER ) {
		if( handlebars_value_get_type(value) == HANDLEBARS_VALUE_TYPE_STRING ) {
			return value->handlers->strlen(value);
		}
	} else if( value->type == HANDLEBARS_VALUE_TYPE_STRING ) {
		return strlen(value->v.strval);
	}

	return 0;
}

short handlebars_value_get_boolval(struct handlebars_value * value)
{
	if( value->type == HANDLEBARS_VALUE_TYPE_USER ) {
		if( handlebars_value_get_type(value) == HANDLEBARS_VALUE_TYPE_BOOLEAN ) {
			return value->handlers->boolval(value);
		}
	} else if( value->type == HANDLEBARS_VALUE_TYPE_BOOLEAN ) {
        return value->v.bval;
	}

	return 0;
}

long handlebars_value_get_intval(struct handlebars_value * value)
{
	if( value->type == HANDLEBARS_VALUE_TYPE_USER ) {
		if( handlebars_value_get_type(value) == HANDLEBARS_VALUE_TYPE_INTEGER ) {
			return value->handlers->intval(value);
		}
	} else if( value->type == HANDLEBARS_VALUE_TYPE_INTEGER ) {
        return value->v.lval;
	}

	return 0;
}

double handlebars_value_get_floatval(struct handlebars_value * value)
{
	if( value->type == HANDLEBARS_VALUE_TYPE_USER ) {
		if( handlebars_value_get_type(value) == HANDLEBARS_VALUE_TYPE_FLOAT ) {
			return value->handlers->floatval(value);
		}
	} else if( value->type == HANDLEBARS_VALUE_TYPE_FLOAT ) {
        return value->v.dval;
	}

	return 0;
}



char * handlebars_value_expression(void * ctx, struct handlebars_value * value, short escape)
{
    char * ret;

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
	return handlebars_talloc_zero(ctx, struct handlebars_value);
}

struct handlebars_value * handlebars_value_from_json_object(void *ctx, struct json_object *json)
{
    struct handlebars_value * ret;

    ret = handlebars_value_ctor(ctx);
    if( ret ) {
        ret->type = HANDLEBARS_VALUE_TYPE_USER;
        ret->handlers = handlebars_value_get_std_json_handlers();
        ret->v.usr = (void *) json;
    }

    return ret;
}

struct handlebars_value * handlebars_value_from_json_string(void *ctx, const char * json)
{
    struct handlebars_value * ret;
    struct json_object * result = json_tokener_parse(json);
    if( result ) {
        ret = handlebars_value_from_json_object(ctx, result);
        if( ret ) {
            talloc_set_destructor(ret, handlebars_value_json_dtor);
        }
    }
    return ret;
}
