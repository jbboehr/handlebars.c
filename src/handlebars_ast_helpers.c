
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
#include "handlebars_scanners.h"
#include "handlebars_utils.h"
#include "handlebars.tab.h"


#define __MEMCHECK(ptr) \
    do { \
        if( !ptr ) { \
            ast_node = NULL; \
            context->errnum = HANDLEBARS_NOMEM; \
            goto error; \
        } \
    } while(0)


static inline char * _handlebars_ast_helper_append_buffer(char ** a, size_t * a_len, char * b, size_t b_len)
{
    if( !a ) {
        return NULL;
    }
    if( !b || !b_len ) {
        return *a;
    }
    if( !*a ) {
        return NULL;
    }
    *a = handlebars_talloc_strndup_append_buffer(*a, b, b_len);
    if( !*a ) {
        *a_len = 0;
    } else {
        *a_len += b_len;
    }
    return *a;
}



int handlebars_ast_helper_check_block(struct handlebars_ast_node * ast_node, 
        struct handlebars_context * context, struct YYLTYPE * yylloc)
{
    // this is retarded...
    struct handlebars_ast_node * open_node;
    struct handlebars_ast_node * close_node;
    char * open;
    char * close;
    int cmp;
    
    if( ast_node == NULL ) {
        return 1;
    }
    
    open_node = ast_node->node.block.mustache;
    if( open_node == NULL ) {
        return 1;
    }
    open_node = open_node->node.mustache.sexpr;
    if( open_node == NULL ) {
        return 1;
    }
    open_node = open_node->node.sexpr.id;
    if( open_node == NULL ) {
        return 1;
    }
    open = open_node->node.id.original;
    if( open == NULL ) {
        return 1;
    }
    
    close_node = ast_node->node.block.close;
    if( close_node == NULL ) {
        return 1;
    }
    close = close_node->node.id.original;
    if( close == NULL ) {
        return 1;
    }
    
    cmp = strcmp(open, close);
    
    if( cmp != 0 ) {
        char errmsgtmp[256];
        snprintf(errmsgtmp, sizeof(errmsgtmp), "%s doesn't match %s", open, close);
        handlebars_yy_error(yylloc, context, errmsgtmp);
    }
    
    return cmp;
}

int handlebars_ast_helper_check_raw_block(struct handlebars_ast_node * ast_node, 
        struct handlebars_context * context, struct YYLTYPE * yylloc)
{
    // this is retarded...
    struct handlebars_ast_node * open_node;
    char * open;
    char * close;
    int cmp;
    
    if( ast_node == NULL ) {
        return HANDLEBARS_ERROR;
    }
    
    open_node = ast_node->node.raw_block.mustache;
    if( open_node == NULL ) {
        return HANDLEBARS_ERROR;
    }
    open_node = open_node->node.mustache.sexpr;
    if( open_node == NULL ) {
        return HANDLEBARS_ERROR;
    }
    open_node = open_node->node.sexpr.id;
    if( open_node == NULL ) {
        return HANDLEBARS_ERROR;
    }
    open = open_node->node.id.original;
    if( open == NULL ) {
        return HANDLEBARS_ERROR;
    }
    
    close = ast_node->node.raw_block.close;
    if( close == NULL ) {
        return HANDLEBARS_ERROR;
    }
    
    cmp = strcmp(open, close);
    
    if( cmp != 0 ) {
        char errmsgtmp[256];
        snprintf(errmsgtmp, sizeof(errmsgtmp), "%s doesn't match %s", open, close);
        handlebars_yy_error(yylloc, context, errmsgtmp);
    }
    
    return cmp;
}

int handlebars_ast_helper_is_next_whitespace(struct handlebars_ast_list * statements,
        struct handlebars_ast_node * statement, short is_root)
{
    struct handlebars_ast_list_item * item;
    struct handlebars_ast_list_item * next;
    struct handlebars_ast_list_item * sibling;
    
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
    struct handlebars_ast_list_item * sibling;

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

