
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <talloc.h>

#include "handlebars_memory.h"
#include "handlebars_token.h"
#include "handlebars_token_list.h"
#include "handlebars_token_printer.h"
#include "handlebars_utils.h"

char * handlebars_token_print(struct handlebars_token * token, int flags)
{
    char * str = NULL;
    const char * name = NULL;
    char * tmp = NULL;
    const char * ws = "\r\n\t\v";
    
    // Sanity check
    if( token == NULL ) {
        goto done;
    }
    
    name = handlebars_token_readable_type(token->token);
    
    // Meh
    str = handlebars_talloc_strdup(token, "");
    str = handlebars_talloc_strdup_append(str, name);
    if( token->text != NULL ) {
        str = handlebars_talloc_strdup_append(str, " [");

        // Escape line breaks and tabs
        tmp = handlebars_addcslashes(token->text, token->length, ws, strlen(ws));
        str = handlebars_talloc_strdup_append(str, tmp);
        handlebars_talloc_free(tmp);

        str = handlebars_talloc_strdup_append(str, "]");
    }

    if( flags & HANDLEBARS_TOKEN_PRINT_NEWLINES ) {
        str = handlebars_talloc_strdup_append(str, "\n");
    } else {
        str = handlebars_talloc_strdup_append(str, " ");
    }

    token->length = strlen(str);
    
done:
    return str;
}

char * handlebars_token_list_print(struct handlebars_token_list * list, int flags)
{
    struct handlebars_token_list_item * el = NULL;
    struct handlebars_token_list_item * tmp = NULL;
    struct handlebars_token * token = NULL;
    char * str = NULL;
    char * token_str = NULL;
    
    str = handlebars_talloc_strdup(list, "");
    handlebars_token_list_foreach(list, el, tmp) {
        token = el->data;
        token_str = handlebars_token_print(token, flags);
        if( token_str != NULL ) {
            str = handlebars_talloc_strdup_append(str, token_str);
        }
    }
    
    return str;
}
