
#include <stdlib.h>
#include <talloc.h>

#include "handlebars.h"
#include "handlebars_ast.h"
#include "handlebars_ast_list.h"

int handlebars_ast_list_append(struct handlebars_ast_list * list, struct handlebars_ast_node * ast_node)
{
    int error = HANDLEBARS_SUCCESS;
    struct handlebars_ast_list_item * item = NULL;
    
    // Check args
    if( list == NULL || ast_node == NULL ) {
        error = HANDLEBARS_NULLARG;
        goto error;
    }
    
    // Initialize list item
    item = handlebars_talloc_zero(list, struct handlebars_ast_list_item);
    if( item == NULL ) {
        error = HANDLEBARS_NOMEM; 
        goto error;
    }
    item->data = ast_node;
    
    // Append item
    if( list->first == NULL ) {
        // Initialize
        list->first = item;
        list->last = item;
    } else {
        // Append
        list->last->next = item;
        list->last = item;
    }
    
error:
    return error;
}

struct handlebars_ast_list * handlebars_ast_list_ctor(void * ctx)
{
    return handlebars_talloc_zero(ctx, struct handlebars_ast_list);
}

void handlebars_ast_list_dtor(struct handlebars_ast_list * list)
{
    handlebars_talloc_free(list);
}

int handlebars_ast_list_prepend(struct handlebars_ast_list * list, struct handlebars_ast_node * ast_node)
{
    int error = HANDLEBARS_SUCCESS;
    struct handlebars_ast_list_item * item = NULL;
    
    // Check args
    if( list == NULL || ast_node == NULL ) {
        error = HANDLEBARS_NULLARG;
        goto error;
    }
    
    // Initialize list item
    item = handlebars_talloc_zero(list, struct handlebars_ast_list_item);
    if( item == NULL ) {
        error = HANDLEBARS_NOMEM; 
        goto error;
    }
    item->data = ast_node;
    
    // Prepend item
    if( list->first == NULL ) {
        // Initialize
        list->first = item;
        list->last = item;
    } else {
        // Prepend
        item->next = list->first;
        list->first = item;
    }
    
error:
    return error;
}
