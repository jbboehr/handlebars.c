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
#include <stdlib.h>
#include <string.h>
#include <talloc.h>

#define HANDLEBARS_AST_PRIVATE
#define HANDLEBARS_AST_LIST_PRIVATE

#include "handlebars.h"
#include "handlebars_ast.h"
#include "handlebars_ast_helpers.h"
#include "handlebars_ast_list.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"
#include "handlebars_string.h"
#include "handlebars_utils.h"
#include "handlebars_whitespace.h"
#include "handlebars.tab.h"



#undef CONTEXT
#define CONTEXT HBSCTX(parser)

struct handlebars_ast_node * handlebars_ast_helper_prepare_block(
    struct handlebars_parser * parser, struct handlebars_ast_node * open_block,
    struct handlebars_ast_node * program, struct handlebars_ast_node * inverse_and_program,
    struct handlebars_ast_node * close, int inverted,
    struct handlebars_locinfo * locinfo)
{
    struct handlebars_ast_node * ast_node;
    struct handlebars_ast_node * open_block_path = open_block->node.intermediate.path;
    struct handlebars_ast_node * close_block_path;
    struct handlebars_ast_node * inverse = NULL;
    long inverse_strip = 0;
    struct handlebars_string * open_str;
    struct handlebars_string * close_str;
    bool is_decorator = false;

    assert(open_block != NULL && open_block->type == HANDLEBARS_AST_NODE_INTERMEDIATE);
    assert(close == NULL || close->type == HANDLEBARS_AST_NODE_INTERMEDIATE || close->type == HANDLEBARS_AST_NODE_INVERSE);

    if( close && close->type == HANDLEBARS_AST_NODE_INTERMEDIATE ) {
        close_block_path = close->node.intermediate.path;
        open_str = handlebars_ast_node_get_string_mode_value(CONTEXT, open_block_path);
        close_str = handlebars_ast_node_get_string_mode_value(CONTEXT, close_block_path);
        if( close_block_path && !handlebars_string_eq(open_str, close_str) ) {
            handlebars_throw_ex(CONTEXT, HANDLEBARS_PARSEERR, locinfo,  "%s doesn't match %s", open_str->val, close_str->val);
        }
    }

    if( open_block->node.intermediate.open && NULL != strchr(open_block->node.intermediate.open->val, '*') ) {
    	is_decorator = true;
    }

    // @todo this isn't supposed to be null I think...
    if( !program ) {
        program = handlebars_ast_node_ctor(HBSCTX(parser), HANDLEBARS_AST_NODE_PROGRAM);
    }
    program->node.program.block_param1 = open_block->node.intermediate.block_param1;
    program->node.program.block_param2 = open_block->node.intermediate.block_param2;

    if( inverse_and_program ) {
        assert(inverse_and_program->type == HANDLEBARS_AST_NODE_INVERSE);

        if( is_decorator ) {
            handlebars_throw_ex(CONTEXT, HANDLEBARS_PARSEERR, locinfo, "Unexpected inverse block on decorator");
        }

        if( inverse_and_program->node.inverse.chained ) {
            struct handlebars_ast_list * statements;
            struct handlebars_ast_node * tmp;
            if( (tmp = inverse_and_program->node.inverse.program) &&
                (statements = tmp->node.program.statements) &&
                    statements->first && (tmp = statements->first->data) &&
                    tmp->type == HANDLEBARS_AST_NODE_BLOCK ) {
                tmp->node.block.close_strip = close->strip;
            }
        }

        inverse = inverse_and_program->node.inverse.program;
        inverse_strip = inverse_and_program->strip;
    }

    if( program && program->type == 0 ) {
        // @todo this probably shouldn't happen
        program = NULL;
    }
    if( inverse && inverse->type == 0 ) {
        // @todo this probably shouldn't happen
        inverse = NULL;
    }
    assert(!program || program->type == HANDLEBARS_AST_NODE_PROGRAM);
    assert(!inverse || inverse->type == HANDLEBARS_AST_NODE_PROGRAM);

