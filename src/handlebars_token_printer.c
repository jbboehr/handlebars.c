
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <talloc.h>

#include "handlebars_token.h"
#include "handlebars_token_list.h"
#include "handlebars_token_printer.h"
#include "handlebars_utils.h"

char * handlebars_token_print(struct handlebars_token * token)
{
    char * str = NULL;
    const char * name = NULL;
    
    // Sanity check
    if( token == NULL ) {
        goto done;
    }
    
    name = handlebars_token_readable_type(token->token);
    
    // Meh
    // @todo escape newlines T_T
    str = talloc_strdup(token, "");
    str = talloc_strdup_append(str, name);
    if( token->text != NULL ) {
        str = talloc_strdup_append(str, " [");
        str = talloc_strdup_append(str, token->text);
        str = talloc_strdup_append(str, "] ");
    } else {
        str = talloc_strdup_append(str, " ");
    }
    token->length = strlen(str);
    
done:
    return str;
}

char * handlebars_token_list_print(struct handlebars_token_list * list)
{
    struct handlebars_token_list_item * el = NULL;
    struct handlebars_token_list_item * tmp = NULL;
    struct handlebars_token * token = NULL;
    char * str = NULL;
    char * token_str = NULL;
    
    str = talloc_strdup(list, "");
    handlebars_token_list_foreach(list, el, tmp) {
        token = el->data;
        token_str = handlebars_token_print(token);
        if( token_str != NULL ) {
            str = talloc_strdup_append(str, token_str);
        }
    }
    
    return str;
}
