/**
 * Copyright (c) anno Domini nostri Jesu Christi MMXVI-MMXXIV John Boehr & contributors
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>

#include "handlebars.h"
#include "handlebars_memory.h"
#include "handlebars_parser.h"
#include "handlebars_private.h"
#include "handlebars_string.h"
#include "handlebars_token.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-Wredundant-decls"
#include "handlebars_parser_private.h"
#include "handlebars.tab.h"
#include "handlebars.lex.h"
#pragma GCC diagnostic pop



const size_t HANDLEBARS_PARSER_SIZE = sizeof(struct handlebars_parser);

struct handlebars_parser_ctor_state {
    struct handlebars_parser * parser;
};

struct handlebars_parse_try_state {
    struct handlebars_parser_ctor_state worker;
    struct handlebars_ast_node * ast;
};

static void handlebars_parser_ctor_init(
    struct handlebars_context * context,
    struct handlebars_parser_ctor_state * state
)
{
    int lexerr = 0;
    struct handlebars_parser * parser = handlebars_talloc_zero(
        context,
        struct handlebars_parser
    );

    HANDLEBARS_MEMCHECK(parser, context);
    state->parser = parser;

    // Bind error context
    handlebars_context_bind(context, HBSCTX(parser));

    // Set the current context in a variable for yyalloc >.>
    handlebars_parser_init_current = parser;

    // Initialize lexer
    lexerr = handlebars_yy_lex_init(&parser->scanner);
    if( unlikely(lexerr != 0) ) {
        handlebars_parser_init_current = NULL;
        handlebars_talloc_free(parser);
        state->parser = NULL;
        handlebars_throw(context, HANDLEBARS_NOMEM, "Lexer initialization failed");
    }

    // Steal the scanner just in case
    parser->scanner = talloc_steal(parser, parser->scanner);

    // Set the extra on the lexer
    handlebars_yy_set_extra(parser, parser->scanner);

    handlebars_parser_init_current = NULL;
}

struct handlebars_parser * handlebars_parser_ctor(struct handlebars_context * ctx)
{
    struct handlebars_parser_ctor_state state = {0};

    handlebars_parser_ctor_init(ctx, &state);
    return state.parser;
}

HBS_ATTR_NOINLINE
static enum handlebars_error_type handlebars_parser_ctor_try_guarded(
    struct handlebars_context * context,
    struct handlebars_parser_ctor_state * state
) {
    struct handlebars_error * error = context->e;
    jmp_buf * volatile previous = error->jmp;
    enum handlebars_error_type volatile caught = HANDLEBARS_SUCCESS;
    jmp_buf buf;

    if( handlebars_setjmp_ex(context, &buf) ) {
        caught = error->num;
        handlebars_parser_init_current = NULL;
        if( state->parser != NULL ) {
            handlebars_parser_dtor(state->parser);
            state->parser = NULL;
        }
    } else {
        handlebars_parser_ctor_init(context, state);
    }

    error->jmp = previous;
    return caught;
}

enum handlebars_error_type handlebars_parser_ctor_try(
    struct handlebars_context * context,
    struct handlebars_parser ** result
) {
    struct handlebars_parser_ctor_state state = {0};
    enum handlebars_error_type error;

    *result = NULL;
    handlebars_error_clear(context);
    error = handlebars_parser_ctor_try_guarded(context, &state);
    if( error == HANDLEBARS_SUCCESS ) {
        *result = state.parser;
    }
    return error;
}

void handlebars_parser_dtor(struct handlebars_parser * parser)
{
    if( likely(parser->scanner != NULL) ) {
        // Note: it has int return value, but appears to always return 0
        handlebars_yy_lex_destroy(parser->scanner);
        parser->scanner = NULL;
    }
    handlebars_talloc_free(parser);
}

#undef CONTEXT
#define CONTEXT HBSCTX(parser)

static void handlebars_parser_validate_template(
    struct handlebars_parser * parser,
    struct handlebars_string * tmpl
) {
    if( unlikely(memchr(hbs_str_val(tmpl), '\0', hbs_str_len(tmpl)) != NULL) ) {
        handlebars_throw(CONTEXT, HANDLEBARS_PARSEERR, "Template contains an embedded NUL byte");
    }
}

struct handlebars_token ** handlebars_lex_ex(
    struct handlebars_parser * parser,
    struct handlebars_string * tmpl
) {
    parser->tmpl = tmpl;
    return handlebars_lex(parser);
}

struct handlebars_token ** handlebars_lex(struct handlebars_parser * parser)
{
    struct handlebars_error * e = HBSCTX(parser)->e;
    jmp_buf * prev = e->jmp;
    jmp_buf buf;

    // Save jump buffer
    if( !prev ) {
        e->jmp = &buf;
        if (setjmp(buf)) {
            e->jmp = prev;
            return NULL;
        }
    }

    handlebars_yy_reset(parser);
    handlebars_parser_validate_template(parser, parser->tmpl);

    YYSTYPE yylval_param;
    YYLTYPE yylloc_param;
    YYSTYPE * lval;
    struct handlebars_token ** tokens;
    struct handlebars_token * token;
    size_t i = 0;

    // Prepare token list
    tokens = MC(handlebars_talloc_array(parser, struct handlebars_token *, 32));
    HANDLEBARS_MEMCHECK(tokens, HBSCTX(parser));

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
        tokens = handlebars_talloc_realloc(parser, tokens, struct handlebars_token *, i + 2);
        HANDLEBARS_MEMCHECK(tokens, HBSCTX(parser));
        tokens[i] = talloc_steal(tokens, token);
        i++;
    } while( 1 );

    tokens[i] = NULL;

    e->jmp = prev;
    return tokens;
}

struct handlebars_ast_node * handlebars_parse_ex(struct handlebars_parser * parser, struct handlebars_string * tmpl, unsigned flags)
{
    struct handlebars_error * e = HBSCTX(parser)->e;
    jmp_buf * volatile prev = e->jmp;
    jmp_buf buf;

    if( unlikely(parser->bison_stack != NULL) ) {
        handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "Parser is already in use");
    }

    // Always install a local jump buffer so the Bison stack can be released
    // before propagating an error to the caller's jump buffer.
    if( handlebars_setjmp_ex(parser, &buf) ) {
        if( parser->bison_stack ) {
            handlebars_talloc_free(parser->bison_stack);
            parser->bison_stack = NULL;
        }
        e->jmp = prev;
        if( prev ) {
            handlebars_longjmp(HBSCTX(parser), prev, e->num);
        }
        return NULL;
    }

    assert(parser->bison_stack == NULL);
    parser->bison_stack = handlebars_talloc_size(parser, 0);
    HANDLEBARS_MEMCHECK(parser->bison_stack, HBSCTX(parser));

    parser->program = NULL;
    parser->whitespace_root_seen = false;
    handlebars_yy_reset(parser);
    handlebars_parser_validate_template(parser, tmpl);

    parser->tmpl = tmpl;
    parser->flags = flags;

    handlebars_yy_parse(parser);

    handlebars_talloc_free(parser->bison_stack);
    parser->bison_stack = NULL;
    e->jmp = prev;
    return parser->program;
}

HBS_ATTR_NOINLINE
static enum handlebars_error_type handlebars_parse_try_guarded(
    struct handlebars_parser * parser,
    struct handlebars_string * tmpl,
    unsigned flags,
    struct handlebars_parse_try_state * state
) {
    struct handlebars_error * error = HBSCTX(parser)->e;
    jmp_buf * volatile previous = error->jmp;
    enum handlebars_error_type volatile caught = HANDLEBARS_SUCCESS;
    jmp_buf buf;

    if( handlebars_setjmp_ex(parser, &buf) ) {
        caught = error->num;
        goto done;
    }

    if( unlikely(parser->bison_stack != NULL) ) {
        handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "Parser is already in use");
    }

    // Parse in a temporary parser so a failed grammar production cannot leave
    // partially connected AST nodes owned by the reusable caller's parser.
    handlebars_parser_ctor_init(HBSCTX(parser), &state->worker);
    state->ast = handlebars_parse_ex(state->worker.parser, tmpl, flags);
    state->ast = talloc_steal(parser, state->ast);

    parser->tmpl = tmpl;
    parser->flags = flags;
    parser->program = state->ast;
    parser->whitespace_root_seen = state->worker.parser->whitespace_root_seen;

done:
    if( state->worker.parser != NULL ) {
        handlebars_parser_dtor(state->worker.parser);
        state->worker.parser = NULL;
    }
    error->jmp = previous;
    return caught;
}

enum handlebars_error_type handlebars_parse_try(
    struct handlebars_parser * parser,
    struct handlebars_string * tmpl,
    unsigned flags,
    struct handlebars_ast_node ** result
) {
    struct handlebars_parse_try_state state = {0};
    enum handlebars_error_type error;

    *result = NULL;
    handlebars_error_clear(HBSCTX(parser));
    error = handlebars_parse_try_guarded(parser, tmpl, flags, &state);
    if( error == HANDLEBARS_SUCCESS ) {
        *result = state.ast;
    }
    return error;
}

bool handlebars_parse(struct handlebars_parser * parser)
{
    return handlebars_parse_ex(parser, parser->tmpl, parser->flags) != NULL;
}
