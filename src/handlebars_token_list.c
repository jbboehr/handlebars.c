
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



#undef CONTEXT
#define CONTEXT parser->ctx

struct handlebars_token_list * handlebars_token_list_ctor(struct handlebars_parser * parser)
{
    struct handlebars_token_list * list = MC(handlebars_talloc_zero(parser, struct handlebars_token_list));
    list->parser = parser;
    return list;
}

#undef CONTEXT
#define CONTEXT list->parser->ctx

int handlebars_token_list_append(struct handlebars_token_list * list, struct handlebars_token * token)
{
    int error = HANDLEBARS_SUCCESS;
    struct handlebars_token_list_item * item = NULL;

    // Check args
    assert(list != NULL);
    assert(token != NULL);
    
    // Initialize list item
    item = MC(handlebars_talloc_zero(list, struct handlebars_token_list_item));
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
    item = MC(handlebars_talloc_zero(list, struct handlebars_token_list_item));
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
