
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <talloc.h>

#include "handlebars.h"
#include "handlebars_ast.h"
#include "handlebars_ast_helpers.h"
#include "handlebars_ast_list.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"
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
    struct handlebars_ast_node * tmp;
    long inverse_strip = 0;
    const char * open_str;
    const char * close_str;
    bool is_decorator = false;
    
    assert(open_block != NULL && open_block->type == HANDLEBARS_AST_NODE_INTERMEDIATE);
    assert(close == NULL || close->type == HANDLEBARS_AST_NODE_INTERMEDIATE || close->type == HANDLEBARS_AST_NODE_INVERSE);
    
    if( close && close->type == HANDLEBARS_AST_NODE_INTERMEDIATE ) {
        close_block_path = close->node.intermediate.path;
        open_str = handlebars_ast_node_get_string_mode_value(open_block_path);
        close_str = handlebars_ast_node_get_string_mode_value(close_block_path);
        if( close_block_path && 0 != strcmp(open_str, close_str) ) {
            handlebars_throw_ex(CONTEXT, HANDLEBARS_PARSEERR, locinfo,  "%s doesn't match %s", open_str, close_str);
        }
    }

    if( open_block->node.intermediate.open && NULL != strchr(open_block->node.intermediate.open, '*') ) {
    	is_decorator = true;
    }

    // @todo this isn't supposed to be null I think...
    if( !program ) {
        program = handlebars_ast_node_ctor(parser, HANDLEBARS_AST_NODE_PROGRAM);
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
    statements = handlebars_ast_list_ctor(parser);
    handlebars_ast_list_append(statements, block_node);
    program_node = handlebars_ast_node_ctor_program(parser, statements, NULL, NULL, 0, 1, locinfo);
    ast_node = handlebars_ast_node_ctor_inverse(parser, program_node, 1,
                    (open_inverse_chain ? open_inverse_chain->strip : 0), locinfo);
    
    return ast_node;
}

