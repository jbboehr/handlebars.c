
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

// Declarations
struct handlebars_ast;
struct handlebars_context;

/**
 * @brief Flags to control printer behaviour. Note: not currently used
 */
enum handlebars_ast_printer_flags
{
  /**
   * @brief Default printer behaviour 
   */
  HANDLEBARS_AST_PRINTER_FLAGS_NONE = 0,

  /**
   * @brief handlebars.js compatible printer behaviour
   */
  HANDLEBARS_AST_PRINTER_FLAGS_COMPAT = 1
};

/**
 * @brief AST printer context object
 */
struct handlebars_ast_printer_context {
    struct handlebars_context * ctx;
    int padding;
    int error;
    struct handlebars_string * output;
    bool in_partial;
};

/**
 * @brief Print an AST into a human-readable string.
 *
 * @param[in] context The handlebars context
 * @param[in] ast_node The AST to print
 * @param[in] flags The printer flags
 * @return The printed string
 */
struct handlebars_string * handlebars_ast_print(
    struct handlebars_context * context,
    struct handlebars_ast_node * ast_node
) HBS_ATTR_RETURNS_NONNULL;

#ifdef	__cplusplus
}
#endif

#endif
