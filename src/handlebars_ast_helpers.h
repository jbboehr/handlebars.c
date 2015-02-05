
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

int handlebars_ast_helper_check_block(struct handlebars_ast_node * ast_node, 
        struct handlebars_context * context, struct YYLTYPE * yylloc);

int handlebars_ast_helper_check_raw_block(struct handlebars_ast_node * ast_node, 
        struct handlebars_context * context, struct YYLTYPE * yylloc);

struct handlebars_ast_node * handlebars_ast_helper_prepare_block(
        struct handlebars_context * context, struct handlebars_ast_node * mustache,
        struct handlebars_ast_node * program, struct handlebars_ast_node * inverse,
        struct handlebars_ast_node * close, int inverted, struct YYLTYPE * yylloc);

struct handlebars_ast_node * handlebars_ast_helper_prepare_id(
        struct handlebars_context * context, struct handlebars_ast_list * list);

struct handlebars_ast_node * handlebars_ast_helper_prepare_raw_block(
        struct handlebars_context * context, struct handlebars_ast_node * mustache, 
        const char * content, const char * close, struct YYLTYPE * yylloc);

#ifdef	__cplusplus
}
#endif

#endif
