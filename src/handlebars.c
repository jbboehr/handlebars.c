
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <stdarg.h>

#include "handlebars.h"
#include "handlebars_ast.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"
#include "handlebars_token.h"
#include "handlebars_token_list.h"
#include "handlebars.tab.h"
#include "handlebars.lex.h"



#undef CONTEXT
#define CONTEXT ctx


struct handlebars_parser * _handlebars_parser_init_current;

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

const char * handlebars_mustache_spec_version_string(void)
{
    return MUSTACHE_SPEC_VERSION_STRING;
}

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


char * handlebars_error_message(struct handlebars_context * context)
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

char * handlebars_error_message_js(struct handlebars_context * context)
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

struct handlebars_parser * handlebars_parser_ctor(struct handlebars_context * ctx)
{
    struct handlebars_parser * parser;
    int lexerr = 0;

    // Allocate struct as new top level talloc context
    parser = MC(handlebars_talloc_zero(ctx, struct handlebars_parser));
    if( unlikely(parser == NULL) ) {
        // Mainly doing this for consistency with lex init
        errno = ENOMEM;
        goto done;
    }
    parser->ctx = ctx;

    // Set the current context in a variable for yyalloc >.>
    _handlebars_parser_init_current = parser;

    // Initialize lexer
    // @todo set a destructor on the context object to deinit the lexer?
    lexerr = handlebars_yy_lex_init(&parser->scanner);
    if( unlikely(lexerr != 0) ) {
        handlebars_context_throw(CONTEXT, HANDLEBARS_NOMEM, "Lexer initialization failed");
    }

    // Set the extra on the lexer
    handlebars_yy_set_extra(parser, parser->scanner);

done:
    _handlebars_parser_init_current = NULL;
    return parser;
}

void handlebars_parser_dtor(struct handlebars_parser * parser)
{
    if( unlikely(parser == NULL) ) {
        return;
    }
    if( likely(parser->scanner != NULL) ) {
        // Note: it has int return value, but appears to always return 0
        handlebars_yy_lex_destroy(parser->scanner);
        parser->scanner = NULL;
    }
    handlebars_talloc_free(parser);
}

struct handlebars_token_list * handlebars_lex(struct handlebars_parser * parser)
{
    YYSTYPE yylval_param;
    YYLTYPE yylloc_param;
    struct handlebars_token_list * list;
    jmp_buf * prev = parser->ctx->e.jmp;
    jmp_buf buf;

    // Save jump buffer
    if( !prev ) {
        parser->ctx->e.jmp = &buf;
        if (setjmp(buf)) {
            goto done;
        }
    }

    // Prepare token list
    list = handlebars_token_list_ctor(parser);
    
    // Run
    do {
        YYSTYPE * lval;
        char * text;
        struct handlebars_token * token;
        int token_int;
        
        token_int = handlebars_yy_lex(&yylval_param, &yylloc_param, parser->scanner);
        if( unlikely(token_int == END || token_int == INVALID) ) {
            break;
        }
        lval = handlebars_yy_get_lval(parser->scanner);
        
        // Make token object
        text = (lval->text == NULL ? "" : lval->text);
        token = talloc_steal(list, handlebars_token_ctor(parser, token_int, text, strlen(text)));
        
        // Append
        handlebars_token_list_append(list, token);
    } while( 1 );

done:
    parser->ctx->e.jmp = prev;
    return list;
}

bool handlebars_parse(struct handlebars_parser * parser)
{
    jmp_buf * prev = parser->ctx->e.jmp;
    jmp_buf buf;

    // Save jump buffer
    if( !prev ) {
        parser->ctx->e.jmp = &buf;
        if (setjmp(buf)) {
            goto done;
        }
    }

    handlebars_yy_parse(parser);

done:
    parser->ctx->e.jmp = prev;
    return parser->program != NULL;
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