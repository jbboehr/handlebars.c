
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <memory.h>
#include <talloc.h>

#include "handlebars.h"
#include "handlebars_ast.h"
#include "handlebars_context.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"
#include "handlebars_utils.h"
#include "handlebars.tab.h"
#include "handlebars.lex.h"

struct handlebars_context * handlebars_context_ctor_ex(void * ctx)
{
    struct handlebars_context * context = NULL;
    
    // Allocate struct as new top level talloc context
    context = handlebars_talloc_zero(ctx, struct handlebars_context);
    if( unlikely(context == NULL) ) {
        // Mainly doing this for consistency with lex init
        errno = ENOMEM;
    }

    return context;
}

void handlebars_context_dtor(struct handlebars_context * context)
{
    handlebars_talloc_free(context);
}

static inline void _set_err(struct handlebars_context * context, enum handlebars_error_type num, struct handlebars_locinfo * loc, const char * msg, va_list ap)
{
    context->e.num = num;
    context->e.msg = talloc_vasprintf(context, msg, ap);
    if( loc ) {
        context->e.loc = *loc;
    } else {
        memset(&context->e.loc, 0, sizeof(context->e.loc));
    }
}

void handlebars_context_throw(struct handlebars_context * context, enum handlebars_error_type num, const char * msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    _set_err(context, num, NULL, msg, ap);
    va_end(ap);
    if( context->e.jmp ) {
        longjmp(*context->e.jmp, num);
    } else {
        fprintf(stderr, "Throw with invalid jmp_buf: %s\n", msg);
        abort();
    }
}

void handlebars_context_throw_ex(struct handlebars_context * context, enum handlebars_error_type num, struct handlebars_locinfo * loc, const char * msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    _set_err(context, num, loc, msg, ap);
    va_end(ap);
    if( context->e.jmp ) {
        longjmp(*context->e.jmp, num);
    } else {
        fprintf(stderr, "Throw with invalid jmp_buf: %s\n", msg);
        abort();
    }
}

char * handlebars_context_get_errmsg(struct handlebars_context * context)
{
    char * errmsg;
    char errbuf[256];
    
    if( context == NULL || context->e.msg == NULL ) {
      return NULL;
    }
    
    snprintf(errbuf, sizeof(errbuf), "%s on line %d, column %d", 
            context->e.msg,
            context->e.loc.last_line,
            context->e.loc.last_column);
    
    errmsg = handlebars_talloc_strdup(context, errbuf);
    if( unlikely(errmsg == NULL) ) {
      // this might be a bad idea... 
      return context->e.msg;
    }
    
    return errmsg;
}

char * handlebars_context_get_errmsg_js(struct handlebars_context * context)
{
    char * errmsg;
    char errbuf[512];
    
    if( context == NULL || context->e.msg == NULL ) {
      return NULL;
    }
    
    // @todo check errno == HANDLEBARS_PARSEERR
    
    snprintf(errbuf, sizeof(errbuf), "Parse error on line %d, column %d : %s", 
            context->e.loc.last_line,
            context->e.loc.last_column,
            context->e.msg);
    
    errmsg = handlebars_talloc_strdup(context, errbuf);
    if( unlikely(errmsg == NULL) ) {
      // this might be a bad idea... 
      return context->e.msg;
    }
    
    return errmsg;
}
