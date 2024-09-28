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

/**
 * @file
 * @brief Whitespace control
 */

#ifndef HANDLEBARS_WHITESPACE_H
#define HANDLEBARS_WHITESPACE_H

#include "handlebars.h"

HBS_EXTERN_C_START

// Declarations
struct handlebars_ast_list;
struct handlebars_ast_node;
struct handlebars_locinfo;
struct handlebars_parser;

bool handlebars_whitespace_is_next_whitespace(
    struct handlebars_ast_list * statements,
    struct handlebars_ast_node * statement,
    bool is_root
) HBS_LOCAL;

bool handlebars_whitespace_is_prev_whitespace(
    struct handlebars_ast_list * statements,
    struct handlebars_ast_node * statement,
    bool is_root
) HBS_LOCAL;

bool handlebars_whitespace_omit_left(
    struct handlebars_ast_list * statements,
    struct handlebars_ast_node * statement,
    bool multiple
) HBS_LOCAL HBS_ATTR_NONNULL(1);

bool handlebars_whitespace_omit_right(
    struct handlebars_ast_list * statements,
    struct handlebars_ast_node * statement,
    bool multiple
) HBS_LOCAL HBS_ATTR_NONNULL(1);

void handlebars_whitespace_accept(
    struct handlebars_parser * parser,
    struct handlebars_ast_node * node
) HBS_LOCAL HBS_ATTR_NONNULL(1);

HBS_EXTERN_C_END

#endif /* HANDLEBARS_WHITESPACE_H */