    if( statement == NULL ) {
        current = statements->last ? statements->last->data : NULL;
    } else {
        item = handlebars_ast_list_find(statements, statement);
        current = item && item->prev ? item->prev->data : NULL;
    }

    if( !current ||
        current->type != HANDLEBARS_AST_NODE_CONTENT ||
        (!multiple && (current->strip & handlebars_ast_strip_flag_left_stripped)) ) {
        return 0;
    }
    
    size_t original_length = strlen(current->node.content.string);

    if( multiple ) {
        current->node.content.string = handlebars_rtrim_ex(current->node.content.string, 
                &current->node.content.length, " \v\t\r\n", strlen(" \v\t\r\n"));
    } else {
        current->node.content.string = handlebars_rtrim_ex(current->node.content.string, 
                &current->node.content.length, " \t", strlen(" \t"));
    }
    
    if( original_length == strlen(current->node.content.string) ) {
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

    if( statement == NULL ) {
        current = statements->first ? statements->first->data : NULL;
    } else {
        item = handlebars_ast_list_find(statements, statement);
        current = item && item->next ? item->next->data : NULL;
    }

    if( !current ||
        current->type != HANDLEBARS_AST_NODE_CONTENT ||
        (!multiple && (current->strip & handlebars_ast_strip_flag_right_stripped)) ) {
        return 0;
    }

    size_t original_length = strlen(current->node.content.string);
    
    if( multiple ) {
        current->node.content.string = handlebars_ltrim_ex(current->node.content.string, 
                &current->node.content.length, " \v\t\r\n", strlen(" \v\t\r\n"));
    } else {
        current->node.content.string = handlebars_ltrim_ex(current->node.content.string, 
                &current->node.content.length, " \t", strlen(" \t"));
        if( *current->node.content.string == '\r' ) {
            memmove(current->node.content.string, current->node.content.string + 1, strlen(current->node.content.string));
            current->node.content.length--;
        }
        if( *current->node.content.string == '\n' ) {
            memmove(current->node.content.string, current->node.content.string + 1, strlen(current->node.content.string));
            current->node.content.length--;
        }
    }
    
    if( original_length == strlen(current->node.content.string) ) {
        current->strip &= ~handlebars_ast_strip_flag_right_stripped;
    } else {
        current->strip |= handlebars_ast_strip_flag_right_stripped;
    }
    current->strip |= handlebars_ast_strip_flag_set;

    return 1 && (current->strip & handlebars_ast_strip_flag_right_stripped);
}

struct handlebars_ast_node * handlebars_ast_helper_prepare_block(
        struct handlebars_context * context, struct handlebars_ast_node * mustache,
        struct handlebars_ast_node * program, struct handlebars_ast_node * inverse_and_program,
        struct handlebars_ast_node * close, int inverted, struct YYLTYPE * yylloc)
{
    struct handlebars_ast_node * ast_node;
    struct handlebars_ast_node * inverse;
    long inverse_strip;
    TALLOC_CTX * ctx;

    if( inverse_and_program ) {
        inverse = inverse_and_program->node.inverse_and_program.program;
        inverse_strip = inverse_and_program->strip;
    } else {
        inverse = NULL;
        inverse_strip = 0;
    }
    
    assert(!inverse_and_program || inverse_and_program->type == HANDLEBARS_AST_NODE_INVERSE_AND_PROGRAM);
    assert(!program || program->type == HANDLEBARS_AST_NODE_PROGRAM);
    assert(!inverse || inverse->type == HANDLEBARS_AST_NODE_PROGRAM);
    
    // Initialize temporary talloc context
    ctx = handlebars_talloc_size(NULL, 0);
    
    // Create the block node
    ast_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_BLOCK, ctx);
    __MEMCHECK(ast_node);
    
