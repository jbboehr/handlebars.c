
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <talloc.h>

#include "handlebars_memory.h"
#include "handlebars_private.h"
#include "handlebars_token.h"
#include "handlebars_token_list.h"
#include "handlebars_token_printer.h"
#include "handlebars_utils.h"



char * handlebars_token_print(struct handlebars_token * token, int flags)
{
    char * str = NULL;
    const char * name;
    char * tmp = NULL;
    const char * tmp2 = "";
    const char * ws = "\r\n\t\v";
    char sep;
    
    // Sanity check
    if( unlikely(token == NULL) ) {
        goto done;
    }
    
    // Get name
    name = handlebars_token_readable_type(token->token);
    
    // Get separator
    if( flags & handlebars_token_printer_flag_newlines ) {
        sep = '\n';
    } else {
        sep = ' ';
    }
    
    // Prepare token text
    if( likely(token->text != NULL) ) {
        tmp = handlebars_addcslashes_ex(token->text, token->length, ws, strlen(ws));
        if( unlikely(tmp == NULL) ) {
            return NULL;
        }
        tmp2 = (const char *) tmp;
    }
    
    // Make output string
    str = handlebars_talloc_asprintf(token, "%s [%s]%c", name, tmp2, sep);
    if( unlikely(str == NULL) ) {
        goto done; // LCOV_EXCL_LINE
    }
    
    // idk what this was doing here >.>
    //token->length = strlen(str);
    
done:
    if( tmp ) {
        handlebars_talloc_free(tmp);
    }
    return str;
}

char * handlebars_token_list_print(struct handlebars_token_list * list, int flags)
{
    struct handlebars_token_list_item * el = NULL;
    struct handlebars_token_list_item * tmp = NULL;
    char * output = NULL;
    
    assert(list != NULL);

    output = handlebars_talloc_strdup(list, "");
    if( unlikely(output == NULL) ) {
        return NULL;
    }

    handlebars_token_list_foreach(list, el, tmp) {
        struct handlebars_token * token = el->data;
        char * token_str;

        //assert(token != NULL);

        token_str = handlebars_token_print(token, flags);
        if( likely(token_str != NULL) ) {
            output = handlebars_talloc_strdup_append(output, token_str);
        }
    }
    
    // Trim whitespace off right end of output
    handlebars_rtrim(output, " \t\r\n");
    
    return output;
}
