
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <string.h>
#include <talloc.h>
#include <yaml.h>

#if defined(HAVE_JSON_C_JSON_H)
#include <json-c/json.h>
#include <json-c/json_object.h>
#include <json-c/json_tokener.h>
#elif defined(HAVE_JSON_JSON_H)
#include <json/json.h>
#include <json/json_object.h>
#include <json/json_tokener.h>
#endif

#include "handlebars_context.h"
#include "handlebars_map.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"
#include "handlebars_stack.h"
#include "handlebars_utils.h"
#include "handlebars_value.h"
#include "handlebars_value_handlers.h"

#define __S1(x) #x
#define __S2(x) __S1(x)
#define __MEMCHECK(cond) \
    do { \
        if( unlikely(!cond) ) { \
            handlebars_context_throw(CONTEXT, HANDLEBARS_NOMEM, "Out of memory  [" __S2(__FILE__) ":" __S2(__LINE__) "]"); \
        } \
    } while(0)



#define CONTEXT ctx

struct handlebars_value * handlebars_value_ctor(struct handlebars_context * ctx)
{
    struct handlebars_value * value = handlebars_talloc_zero(ctx, struct handlebars_value);
    __MEMCHECK(value);
    value->ctx = ctx;
    value->refcount = 1;
    return value;
}

#undef CONTEXT
#define CONTEXT value->ctx

enum handlebars_value_type handlebars_value_get_type(struct handlebars_value * value)
{
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

char * handlebars_value_get_strval(struct handlebars_value * value)
{
    char * ret;
    enum handlebars_value_type type = value ? value->type : HANDLEBARS_VALUE_TYPE_NULL;

    switch( type ) {
        case HANDLEBARS_VALUE_TYPE_STRING:
            ret = handlebars_talloc_strdup(value, value->v.strval);
            break;
        case HANDLEBARS_VALUE_TYPE_INTEGER:
            ret = handlebars_talloc_asprintf(value, "%ld", value->v.lval);
            break;
        case HANDLEBARS_VALUE_TYPE_FLOAT:
            ret = handlebars_talloc_asprintf(value, "%g", value->v.dval);
            break;
        case HANDLEBARS_VALUE_TYPE_BOOLEAN:
            ret = handlebars_talloc_strdup(value, value->v.bval ? "true" : "false");
            break;
        default:
            ret = handlebars_talloc_strdup(value, "");
            break;
    }

    __MEMCHECK(ret);

    return ret;
}

size_t handlebars_value_get_strlen(struct handlebars_value * value)
{
	if( value->type == HANDLEBARS_VALUE_TYPE_STRING ) {
		return strlen(value->v.strval);
	}

	return 0;
}

bool handlebars_value_get_boolval(struct handlebars_value * value)
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

void handlebars_value_convert_ex(struct handlebars_value * value, bool recurse)
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
            handlebars_talloc_free(it);
            break;
        default:
            // do nothing
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
            __MEMCHECK(it);
            it->value = value;
            it->current = handlebars_stack_get(value->v.stack, 0);
            break;
        case HANDLEBARS_VALUE_TYPE_MAP:
            it = handlebars_talloc_zero(value, struct handlebars_value_iterator);
            __MEMCHECK(it);
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
            it = handlebars_talloc_zero(value, struct handlebars_value_iterator);
            __MEMCHECK(it);
            break;
    }

    return it;
}

bool handlebars_value_iterator_next(struct handlebars_value_iterator * it)
{
    assert(it != NULL);
    assert(it->value != NULL);

    struct handlebars_value * value = it->value;
    struct handlebars_map_entry * entry;
    bool ret = false;

    if( it->current != NULL ) {
        handlebars_value_delref(it->current);
        it->current = NULL;
    }

    switch( value->type ) {
        case HANDLEBARS_VALUE_TYPE_ARRAY:
            if( it->index < handlebars_stack_length(value->v.stack) - 1 ) {
                ret = true;
                it->index++;
                it->current = handlebars_stack_get(value->v.stack, it->index);
            }
            break;
        case HANDLEBARS_VALUE_TYPE_MAP:
            entry = talloc_get_type(it->usr, struct handlebars_map_entry);
            if( entry && entry->next ) {
                ret = true;
                it->usr = (void *) (entry = entry->next);
                it->key = entry->key;
                it->current = entry->value;
                handlebars_value_addref(it->current);
            }
            break;
        case HANDLEBARS_VALUE_TYPE_USER:
            ret = value->handlers->next(it);
            break;
        default:
            // do nothing
            break;
    }

    return ret;
}

