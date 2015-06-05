
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
#include "handlebars_context.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"
#include "handlebars_scanners.h"
#include "handlebars_utils.h"
#include "handlebars.tab.h"


#define __MEMCHECK(ptr) \
    do { \
        assert(ptr); \
        if( unlikely(ptr == NULL) ) { \
            ast_node = NULL; \
            context->errnum = HANDLEBARS_NOMEM; \
            goto error; \
        } \
    } while(0)



int handlebars_ast_helper_is_next_whitespace(struct handlebars_ast_list * statements,
        struct handlebars_ast_node * statement, short is_root)
{
    struct handlebars_ast_list_item * item;
    struct handlebars_ast_list_item * next;
    struct handlebars_ast_node * sibling;
    
    if( !statements ) {
        return is_root;
    }

    if( statement == NULL ) {
        next = statements->first;
    } else {
        item = handlebars_ast_list_find(statements, statement);
        next = (item ? item->next : NULL);
    }
    
    if( !next || !next->data ) {
        return (int) is_root;
    }
    
    sibling = (next->next ? next->next->data : NULL);
    
    if( next->data->type == HANDLEBARS_AST_NODE_CONTENT ) {
        return handlebars_scanner_next_whitespace(next->data->node.content.original, !sibling && is_root);
    }
    
    return 0;
}

int handlebars_ast_helper_is_prev_whitespace(struct handlebars_ast_list * statements,
        struct handlebars_ast_node * statement, short is_root)
{
    struct handlebars_ast_list_item * item;
    struct handlebars_ast_list_item * prev;
    struct handlebars_ast_node * sibling;

    if( !statements ) {
        return is_root;
    }

    if( statement == NULL ) {
        prev = statements->last;
    } else {
        item = handlebars_ast_list_find(statements, statement);
        prev = (item ? item->prev : NULL);
    }
    
    if( !prev || !prev->data ) {
        return (int) is_root;
    }
    
    sibling = (prev->prev ? prev->prev->data : NULL);
    
    if( prev->data->type == HANDLEBARS_AST_NODE_CONTENT ) {
        return handlebars_scanner_prev_whitespace(prev->data->node.content.original, !sibling && is_root);
    }
    
    return 0;
}

int handlebars_ast_helper_omit_left(struct handlebars_ast_list * statements,
        struct handlebars_ast_node * statement, short multiple)
{
    struct handlebars_ast_node * current;
    struct handlebars_ast_list_item * item;
    size_t original_length;
    
    if( statement == NULL ) {
        current = statements->last ? statements->last->data : NULL;
    } else {
        item = handlebars_ast_list_find(statements, statement);
        current = item && item->prev ? item->prev->data : NULL;
    }

    if( !current || current->type != HANDLEBARS_AST_NODE_CONTENT ||
            (!multiple && (current->strip & handlebars_ast_strip_flag_left_stripped)) ) {
        return 0;
    }
    
    original_length = strlen(current->node.content.value);

    if( multiple ) {
        current->node.content.value = handlebars_rtrim(current->node.content.value, " \v\t\r\n");
    } else {
        current->node.content.value = handlebars_rtrim(current->node.content.value, " \t");
    }
    
    if( original_length == strlen(current->node.content.value) ) {
        current->strip &= ~handlebars_ast_strip_flag_left_stripped;
    } else {
        current->strip |= handlebars_ast_strip_flag_left_stripped;
    }
    current->strip |= handlebars_ast_strip_flag_set;
    
    return 1 && (current->strip & handlebars_ast_strip_flag_left_stripped);
}

int handlebars_ast_helper_omit_right(struct handlebars_ast_list * statements,
        struct handlebars_ast_node * statement, short multiple)
{
    struct handlebars_ast_node * current;
    struct handlebars_ast_list_item * item;
    size_t original_length;
    
    if( statement == NULL ) {
        current = statements->first ? statements->first->data : NULL;
    } else {
        item = handlebars_ast_list_find(statements, statement);
        current = item && item->next ? item->next->data : NULL;
    }

    if( !current || current->type != HANDLEBARS_AST_NODE_CONTENT ||
            (!multiple && (current->strip & handlebars_ast_strip_flag_right_stripped)) ) {
        return 0;
    }

    original_length = strlen(current->node.content.value);
    
