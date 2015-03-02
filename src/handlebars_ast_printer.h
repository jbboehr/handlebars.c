
/**
 * @file
 * @brief Functions for printing an AST into a human-readable string
 */

#ifndef HANDLEBARS_AST_PRINTER_H
#define HANDLEBARS_AST_PRINTER_H

#ifdef	__cplusplus
extern "C" {
#endif

// Declarations
struct handlebars_ast;

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
  int flags;
  int padding;
  int error;
  char * output;
  size_t length;
  long counter;
  int in_partial;
};

/**
 * @brief Print an AST into a human-readable string.
 * 
 * @param[in] ast_node The AST to print
 * @param[in] flags The printer flags
 * @return The printed string
 */
char * handlebars_ast_print(struct handlebars_ast_node * ast_node, int flags);

/**
 * @brief Print an AST into a human-readable string.
 * 
 * @param[in] ast_node The AST to print
 * @param[in] flags The printer flags
 * @return The printer context
 */
struct handlebars_ast_printer_context handlebars_ast_print2(struct handlebars_ast_node * ast_node, int flags);

#ifdef	__cplusplus
}
#endif

#endif
