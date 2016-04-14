
/**
 * @file
 * @brief Functions for printing an AST into a human-readable string
 */

#ifndef HANDLEBARS_AST_PRINTER_H
#define HANDLEBARS_AST_PRINTER_H

#include "handlebars.h"

#ifdef	__cplusplus
extern "C" {
#endif

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
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL;

#ifdef	__cplusplus
}
#endif

#endif
