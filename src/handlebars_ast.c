
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "handlebars.h"
#include "handlebars_ast.h"
#include "handlebars_ast_list.h"
#include "handlebars_context.h"
#include "handlebars_memory.h"
#include "handlebars_utils.h"

struct handlebars_ast_node * handlebars_ast_node_ctor(enum handlebars_node_type type, void * ctx)
{
    struct handlebars_ast_node * ast_node;
    
    ast_node = handlebars_talloc_zero(ctx, struct handlebars_ast_node);
    if( ast_node == NULL ) {
        errno = ENOMEM;
        goto done;
    }
    ast_node->type = type;
    
done:
    return ast_node;
}

void handlebars_ast_node_dtor(struct handlebars_ast_node * ast_node)
{
    handlebars_talloc_free(ast_node);
}

int handlebars_ast_node_id_init(struct handlebars_ast_node * ast_node, void * ctx)
{
    struct handlebars_ast_list * list = ast_node->node.id.parts;
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
    
    handlebars_ast_list_foreach(list, item, tmp) {
        part = item->data->node.path_segment.part;
        part_length = item->data->node.path_segment.part_length;
        if( !part ) {
            continue;
        }
        
        // Append to original
        if( item->data->node.path_segment.separator ) {
            original = handlebars_talloc_strndup_append_buffer(original, 
                    item->data->node.path_segment.separator, 
                    item->data->node.path_segment.separator_length);
            original_length += item->data->node.path_segment.separator_length;
        }
        original = handlebars_talloc_strndup_append_buffer(original, part, part_length);
        original_length += part_length;
        
        // Handle paths
        if( strcmp(part, "..") == 0 || strcmp(part, ".") == 0 || strcmp(part, "this") == 0 ) {
            if( count > 0 ) {
                // error :(
                return HANDLEBARS_ERROR;
                // throw new Exception("Invalid path: " + original, this);
            } else if( strcmp(part, "..") == 0 ) {
                depth++;
                // depthString += '../';
            } else {
                is_scoped = 1;
            }
            // I guess we're supposed to remove it from parts
            handlebars_ast_list_remove(list, item->data);
        } else {
            string = handlebars_talloc_strndup_append_buffer(string, part, part_length);
            string = handlebars_talloc_strndup_append_buffer(string, ".", 1);
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
        id_name = handlebars_talloc_size(ast_node, id_name_length);
        memset(id_name, '.', id_name_length);
        for( i = 1; i <= depth; i++ ) {
            id_name[i * 3 - 1] = '/';
        }
        id_name = handlebars_talloc_strndup_append_buffer(id_name, string, string_length);
        id_name_length += string_length;
        ast_node->node.id.id_name = id_name;
        ast_node->node.id.id_name_length = id_name_length;
    }
    
    return 0;
}

int handlebars_check_open_close(struct handlebars_ast_node * ast_node, struct handlebars_context * context, struct YYLTYPE * yylloc)
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

int handlebars_check_raw_open_close(struct handlebars_ast_node * ast_node, struct handlebars_context * context, struct YYLTYPE * yylloc)
{
    // this is retarded...
    struct handlebars_ast_node * open_node;
    char * open;
    char * close;
    int cmp;
    
    if( ast_node == NULL ) {
        return 1;
    }
    
    open_node = ast_node->node.raw_block.mustache;
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
    
    close = ast_node->node.raw_block.close;
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
