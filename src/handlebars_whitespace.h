
/**
 * @file
 * @brief Whitespace control
 */

#ifndef HANDLEBARS_WHITESPACE_H
#define HANDLEBARS_WHITESPACE_H

#include <stddef.h>

#ifdef	__cplusplus
extern "C" {
#endif

// Declarations
struct handlebars_context;
struct handlebars_ast_list;
struct handlebars_ast_node;
struct handlebars_locinfo;

int handlebars_whitespace_is_next_whitespace(struct handlebars_ast_list * statements,
        struct handlebars_ast_node * statement, short is_root);

int handlebars_whitespace_is_prev_whitespace(struct handlebars_ast_list * statements,
        struct handlebars_ast_node * statement, short is_root);

int handlebars_whitespace_omit_left(struct handlebars_ast_list * statements,
        struct handlebars_ast_node * statement, short multiple);

int handlebars_whitespace_omit_right(struct handlebars_ast_list * statements,
        struct handlebars_ast_node * statement, short multiple);

void handlebars_whitespace_accept(struct handlebars_context * context,
        struct handlebars_ast_node * node);

#ifdef	__cplusplus
}
#endif

#endif