    if( multiple ) {
        current->node.content.value = handlebars_ltrim(current->node.content.value, " \v\t\r\n");
    } else {
        current->node.content.value = handlebars_ltrim(current->node.content.value, " \t");
        if( *current->node.content.value == '\r' ) {
            memmove(current->node.content.value, current->node.content.value + 1, strlen(current->node.content.value));
        }
        if( *current->node.content.value == '\n' ) {
            memmove(current->node.content.value, current->node.content.value + 1, strlen(current->node.content.value));
        }
    }
    
    if( original_length == strlen(current->node.content.value) ) {
        current->strip &= ~handlebars_ast_strip_flag_right_stripped;
    } else {
        current->strip |= handlebars_ast_strip_flag_right_stripped;
    }
    current->strip |= handlebars_ast_strip_flag_set;

    return 1 && (current->strip & handlebars_ast_strip_flag_right_stripped);
}

struct handlebars_ast_node * handlebars_ast_helper_prepare_block(
        struct handlebars_context * context, struct handlebars_ast_node * open_block,
        struct handlebars_ast_node * program, struct handlebars_ast_node * inverse_and_program,
        struct handlebars_ast_node * close, int inverted,
        struct handlebars_locinfo * locinfo)
{
    struct handlebars_ast_node * open_block_path = open_block->node.intermediate.path;
    struct handlebars_ast_node * close_block_path;
    struct handlebars_ast_node * inverse = NULL;
    struct handlebars_ast_node * tmp;
    long inverse_strip;
    
    assert(open_block != NULL && open_block->type == HANDLEBARS_AST_NODE_INTERMEDIATE);
    assert(close == NULL || close->type == HANDLEBARS_AST_NODE_INTERMEDIATE || close->type == HANDLEBARS_AST_NODE_INVERSE);
    
    if( close && close->type == HANDLEBARS_AST_NODE_INTERMEDIATE ) {
        close_block_path = close->node.intermediate.path;
        if( close_block_path && 0 != strcmp(open_block_path->node.path.original, close_block_path->node.path.original) ) {
            char errmsgtmp[256];
            snprintf(errmsgtmp, sizeof(errmsgtmp), "%s doesn't match %s", 
                    open_block_path->node.path.original, 
                    close_block_path->node.path.original);
            handlebars_yy_error(locinfo, context, errmsgtmp);
            return NULL;
        }
    }
    
    // @todo this isn't supposed to be null I think...
    if( !program ) {
        program = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_PROGRAM, context);
    }
    program->node.program.block_param1 = open_block->node.intermediate.block_param1;
    program->node.program.block_param2 = open_block->node.intermediate.block_param2;
    
    if( inverse_and_program ) {
        assert(inverse_and_program->type == HANDLEBARS_AST_NODE_INVERSE);

        if( inverse_and_program->node.inverse.chained ) {
            // inverseAndProgram.program.body[0].closeStrip = close.strip;
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
    
    return handlebars_ast_node_ctor_block(context, open_block, program, inverse,
                open_block->strip, inverse_strip, close ? close->strip : 0, locinfo);
}

struct handlebars_ast_node * handlebars_ast_helper_prepare_inverse_chain(
        struct handlebars_context * context, struct handlebars_ast_node * open_inverse_chain,
        struct handlebars_ast_node * program, struct handlebars_ast_node * inverse_chain,
        struct handlebars_locinfo * locinfo)
{
    struct handlebars_ast_node * block_node;
    struct handlebars_ast_list * statements;
    struct handlebars_ast_node * program_node;
    struct handlebars_ast_node * ast_node;
    
    block_node = handlebars_ast_helper_prepare_block(context, open_inverse_chain, program, inverse_chain, inverse_chain, 0, locinfo);
    statements = handlebars_ast_list_ctor(context);
    handlebars_ast_list_append(statements, block_node);
    program_node = handlebars_ast_node_ctor_program(context, statements, NULL, NULL, 0, 1, locinfo);
    ast_node = handlebars_ast_node_ctor_inverse(context, program_node, 1, 
                    (open_inverse_chain ? open_inverse_chain->strip : 0), locinfo);
    
    return ast_node;
}

struct handlebars_ast_node * handlebars_ast_helper_prepare_mustache(
        struct handlebars_context * context, struct handlebars_ast_node * intermediate,
        struct handlebars_ast_node * block_params,
        char * open, unsigned strip, struct handlebars_locinfo * locinfo)
{
    char c = 0;
    size_t open_len;
    struct handlebars_ast_node * path = intermediate->node.intermediate.path;
    struct handlebars_ast_list * params = intermediate->node.intermediate.params;
    struct handlebars_ast_node * hash = intermediate->node.intermediate.hash;
    struct handlebars_ast_node * ast_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_MUSTACHE, context);
    __MEMCHECK(ast_node);
    
    ast_node->loc = *locinfo;
    ast_node->strip = strip;
    
    if( path ) {
        ast_node->node.mustache.path = talloc_steal(ast_node, path);
    }
    if( params ) {
        ast_node->node.mustache.params = talloc_steal(ast_node, params);
    }
    if( hash ) {
        ast_node->node.mustache.hash = talloc_steal(ast_node, hash);
    }
    
    // Check escaped
    if( open ) {
        open_len = strlen(open);
        if( open_len >= 4 ) {
            c = *(open + 3);
        } else if( open_len >= 3 ) {
            c = *(open + 2);
        }
    }
    ast_node->node.mustache.unescaped = (c == '{' || c == '&');
    
    // Free the intermediate node
    handlebars_talloc_free(intermediate);
    
error:
    return ast_node;
}

