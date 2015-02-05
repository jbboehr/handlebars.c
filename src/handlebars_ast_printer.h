
#ifndef HANDLEBARS_AST_PRINTER_H
#define HANDLEBARS_AST_PRINTER_H

#ifdef	__cplusplus
extern "C" {
#endif

// Declarations
struct handlebars_ast;

enum handlebars_ast_printer_flags {
  HANDLEBARS_AST_PRINTER_FLAGS_NONE = 0,
  HANDLEBARS_AST_PRINTER_FLAGS_COMPAT = 1
};

struct handlebars_ast_printer_context {
  int flags;
  int padding;
  int error;
  char * output;
  size_t length;
  long counter;
  int in_partial;
};

char * handlebars_ast_print(struct handlebars_ast_node * ast_node, int flags);
struct handlebars_ast_printer_context handlebars_ast_print2(struct handlebars_ast_node * ast_node, int flags);

#ifdef	__cplusplus
}
#endif

#endif
