
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <errno.h>
#include <stdarg.h>

#include "handlebars.h"
#include "handlebars_ast.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"
#include "handlebars_token.h"
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
        return NULL;
    }

    context->e = handlebars_talloc_zero(context, struct handlebars_error);
    if( unlikely(context->e == NULL) ) {
        handlebars_talloc_free(context);
        errno = ENOMEM;
        context = NULL;
    }

    return context;
}

void handlebars_context_bind(struct handlebars_context * parent, struct handlebars_context * child)
{
    if( child->e ) {
        handlebars_throw(parent, HANDLEBARS_ERROR, "Error context already assigned");
    }
    child->e = parent->e;
}

void handlebars_context_dtor(struct handlebars_context * context)
{
    handlebars_talloc_free(context);
}

enum handlebars_error_type handlebars_error_num(struct handlebars_context * context)
{
    assert(context->e != NULL);
    return context->e->num;
}

struct handlebars_locinfo handlebars_error_loc(struct handlebars_context * context)
{
    assert(context->e != NULL);
    return context->e->loc;
}

const char * handlebars_error_msg(struct handlebars_context * context)
{
    assert(context->e != NULL);
    return context->e->msg;
}

char * handlebars_error_message(struct handlebars_context * context)
{
    char * errmsg;
    char errbuf[256];
    struct handlebars_error * e = context->e;

    assert(context != NULL);
    assert(context->e != NULL);

    if( e->msg == NULL ) {
        return NULL;
    }

    snprintf(errbuf, sizeof(errbuf), "%s on line %d, column %d",
             e->msg,
             e->loc.last_line,
             e->loc.last_column);

    errmsg = handlebars_talloc_strdup(context, errbuf);
    if( unlikely(errmsg == NULL) ) {
        // this might be a bad idea...
        return e->msg;
    }

    return errmsg;
}

char * handlebars_error_message_js(struct handlebars_context * context)
{
    char * errmsg;
    char errbuf[512];
    struct handlebars_error * e = context->e;

    assert(context != NULL);

    if( e->msg == NULL ) {
        return NULL;
    }

    // @todo check errno == HANDLEBARS_PARSEERR

    snprintf(errbuf, sizeof(errbuf), "Parse error on line %d, column %d : %s",
             e->loc.last_line,
             e->loc.last_column,
             e->msg);

    errmsg = handlebars_talloc_strdup(context, errbuf);
    if( unlikely(errmsg == NULL) ) {
        // this might be a bad idea...
        return e->msg;
    }

    return errmsg;
}

struct handlebars_parser * handlebars_parser_ctor(struct handlebars_context * ctx)
{
    int lexerr = 0;
    struct handlebars_parser * parser = MC(handlebars_talloc_zero(ctx, struct handlebars_parser));

    // Bind error context
    handlebars_context_bind(ctx, HBSCTX(parser));

    // Set the current context in a variable for yyalloc >.>
    _handlebars_parser_init_current = parser;

    // Initialize lexer
    // @todo set a destructor on the context object to deinit the lexer?
    lexerr = handlebars_yy_lex_init(&parser->scanner);
    if( unlikely(lexerr != 0) ) {
        handlebars_talloc_free(parser);
        handlebars_throw(CONTEXT, HANDLEBARS_NOMEM, "Lexer initialization failed");
    }

    // Steal the scanner just in case
    parser->scanner = talloc_steal(parser, parser->scanner);

    // Set the extra on the lexer
    handlebars_yy_set_extra(parser, parser->scanner);

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

#undef CONTEXT
#define CONTEXT HBSCTX(parser)

struct handlebars_token ** handlebars_lex(struct handlebars_parser * parser)
{
    YYSTYPE yylval_param;
    YYLTYPE yylloc_param;
    struct handlebars_error * e = HBSCTX(parser)->e;
    jmp_buf * prev = e->jmp;
    jmp_buf buf;
    YYSTYPE * lval;
    struct handlebars_token ** tokens;
    struct handlebars_token * token;
    size_t i = 0;

    // Save jump buffer
    if( !prev ) {
        e->jmp = &buf;
        if (setjmp(buf)) {
            goto done;
        }
    }

    // Prepare token list
    tokens = MC(talloc_array(parser, struct handlebars_token *, 32));
    
    // Run
    do {
        int token_int = handlebars_yy_lex(&yylval_param, &yylloc_param, parser->scanner);
        if( unlikely(token_int == END || token_int == INVALID) ) {
            break;
        }
        lval = handlebars_yy_get_lval(parser->scanner);
        
        // Make token object
        token = handlebars_token_ctor(HBSCTX(parser), token_int, lval->string);
        
        // Append
        tokens = talloc_realloc(parser, tokens, struct handlebars_token *, i + 2);
        tokens[i] = talloc_steal(tokens, token);
        tokens[i + 1] = NULL;
        i++;
    } while( 1 );

done:
    e->jmp = prev;
    return tokens;
}

bool handlebars_parse(struct handlebars_parser * parser)
{
    struct handlebars_error * e = HBSCTX(parser)->e;
    jmp_buf * prev = e->jmp;
    jmp_buf buf;

    // Save jump buffer
    if( !prev ) {
        if( handlebars_setjmp_ex(parser, &buf) ) {
            goto done;
        }
    }

    handlebars_yy_parse(parser);

done:
    e->jmp = prev;
    return parser->program != NULL;
}

static inline void _set_err(struct handlebars_context * context, enum handlebars_error_type num, struct handlebars_locinfo * loc, const char * msg, va_list ap)
{
    struct handlebars_error * e = context->e;
    e->num = num;
    e->msg = talloc_vasprintf(context, msg, ap);
    if( unlikely(e->msg == NULL) ) {
        e->num = HANDLEBARS_NOMEM;
        e->msg = HANDLEBARS_MEMCHECK_MSG;
    }
    if( loc ) {
        e->loc = *loc;
    } else {
        memset(&e->loc, 0, sizeof(e->loc));
    }
}

void handlebars_throw(struct handlebars_context * context, enum handlebars_error_type num, const char * msg, ...)
{
    struct handlebars_error * e = context->e;
    va_list ap;
    va_start(ap, msg);
    _set_err(context, num, NULL, msg, ap);
    va_end(ap);
    if( e->jmp ) {
        longjmp(*e->jmp, num);
    } else {
        fprintf(stderr, "Throw with invalid jmp_buf: %s\n", e->msg);
        abort();
    }
}

void handlebars_throw_ex(struct handlebars_context * context, enum handlebars_error_type num, struct handlebars_locinfo * loc, const char * msg, ...)
{
    struct handlebars_error * e = context->e;
    va_list ap;
    va_start(ap, msg);
    _set_err(context, num, loc, msg, ap);
    va_end(ap);
    if( e->jmp ) {
        longjmp(*e->jmp, num);
    } else {
        fprintf(stderr, "Throw with invalid jmp_buf: %s\n", e->msg);
        abort();
    }
}

struct handlebars_context * handlebars_get_context(void * ctx, const char * loc)
{
    struct handlebars_context * r = (
            talloc_get_type(ctx, struct handlebars_context) ?:
            (struct handlebars_context *) talloc_get_type(ctx, struct handlebars_parser) ?:
            (struct handlebars_context *) talloc_get_type(ctx, struct handlebars_compiler) ?:
            (struct handlebars_context *) talloc_get_type(ctx, struct handlebars_vm) ?:
            (struct handlebars_context *) talloc_get_type(ctx, struct handlebars_cache)
    );
    if( !r ) {
        fprintf(stderr, "Not a context at %s\n", loc);
        abort();
    }
    return r;
}
