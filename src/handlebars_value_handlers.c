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

#define HANDLEBARS_VALUE_HANDLERS_PRIVATE

#include "handlebars.h"
#include "handlebars_private.h"
#include "handlebars_memory.h"
#include "handlebars_value.h"
#include "handlebars_value_handlers.h"



#define HANDLEBARS_JSON(value) ((struct handlebars_json *) talloc_get_type(value->v.usr, struct handlebars_json))
#define HANDLEBARS_JSON_OBJ(value) HANDLEBARS_JSON(value)->object

struct handlebars_json {
    struct handlebars_user usr;
    struct json_object * object;
};

struct _yaml_ctx {
    yaml_parser_t parser;
    yaml_document_t document;
};


static int handlebars_json_dtor(struct handlebars_json * obj)
{
    if( obj ) {
        json_object_put(obj->object);
    }

    return 0;
}



#undef CONTEXT
#define CONTEXT HBSCTX(value->ctx)

static struct handlebars_value * std_json_copy(struct handlebars_value * value)
{
    struct json_object * object = HANDLEBARS_JSON_OBJ(value);
    const char * str = json_object_to_json_string(object);
    return handlebars_value_from_json_string(CONTEXT, str);
}

static void std_json_dtor(struct handlebars_value * value)
{
    struct handlebars_json * json = HANDLEBARS_JSON(value);
    handlebars_talloc_free(json);
    value->v.usr = NULL;
}

static void std_json_convert(struct handlebars_value * value, bool recurse)
{
    struct json_object * intern = HANDLEBARS_JSON_OBJ(value);
    struct handlebars_value * new_value;
    size_t i;
    size_t l;

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

        default: break; // LCOV_EXCL_LINE
    }
}

static enum handlebars_value_type std_json_type(struct handlebars_value * value)
{
    struct json_object * intern = HANDLEBARS_JSON_OBJ(value);

    switch( json_object_get_type(intern) ) {
        case json_type_object: return HANDLEBARS_VALUE_TYPE_MAP;
        case json_type_array: return HANDLEBARS_VALUE_TYPE_ARRAY;
        default: assert(0); break; // LCOV_EXCL_LINE
    }

    return HANDLEBARS_VALUE_TYPE_NULL;
}

static struct handlebars_value * std_json_map_find(struct handlebars_value * value, struct handlebars_string * key)
{
    struct json_object * intern = HANDLEBARS_JSON_OBJ(value);
    struct json_object * item = json_object_object_get(intern, key->val);
    if( item == NULL ) {
        return NULL;
    }
    return handlebars_value_from_json_object(CONTEXT, item);
}

static struct handlebars_value * std_json_array_find(struct handlebars_value * value, size_t index)
{
    struct json_object * intern = HANDLEBARS_JSON_OBJ(value);
    struct json_object * item = json_object_array_get_idx(intern, (int) index);
    if( item == NULL ) {
        return NULL;
    }
    return handlebars_value_from_json_object(CONTEXT, item);
}

static bool std_json_iterator_next_void(struct handlebars_value_iterator * it)
{
    return false;
}

static bool std_json_iterator_next_object(struct handlebars_value_iterator * it)
{
    struct handlebars_value * value = it->value;
    struct lh_entry * entry;
    char * tmp;

    assert(value != NULL);
    assert(value->type == HANDLEBARS_VALUE_TYPE_USER);
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
    it->key = handlebars_string_ctor(value->ctx, tmp, strlen(tmp));
    it->current = handlebars_value_from_json_object(CONTEXT, (struct json_object *) entry->v);
    return true;
}

static bool std_json_iterator_next_array(struct handlebars_value_iterator * it)
{
    struct handlebars_value * value = it->value;
    struct json_object * intern = HANDLEBARS_JSON_OBJ(value);

    assert(value != NULL);
    assert(value->type == HANDLEBARS_VALUE_TYPE_USER);
    assert(it->current != NULL);

    handlebars_value_delref(it->current);
    it->current = NULL;

    if( it->index >= json_object_array_length(intern) - 1 ) {
        return false;
    }

    it->index++;
    it->current = handlebars_value_from_json_object(CONTEXT, json_object_array_get_idx(intern, it->index));
    return true;
}

bool std_json_iterator_init(struct handlebars_value_iterator * it, struct handlebars_value * value)
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
                it->key = handlebars_string_ctor(value->ctx, tmp, strlen(tmp));
                it->current = handlebars_value_from_json_object(CONTEXT, (json_object *) entry->v);
                it->length = (size_t) json_object_object_length(intern);
                it->next = &std_json_iterator_next_object;
                return true;
            } else {
                it->next = &std_json_iterator_next_void;
            }
            break;
        case json_type_array:
            it->index = 0;
            it->current = handlebars_value_from_json_object(CONTEXT, json_object_array_get_idx(intern, (int) it->index));
            it->length = (size_t) json_object_array_length(intern);
            it->next = &std_json_iterator_next_array;
            return true;
        default:
            it->next = &std_json_iterator_next_void;
            break;
    }

    return false;
}

