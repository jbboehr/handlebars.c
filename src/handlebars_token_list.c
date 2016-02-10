
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <stdlib.h>
#include <talloc.h>

#include "handlebars.h"
#include "handlebars_context.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"
#include "handlebars_token_list.h"

#define __S1(x) #x
#define __S2(x) __S1(x)
#define __MEMCHECK(ptr) \
    do { \
        if( unlikely(ptr == NULL) ) { \
            handlebars_context_throw(CONTEXT, HANDLEBARS_NOMEM, "Out of memory  [" __S2(__FILE__) ":" __S2(__LINE__) "]"); \
        } \
    } while(0)



#define CONTEXT context

struct handlebars_token_list * handlebars_token_list_ctor(struct handlebars_context * context)
{
    struct handlebars_token_list * list = handlebars_talloc_zero(context, struct handlebars_token_list);
    __MEMCHECK(list);
    list->ctx = context;
    return list;
}

#undef CONTEXT
#define CONTEXT list->ctx

int handlebars_token_list_append(struct handlebars_token_list * list, struct handlebars_token * token)
{
    int error = HANDLEBARS_SUCCESS;
    struct handlebars_token_list_item * item = NULL;

    // Check args
    assert(list != NULL);
    assert(token != NULL);
    
    // Initialize list item
    item = handlebars_talloc_zero(list, struct handlebars_token_list_item);
    __MEMCHECK(item);

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

    return error;
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
    assert(list != NULL);
    assert(token != NULL);
    
    // Initialize list item
    item = handlebars_talloc_zero(list, struct handlebars_token_list_item);
    __MEMCHECK(item);

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

    return error;
}