    // Assign
    // @todo maybe we should reparent these
    ast_node->node.block.mustache = mustache;
    ast_node->node.block.program = inverted ? inverse : program;
    ast_node->node.block.inverse = inverted ? program : inverse;
    ast_node->node.block.close = close;
    ast_node->node.block.inverted = inverted;
    
    // Check if open/close match
    if( handlebars_ast_helper_check_block(ast_node, context, yylloc) != HANDLEBARS_SUCCESS ) {
        ast_node = NULL;
        goto error;
    }
    
    // Whitespace control
    if( program && mustache && (mustache->strip & handlebars_ast_strip_flag_right) ) {
       handlebars_ast_helper_omit_right(program->node.program.statements, NULL, 1);
    }
    if( inverse ) {
        if( program && (inverse_strip & handlebars_ast_strip_flag_left) ) {
            handlebars_ast_helper_omit_left(program->node.program.statements, NULL, 1);
        }
        if( (inverse_strip & handlebars_ast_strip_flag_right) ) {
            handlebars_ast_helper_omit_right(inverse->node.program.statements, NULL, 1);
        }
        if( close && (close->strip & handlebars_ast_strip_flag_left) ) {
            handlebars_ast_helper_omit_left(inverse->node.program.statements, NULL, 1);
        }

        // Find standalone else statments
        if( program &&
                handlebars_ast_helper_is_prev_whitespace(program->node.program.statements, NULL, 0) &&
                handlebars_ast_helper_is_next_whitespace(inverse->node.program.statements, NULL, 0) ) {
            handlebars_ast_helper_omit_left(program->node.program.statements, NULL, 0);
            handlebars_ast_helper_omit_right(inverse->node.program.statements, NULL, 0);
        }
    } else {
        if( close && (close->strip & handlebars_ast_strip_flag_left) ) {
            handlebars_ast_helper_omit_left(program->node.program.statements, NULL, 1);
        }
    }

    // Save strip info
    if( mustache && (mustache->strip & handlebars_ast_strip_flag_left) ) {
        ast_node->strip |= handlebars_ast_strip_flag_left;
    }
    if( close && (close->strip & handlebars_ast_strip_flag_right) ) {
        ast_node->strip |= handlebars_ast_strip_flag_right;
    }
    if( program && handlebars_ast_helper_is_next_whitespace(program->node.program.statements, NULL, 0) ) {
        ast_node->strip |= handlebars_ast_strip_flag_open_standalone;
    }
    if( (program || inverse) && handlebars_ast_helper_is_prev_whitespace((inverse ? inverse : program)->node.program.statements, NULL, 0) ) {
        ast_node->strip |= handlebars_ast_strip_flag_close_standalone;
    }
    ast_node->strip |= handlebars_ast_strip_flag_set;

    // Now steal the block node so it won't be freed below
    talloc_steal(context, ast_node);
    
error:
    handlebars_talloc_free(ctx);
    return ast_node;
}

struct handlebars_ast_node * handlebars_ast_helper_prepare_id(
        struct handlebars_context * context, struct handlebars_ast_list * list)
{

    TALLOC_CTX * ctx;
    struct handlebars_ast_node * ast_node;
    struct handlebars_ast_list_item * item;
    struct handlebars_ast_list_item * tmp;
    char * part = NULL;
    size_t part_length = 0;
    int count = 0;
    int depth = 0;
    int is_scoped = 0;
    char * string = NULL;
    size_t string_length = 0;
    char * id_name = NULL;
    size_t id_name_length = 0;
    char * original = NULL;
    size_t original_length = 0;
    int i = 0;
    
    // Initialize temporary talloc context
    ctx = handlebars_talloc_size(NULL, 0);
    
