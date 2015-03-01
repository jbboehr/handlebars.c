
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <stdlib.h>
#include <talloc.h>

#include "handlebars.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"
#include "handlebars_token_list.h"

int handlebars_token_list_append(struct handlebars_token_list * list, struct handlebars_token * token)
{
    int error = HANDLEBARS_SUCCESS;
    struct handlebars_token_list_item * item = NULL;

    // Check args
    if( unlikely(list == NULL || token == NULL) ) {
        error = HANDLEBARS_NULLARG;
        goto error;
    }
    
    // Initialize list item
    item = handlebars_talloc_zero(list, struct handlebars_token_list_item);
    if( unlikely(item == NULL) ) {
        error = HANDLEBARS_NOMEM; 
        goto error;
    }
    item->data = token;
    
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

struct handlebars_token_list * handlebars_token_list_ctor(void * ctx)
{
    return handlebars_talloc_zero((struct handlebars_context *) ctx, struct handlebars_token_list);
}

void handlebars_token_list_dtor(struct handlebars_token_list * list)
{
    assert(list != NULL);

    handlebars_talloc_free(list);
}

int handlebars_token_list_prepend(struct handlebars_token_list * list, struct handlebars_token * token)
{
    int error = HANDLEBARS_SUCCESS;
    struct handlebars_token_list_item * item = NULL;
    
    // Check args
    if( unlikely(list == NULL || token == NULL) ) {
        error = HANDLEBARS_NULLARG;
        goto error;
    }
    
    // Initialize list item
    item = handlebars_talloc_zero(list, struct handlebars_token_list_item);
    if( unlikely(item == NULL) ) {
        error = HANDLEBARS_NOMEM; 
        goto error;
    }
    item->data = token;
    
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
