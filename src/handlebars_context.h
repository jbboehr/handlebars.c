
/**
 * @file
 * @brief Context to hold parser and lexer state
 */

#ifndef HANDLEBARS_CONTEXT_H
#define HANDLEBARS_CONTEXT_H

#include <setjmp.h>
#include "handlebars.h"

#ifdef	__cplusplus
extern "C" {
#endif

// Declarations
struct handlebars_ast_node;
struct handlebars_locinfo;

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
    struct {
        long num;
        char * msg;
        struct handlebars_locinfo loc;
        jmp_buf jmp;
        bool ok;
    } e;

    const char * tmpl;
    int tmplReadOffset;
    void * scanner;
    struct handlebars_ast_node * program;
    bool whitespace_root_seen;
    bool ignore_standalone;
};

/**
 * @brief Construct a context object. Used as the root talloc pointer.
 * 
 * @return the context pointer
 */
struct handlebars_context * handlebars_context_ctor_ex(void * ctx);
#define handlebars_context_ctor() handlebars_context_ctor_ex(NULL)

/**
 * @brief Free a context and it's resources.
 * 
 * @param[in] context
 * @return void
 */
void handlebars_context_dtor(struct handlebars_context * context);

//#define handlebars_context_throw(context, errnum, errmsg, ...) handlebars_context_throw_ex(context, errnum, NULL, errmsg, __VA_ARGS__)
void handlebars_context_throw(struct handlebars_context * context, enum handlebars_error_type num, const char * msg, ...) HBS_NORETURN;
void handlebars_context_throw_ex(struct handlebars_context * context, enum handlebars_error_type num, struct handlebars_locinfo * loc, const char * msg, ...) HBS_NORETURN;

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