struct handlebars_ast_node * handlebars_ast_helper_prepare_mustache(
        struct handlebars_parser * parser, struct handlebars_ast_node * intermediate,
        char * open, unsigned strip, struct handlebars_locinfo * locinfo)
{
    char c = 0;
    size_t open_len;
    struct handlebars_ast_node * path = intermediate->node.intermediate.path;
    struct handlebars_ast_list * params = intermediate->node.intermediate.params;
    struct handlebars_ast_node * hash = intermediate->node.intermediate.hash;
    struct handlebars_ast_node * ast_node = handlebars_ast_node_ctor(parser, HANDLEBARS_AST_NODE_MUSTACHE);
    
    ast_node->loc = *locinfo;
    ast_node->strip = strip;
    ast_node->node.mustache.path = talloc_steal(ast_node, path);
    ast_node->node.mustache.params = talloc_steal(ast_node, params);
    ast_node->node.mustache.hash = talloc_steal(ast_node, hash);
    
    // Check escaped
    if( open ) {
        open_len = strlen(open);
        if( open_len >= 4 ) {
            c = *(open + 3);
        } else if( open_len >= 3 ) {
            c = *(open + 2);
        }
        if( NULL != strchr(open, '*') ) {
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
    const char * open_str;
    const char * close_str;

    assert(open != NULL && open->type == HANDLEBARS_AST_NODE_INTERMEDIATE);
    assert(close == NULL || close->type == HANDLEBARS_AST_NODE_INTERMEDIATE || close->type == HANDLEBARS_AST_NODE_INVERSE);

    if( close && close->type == HANDLEBARS_AST_NODE_INTERMEDIATE ) {
        close_block_path = close->node.intermediate.path;
        open_str = handlebars_ast_node_get_string_mode_value(open_block_path);
        close_str = handlebars_ast_node_get_string_mode_value(close_block_path);
        if( close_block_path && 0 != strcmp(open_str, close_str) ) {
            handlebars_throw_ex(CONTEXT, HANDLEBARS_PARSEERR, locinfo, "%s doesn't match %s", open_str, close_str);
        }
    }

	return handlebars_ast_node_ctor_partial_block(parser, open, program, close, locinfo);
}

struct handlebars_ast_node * handlebars_ast_helper_prepare_path(
        struct handlebars_parser * parser, struct handlebars_ast_list * parts,
        bool data, struct handlebars_locinfo * locinfo)
{
    struct handlebars_ast_list_item * item;
    struct handlebars_ast_list_item * tmp;
    char * part = NULL;
    char * separator;
    char * original = NULL;
    bool is_literal;
    int depth = 0;
    int count = 0;
    
    // Allocate the original strings
    original = MC(handlebars_talloc_strdup(parser, data ? "@" : ""));
    
    // Iterate over parts and process
    handlebars_ast_list_foreach(parts, item, tmp) {
        part = item->data->node.path_segment.part;
        if( unlikely(part == NULL) ) {
            continue;
        }
        separator = item->data->node.path_segment.separator;
        is_literal = (0 != strcmp(part, item->data->node.path_segment.original));
        
        // Append to original
        if( separator ) {
            original = MC(handlebars_talloc_strdup_append(original, separator));
        }
        original = MC(handlebars_talloc_strdup_append(original, part));
        
        // Handle paths
        if( !is_literal && (strcmp(part, "..") == 0 || strcmp(part, ".") == 0 || strcmp(part, "this") == 0) ) {
            if( count > 0 ) {
                handlebars_throw_ex(CONTEXT, HANDLEBARS_ERROR, locinfo, "Invalid path: %s", original);
            } else if( strcmp(part, "..") == 0 ) {
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
        struct handlebars_parser * parser, struct handlebars_ast_node * open_raw_block, 
        const char * content, const char * close, struct handlebars_locinfo * locinfo)
{
    struct handlebars_ast_node * content_node;
    struct handlebars_ast_node * open_block_path;
    
    assert(open_raw_block != NULL);
    assert(open_raw_block->type == HANDLEBARS_AST_NODE_INTERMEDIATE);
    assert(close != NULL);
    
    open_block_path = open_raw_block->node.intermediate.path;
    if( 0 != strcmp(open_block_path->node.path.original, close) ) {
        handlebars_throw_ex(CONTEXT, HANDLEBARS_ERROR, locinfo, "%s doesn't match %s", open_block_path->node.path.original, close);
    }
    
    // Create the content node
    content_node = handlebars_ast_node_ctor_content(parser, content, locinfo);
    
    // Create the raw block node
    return handlebars_ast_node_ctor_raw_block(parser, open_raw_block, content_node, locinfo);
}

static void handlebars_ast_helper_strip_comment_left(char * comment)
{
    char * c = comment;

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

    if( c > comment ) {
        memmove(comment, c, strlen(c) + 1);
    }
}

static void handlebars_ast_helper_strip_comment_right(char * comment)
{
    size_t len = strlen(comment);
    char * end = comment + len;
    char * c = end;

    if( len < 2 ) {
        return;
    }

    if( *--c != '}' ) {
        return;
    }

    if( *--c != '}' ) {
        return;
    }

    if( c > comment && *(c - 1) == '~' ) {
        c--;
    }

    if( c > comment && *(c - 1) == '-' ) {
        c--;
    }

    if( c > comment && *(c - 1) == '-' ) {
        c--;
    }

    if( c < end ) {
        *c = 0;
    }
}

char * handlebars_ast_helper_strip_comment(char * comment)
{
    assert(comment != NULL);
    handlebars_ast_helper_strip_comment_left(comment);
    handlebars_ast_helper_strip_comment_right(comment);
    return comment;
}

char * handlebars_ast_helper_strip_id_literal(char * comment)
{
	size_t len = strlen(comment);
	if( *comment == '[' && *(comment + len - 1) == ']' ) {
		if( len <= 2 ) {
			*comment = 0;
		} else {
			memmove(comment, comment + 1, len - 2);
			*(comment + len - 2) = 0;
		}
	}

	return comment;
}

void handlebars_ast_helper_set_strip_flags(
        struct handlebars_ast_node * ast_node, const char * open, const char * close)
{
    ast_node->strip = handlebars_ast_helper_strip_flags(open, close);
    if( ast_node->type == HANDLEBARS_AST_NODE_PARTIAL ) {
        ast_node->strip |= handlebars_ast_strip_flag_inline_standalone;
    }
}

unsigned handlebars_ast_helper_strip_flags(const char * open, const char * close)
{
    unsigned strip = 0;
    size_t close_length = close ? strlen(close) : 0;
    if( open && strlen(open) >= 3 && *(open + 2) == '~' ) {
        strip |= handlebars_ast_strip_flag_left;
    } else {
        strip &= ~handlebars_ast_strip_flag_left;
    }
    if( close_length && close_length >= 3 && *(close + close_length - 3) == '~' ) {
        strip |= handlebars_ast_strip_flag_right;
    } else {
        strip &= ~handlebars_ast_strip_flag_right;
    }
    strip |= handlebars_ast_strip_flag_set;
    return strip;
}

bool handlebars_ast_helper_scoped_id(struct handlebars_ast_node * path)
{
    char * original;
    char * found;
    size_t len;
    if( path && (original = path->node.path.original) ) {
        len = strlen(original);
        if( len >= 1 && *original == '.' ) {
            return true;
        } else if( len == 4 && 0 == strcmp(original, "this") ) {
            return true;
        } else if( len > 4 && NULL != (found = strstr(original, "this")) ) {
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
    return ( path &&
        path->node.path.parts && 
        handlebars_ast_list_count(path->node.path.parts) == 1 &&
        !handlebars_ast_helper_scoped_id(path) && 
        !path->node.path.depth );
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
