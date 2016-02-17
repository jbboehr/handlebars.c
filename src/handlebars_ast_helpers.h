
/**
 * @file
 * @brief Helpers for manipulating the AST 
 */

#ifndef HANDLEBARS_AST_HELPERS_H
#define HANDLEBARS_AST_HELPERS_H

#include "handlebars.h"

#ifdef	__cplusplus
extern "C" {
#endif

// Declarations
struct handlebars_parser;
struct handlebars_ast_list;
struct handlebars_ast_node;
struct handlebars_locinfo;
struct handlebars_yy_intermediate5;
struct handlebars_yy_inverse_and_program;

/**
 * @brief Prepare a block node.
 * 
 * @param[in] context The handlebars context
 * @param[in] mustache A mustache AST node
 * @param[in] program A program AST node
 * @param[in] inverse A program AST node
 * @param[in] close A mustache AST node
 * @param[in] inverted If it's an inverse block
 * @param[in] locinfo The parser location
 * @return A newly constructed raw block AST node
 */
struct handlebars_ast_node * handlebars_ast_helper_prepare_block(
    struct handlebars_parser * parser,
    struct handlebars_ast_node * open_block,
    struct handlebars_ast_node * program,
    struct handlebars_ast_node * inverse,
    struct handlebars_ast_node * close,
    int inverted,
    struct handlebars_locinfo * locinfo
) HBSARN;

struct handlebars_ast_node * handlebars_ast_helper_prepare_inverse_chain(
    struct handlebars_parser * parser,
    struct handlebars_ast_node * open_inverse_chain,
    struct handlebars_ast_node * program,
    struct handlebars_ast_node * inverse_chain,
    struct handlebars_locinfo * locinfo
) HBSARN;

struct handlebars_ast_node * handlebars_ast_helper_prepare_mustache(
    struct handlebars_parser * parser,
    struct handlebars_ast_node * intermediate,
    char * open,
    unsigned strip,
    struct handlebars_locinfo * locinfo
) HBSARN;

struct handlebars_ast_node * handlebars_ast_helper_prepare_partial_block(
    struct handlebars_parser * parser,
    struct handlebars_ast_node * open,
    struct handlebars_ast_node * program,
    struct handlebars_ast_node * close,
    struct handlebars_locinfo * locinfo
) HBSARN;

struct handlebars_ast_node * handlebars_ast_helper_prepare_path(
    struct handlebars_parser * parser,
    struct handlebars_ast_list * list,
    bool data,
    struct handlebars_locinfo * locinfo
) HBSARN;

/**
 * @brief Prepare a block node.
 * 
 * @param[in] context The handlebars context
 * @param[in] mustache A mustache AST node
 * @param[in] content The content of the raw block node
 * @param[in] close The text of the closing tag
 * @param[in] locinfo The parser location
 * @return A newly constructed block AST node
 */
struct handlebars_ast_node * handlebars_ast_helper_prepare_raw_block(
        struct handlebars_parser * parser,
        struct handlebars_ast_node * open_raw_block,
        const char * content,
        const char * close,
        struct handlebars_locinfo * locinfo
) HBSARN;

char * handlebars_ast_helper_strip_comment(char * comment) HBSARN;

char * handlebars_ast_helper_strip_id_literal(char * comment) HBSARN;

/**
 * @brief Set the strip flags of a node
 *
 * @param[in] ast_node The AST node
 * @param[in] open The open tag
 * @param[in] close The close tag
 * @return void
 */
void handlebars_ast_helper_set_strip_flags(
        struct handlebars_ast_node * ast_node, const char * open, const char * close);

/**
 * @brief Calculate the strip flags of a node
 *
 * @param[in] open The open tag
 * @param[in] close The close tag
 * @return unsigned
 */
unsigned handlebars_ast_helper_strip_flags(const char * open, const char * close);

bool handlebars_ast_helper_scoped_id(struct handlebars_ast_node * path);

bool handlebars_ast_helper_simple_id(struct handlebars_ast_node * path);

bool handlebars_ast_helper_helper_expression(struct handlebars_ast_node * node);

#ifdef	__cplusplus
}
#endif

#endif