    // Create the block node
    ast_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_ID, ctx);
    __MEMCHECK(ast_node);
    
    // Allocate and check the initial strings
    string = handlebars_talloc_strdup(ast_node, "");
    __MEMCHECK(string);
    original = handlebars_talloc_strdup(ast_node, "");
    __MEMCHECK(original);

    // Assign and reparent the list
    ast_node->node.id.parts = talloc_steal(ast_node, list);
    
    // Iterate over parts and process
    handlebars_ast_list_foreach(list, item, tmp) {
        part = item->data->node.path_segment.part;
        part_length = item->data->node.path_segment.part_length;
        if( !part ) {
            continue;
        }
        
        // Append to original
        _handlebars_ast_helper_append_buffer(&original, &original_length,
                item->data->node.path_segment.separator, 
                item->data->node.path_segment.separator_length);
        __MEMCHECK(original);
        _handlebars_ast_helper_append_buffer(&original, &original_length, part, part_length);
        __MEMCHECK(original);
        
        // Handle paths
        if( strcmp(part, "..") == 0 || strcmp(part, ".") == 0 || strcmp(part, "this") == 0 ) {
            if( count > 0 ) {
                // @todo fix message, was: "Invalid path: " + original
                ast_node = NULL;
                goto error;
            } else if( strcmp(part, "..") == 0 ) {
                depth++;
                // depthString += '../';
            } else {
                is_scoped = 1;
            }
            // I guess we're supposed to remove it from parts
            handlebars_ast_list_remove(list, item->data);
        } else {
            // @todo mock this out
            string = handlebars_talloc_asprintf_append_buffer(string, "%.*s.", part_length, part);
            __MEMCHECK(string);
            string_length += part_length + 1;
            count++;
        }
    }
    
    // Assign original, depth, and is_scoped
    ast_node->node.id.original = original;
    ast_node->node.id.original_length = original_length;
    ast_node->node.id.depth = depth;
    ast_node->node.id.is_scoped = is_scoped;
    
    // Assign is_simple
    ast_node->node.id.is_simple = (count == 1 && !is_scoped && depth == 0);
    
    // Trim the last period off the end of string and assign
    if( count ) {
        string[string_length - 1] = 0;
        string_length--;
        ast_node->node.id.string = string;
        ast_node->node.id.string_length = string_length;
    }
    
    // Make idName and assign
    if( depth > 0 ) {
        id_name_length = depth * 3;
        id_name = handlebars_talloc_size(ast_node, id_name_length + string_length + 1);
        __MEMCHECK(id_name);
        memset(id_name + id_name_length, 0, string_length + 1);
        memset(id_name, '.', id_name_length);
        for( i = 1; i <= depth; i++ ) {
            id_name[i * 3 - 1] = '/';
        }
        strncpy(id_name + id_name_length, string, string_length);
        id_name_length += string_length;
        id_name[id_name_length] = 0;
        ast_node->node.id.id_name = id_name;
        ast_node->node.id.id_name_length = id_name_length;
    }
    
    // Now steal the node so it won't be freed below
    talloc_steal(context, ast_node);
    
error:
    handlebars_talloc_free(ctx);
    return ast_node;
}

int handlebars_ast_helper_prepare_program(struct handlebars_context * context, 
        struct handlebars_ast_node * program, short is_root)
{
    int error = HANDLEBARS_SUCCESS;
    struct handlebars_ast_list * statements = program->node.program.statements;
    struct handlebars_ast_list_item * item;
    struct handlebars_ast_list_item * tmp;
    
