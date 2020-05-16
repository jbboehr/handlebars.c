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

#include <yaml.h>

#include "handlebars.h"
#include "handlebars_private.h"
#include "handlebars_memory.h"
#include "handlebars_value.h"
#include "handlebars_yaml.h"



#undef CONTEXT
#define CONTEXT HBSCTX(handlebars_value_get_ctx(value))

struct _yaml_ctx {
    yaml_parser_t parser;
    yaml_document_t document;
};

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
        case YAML_MAPPING_NODE: {
            struct handlebars_map * map = handlebars_map_ctor(ctx, node->data.mapping.pairs.top - node->data.mapping.pairs.start);
            for( pair = node->data.mapping.pairs.start; pair < node->data.mapping.pairs.top; pair++ ) {
                yaml_node_t * keyNode = yaml_document_get_node(document, pair->key);
                yaml_node_t * valueNode = yaml_document_get_node(document, pair->value);
                assert(keyNode->type == YAML_SCALAR_NODE);
                tmp = handlebars_value_from_yaml_node(ctx, document, valueNode);
                handlebars_map_str_update(map, (const char *) keyNode->data.scalar.value, keyNode->data.scalar.length, tmp);
            }
            handlebars_value_map(value, map);
            break;
        }
        case YAML_SEQUENCE_NODE: {
            struct handlebars_stack * stack = handlebars_stack_ctor(ctx, node->data.sequence.items.top - node->data.sequence.items.start);
            for( item = node->data.sequence.items.start; item < node->data.sequence.items.top; item++) {
                yaml_node_t * valueNode = yaml_document_get_node(document, *item);
                tmp = handlebars_value_from_yaml_node(ctx, document, valueNode);
                stack = handlebars_stack_push(stack, tmp);
            }
            handlebars_value_array(value, stack);
            break;
        }
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