char * handlebars_value_dump(struct handlebars_value * value, size_t depth)
{
    char * buf = handlebars_talloc_strdup(value->ctx, "");
    struct handlebars_value_iterator * it;
    char indent[64];
    char indent2[64];

    __MEMCHECK(buf);

    if( value == NULL ) {
        handlebars_talloc_strdup_append_buffer(buf, "(nil)");
        return buf;
    }

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

    __MEMCHECK(buf);

    return buf;
}

char * handlebars_value_expression(struct handlebars_value * value, bool escape)
{
    char * ret = NULL;
    struct handlebars_value_iterator * it;

    switch( handlebars_value_get_type(value) ) {
        case HANDLEBARS_VALUE_TYPE_BOOLEAN:
            if( handlebars_value_get_boolval(value) ) {
                ret = handlebars_talloc_strdup(value->ctx, "true");
            } else {
                ret = handlebars_talloc_strdup(value->ctx, "false");
            }
            break;

        case HANDLEBARS_VALUE_TYPE_FLOAT:
            ret = handlebars_talloc_asprintf(value->ctx, "%g", handlebars_value_get_floatval(value));
            break;

        case HANDLEBARS_VALUE_TYPE_INTEGER:
            ret = handlebars_talloc_asprintf(value->ctx, "%ld", handlebars_value_get_intval(value));
            break;

        case HANDLEBARS_VALUE_TYPE_NULL:
            ret = handlebars_talloc_strdup(value->ctx, "");
            break;

        case HANDLEBARS_VALUE_TYPE_STRING:
            ret = handlebars_talloc_strdup(value->ctx, handlebars_value_get_strval(value));
            break;

        case HANDLEBARS_VALUE_TYPE_ARRAY:
            // Convert to string >.>
            ret = handlebars_talloc_strdup(value->ctx, "");
            it = handlebars_value_iterator_ctor(value);
            bool first = true;
            for( ; it->current != NULL; handlebars_value_iterator_next(it) ) {
                char * tmp = handlebars_value_expression(it->current, escape);
                ret = handlebars_talloc_asprintf_append_buffer(ret, "%s%s", first ? "" : ",", tmp);
                handlebars_talloc_free(tmp);
                first = false;
            }
            handlebars_talloc_free(it);
            break;

        case HANDLEBARS_VALUE_TYPE_MAP:
        case HANDLEBARS_VALUE_TYPE_USER:
        default:
            ret = handlebars_talloc_strdup(value->ctx, "");
            break;
    }

    __MEMCHECK(ret);

    if( escape && !(value->flags & HANDLEBARS_VALUE_FLAG_SAFE_STRING) ) {
        char * esc = handlebars_htmlspecialchars(ret);
        handlebars_talloc_free(ret);
        ret = esc;
    }

    __MEMCHECK(ret);

    return ret;
}

struct handlebars_value * handlebars_value_copy(struct handlebars_value * value)
{
    struct handlebars_value * new_value;
    struct handlebars_value_iterator * it;

    switch( value->type ) {
        case HANDLEBARS_VALUE_TYPE_ARRAY:
            new_value = handlebars_value_ctor(value->ctx);
            handlebars_value_array_init(new_value);
            it = handlebars_value_iterator_ctor(value);
            for( ; it->current != NULL; handlebars_value_iterator_next(it) ) {
                handlebars_stack_set(new_value->v.stack, it->index, it->current);
            }
            break;
        case HANDLEBARS_VALUE_TYPE_MAP:
            new_value = handlebars_value_ctor(value->ctx);
            handlebars_value_map_init(new_value);
            it = handlebars_value_iterator_ctor(value);
            for( ; it->current != NULL; handlebars_value_iterator_next(it) ) {
                handlebars_map_add(new_value->v.map, it->key, it->current);
            }
            break;
        case HANDLEBARS_VALUE_TYPE_USER:
            new_value = value->handlers->copy(value);
            break;

        default:
            new_value = handlebars_value_ctor(value->ctx);
            memcpy(&new_value->v, &value->v, sizeof(value->v));
            break;
    }

