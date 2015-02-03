
#include <errno.h>
#include <memory.h>
#include <stdlib.h>
#include <talloc.h>

#include "handlebars.h"
#include "handlebars_context.h"
#include "handlebars_memory.h"
#include "handlebars_utils.h"
#include "handlebars.tab.h"
#include "handlebars.lex.h"

struct handlebars_context * handlebars_context_ctor(void)
{
    struct handlebars_context * context = NULL;
    int lexerr = 0;
    
    // Allocate struct as new top level talloc context
    context = handlebars_talloc_zero(NULL, struct handlebars_context);
    if( context == NULL ) {
        // Mainly doing this for consistency with lex init
        errno = ENOMEM;
        goto done;
    }
    
    // Set the current context in a variable for yyalloc >.>
    _handlebars_context_tmp = context;
    
    // Initialize lexer
    // @todo set a destructor on the context object to deinit the lexerf
    lexerr = handlebars_yy_lex_init(&context->scanner);
    if( lexerr != 0 ) {
        // Failure, free context and return null
        handlebars_talloc_free(context);
        context = NULL;
        goto done;
    }
  
  // Set the extra on the lexer
  handlebars_yy_set_extra(context, context->scanner);
  
done:
    _handlebars_context_tmp = NULL;
    return context;
}

void handlebars_context_dtor(struct handlebars_context * context)
{
  if( context != NULL ) {
    if( context->scanner != NULL ) {
      // Note: it has int return value, but appears to always return 0
      handlebars_yy_lex_destroy(context->scanner);
      context->scanner = NULL;
    }
  }
  
  handlebars_talloc_free(context);
}

char * handlebars_context_get_errmsg(struct handlebars_context * context)
{
    char * errmsg;
    char errbuf[256];
    
    if( context == NULL || context->error == NULL ) {
      return NULL;
    }
    
    snprintf(errbuf, sizeof(errbuf), "%s on line %d, column %d", 
            context->error,
            context->errloc->last_line, 
            context->errloc->last_column);
    
    errmsg = handlebars_talloc_strdup(context, errbuf);
    if( errmsg == NULL ) {
      // this might be a bad idea... 
      return context->error;
    }
    
    return errmsg;
}

char * handlebars_context_get_errmsg_js(struct handlebars_context * context)
{
    char * errmsg;
    char errbuf[512];
    
    if( context == NULL || context->error == NULL ) {
      return NULL;
    }
    
    // @todo check errno == HANDLEBARS_PARSEERR
    
    snprintf(errbuf, sizeof(errbuf), "Parse error on line %d, column %d : %s", 
            context->errloc->last_line, 
            context->errloc->last_column,
            context->error);
    
    errmsg = handlebars_talloc_strdup(context, errbuf);
    if( errmsg == NULL ) {
      // this might be a bad idea... 
      return context->error;
    }
    
    return errmsg;
}