struct handlebars_ast_node * handlebars_ast_helper_prepare_path(
        struct handlebars_context * context, struct handlebars_ast_list * parts,
        short data, struct handlebars_locinfo * locinfo)
{
    TALLOC_CTX * ctx;
    struct handlebars_ast_node * ast_node;
    struct handlebars_ast_list_item * item;
    struct handlebars_ast_list_item * tmp;
    char * part = NULL;
    char * separator;
    char * original = NULL;
    short is_literal;
    int depth = 0;
    int count = 0;
    
    // Initialize temporary talloc context
    ctx = talloc_new(context);
    if( unlikely(ctx == NULL) ) {
        return NULL;
    }
    
    // Allocate the original strings
    original = handlebars_talloc_strdup(ctx, data ? "@" : "");
    __MEMCHECK(original);
    
    // Iterate over parts and process
    handlebars_ast_list_foreach(parts, item, tmp) {
        part = item->data->node.path_segment.part;
        if( unlikely(part == NULL) ) {
            continue;
        }
        separator = item->data->node.path_segment.separator;
        is_literal = 0;
        // @todo implement
        //is_literal = 0 == strcmp(part, item->data->node.path_segment.original);
        
        // Append to original
        if( separator ) {
            original = handlebars_talloc_strdup_append(original, separator);
            __MEMCHECK(original);
        }
        original = handlebars_talloc_strdup_append(original, part);
        __MEMCHECK(original);
        
        // Handle paths
        if( !is_literal && (strcmp(part, "..") == 0 || strcmp(part, ".") == 0 || strcmp(part, "this") == 0) ) {
            if( count > 0 ) {
                context->error = handlebars_talloc_asprintf(context, "Invalid path: %s", original);
                context->errloc = locinfo;
                ast_node = NULL;
                goto error;
            } else if( strcmp(part, "..") == 0 ) {
                depth++;
            }
            // Instead of adding it below, remove it here
            handlebars_ast_list_remove(parts, item->data);
        } else {
            count++;
        }
    }
    
    ast_node = handlebars_ast_node_ctor_path(context, parts, original, depth, data, locinfo);
    
error:
    handlebars_talloc_free(ctx);
    return ast_node;
}

int handlebars_ast_helper_prepare_program(
        HANDLEBARS_ATTR_UNUSED struct handlebars_context * context,
        struct handlebars_ast_node * program, short is_root,
        struct handlebars_locinfo * locinfo)
{
    int error = HANDLEBARS_SUCCESS;
    struct handlebars_ast_list * statements = program->node.program.statements;
    struct handlebars_ast_list_item * item;
    struct handlebars_ast_list_item * tmp;
    
    if( locinfo ) {
        program->loc = *locinfo;
    }
    
