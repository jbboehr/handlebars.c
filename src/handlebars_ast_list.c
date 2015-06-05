
#include <assert.h>
#include <stdlib.h>
#include <talloc.h>

#include "handlebars.h"
#include "handlebars_ast.h"
#include "handlebars_ast_list.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"

int handlebars_ast_list_append(struct handlebars_ast_list * list, struct handlebars_ast_node * ast_node)
{
    int error = HANDLEBARS_SUCCESS;
    struct handlebars_ast_list_item * item = NULL;
    
    // Check args
    if( unlikely(list == NULL || ast_node == NULL) ) {
        error = HANDLEBARS_NULLARG;
        goto error;
    }
    
    // Initialize list item
    item = handlebars_talloc_zero(list, struct handlebars_ast_list_item);
    if( unlikely(item == NULL) ) {
        error = HANDLEBARS_NOMEM; 
        goto error;
    }
    item->data = ast_node;
    
    // Append item
    if( list->last == NULL ) {
        // Initialize
        list->first = item;
        list->last = item;
    } else {
        // Append
        handlebars_ast_list_insert_after(list, list->last, item);
    }
    list->count++;
    
error:
    return error;
}

int handlebars_ast_list_count(struct handlebars_ast_list * list)
{
    if( unlikely(list == NULL) ) {
        return 0;
    } else {
        return list->count;
    }
}

struct handlebars_ast_list * handlebars_ast_list_ctor(void * ctx)
{
    return handlebars_talloc_zero(ctx, struct handlebars_ast_list);
}

void handlebars_ast_list_dtor(struct handlebars_ast_list * list)
{
   handlebars_talloc_free(list);
}

struct handlebars_ast_list_item * handlebars_ast_list_find(
        struct handlebars_ast_list * list, struct handlebars_ast_node * ast_node)
{
    struct handlebars_ast_list_item * item = NULL;
    struct handlebars_ast_list_item * tmp = NULL;
    
    handlebars_ast_list_foreach(list, item, tmp) {
        if( item->data == ast_node ) {
            return item;
        }
    }
    
    return NULL;
}

void handlebars_ast_list_insert_after(struct handlebars_ast_list * list, 
        struct handlebars_ast_list_item * item,
        struct handlebars_ast_list_item * new_item)
{
    new_item->prev = item;
    new_item->next = item->next;
    if( item->next == NULL ) {
        list->last = new_item;
    } else {
        item->next->prev = new_item;
    }
    item->next = new_item;
}

void handlebars_ast_list_insert_before(struct handlebars_ast_list * list, 
        struct handlebars_ast_list_item * item,
        struct handlebars_ast_list_item * new_item)
{
    new_item->prev = item->prev;
    new_item->next = item;
    if( item->prev == NULL ) {
        list->first = new_item;
    } else {
        item->prev->next = new_item;
    }
    item->prev = new_item;
}

int handlebars_ast_list_remove(struct handlebars_ast_list * list, struct handlebars_ast_node * ast_node)
{
    struct handlebars_ast_list_item * it;
    struct handlebars_ast_list_item * tmp;
    struct handlebars_ast_list_item * found = NULL;
    
    if( !list->first || !list->last ) {
        return 0;
    }
    
    handlebars_ast_list_foreach(list, it, tmp) {
        if( it->data == ast_node ) {
            found = it;
            break;
        }
    }
    
    if( !found ) {
        return 0;
    }
    
    if( found->prev == NULL ) {
        list->first = found->next;
    } else {
        found->prev->next = found->next;
    }
    if( found->next == NULL ) {
        list->last = found->prev;
    } else {
        found->next->prev = found->prev;
    }
    list->count--;
    
    handlebars_talloc_free(found);
    
    return 1;
}

int handlebars_ast_list_prepend(struct handlebars_ast_list * list, struct handlebars_ast_node * ast_node)
{
    int error = HANDLEBARS_SUCCESS;
    struct handlebars_ast_list_item * item = NULL;
    
    // Check args
    if( unlikely(list == NULL || ast_node == NULL) ) {
        error = HANDLEBARS_NULLARG;
        goto error;
    }
    
    // Initialize list item
    item = handlebars_talloc_zero(list, struct handlebars_ast_list_item);
    if( unlikely(item == NULL) ) {
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
        handlebars_ast_list_insert_before(list, list->first, item);
    }
    list->count++;
    
error:
    return error;
}
