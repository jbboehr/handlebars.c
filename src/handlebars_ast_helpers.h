
/**
 * @file
 * @brief Helpers for manipulating the AST 
 */

#ifndef HANDLEBARS_AST_HELPERS_H
#define HANDLEBARS_AST_HELPERS_H

#include <stddef.h>

#ifdef	__cplusplus
extern "C" {
#endif

// Declarations
struct handlebars_context;
struct handlebars_ast_list;
struct handlebars_ast_node;
struct YYLTYPE;

/**
 * @brief Check that a block's open and close tags match. On failure,
 *        stores an error message in context->error and returns an error code.
 * 
 * @param[in] ast_node The block node to check
 * @param[in] context The handlebars context
 * @param[in] yylloc The parser location
 * @return Returns an integer from enum handlebars_error_type
 */
int handlebars_ast_helper_check_block(struct handlebars_ast_node * ast_node, 
        struct handlebars_context * context, struct YYLTYPE * yylloc);

/**
 * @brief Check that a raw block's open and close tags match. On failure,
 *        stores an error message in context->error and returns an error code.
 * 
 * @param[in] ast_node The block node to check
 * @param[in] context The handlebars context
 * @param[in] yylloc The parser location
 * @return Returns an integer from enum handlebars_error_type
 */
int handlebars_ast_helper_check_raw_block(struct handlebars_ast_node * ast_node, 
        struct handlebars_context * context, struct YYLTYPE * yylloc);

/**
 * @brief Prepare a block node.
 * 
 * @param[in] context The handlebars context
 * @param[in] mustache A mustache AST node
 * @param[in] program A program AST node
 * @param[in] inverse A program AST node
 * @param[in] close A mustache AST node
 * @param[in] inverted If it's an inverse block
 * @param[in] yylloc The parser location
 * @return A newly constructed raw block AST node
 */
struct handlebars_ast_node * handlebars_ast_helper_prepare_block(
        struct handlebars_context * context, struct handlebars_ast_node * mustache,
        struct handlebars_ast_node * program, struct handlebars_ast_node * inverse,
        struct handlebars_ast_node * close, int inverted, struct YYLTYPE * yylloc);

/**
 * @brief Prepare an ID node.
 * 
 * @param[in] context The handlebars context
 * @param[in] list The path segment list
 * @return A newly constructed ID AST node
 */
struct handlebars_ast_node * handlebars_ast_helper_prepare_id(
        struct handlebars_context * context, struct handlebars_ast_list * list);

/**
 * @brief Prepare a block node.
 * 
 * @param[in] context The handlebars context
 * @param[in] mustache A mustache AST node
 * @param[in] content The content of the raw block node
 * @param[in] close The text of the closing tag
 * @param[in] yylloc The parser location
 * @return A newly constructed block AST node
 */
struct handlebars_ast_node * handlebars_ast_helper_prepare_raw_block(
        struct handlebars_context * context, struct handlebars_ast_node * mustache, 
        const char * content, const char * close, struct YYLTYPE * yylloc);

#ifdef	__cplusplus
}
#endif

#endif
