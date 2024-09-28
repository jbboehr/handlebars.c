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
 * @brief Functions for printing an AST into a human-readable string
 */

#ifndef HANDLEBARS_AST_PRINTER_H
#define HANDLEBARS_AST_PRINTER_H

#include "handlebars.h"

HBS_EXTERN_C_START

struct handlebars_ast_node;
struct handlebars_context;

/**
 * @brief Print an AST into a human-readable string.
 *
 * @param[in] context The handlebars context
 * @param[in] ast_node The AST to print
 * @return The printed string
 */
struct handlebars_string * handlebars_ast_print(
    struct handlebars_context * context,
    struct handlebars_ast_node * ast_node
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

struct handlebars_string * handlebars_ast_to_string(
    struct handlebars_context * context,
    struct handlebars_ast_node * ast_node
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

HBS_EXTERN_C_END /* HANDLEBARS_AST_PRINTER_H */

#endif
