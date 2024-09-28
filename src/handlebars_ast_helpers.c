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
            handlebars_throw_ex(
                CONTEXT,
                HANDLEBARS_PARSEERR,
                locinfo,
                "%.*s doesn't match %.*s",
                (int) hbs_str_len(open_str), hbs_str_val(open_str),
                (int) hbs_str_len(close_str), hbs_str_val(close_str)
            );
        }
    }

    if( open_block->node.intermediate.open && NULL != strchr(hbs_str_val(open_block->node.intermediate.open), '*') ) {
    	is_decorator = true;
    }

    // this isn't supposed to be null I think...
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
        // this probably shouldn't happen
        program = NULL;
    }
    if( inverse && inverse->type == 0 ) {
        // this probably shouldn't happen
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
        if( hbs_str_len(open) >= 4 ) {
            c = *(hbs_str_val(open) + 3);
        } else if( hbs_str_len(open) >= 3 ) {
            c = *(hbs_str_val(open) + 2);
        }
        if( NULL != strchr(hbs_str_val(open), '*') ) {
        	ast_node->node.mustache.is_decorator = 1;
        }
    }
    if (c == '{') {
        ast_node->node.mustache.unescaped = 1;
    } else if (c == '&') {
        ast_node->node.mustache.unescaped = 3;
    }

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
            handlebars_throw_ex(
                CONTEXT,
                HANDLEBARS_PARSEERR,
                locinfo,
                "%.*s doesn't match %.*s",
                (int) hbs_str_len(open_str), hbs_str_val(open_str),
                (int) hbs_str_len(close_str), hbs_str_val(close_str)
            );
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
            original = handlebars_string_append_str(HBSCTX(parser), original, separator);
        }
        original = handlebars_string_append_str(HBSCTX(parser), original, part);

        // Handle paths

        if( !is_literal && (hbs_str_eq_strl(part, HBS_STRL("..")) || hbs_str_eq_strl(part, HBS_STRL(".")) || hbs_str_eq_strl(part, HBS_STRL("this"))) ) {
            if( count > 0 ) {
                handlebars_throw_ex(
                    CONTEXT,
                    HANDLEBARS_ERROR,
                    locinfo,
                    "Invalid path: %.*s",
                    (int) hbs_str_len(original), hbs_str_val(original)
                );
            } else if( hbs_str_eq_strl(part, HBS_STRL("..")) ) {
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
        struct handlebars_string * open = open_block_path->node.path.original;
        handlebars_throw_ex(
            CONTEXT,
            HANDLEBARS_ERROR,
            locinfo,
            "%.*s doesn't match %.*s",
            (int) hbs_str_len(open), hbs_str_val(open),
            (int) hbs_str_len(close), hbs_str_val(close)
        );
    }

    // Create the content node
    content_node = handlebars_ast_node_ctor_content(parser, content, locinfo);

    // Create the raw block node
    return handlebars_ast_node_ctor_raw_block(parser, open_raw_block, content_node, locinfo);
}

static inline size_t handlebars_ast_helper_strip_comment_left(struct handlebars_string * comment)
{
    char * c = hbs_str_val(comment);
    size_t start = 0;

    if( *c == '{' ) {
        c++;
        start++;
    } else {
        return 0;
    }

    if( *c == '{' ) {
        c++;
        start++;
    } else {
        return 0;
    }

    if( *c == '~' ) {
        c++;
        start++;
    } else if( !*c ) {
        return 0;
    }

    if( *c == '!' ) {
        c++;
        start++;
    } else {
        return 0;
    }

    if( *c == '-' ) {
        c++;
        start++;
    }

    if( *c == '-' ) {
        c++;
        start++;
    }

    return start;
}

static inline size_t handlebars_ast_helper_strip_comment_right(struct handlebars_string * comment)
{
    char * orig = hbs_str_val(comment);
    char * c = orig + hbs_str_len(comment);
    size_t len = hbs_str_len(comment);

    if( hbs_str_len(comment) < 2 ) {
        return hbs_str_len(comment);
    }

    if( *--c != '}' ) {
        return hbs_str_len(comment);
    }

    if( *--c != '}' ) {
        return hbs_str_len(comment);
    }
    len--;
    len--;

    if( c > orig && *(c - 1) == '~' ) {
        c--;
        len--;
    }

    if( c > orig && *(c - 1) == '-' ) {
        c--;
        len--;
    }

    if( c > orig && *(c - 1) == '-' ) {
        c--;
        len--;
    }

    return len;
}

struct handlebars_string * handlebars_ast_helper_strip_comment(struct handlebars_string * comment)
{
    assert(comment != NULL);
    size_t start = handlebars_ast_helper_strip_comment_left(comment);
    size_t len = handlebars_ast_helper_strip_comment_right(comment);
    comment = handlebars_string_truncate(comment, start, len);
    return comment;
}

struct handlebars_string * handlebars_ast_helper_strip_id_literal(struct handlebars_string * comment)
{
    if (!comment) {
        return comment;
    }

    char * val = hbs_str_val(comment);
	if( val[0] == '[' && val[hbs_str_len(comment) - 1] == ']' ) {
        comment = handlebars_string_truncate(comment, 1, hbs_str_len(comment) - 1);
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
    if( open && hbs_str_len(open) >= 3 && *(hbs_str_val(open) + 2) == '~' ) {
        strip |= handlebars_ast_strip_flag_left;
    } else {
        strip &= ~handlebars_ast_strip_flag_left;
    }
    if( close && hbs_str_len(close) >= 3 && *(hbs_str_val(close) + hbs_str_len(close) - 3) == '~' ) {
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
        if( hbs_str_len(original) >= 1 && hbs_str_val(original)[0] == '.' ) {
            return true;
        } else if( hbs_str_len(original) == 4 && 0 == strcmp(hbs_str_val(original), "this") ) {
            return true;
        } else if( hbs_str_len(original) > 4 && NULL != (found = strstr(hbs_str_val(original), "this")) ) {
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
