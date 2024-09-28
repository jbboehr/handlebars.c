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

#ifndef HANDLEBARS_PARSER_H
#define HANDLEBARS_PARSER_H

#include "handlebars.h"

HBS_EXTERN_C_START

struct handlebars_ast_node;
struct handlebars_string;

extern const size_t HANDLEBARS_PARSER_SIZE;

/**
 * @brief Construct a parser
 * @param[in] ctx The parent handlebars and talloc context
 * @return the parser pointer
 */
struct handlebars_parser * handlebars_parser_ctor(
    struct handlebars_context * ctx
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

/**
 * @brief Free a parser and it's resources.
 * @param[in] parser The parser to free
 * @return void
 */
void handlebars_parser_dtor(
    struct handlebars_parser * parser
) HBS_ATTR_NONNULL_ALL;

struct handlebars_token ** handlebars_lex_ex(
    struct handlebars_parser * parser,
    struct handlebars_string * tmpl
) HBS_ATTR_NONNULL_ALL HBS_ATTR_WARN_UNUSED_RESULT;

/**
 * @brief Convenience function for lexing to a token list
 * @param[in] parser The parser
 * @return the token list
 */
struct handlebars_token ** handlebars_lex(
    struct handlebars_parser * parser
) HBS_ATTR_NONNULL_ALL HBS_ATTR_WARN_UNUSED_RESULT;

struct handlebars_ast_node * handlebars_parse_ex(
    struct handlebars_parser * parser,
    struct handlebars_string * tmpl,
    unsigned flags
) HBS_ATTR_NONNULL_ALL HBS_ATTR_WARN_UNUSED_RESULT;

/**
 * @brief Parser a template. The template is stored in handlebars_parser#tmpl and the resultant
 *        AST is stored in handlebars_parser#program
 * @param[in] parser The parser
 * @return true on success
 */
bool handlebars_parse(
    struct handlebars_parser * parser
) HBS_ATTR_NONNULL_ALL;

#endif /* HANDLEBARS_PARSER_H */