    handlebars_ast_list_foreach(statements, item, tmp) {
        struct handlebars_ast_node * current = item->data;
        short is_prev_whitespace, is_next_whitespace, open_standalone,
              close_standalone, inline_standalone;
        if( !current || !(current->strip & handlebars_ast_strip_flag_set) ) {
            continue;
        }
        is_prev_whitespace = handlebars_ast_helper_is_prev_whitespace(statements, current, is_root);
        is_next_whitespace = handlebars_ast_helper_is_next_whitespace(statements, current, is_root);
        open_standalone = (current->strip & handlebars_ast_strip_flag_open_standalone) && is_prev_whitespace;
        close_standalone = (current->strip & handlebars_ast_strip_flag_close_standalone) && is_next_whitespace;
        inline_standalone = (current->strip & handlebars_ast_strip_flag_inline_standalone) && is_prev_whitespace && is_next_whitespace;
        
        if( current->strip & handlebars_ast_strip_flag_right ) {
            handlebars_ast_helper_omit_right(statements, current, 1);
        }
        if( current->strip & handlebars_ast_strip_flag_left ) {
            handlebars_ast_helper_omit_left(statements, current, 1);
        }
        if( inline_standalone ) {
            handlebars_ast_helper_omit_right(statements, current, 0);
            if( handlebars_ast_helper_omit_left(statements, current, 0) ) {
                struct handlebars_ast_node * prev = item->prev ? item->prev->data : NULL;
                if( current->type == HANDLEBARS_AST_NODE_PARTIAL &&
                        prev && prev->type == HANDLEBARS_AST_NODE_CONTENT ) {
                    char * start = prev->node.content.original;
                    char * ptr;
                    char * match = NULL;
                    for( ptr = start; *ptr; ++ptr ) {
                        if( *ptr == ' ' || *ptr == '\t' ) {
                            if( !match ) {
                                match = ptr;
                            }
                        } else if( *ptr ) {
                            match = NULL;
                        }
                    }
                    if( match ) {
                        current->node.partial.indent = handlebars_talloc_strdup(current, match);
                    }
                }
            }
        }
        if( open_standalone ) {
            if( current->type == HANDLEBARS_AST_NODE_BLOCK ) {
                if( current->node.block.program ) {
                    assert(current->node.block.program->type == HANDLEBARS_AST_NODE_PROGRAM);
                    handlebars_ast_helper_omit_right(current->node.block.program->node.program.statements, NULL, 0);
                } else if( current->node.block.inverse ) {
                    assert(current->node.block.inverse->type == HANDLEBARS_AST_NODE_PROGRAM);
                    handlebars_ast_helper_omit_right(current->node.block.inverse->node.program.statements, NULL, 0);
                }
            }
            handlebars_ast_helper_omit_left(statements, current, 0);
        }
        if( close_standalone ) {
            handlebars_ast_helper_omit_right(statements, current, 0);
            if( current->type == HANDLEBARS_AST_NODE_BLOCK ) {
                if( current->node.block.inverse ) {
                    assert(current->node.block.inverse->type == HANDLEBARS_AST_NODE_PROGRAM);
                    handlebars_ast_helper_omit_left(current->node.block.inverse->node.program.statements, NULL, 0);
                } else if( current->node.block.program ) {
                    assert(current->node.block.program->type == HANDLEBARS_AST_NODE_PROGRAM);
                    handlebars_ast_helper_omit_left(current->node.block.program->node.program.statements, NULL, 0);
                }
            }
        }
    }
    
//error:
    return error;
}

struct handlebars_ast_node * handlebars_ast_helper_prepare_raw_block(
        struct handlebars_context * context, struct handlebars_ast_node * open_raw_block, 
        const char * content, const char * close, struct handlebars_locinfo * locinfo)
{
    struct handlebars_ast_node * ast_node;
    struct handlebars_ast_node * content_node;
    struct handlebars_ast_node * open_block_path;
    char * tmp;
    TALLOC_CTX * ctx;
    
    // Initialize temporary talloc context
    ctx = talloc_new(context);
    if( unlikely(ctx == NULL) ) {
        return NULL;
    }
    
    assert(open_raw_block != NULL);
    assert(open_raw_block->type == HANDLEBARS_AST_NODE_INTERMEDIATE);
    assert(close != NULL);
    
    open_block_path = open_raw_block->node.intermediate.path;
    if( 0 != strcmp(open_block_path->node.path.original, close) ) {
        char errmsgtmp[256];
        snprintf(errmsgtmp, sizeof(errmsgtmp), "%s doesn't match %s", 
                open_block_path->node.path.original, 
                close);
        handlebars_yy_error(locinfo, context, errmsgtmp);
        return NULL;
    }
    
    // Create the content node
    content_node = handlebars_ast_node_ctor_content(context, content, locinfo);
    talloc_steal(ctx, content_node);
    
    // Create the raw block node
    ast_node = handlebars_ast_node_ctor_raw_block(context, open_raw_block, 
        content_node, locinfo);
    __MEMCHECK(ast_node);
    talloc_steal(ctx, ast_node);
    
    // Steal the raw block node so it won't be freed below
    talloc_steal(context, ast_node);
    
error:
    handlebars_talloc_free(ctx);
    return ast_node;
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
    handlebars_ast_helper_strip_comment_left(comment);
    handlebars_ast_helper_strip_comment_right(comment);
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
