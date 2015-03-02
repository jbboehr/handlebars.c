
/**
 * @file
 * @brief Context to hold parser and lexer state
 */

#ifndef HANDLEBARS_CONTEXT_H
#define HANDLEBARS_CONTEXT_H

#ifdef	__cplusplus
extern "C" {
#endif

// Declarations
struct handlebars_ast_node;
struct YYLTYPE;

/**
 * @brief Stores the context that is currently being allocated. Used for
 *        handlebars_yy_alloc during the initial allocation.
 */
extern struct handlebars_context * _handlebars_context_init_current;

/**
 * @brief Contains the scanning state information
 */
struct handlebars_context
{
  char * tmpl;
  int tmplReadOffset;
  void * scanner;
  struct handlebars_ast_node * program;
  int eof;
  int errnum;
  char * error;
  struct YYLTYPE * errloc;
};

/**
 * @brief Construct a context object. Used as the root talloc pointer.
 * 
 * @return the context pointer
 */
struct handlebars_context * handlebars_context_ctor(void);

/**
 * @brief Free a context and it's resources.
 * 
 * @param[in] context
 * @return void
 */
void handlebars_context_dtor(struct handlebars_context * context);

/**
 * @brief Get the error message from a context, or NULL.
 * 
 * @param[in] context
 * @return the error message
 */
char * handlebars_context_get_errmsg(struct handlebars_context * context);

/**
 * @brief Get the error message from a context, or NULL (compatibility for handlebars specification)
 * 
 * @param[in] context
 * @return the error message
 */
char * handlebars_context_get_errmsg_js(struct handlebars_context * context);

#ifdef	__cplusplus
}
#endif

#endif
