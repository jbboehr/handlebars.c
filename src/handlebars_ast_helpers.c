
#include <stdlib.h>
#include <string.h>
#include <talloc.h>

#include "handlebars.h"
#include "handlebars_ast.h"
#include "handlebars_ast_helpers.h"
#include "handlebars_ast_list.h"
#include "handlebars_context.h"
#include "handlebars_memory.h"
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

struct handlebars_ast_node * handlebars_ast_helper_prepare_block(
        struct handlebars_context * context, struct handlebars_ast_node * mustache,
        struct handlebars_ast_node * program, struct handlebars_ast_node * inverse,
        struct handlebars_ast_node * close, int inverted, struct YYLTYPE * yylloc)
{
    struct handlebars_ast_node * ast_node;
    TALLOC_CTX * ctx;
    
    // Initialize temporary talloc context
    ctx = talloc_init(NULL);
    
    // Create the block node
    ast_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_BLOCK, ctx);
    __MEMCHECK(ast_node);
    
    // Assign
    // @todo maybe we should reparent these
    ast_node->node.block.mustache = mustache;
    ast_node->node.block.program = program;
    ast_node->node.block.inverse = inverse;
    ast_node->node.block.close = close;
    ast_node->node.block.inverted = inverted;
    
    // Check if open/close match
    if( handlebars_ast_helper_check_block(ast_node, context, yylloc) != HANDLEBARS_SUCCESS ) {
        ast_node = NULL;
        goto error;
    }
    
    // Now steal the raw block node so it won't be freed below
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
    char * string = handlebars_talloc_strdup(ast_node, "");
    size_t string_length = 0;
    char * id_name = NULL;
    size_t id_name_length = 0;
    char * original = handlebars_talloc_strdup(ast_node, "");
    size_t original_length = 0;
    int i = 0;
    
    // Initialize temporary talloc context
    ctx = talloc_init(NULL);
    
    // Create the block node
    ast_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_ID, ctx);
    if( !ast_node ) {
        context->errnum = HANDLEBARS_NOMEM;
        ast_node = NULL;
        goto error;
    }
    
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
            string = talloc_asprintf_append_buffer(string, "%.*s.", part_length, part);
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
        id_name_length = depth * 3 + 1;
        id_name = handlebars_talloc_size(ast_node, id_name_length + string_length + 1);
        __MEMCHECK(id_name);
        memset(id_name, '.', id_name_length);
        for( i = 1; i <= depth; i++ ) {
            id_name[i * 3 - 1] = '/';
        }
        strncpy(id_name + id_name_length, string, string_length);
        id_name_length += string_length;
        ast_node->node.id.id_name = id_name;
        ast_node->node.id.id_name_length = id_name_length;
    }
    
    // Now steal the node so it won't be freed below
    talloc_steal(context, ast_node);
    
error:
    handlebars_talloc_free(ctx);
    return ast_node;
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
    ctx = talloc_init(NULL);
    
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