    if( inverted ) {
        struct handlebars_ast_node * tmp;
        tmp = program;
        program = inverse;
        inverse = tmp;
    }

    ast_node = handlebars_ast_node_ctor_block(parser, open_block, program, inverse,
                open_block->strip, inverse_strip, close ? close->strip : 0, locinfo);

    ast_node->node.block.is_decorator = is_decorator;
    return ast_node;
}

struct handlebars_ast_node * handlebars_ast_helper_prepare_inverse_chain(
        struct handlebars_parser * parser, struct handlebars_ast_node * open_inverse_chain,
        struct handlebars_ast_node * program, struct handlebars_ast_node * inverse_chain,
        struct handlebars_locinfo * locinfo)
{
    struct handlebars_ast_node * block_node;
    struct handlebars_ast_list * statements;
    struct handlebars_ast_node * program_node;
    struct handlebars_ast_node * ast_node;

    block_node = handlebars_ast_helper_prepare_block(parser, open_inverse_chain, program, inverse_chain, inverse_chain, 0, locinfo);
    statements = handlebars_ast_list_ctor(HBSCTX(parser));
    handlebars_ast_list_append(statements, block_node);
    program_node = handlebars_ast_node_ctor_program(parser, statements, NULL, NULL, 0, 1, locinfo);
    ast_node = handlebars_ast_node_ctor_inverse(parser, program_node, 1,
                    (open_inverse_chain ? open_inverse_chain->strip : 0), locinfo);

    return ast_node;
}

struct handlebars_ast_node * handlebars_ast_helper_prepare_mustache(
        struct handlebars_parser * parser,
        struct handlebars_ast_node * intermediate,
        struct handlebars_string * open,
        unsigned strip,
        struct handlebars_locinfo * locinfo
) {
    char c = 0;
    struct handlebars_ast_node * path = intermediate->node.intermediate.path;
    struct handlebars_ast_list * params = intermediate->node.intermediate.params;
    struct handlebars_ast_node * hash = intermediate->node.intermediate.hash;
    struct handlebars_ast_node * ast_node = handlebars_ast_node_ctor(HBSCTX(parser), HANDLEBARS_AST_NODE_MUSTACHE);

    ast_node->loc = *locinfo;
    ast_node->strip = strip;
    ast_node->node.mustache.path = talloc_steal(ast_node, path);
    ast_node->node.mustache.params = talloc_steal(ast_node, params);
    ast_node->node.mustache.hash = talloc_steal(ast_node, hash);

    // Check escaped
    if( open ) {
        if( open->len >= 4 ) {
            c = *(open->val + 3);
        } else if( open->len >= 3 ) {
            c = *(open->val + 2);
        }
        if( NULL != strchr(open->val, '*') ) {
        	ast_node->node.mustache.is_decorator = 1;
        }
    }
    ast_node->node.mustache.unescaped = (c == '{' || c == '&');

    // Free the intermediate node
    handlebars_talloc_free(intermediate);

    return ast_node;
}

struct handlebars_ast_node * handlebars_ast_helper_prepare_partial_block(
    struct handlebars_parser * parser, struct handlebars_ast_node * open,
    struct handlebars_ast_node * program, struct handlebars_ast_node * close,
    struct handlebars_locinfo * locinfo)
{
    struct handlebars_ast_node * open_block_path = open->node.intermediate.path;
    struct handlebars_ast_node * close_block_path;
    struct handlebars_string * open_str;
    struct handlebars_string * close_str;

    assert(open != NULL && open->type == HANDLEBARS_AST_NODE_INTERMEDIATE);
    assert(close == NULL || close->type == HANDLEBARS_AST_NODE_INTERMEDIATE || close->type == HANDLEBARS_AST_NODE_INVERSE);

    if( close && close->type == HANDLEBARS_AST_NODE_INTERMEDIATE ) {
        close_block_path = close->node.intermediate.path;
        open_str = handlebars_ast_node_get_string_mode_value(CONTEXT, open_block_path);
        close_str = handlebars_ast_node_get_string_mode_value(CONTEXT, close_block_path);
        if( close_block_path && !handlebars_string_eq(open_str, close_str) ) {
            handlebars_throw_ex(CONTEXT, HANDLEBARS_PARSEERR, locinfo, "%s doesn't match %s", open_str->val, close_str->val);
        }
    }