long std_json_count(struct handlebars_value * value)
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

static struct handlebars_value_handlers handlebars_value_std_json_handlers = {
    "json",
    &std_json_copy,
    &std_json_dtor,
    &std_json_convert,
    &std_json_type,
    &std_json_map_find,
    &std_json_array_find,
    &std_json_iterator_init,
    NULL, // call
    &std_json_count
};

struct handlebars_value_handlers * handlebars_value_get_std_json_handlers()
{
    return &handlebars_value_std_json_handlers;
}

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
            if( json_object_get_boolean(json) ) {
                value->type = HANDLEBARS_VALUE_TYPE_TRUE;
            } else {
                value->type = HANDLEBARS_VALUE_TYPE_FALSE;
            }
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
            handlebars_value_stringl(value, json_object_get_string(json), json_object_get_string_len(json));
            break;

        case json_type_object:
        case json_type_array:
            // Increment refcount
            json_object_get(json);

            obj = MC(handlebars_talloc(ctx, struct handlebars_json));
            obj->usr.handlers = handlebars_value_get_std_json_handlers();
            obj->object = json;
            talloc_set_destructor(obj, handlebars_json_dtor);

            value->type = HANDLEBARS_VALUE_TYPE_USER;
            value->v.usr = (struct handlebars_user *) obj;
            break;
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
    return value;
}

struct handlebars_value * handlebars_value_from_json_string(struct handlebars_context *ctx, const char * json)
{
    struct handlebars_value * value = handlebars_value_ctor(ctx);
    handlebars_value_init_json_string(ctx, value, json);
    return value;
}




static int _yaml_ctx_dtor(struct _yaml_ctx * holder)
{
    yaml_document_delete(&holder->document);
    yaml_parser_delete(&holder->parser);
    return 0;
}

void handlebars_value_init_yaml_node(struct handlebars_context *ctx, struct handlebars_value * value, struct yaml_document_s * document, struct yaml_node_s * node)
{
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
                handlebars_map_str_update(value->v.map, (const char *) keyNode->data.scalar.value, keyNode->data.scalar.length, tmp);
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
            if( 0 == strcmp((const char *) node->data.scalar.value, "true") ) {
                handlebars_value_boolean(value, 1);
            } else if( 0 == strcmp((const char *) node->data.scalar.value, "false") ) {
                handlebars_value_boolean(value, 0);
            } else {
                long lval;
                double dval;
                // Long
                lval = strtol((const char *) node->data.scalar.value, &end, 10);
                if( !*end ) {
                    handlebars_value_integer(value, lval);
                    return;
                }
                // Double
                dval = strtod((const char *) node->data.scalar.value, &end);
                if( !*end ) {
                    handlebars_value_float(value, dval);
                    return;
                }
                // String
                handlebars_value_stringl(value, (const char *) node->data.scalar.value, node->data.scalar.length);
            }
            break;
        default:
            // ruh roh
            assert(0);
            break;
    }
}

void handlebars_value_init_yaml_string(struct handlebars_context * ctx, struct handlebars_value * value, const char * yaml)
{
    struct _yaml_ctx * yctx = MC(handlebars_talloc_zero(ctx, struct _yaml_ctx));
    talloc_set_destructor(yctx, _yaml_ctx_dtor);
    yaml_parser_initialize(&yctx->parser);
    yaml_parser_set_input_string(&yctx->parser, (unsigned char *) yaml, strlen(yaml));
    yaml_parser_load(&yctx->parser, &yctx->document);
    yaml_node_t * node = yaml_document_get_root_node(&yctx->document);
    // @todo test parse error
    if( node ) {
        handlebars_value_init_yaml_node(ctx, value, &yctx->document, node);
    } else {
        handlebars_throw(ctx, HANDLEBARS_ERROR, "YAML Parse Error: [%d] %s", yctx->parser.error, yctx->parser.problem);
    }
    handlebars_talloc_free(yctx);
}

struct handlebars_value * handlebars_value_from_yaml_node(struct handlebars_context *ctx, struct yaml_document_s * document, struct yaml_node_s * node)
{
    struct handlebars_value * value = handlebars_value_ctor(ctx);
    handlebars_value_init_yaml_node(ctx, value, document, node);
    return value;
}

struct handlebars_value * handlebars_value_from_yaml_string(struct handlebars_context * ctx, const char * yaml)
{
    struct handlebars_value * value = handlebars_value_ctor(ctx);
    handlebars_value_init_yaml_string(ctx, value, yaml);
    return value;
}

extern inline handlebars_count_func handlebars_value_handlers_get_count_fn(struct handlebars_value_handlers * handlers);