    handlebars_ast_list_foreach(statements, item, tmp) {
        struct handlebars_ast_node * current = item->data;
        if( !current || !(current->strip & handlebars_ast_strip_flag_set) ) {
            continue;
        }
        short is_prev_whitespace = handlebars_ast_helper_is_prev_whitespace(statements, current, is_root);
        short is_next_whitespace = handlebars_ast_helper_is_next_whitespace(statements, current, is_root);
        short open_standalone = (current->strip & handlebars_ast_strip_flag_open_standalone) && is_prev_whitespace;
        short close_standalone = (current->strip & handlebars_ast_strip_flag_close_standalone) && is_next_whitespace;
        short inline_standalone = (current->strip & handlebars_ast_strip_flag_inline_standalone) && is_prev_whitespace && is_next_whitespace;
        
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
        struct handlebars_context * context, struct handlebars_ast_node * mustache, 
        const char * content, const char * close, struct YYLTYPE * yylloc)
{
    struct handlebars_ast_node * ast_node;
    struct handlebars_ast_node * content_node;
    char * tmp;
    TALLOC_CTX * ctx;
    
    // Initialize temporary talloc context
    ctx = handlebars_talloc_size(NULL, 0);
    
    // Create the raw block node
    ast_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_RAW_BLOCK, ctx);
    __MEMCHECK(ast_node);
    
    // Create and assign the content node
    content_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_CONTENT, ast_node);
    __MEMCHECK(content_node);
    ast_node->node.raw_block.program = content_node;
    
    // Duplicate and assign the content string
    tmp = handlebars_talloc_strdup(content_node, content);
    __MEMCHECK(tmp);
    content_node->node.content.string = tmp;
    content_node->node.content.length = strlen(tmp);
    
    // Duplicate and assign the close string
    tmp = handlebars_talloc_strdup(ast_node, close);
    __MEMCHECK(tmp);
    ast_node->node.raw_block.close = tmp;
    
    // Steal and assign the mustache node
    ast_node->node.raw_block.mustache = talloc_steal(ast_node, mustache);
    
    // Check if open/close match
    if( handlebars_ast_helper_check_raw_block(ast_node, context, yylloc) != HANDLEBARS_SUCCESS ) {
        ast_node = NULL;
        goto error;
    }
    
    // Now steal the raw block node so it won't be freed below
    talloc_steal(context, ast_node);
    
error:
    handlebars_talloc_free(ctx);
    return ast_node;
}

struct handlebars_ast_node * handlebars_ast_helper_prepare_sexpr(
        struct handlebars_context * context, struct handlebars_ast_node * id,
        struct handlebars_ast_list * params, struct handlebars_ast_node * hash)
{
    struct handlebars_ast_node * ast_node;
    
    // Create the sexpr node
    ast_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_SEXPR, context);
    __MEMCHECK(ast_node);
    
    // Assign the params
    ast_node->node.sexpr.id = id;
    ast_node->node.sexpr.params = params;
    ast_node->node.sexpr.hash = hash;
    
    // Is it a helper?
    if( (params != NULL && params->first != NULL) || hash != NULL ) {
        ast_node->node.sexpr.is_helper = 1;
        ast_node->node.sexpr.eligible_helper = 1;
    }
    
    // Is it an eligible helper?
    if( id != NULL && id->node.id.is_simple ) {
        ast_node->node.sexpr.eligible_helper = 1;
    }

error:
    //handlebars_talloc_free(ctx);
    return ast_node;
}

void handlebars_ast_helper_set_strip_flags(
        struct handlebars_ast_node * ast_node, char * open, char * close)
{
    size_t close_length = close ? strlen(close) : 0;
    if( open && strlen(open) >= 3 && *(open + 2) == '~' ) {
        ast_node->strip |= handlebars_ast_strip_flag_left;
    } else {
        ast_node->strip &= ~handlebars_ast_strip_flag_left;
    }
    if( close_length && close_length >= 3 && *(close + close_length - 3) == '~' ) {
        ast_node->strip |= handlebars_ast_strip_flag_right;
    } else {
        ast_node->strip &= ~handlebars_ast_strip_flag_right;
    }
    if( ast_node->type == HANDLEBARS_AST_NODE_PARTIAL ) {
        ast_node->strip |= handlebars_ast_strip_flag_inline_standalone;
    }
    ast_node->strip |= handlebars_ast_strip_flag_set;
}