	return handlebars_ast_node_ctor_partial_block(parser, open, program, close, locinfo);
}

struct handlebars_ast_node * handlebars_ast_helper_prepare_path(
        struct handlebars_parser * parser,
        struct handlebars_ast_list * parts,
        bool data,
        struct handlebars_locinfo * locinfo
) {
    struct handlebars_ast_list_item * item;
    struct handlebars_ast_list_item * tmp;
    struct handlebars_string * part;
    struct handlebars_string * separator;
    struct handlebars_string * original;
    bool is_literal;
    int depth = 0;
    int count = 0;

    // Allocate the original strings
    original = handlebars_string_ctor(HBSCTX(parser), data ? "@" : "", data ? 1 : 0);
    //original = MC(handlebars_talloc_strdup(parser, data ? "@" : ""));

    // Iterate over parts and process
    handlebars_ast_list_foreach(parts, item, tmp) {
        part = item->data->node.path_segment.part;
        if( unlikely(part == NULL) ) {
            continue;
        }
        separator = item->data->node.path_segment.separator;
        is_literal = !handlebars_string_eq(part, item->data->node.path_segment.original);

        // Append to original
        if( separator ) {
            original = handlebars_string_append(HBSCTX(parser), original, separator->val, separator->len);
        }
        original = handlebars_string_append(HBSCTX(parser), original, part->val, part->len);

        // Handle paths
        if( !is_literal && (strcmp(part->val, "..") == 0 || strcmp(part->val, ".") == 0 || strcmp(part->val, "this") == 0) ) {
            if( count > 0 ) {
                handlebars_throw_ex(CONTEXT, HANDLEBARS_ERROR, locinfo, "Invalid path: %s", original->val);
            } else if( strcmp(part->val, "..") == 0 ) {
                depth++;
            }
            // Instead of adding it below, remove it here
            handlebars_ast_list_remove(parts, item->data);
        } else {
            count++;
        }
    }

    return handlebars_ast_node_ctor_path(parser, parts, original, depth, data, locinfo);
}

struct handlebars_ast_node * handlebars_ast_helper_prepare_raw_block(
        struct handlebars_parser * parser,
        struct handlebars_ast_node * open_raw_block,
        struct handlebars_string * content,
        struct handlebars_string * close,
        struct handlebars_locinfo * locinfo
) {
    struct handlebars_ast_node * content_node;
    struct handlebars_ast_node * open_block_path;

    assert(open_raw_block != NULL);
    assert(open_raw_block->type == HANDLEBARS_AST_NODE_INTERMEDIATE);
    assert(close != NULL);

    open_block_path = open_raw_block->node.intermediate.path;
    if( !handlebars_string_eq(open_block_path->node.path.original, close) ) {
        handlebars_throw_ex(CONTEXT, HANDLEBARS_ERROR, locinfo, "%s doesn't match %s", open_block_path->node.path.original->val, close->val);
    }

    // Create the content node
    content_node = handlebars_ast_node_ctor_content(parser, content, locinfo);

    // Create the raw block node
    return handlebars_ast_node_ctor_raw_block(parser, open_raw_block, content_node, locinfo);
}

static void handlebars_ast_helper_strip_comment_left(struct handlebars_string * comment)
{
    char * c = comment->val;

    if( *c == '{' ) {
        c++;
    } else {
        return;
    }

    if( *c == '{' ) {
        c++;
    } else {
        return;
    }

    if( *c == '~' ) {
        c++;
    } else if( !*c ) {
        return;
    }

    if( *c == '!' ) {
        c++;
    } else {
        return;
    }

    if( *c == '-' ) {
        c++;
    }

    if( *c == '-' ) {
        c++;
    }

    if( c > comment->val ) {
        comment->len = strlen(c);
        memmove(comment->val, c, comment->len);
        comment->val[comment->len] = 0;
    }
}

