
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "handlebars.h"
#include "handlebars_ast.h"
#include "handlebars_context.h"
#include "handlebars_private.h"
#include "handlebars_token.h"
#include "handlebars_token_list.h"
#include "handlebars.tab.h"
#include "handlebars.lex.h"

int handlebars_version(void) {
    return HANDLEBARS_VERSION_PATCH 
        + (HANDLEBARS_VERSION_MINOR * 100)
        + (HANDLEBARS_VERSION_MAJOR * 10000);
}

const char * handlebars_version_string(void) {
    return HANDLEBARS_VERSION_STRING;
}

const char * handlebars_spec_version_string(void)
{
    return HANDLEBARS_SPEC_VERSION_STRING;
}

struct handlebars_token_list * handlebars_lex(struct handlebars_context * ctx)
{
    YYSTYPE yylval_param;
    YYLTYPE yylloc_param;
    struct handlebars_token_list * list;
    
    // Prepare token list
    list = handlebars_token_list_ctor(ctx);
    if( unlikely(list == NULL) ) {
        ctx->errnum = HANDLEBARS_NOMEM;
        return NULL;
    }

    // Save jump buffer
    if( setjmp(ctx->jmp) ) {
        goto done;
    }
    
    // Run
    do {
        YYSTYPE * lval;
        char * text;
        struct handlebars_token * token;
        int token_int;
        
        token_int = handlebars_yy_lex(&yylval_param, &yylloc_param, ctx->scanner);
        if( unlikely(token_int == END || token_int == INVALID) ) {
            break;
        }
        lval = handlebars_yy_get_lval(ctx->scanner);
        
        // Make token object
        text = (lval->text == NULL ? "" : lval->text);
        token = handlebars_token_ctor(token_int, text, strlen(text), list);
        if( unlikely(token == NULL) ) {
            break;
        }
        
        // Append
        handlebars_token_list_append(list, token);
    } while( 1 );

done:
    return list;
}

bool handlebars_parse(struct handlebars_context * ctx)
{
    // Save jump buffer
    if( setjmp(ctx->jmp) ) {
        goto done;
    }

    handlebars_yy_parse(ctx);

done:
    return ctx->program != NULL;
}
