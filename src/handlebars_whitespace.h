
/**
 * @file
 * @brief Whitespace control
 */

#ifndef HANDLEBARS_WHITESPACE_H
#define HANDLEBARS_WHITESPACE_H

#include "handlebars.h"

#ifdef	__cplusplus
extern "C" {
#endif

// Declarations
struct handlebars_ast_list;
struct handlebars_ast_node;
struct handlebars_locinfo;
struct handlebars_parser;

bool handlebars_whitespace_is_next_whitespace(
    struct handlebars_ast_list * statements,
    struct handlebars_ast_node * statement,
    bool is_root
) HBS_ATTR_NONNULL_ALL;

bool handlebars_whitespace_is_prev_whitespace(
    struct handlebars_ast_list * statements,
    struct handlebars_ast_node * statement,
    bool is_root
) HBS_ATTR_NONNULL_ALL;

bool handlebars_whitespace_omit_left(
    struct handlebars_ast_list * statements,
    struct handlebars_ast_node * statement,
    bool multiple
) HBS_ATTR_NONNULL_ALL;

bool handlebars_whitespace_omit_right(
    struct handlebars_ast_list * statements,
    struct handlebars_ast_node * statement,
    bool multiple
) HBS_ATTR_NONNULL_ALL;

void handlebars_whitespace_accept(
    struct handlebars_parser * parser,
    struct handlebars_ast_node * node
) HBS_ATTR_NONNULL_ALL;

#ifdef	__cplusplus
}
#endif

#endif