static void handlebars_ast_helper_strip_comment_right(struct handlebars_string * comment)
{
    char * end = comment->val + comment->len;
    char * c = end;

    if( comment->len < 2 ) {
        return;
    }

    if( *--c != '}' ) {
        return;
    }

    if( *--c != '}' ) {
        return;
    }

    if( c > comment->val && *(c - 1) == '~' ) {
        c--;
    }

    if( c > comment->val && *(c - 1) == '-' ) {
        c--;
    }

    if( c > comment->val && *(c - 1) == '-' ) {
        c--;
    }

    if( c < end ) {
        *c = 0;
        comment->len = strlen(comment->val);
    }
}

struct handlebars_string * handlebars_ast_helper_strip_comment(struct handlebars_string * comment)
{
    assert(comment != NULL);
    handlebars_ast_helper_strip_comment_left(comment);
    handlebars_ast_helper_strip_comment_right(comment);
    return comment;
}

struct handlebars_string * handlebars_ast_helper_strip_id_literal(struct handlebars_string * comment)
{
	if( comment && comment->val[0] == '[' && comment->val[comment->len - 1] == ']' ) {
		if( comment->len <= 2 ) {
			comment->val[0] = 0;
            comment->len = 0;
		} else {
			memmove(comment->val, comment->val + 1, comment->len - 2);
            comment->len -= 2;
            comment->val[comment->len] = 0;
		}
	}

	return comment;
}

void handlebars_ast_helper_set_strip_flags(
        struct handlebars_ast_node * ast_node, struct handlebars_string * open, struct handlebars_string * close)
{
    ast_node->strip = handlebars_ast_helper_strip_flags(open, close);
    if( ast_node->type == HANDLEBARS_AST_NODE_PARTIAL ) {
        ast_node->strip |= handlebars_ast_strip_flag_inline_standalone;
    }
}

unsigned handlebars_ast_helper_strip_flags(struct handlebars_string * open, struct handlebars_string * close)
{
    unsigned strip = 0;
    if( open && open->len >= 3 && *(open->val + 2) == '~' ) {
        strip |= handlebars_ast_strip_flag_left;
    } else {
        strip &= ~handlebars_ast_strip_flag_left;
    }
    if( close && close->len >= 3 && *(close->val + close->len - 3) == '~' ) {
        strip |= handlebars_ast_strip_flag_right;
    } else {
        strip &= ~handlebars_ast_strip_flag_right;
    }
    strip |= handlebars_ast_strip_flag_set;
    return strip;
}

bool handlebars_ast_helper_scoped_id(struct handlebars_ast_node * path)
{
    struct handlebars_string * original;
    char * found;
    if( path && (original = path->node.path.original) ) {
        if( original->len >= 1 && original->val[0] == '.' ) {
            return true;
        } else if( original->len == 4 && 0 == strcmp(original->val, "this") ) {
            return true;
        } else if( original->len > 4 && NULL != (found = strstr(original->val, "this")) ) {
        //} else if( len > 4 && 0 == strncmp(original, "this", 4) ) {
            char c = *(found + 4);
            // [^a-zA-Z0-9_]
            return c < '0' || (c > '9' && c < 'A') || (c > 'Z' && c < '_') || (c > '_' && c < 'a') || c > 'z';
        }
    }
    return false;
}

bool handlebars_ast_helper_simple_id(struct handlebars_ast_node * path)
{
    return (
        path &&
        path->node.path.parts &&
        handlebars_ast_list_count(path->node.path.parts) == 1 &&
        !handlebars_ast_helper_scoped_id(path) &&
        !path->node.path.depth
    );
}

bool handlebars_ast_helper_helper_expression(struct handlebars_ast_node * node)
{
	struct handlebars_ast_list * params;
	struct handlebars_ast_node * hash;
    if( node->type == HANDLEBARS_AST_NODE_SEXPR ) {
        return true;
    }
    params = handlebars_ast_node_get_params(node);
    hash = handlebars_ast_node_get_hash(node);
	return hash || (params && handlebars_ast_list_count(params));
}