    return new_value;
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

    talloc_free_children(value);

    // Initialize to null
    value->type = HANDLEBARS_VALUE_TYPE_NULL;
    value->handlers = NULL;
    memset(&value->v, 0, sizeof(value->v));
}

#undef CONTEXT




#define CONTEXT ctx

struct handlebars_value * handlebars_value_from_json_object(struct handlebars_context *ctx, struct json_object *json)
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
                __MEMCHECK(value->v.strval);
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

struct handlebars_value * handlebars_value_from_json_string(struct handlebars_context *ctx, const char * json)
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




struct _yaml_ctx {
    yaml_parser_t parser;
    yaml_document_t document;
};

static void _yaml_ctx_dtor(struct _yaml_ctx * holder)
{
    yaml_document_delete(&holder->document);
    yaml_parser_delete(&holder->parser);
}

struct handlebars_value * handlebars_value_from_yaml_node(struct handlebars_context *ctx, struct yaml_document_s * document, struct yaml_node_s * node)
{
    struct handlebars_value * value = handlebars_value_ctor(ctx);
    struct handlebars_value * tmp;
    yaml_node_pair_t * pair;
    yaml_node_item_t * item;
    char * end = NULL;

    switch( node->type ) {
        case YAML_MAPPING_NODE:
            handlebars_value_map_init(value);
            for( pair = node->data.mapping.pairs.start; pair < node->data.mapping.pairs.top; pair++ ) {
                yaml_node_t * keyNode = yaml_document_get_node(document, pair->key);
                yaml_node_t * valueNode = yaml_document_get_node(document, pair->value);
                assert(keyNode->type == YAML_SCALAR_NODE);
                tmp = handlebars_value_from_yaml_node(ctx, document, valueNode);
                handlebars_map_add(value->v.map, keyNode->data.scalar.value, tmp);
            }
            break;
        case YAML_SEQUENCE_NODE:
            handlebars_value_array_init(value);
            for( item = node->data.sequence.items.start; item < node->data.sequence.items.top; item ++) {
                yaml_node_t * valueNode = yaml_document_get_node(document, *item);
                tmp = handlebars_value_from_yaml_node(ctx, document, valueNode);
                handlebars_stack_push(value->v.stack, tmp);
            }
            break;
        case YAML_SCALAR_NODE:
            if( 0 == strcmp(node->data.scalar.value, "true") ) {
                handlebars_value_boolean(value, 1);
            } else if( 0 == strcmp(node->data.scalar.value, "false") ) {
                handlebars_value_boolean(value, 0);
            } else {
                long lval;
                double dval;
                // Long
                lval = strtol(node->data.scalar.value, &end, 10);
                if( !*end ) {
                    handlebars_value_integer(value, lval);
                    goto done;
                }
                // Double
                dval = strtod(node->data.scalar.value, &end);
                if( !*end ) {
                    handlebars_value_float(value, dval);
                    goto done;
                }
                // String
                value->type = HANDLEBARS_VALUE_TYPE_STRING;
                value->v.strval = handlebars_talloc_strndup(value, node->data.scalar.value, node->data.scalar.length);
                __MEMCHECK(value->v.strval);
            }
            break;
        default:
            // ruh roh
            assert(0);
            break;
    }

done:
    return value;
}

struct handlebars_value * handlebars_value_from_yaml_string(struct handlebars_context * ctx, const char * yaml)
{
    struct handlebars_value * value = NULL;
    struct _yaml_ctx * yctx = talloc_zero(ctx, struct _yaml_ctx);
    __MEMCHECK(yctx);
    talloc_set_destructor(yctx, _yaml_ctx_dtor);
    yaml_parser_initialize(&yctx->parser);
    yaml_parser_set_input_string(&yctx->parser, (unsigned char *) yaml, strlen(yaml));
    yaml_parser_load(&yctx->parser, &yctx->document);
    yaml_node_t * node = yaml_document_get_root_node(&yctx->document);
    if( node ) {
        value = handlebars_value_from_yaml_node(ctx, &yctx->document, node);
#ifndef NDEBUG
    } else {
        fprintf(stderr, "YAML error: %d:\n", yctx->parser.error);
#endif
    }
    handlebars_talloc_free(yctx);
    return value;
}
