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

#include "handlebars.h"
#include "handlebars_memory.h"
#include "handlebars_parser.h"
#include "handlebars_private.h"
#include "handlebars_token.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-Wredundant-decls"
#include "handlebars_parser_private.h"
#include "handlebars.tab.h"
#include "handlebars.lex.h"
#pragma GCC diagnostic pop



const size_t HANDLEBARS_PARSER_SIZE = sizeof(struct handlebars_parser);

struct handlebars_parser * handlebars_parser_ctor(struct handlebars_context * ctx)
{
    int lexerr = 0;
    struct handlebars_parser * parser = handlebars_talloc_zero(ctx, struct handlebars_parser);
    HANDLEBARS_MEMCHECK(parser, ctx);

    // Bind error context
    handlebars_context_bind(ctx, HBSCTX(parser));

    // Set the current context in a variable for yyalloc >.>
    handlebars_parser_init_current = parser;

    // Initialize lexer
    lexerr = handlebars_yy_lex_init(&parser->scanner);
    if( unlikely(lexerr != 0) ) {
        handlebars_parser_init_current = NULL;
        handlebars_talloc_free(parser);
        handlebars_throw(ctx, HANDLEBARS_NOMEM, "Lexer initialization failed");
    }

    // Steal the scanner just in case
    parser->scanner = talloc_steal(parser, parser->scanner);

    // Set the extra on the lexer
    handlebars_yy_set_extra(parser, parser->scanner);

    handlebars_parser_init_current = NULL;
    return parser;
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
    jmp_buf * prev = e->jmp;
    jmp_buf buf;

    // Save jump buffer
    if( !prev ) {
        if( handlebars_setjmp_ex(parser, &buf) ) {
            e->jmp = prev;
            return NULL;
        }
    }

    parser->tmpl = tmpl;
    parser->flags = flags;

    handlebars_yy_parse(parser);

    e->jmp = prev;
    return parser->program;
}

bool handlebars_parse(struct handlebars_parser * parser)
{
    return handlebars_parse_ex(parser, parser->tmpl, parser->flags) != NULL;
}
