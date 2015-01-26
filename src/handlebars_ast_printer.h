
#ifndef HANDLEBARS_AST_PRINTER_H
#define HANDLEBARS_AST_PRINTER_H

#ifdef	__cplusplus
extern "C" {
#endif

struct handlebars_ast;

char * handlebars_ast_print(struct handlebars_ast * ast, int flags);

#ifdef	__cplusplus
}
#endif

#endif
