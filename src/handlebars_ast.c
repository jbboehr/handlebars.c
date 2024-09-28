/**
 * Copyright (c) anno Domini nostri Jesu Christi MMXVI-MMXXIV John Boehr & contributors
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define HANDLEBARS_AST_PRIVATE
#define HANDLEBARS_AST_LIST_PRIVATE

#include "handlebars.h"
#include "handlebars_ast.h"
#include "handlebars_ast_helpers.h"
#include "handlebars_ast_list.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"
#include "handlebars_string.h"



const size_t HANDLEBARS_AST_NODE_SIZE = sizeof(struct handlebars_ast_node);

#undef CONTEXT
#define CONTEXT HBSCTX(context)

struct handlebars_ast_node * handlebars_ast_node_ctor(struct handlebars_context * context, enum handlebars_ast_node_type type)
{
    struct handlebars_ast_node * ast_node;
    ast_node = MC(handlebars_talloc_zero(context, struct handlebars_ast_node));
    ast_node->type = type;
    return ast_node;
}

#undef CONTEXT
#define CONTEXT HBSCTX(parser)

void handlebars_ast_node_dtor(struct handlebars_ast_node * ast_node)
{
    handlebars_talloc_free(ast_node);
}

struct handlebars_string * handlebars_ast_node_get_id_part(struct handlebars_ast_node * ast_node)
{
    struct handlebars_ast_list * parts;
    struct handlebars_ast_node * path_segment;

    assert(ast_node != NULL);
    assert(ast_node->type == HANDLEBARS_AST_NODE_PATH);

    parts = ast_node->node.path.parts;
    if( parts == NULL || parts->first == NULL || parts->first->data == NULL ) {
        return NULL;
    }

    path_segment = parts->first->data;

    assert(path_segment->type == HANDLEBARS_AST_NODE_PATH_SEGMENT);
    return path_segment->node.path_segment.part;
}

struct handlebars_string * handlebars_ast_node_get_string_mode_value(
        struct handlebars_context * context,
        struct handlebars_ast_node * node
) {
    struct handlebars_string * string;

    assert(node != NULL);

    switch( node->type ) {
        case HANDLEBARS_AST_NODE_PATH:
            string = node->node.path.original;
            break;
        case HANDLEBARS_AST_NODE_STRING:
            string = node->node.string.value;
            break;
        case HANDLEBARS_AST_NODE_NUMBER:
            string = node->node.number.value;
            break;
        case HANDLEBARS_AST_NODE_BOOLEAN:
            string = node->node.boolean.value;
            break;
        case HANDLEBARS_AST_NODE_UNDEFINED:
            string = node->node.undefined.value;
            break;
        case HANDLEBARS_AST_NODE_NUL:
            string = node->node.nul.value;
            break;
        default:
            string = handlebars_string_ctor(context, HBS_STRL(""));
            break;
    }

    return string;
}

struct handlebars_ast_node * handlebars_ast_node_get_path(struct handlebars_ast_node * node)
{
    switch( node->type ) {
        case HANDLEBARS_AST_NODE_BLOCK:
            return node->node.block.path;
        case HANDLEBARS_AST_NODE_INTERMEDIATE:
            return node->node.intermediate.path;
        case HANDLEBARS_AST_NODE_MUSTACHE:
            return node->node.mustache.path;
        case HANDLEBARS_AST_NODE_PARTIAL:
            return node->node.partial.name;
        case HANDLEBARS_AST_NODE_PARTIAL_BLOCK:
            return node->node.partial_block.path;
        case HANDLEBARS_AST_NODE_SEXPR:
            return node->node.sexpr.path;
        case HANDLEBARS_AST_NODE_RAW_BLOCK:
            return node->node.raw_block.path;
        default:
            return NULL;
    }
}

struct handlebars_ast_list * handlebars_ast_node_get_params(struct handlebars_ast_node * node)
{
    switch( node->type ) {
        case HANDLEBARS_AST_NODE_BLOCK:
            return node->node.block.params;
        case HANDLEBARS_AST_NODE_INTERMEDIATE:
            return node->node.intermediate.params;
        case HANDLEBARS_AST_NODE_MUSTACHE:
            return node->node.mustache.params;
        case HANDLEBARS_AST_NODE_PARTIAL:
            return node->node.partial.params;
        case HANDLEBARS_AST_NODE_PARTIAL_BLOCK:
            return node->node.partial_block.params;
        case HANDLEBARS_AST_NODE_SEXPR:
            return node->node.sexpr.params;
        case HANDLEBARS_AST_NODE_RAW_BLOCK:
            return node->node.raw_block.params;
        default:
            return NULL;
    }
}

struct handlebars_ast_node * handlebars_ast_node_get_hash(struct handlebars_ast_node * node)
{
    switch( node->type ) {
        case HANDLEBARS_AST_NODE_BLOCK:
            return node->node.block.hash;
        case HANDLEBARS_AST_NODE_INTERMEDIATE:
            return node->node.intermediate.hash;
        case HANDLEBARS_AST_NODE_MUSTACHE:
            return node->node.mustache.hash;
        case HANDLEBARS_AST_NODE_PARTIAL:
            return node->node.partial.hash;
        case HANDLEBARS_AST_NODE_PARTIAL_BLOCK:
            return node->node.partial_block.hash;
        case HANDLEBARS_AST_NODE_SEXPR:
            return node->node.sexpr.hash;
        case HANDLEBARS_AST_NODE_RAW_BLOCK:
            return node->node.raw_block.hash;
        default:
            return NULL;
    }
}

const char * handlebars_ast_node_readable_type(int type)
{
#define _RTYPE_STR(str) #str
#define _RTYPE_MK(str) HANDLEBARS_AST_NODE_ ## str
#define _RTYPE_CASE(str, name) \
    case _RTYPE_MK(str): return _RTYPE_STR(name); break
  switch( type ) {
    _RTYPE_CASE(NIL, NIL);
    _RTYPE_CASE(BLOCK, block);
    _RTYPE_CASE(BOOLEAN, BooleanLiteral);
    _RTYPE_CASE(COMMENT, comment);
    _RTYPE_CASE(CONTENT, content);
    _RTYPE_CASE(HASH, hash);
    _RTYPE_CASE(HASH_PAIR, HASH_PAIR);
    _RTYPE_CASE(INTERMEDIATE, INTERMEDIATE);
    _RTYPE_CASE(INVERSE, INVERSE);
    _RTYPE_CASE(MUSTACHE, mustache);
    _RTYPE_CASE(NUMBER, NumberLiteral);
    _RTYPE_CASE(PARTIAL, partial);
    _RTYPE_CASE(PARTIAL_BLOCK, PartialBlockStatement);
    _RTYPE_CASE(PATH, PathExpression);
    _RTYPE_CASE(PATH_SEGMENT, PATH_SEGMENT);
    _RTYPE_CASE(PROGRAM, program);
    _RTYPE_CASE(RAW_BLOCK, raw_block);
    _RTYPE_CASE(SEXPR, SubExpression);
    _RTYPE_CASE(STRING, StringLiteral);
    _RTYPE_CASE(UNDEFINED, UNDEFINED);
    case HANDLEBARS_AST_NODE_NUL: return "NULL";
    default: return "UNKNOWN";
  }
}


struct handlebars_ast_node * handlebars_ast_node_ctor_block(
    struct handlebars_parser * parser, struct handlebars_ast_node * intermediate,
    struct handlebars_ast_node * program, struct handlebars_ast_node * inverse,
    unsigned open_strip, unsigned inverse_strip, unsigned close_strip,
    struct handlebars_locinfo * locinfo)
{
    struct handlebars_ast_node * ast_node;

    // Construct the ast node
    ast_node = handlebars_ast_node_ctor(HBSCTX(parser), HANDLEBARS_AST_NODE_BLOCK);
    ast_node->loc = *locinfo;

    if( intermediate ) {
        ast_node->node.block.path = talloc_steal(ast_node, intermediate->node.intermediate.path);
        ast_node->node.block.params = talloc_steal(ast_node, intermediate->node.intermediate.params);
        ast_node->node.block.hash = talloc_steal(ast_node, intermediate->node.intermediate.hash);

        // We can free the intermediate
        talloc_steal(ast_node, intermediate);
        // Not sure why this isn't working
        // handlebars_talloc_free(intermediate);
    }

    ast_node->node.block.program = talloc_steal(ast_node, program);
    ast_node->node.block.inverse = talloc_steal(ast_node, inverse);
    ast_node->node.block.open_strip = open_strip;
    ast_node->node.block.inverse_strip = inverse_strip;
    ast_node->node.block.close_strip = close_strip;

    return ast_node;
}

struct handlebars_ast_node * handlebars_ast_node_ctor_boolean(
    struct handlebars_parser * parser,
    struct handlebars_string * boolean,
    struct handlebars_locinfo * locinfo
) {
    struct handlebars_ast_node * ast_node;

    ast_node = handlebars_ast_node_ctor(HBSCTX(parser), HANDLEBARS_AST_NODE_BOOLEAN);
    ast_node->loc = *locinfo;
    ast_node->node.boolean.value = talloc_steal(ast_node, handlebars_string_copy_ctor(HBSCTX(parser), boolean));

    return ast_node;
}

struct handlebars_ast_node * handlebars_ast_node_ctor_comment(
    struct handlebars_parser * parser,
    struct handlebars_string * comment,
    bool is_long,
    struct handlebars_locinfo * locinfo
) {
    struct handlebars_ast_node * ast_node;
    ast_node = handlebars_ast_node_ctor(HBSCTX(parser), HANDLEBARS_AST_NODE_COMMENT);
    ast_node->loc = *locinfo;
    ast_node->strip |= handlebars_ast_strip_flag_set | handlebars_ast_strip_flag_inline_standalone;
    ast_node->node.comment.value = talloc_steal(ast_node, handlebars_string_copy_ctor(HBSCTX(parser), comment));
    ast_node->node.comment.is_long = is_long;
    return ast_node;
}

struct handlebars_ast_node * handlebars_ast_node_ctor_content(
        struct handlebars_parser * parser,
        struct handlebars_string * content,
        struct handlebars_locinfo * locinfo
) {
    struct handlebars_ast_node * ast_node;
    ast_node = handlebars_ast_node_ctor(HBSCTX(parser), HANDLEBARS_AST_NODE_CONTENT);
    ast_node->loc = *locinfo;
    ast_node->node.content.value = talloc_steal(ast_node, handlebars_string_copy_ctor(HBSCTX(parser), content));
    ast_node->node.content.original = talloc_steal(ast_node, handlebars_string_copy_ctor(HBSCTX(parser), content));
    return ast_node;
}

struct handlebars_ast_node * handlebars_ast_node_ctor_hash_pair(
    struct handlebars_parser * parser,
    struct handlebars_string * key,
    struct handlebars_ast_node * value,
    struct handlebars_locinfo * locinfo
) {
    struct handlebars_ast_node * ast_node;
    ast_node = handlebars_ast_node_ctor(HBSCTX(parser), HANDLEBARS_AST_NODE_HASH_PAIR);
    ast_node->loc = *locinfo;
    ast_node->node.hash_pair.key = talloc_steal(ast_node, key);
    ast_node->node.hash_pair.value = talloc_steal(ast_node, value);
    return ast_node;
}

struct handlebars_ast_node * handlebars_ast_node_ctor_intermediate(
	    struct handlebars_parser * parser, struct handlebars_ast_node * path,
	    struct handlebars_ast_list * params, struct handlebars_ast_node * hash,
	    unsigned strip, struct handlebars_locinfo * locinfo)
{
    struct handlebars_ast_node * ast_node;
    ast_node = handlebars_ast_node_ctor(HBSCTX(parser), HANDLEBARS_AST_NODE_INTERMEDIATE);
    ast_node->loc = *locinfo;
    ast_node->strip = strip;
    ast_node->node.intermediate.path = talloc_steal(ast_node, path);
    ast_node->node.intermediate.params = talloc_steal(ast_node, params);
    ast_node->node.intermediate.hash = talloc_steal(ast_node, hash);
    return ast_node;
}

struct handlebars_ast_node * handlebars_ast_node_ctor_inverse(
	    struct handlebars_parser * parser, struct handlebars_ast_node * program,
        bool chained, unsigned strip, struct handlebars_locinfo * locinfo)
{
    struct handlebars_ast_node * ast_node;
    ast_node = handlebars_ast_node_ctor(HBSCTX(parser), HANDLEBARS_AST_NODE_INVERSE);
    ast_node->loc = *locinfo;
    ast_node->strip = strip;
    ast_node->node.inverse.program = talloc_steal(ast_node, program);
    ast_node->node.inverse.chained = chained;
    return ast_node;
}

struct handlebars_ast_node * handlebars_ast_node_ctor_null(
    struct handlebars_parser * parser,
    struct handlebars_string * string,
    struct handlebars_locinfo * locinfo
) {
    struct handlebars_ast_node * ast_node;
    ast_node = handlebars_ast_node_ctor(HBSCTX(parser), HANDLEBARS_AST_NODE_NUL);
    ast_node->loc = *locinfo;
    ast_node->node.nul.value = talloc_steal(ast_node, string);
    return ast_node;
}

struct handlebars_ast_node * handlebars_ast_node_ctor_number(
    struct handlebars_parser * parser,
    struct handlebars_string * number,
    struct handlebars_locinfo * locinfo
) {
    struct handlebars_ast_node * ast_node;
    ast_node = handlebars_ast_node_ctor(HBSCTX(parser), HANDLEBARS_AST_NODE_NUMBER);
    ast_node->loc = *locinfo;
    ast_node->node.number.value = talloc_steal(ast_node, handlebars_string_copy_ctor(HBSCTX(parser), number));
    return ast_node;
}

struct handlebars_ast_node * handlebars_ast_node_ctor_partial(
        struct handlebars_parser * parser, struct handlebars_ast_node * name,
        struct handlebars_ast_list * params, struct handlebars_ast_node * hash,
        unsigned strip, struct handlebars_locinfo * locinfo)
{
    struct handlebars_ast_node * ast_node = handlebars_ast_node_ctor(HBSCTX(parser), HANDLEBARS_AST_NODE_PARTIAL);
    ast_node->loc = *locinfo;
    ast_node->strip = strip;
    ast_node->node.partial.name = talloc_steal(ast_node, name);
    ast_node->node.partial.params = talloc_steal(ast_node, params);
    ast_node->node.partial.hash = talloc_steal(ast_node, hash);
    return ast_node;
}

struct handlebars_ast_node * handlebars_ast_node_ctor_partial_block(
    struct handlebars_parser * parser, struct handlebars_ast_node * open,
    struct handlebars_ast_node * program, struct handlebars_ast_node * close,
    struct handlebars_locinfo * loc)
{
    struct handlebars_ast_node * ast_node;

    ast_node = handlebars_ast_node_ctor(HBSCTX(parser), HANDLEBARS_AST_NODE_PARTIAL_BLOCK);
    ast_node->loc = *loc;

    if( open ) {
        ast_node->node.block.path = talloc_steal(ast_node, open->node.intermediate.path);
        ast_node->node.block.params = talloc_steal(ast_node, open->node.intermediate.params);
        ast_node->node.block.hash = talloc_steal(ast_node, open->node.intermediate.hash);
        ast_node->node.block.open_strip = open->strip;
        handlebars_talloc_free(open);
    }

    if (close) {
        ast_node->node.block.close_strip = close->strip;
        handlebars_talloc_free(close);
    }

    ast_node->node.block.program = talloc_steal(ast_node, program);

    return ast_node;
}

struct handlebars_ast_node * handlebars_ast_node_ctor_program(
    struct handlebars_parser * parser,
    struct handlebars_ast_list * statements,
    struct handlebars_string * block_param1,
    struct handlebars_string * block_param2, unsigned strip,
    bool chained,
    struct handlebars_locinfo * locinfo)
{
    struct handlebars_ast_node * ast_node;
    ast_node = handlebars_ast_node_ctor(HBSCTX(parser), HANDLEBARS_AST_NODE_PROGRAM);
    ast_node->loc = *locinfo;
    ast_node->strip = strip;
    ast_node->node.program.statements = talloc_steal(ast_node, statements);
    ast_node->node.program.block_param1 = talloc_steal(ast_node, block_param1);
    ast_node->node.program.block_param2 = talloc_steal(ast_node, block_param2);
    ast_node->node.program.chained = chained;
    return ast_node;
}

struct handlebars_ast_node * handlebars_ast_node_ctor_path(
    struct handlebars_parser * parser,
    struct handlebars_ast_list * parts,
    struct handlebars_string * original,
    unsigned int depth,
    bool data,
    struct handlebars_locinfo * locinfo
) {
    struct handlebars_ast_node * ast_node;
    ast_node = handlebars_ast_node_ctor(HBSCTX(parser), HANDLEBARS_AST_NODE_PATH);
    ast_node->loc = *locinfo;
    ast_node->node.path.parts = talloc_steal(ast_node, parts);
    ast_node->node.path.original = talloc_steal(ast_node, handlebars_string_copy_ctor(HBSCTX(parser), original));
    ast_node->node.path.data = data;
    ast_node->node.path.depth = depth;
    return ast_node;
}

struct handlebars_ast_node * handlebars_ast_node_ctor_path_segment(
    struct handlebars_parser * parser,
    struct handlebars_string * part,
    struct handlebars_string * separator,
    struct handlebars_locinfo * locinfo
) {
    struct handlebars_ast_node * ast_node;

    ast_node = handlebars_ast_node_ctor(HBSCTX(parser), HANDLEBARS_AST_NODE_PATH_SEGMENT);
    ast_node->loc = *locinfo;
    ast_node->node.path_segment.original = talloc_steal(ast_node, handlebars_string_copy_ctor(HBSCTX(parser), part));
    ast_node->node.path_segment.part = handlebars_string_copy_ctor(HBSCTX(parser), part);
    ast_node->node.path_segment.part = handlebars_ast_helper_strip_id_literal(ast_node->node.path_segment.part);

    if( separator != NULL ) {
        ast_node->node.path_segment.separator = talloc_steal(ast_node, handlebars_string_copy_ctor(HBSCTX(parser), separator));
    }

    return ast_node;
}

struct handlebars_ast_node * handlebars_ast_node_ctor_raw_block(
    struct handlebars_parser * parser, struct handlebars_ast_node * intermediate,
    struct handlebars_ast_node * content,  struct handlebars_locinfo * locinfo
) {
    struct handlebars_ast_node * ast_node;
    struct handlebars_ast_node * path;
    struct handlebars_ast_list * params;
    struct handlebars_ast_node * hash;
    struct handlebars_ast_node * program;
    struct handlebars_ast_list * statements;

    // Construct the ast node
    ast_node = handlebars_ast_node_ctor(HBSCTX(parser), HANDLEBARS_AST_NODE_RAW_BLOCK);
    ast_node->loc = *locinfo;

    // Assign the content
    assert(content != NULL);
    assert(content->type == HANDLEBARS_AST_NODE_CONTENT);
    assert(intermediate != NULL);

    path = intermediate->node.intermediate.path;
    params = intermediate->node.intermediate.params;
    hash = intermediate->node.intermediate.hash;

    assert(path == NULL || path->type == HANDLEBARS_AST_NODE_PATH);
    assert(hash == NULL || hash->type == HANDLEBARS_AST_NODE_HASH);

    // Create the program node
    program = handlebars_ast_node_ctor(HBSCTX(parser), HANDLEBARS_AST_NODE_PROGRAM);
    statements = talloc_steal(program, handlebars_ast_list_ctor(HBSCTX(parser)));
    program->node.program.statements = statements;
    handlebars_ast_list_append(statements, talloc_steal(program, content));
    ast_node->node.raw_block.program = program;
    //ast_node->node.raw_block.program = talloc_steal(ast_node, content);

    // Assign the other nodes
    ast_node->node.raw_block.path = talloc_steal(ast_node, path);
    ast_node->node.raw_block.params = talloc_steal(ast_node, params);
    ast_node->node.raw_block.hash = talloc_steal(ast_node, hash);

    return ast_node;
}

struct handlebars_ast_node * handlebars_ast_node_ctor_sexpr(
	    struct handlebars_parser * parser,
	    struct handlebars_ast_node * intermediate,
	    struct handlebars_locinfo * locinfo)
{
    struct handlebars_ast_node * ast_node;

    assert(intermediate != NULL);

    ast_node = handlebars_ast_node_ctor(HBSCTX(parser), HANDLEBARS_AST_NODE_SEXPR);
    ast_node->loc = *locinfo;
    ast_node->node.sexpr.path = talloc_steal(ast_node, intermediate->node.intermediate.path);
    ast_node->node.sexpr.params = talloc_steal(ast_node, intermediate->node.intermediate.params);
    ast_node->node.sexpr.hash = talloc_steal(ast_node, intermediate->node.intermediate.hash);

    return ast_node;
}

struct handlebars_ast_node * handlebars_ast_node_ctor_string(
    struct handlebars_parser * parser,
    struct handlebars_string * string,
    bool is_single_quoted,
    struct handlebars_locinfo * locinfo
) {
    struct handlebars_ast_node * ast_node = handlebars_ast_node_ctor(HBSCTX(parser), HANDLEBARS_AST_NODE_STRING);
    ast_node->loc = *locinfo;
    ast_node->node.string.value = talloc_steal(ast_node, handlebars_string_copy_ctor(HBSCTX(parser), string));
    ast_node->node.string.is_single_quoted = is_single_quoted;
    return ast_node;
}

struct handlebars_ast_node * handlebars_ast_node_ctor_undefined(
    struct handlebars_parser * parser,
    struct handlebars_string * string,
    struct handlebars_locinfo * locinfo
) {
    struct handlebars_ast_node * ast_node = handlebars_ast_node_ctor(HBSCTX(parser), HANDLEBARS_AST_NODE_UNDEFINED);
    ast_node->loc = *locinfo;
    ast_node->node.undefined.value = talloc_steal(ast_node, string);
    return ast_node;
}

void handlebars_ast_node_set_strip(struct handlebars_ast_node * node, unsigned flags)
{
    node->strip = flags;
}
