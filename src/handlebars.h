/**
 * Copyright (C) 2016 John Boehr
 *
 * This file is part of handlebars.c.
 *
 * handlebars.c is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * handlebars.c is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with handlebars.c.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file
 * @brief General 
 */

#ifndef HANDLEBARS_H
#define HANDLEBARS_H

#include "handlebars_config.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>

#ifdef	__cplusplus
extern "C" {
#endif

// Declarations
struct handlebars_ast_node;
struct handlebars_string;

// Attributes
#if (__GNUC__ >= 3)
#define HBS_ATTR_NORETURN __attribute__ ((noreturn))
#define HBS_ATTR_PRINTF(a1, a2) __attribute__ ((format (__printf__, a1, a2)))
#define HBS_ATTR_UNUSED  __attribute__((__unused__))
#define HBS_ATTR_NONNULL(...) __attribute__((nonnull (__VA_ARGS__)))
#define HBS_ATTR_NONNULL_ALL __attribute__((nonnull))
#else
#define HBS_ATTR_NORETURN
#define HBS_ATTR_PRINTF(a1, a2)
#define HBS_ATTR_UNUSED
#define HBS_ATTR_NONNULL
#define HBS_ATTR_NONNULL_ALL
#endif

// returns_nonnull
#if (__GNUC__ >= 5) || ((__GNUC__ >= 4) && (__GNUC_MINOR__ >= 9))
#define HBS_ATTR_RETURNS_NONNULL  __attribute__((returns_nonnull))
#else
#define HBS_ATTR_RETURNS_NONNULL
#endif

// typeof
#if (__GNUC__ >= 3)
#define HBS_TYPEOF(ptr) __typeof__(ptr)
#else
#define HBS_TYPEOF(ptr) void *
#endif

// builtin_expect
#ifdef HAVE___BUILTIN_EXPECT
#  define handlebars_likely(x)   __builtin_expect(!!(x), 1)
#  define handlebars_unlikely(x) __builtin_expect(!!(x), 0)
#else
#  define handlebars_likely(x) (x)
#  define handlebars_unlikely(x) (x)
#endif

// unused
#ifdef HAVE_VAR_ATTRIBUTE_UNUSED
#  define HANDLEBARS_ATTR_UNUSED __attribute__((__unused__))
#else
#  define HANDLEBARS_ATTR_UNUSED
#endif

// Macros
#define HBS_STRL(str) str, sizeof(str) - 1
#define HBS_S1(x) #x
#define HBS_S2(x) HBS_S1(x)
#define HBS_LOC HBS_S2(__FILE__) ":" HBS_S2(__LINE__)

//! Message macro for out-of-memory errors
#define HANDLEBARS_MEMCHECK_MSG "Out of memory  [" HBS_LOC "]"

//! Check if an allocation failed
#define HANDLEBARS_MEMCHECK(cond, ctx) \
    do { \
        if( handlebars_unlikely(!cond) ) { \
            handlebars_throw(ctx, HANDLEBARS_NOMEM, HANDLEBARS_MEMCHECK_MSG); \
        } \
    } while(0)
#define HBS_MEMCHK(cond) MEMCHKEX(cond, CONTEXT)

#define handlebars_setjmp(ctx) setjmp(*(HBSCTX(ctx)->e->jmp))
#define handlebars_setjmp_cp(ctx, ctx2) setjmp(*(HBSCTX(ctx)->e->jmp = ctx2->jmp))
#define handlebars_setjmp_ex(ctx, buf) setjmp(*(HBSCTX(ctx)->e->jmp = (buf)))
#define handlebars_rethrow(parent, child) handlebars_throw_ex(parent, child->e->num, &child->e->loc, "%s", child->e->msg);

/**
 * @brief Enumeration of error types
 */
enum handlebars_error_type
{
    //! Indicates that no error has occurred
    HANDLEBARS_SUCCESS = 0,

    //! Indicates that a generic error has occurred
    HANDLEBARS_ERROR = 1,

    //! Indicates that failure was due to an allocation failure
    HANDLEBARS_NOMEM = 2,

    //! Indicates that failure was due to a null pointer argument
    HANDLEBARS_NULLARG = 3,

    //! Indicates that failure was due to a parse error
    HANDLEBARS_PARSEERR = 4,

    //! The compiler encountered an unknown helper in known helpers only mode
    HANDLEBARS_UNKNOWN_HELPER = 5,

    //! An unsupported number of partial arguments were given
    HANDLEBARS_UNSUPPORTED_PARTIAL_ARGS = 6,

    //! A stack was overflown
    HANDLEBARS_STACK_OVERFLOW = 7,

    //! A helper caused an error or exception
    HANDELBARS_EXTERNAL_ERROR = 8
};

/**
 * @brief Location type. Stores information about the location of an error
 */
struct handlebars_locinfo
{
    int first_line;
    int first_column;
    int last_line;
    int last_column;
};

struct handlebars_error
{
    //! The type of error that has occurred
    enum handlebars_error_type num;

    //! The message of the error that has occurred
    char * msg;

    //! The location of the error that has occurred
    struct handlebars_locinfo loc;

    //! On an unrecoverable error, if `jmp` is non-null, `longjmp` will be called with it as an argument,
    //! otherwise `abort`
    jmp_buf * jmp;
};

/**
 * @brief Common structure header, used to store error info and a `jmp_buf`
 */
struct handlebars_context
{
    struct handlebars_error * e;
};

/**
 * @brief Structure for parsing or lexing a template
 */
struct handlebars_parser
{
    //! The internal context
    struct handlebars_context ctx;

    //! The template to parse
    struct handlebars_string * tmpl;

    int tmplReadOffset;
    void * scanner;
    struct handlebars_ast_node * program;
    bool whitespace_root_seen;
    bool ignore_standalone;
};

/**
 * @brief Get the library version as an integer
 * @return The version of handlebars as an integer
 */
int handlebars_version(void);

/**
 * @brief Get the library version as a string
 * @return The version of handlebars as a string
 */
const char * handlebars_version_string(void) HBS_ATTR_RETURNS_NONNULL;

/**
 * @brief Get the compatible handlebars spec version as a string
 * @return The version of the handlebars spec as a string
 */
const char * handlebars_spec_version_string(void) HBS_ATTR_RETURNS_NONNULL;

/**
 * @brief Get the compatible mustache spec version as a string
 * @return The version of the mustache spec as a string
 */
const char * handlebars_mustache_spec_version_string(void) HBS_ATTR_RETURNS_NONNULL;

/**
 * @brief Construct a context object. Used as the root talloc pointer.
 * @param[in] ctx The talloc memory context
 * @return the context pointer
 */
struct handlebars_context * handlebars_context_ctor_ex(void * ctx);

/**
 * @brief Construct a root context object. Used as the root talloc pointer.
 * @return the context pointer
 */
static inline struct handlebars_context * handlebars_context_ctor(void) {
    return handlebars_context_ctor_ex(NULL);
}

void handlebars_context_bind(struct handlebars_context * parent, struct handlebars_context * child) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Free a context and it's resources.
 * @param[in] context The parent handlebars and talloc context
 * @return void
 */
void handlebars_context_dtor(struct handlebars_context * context) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Construct a parser
 * @param[in] ctx The parent handlebars and talloc context
 * @return the parser pointer
 */
struct handlebars_parser * handlebars_parser_ctor(
    struct handlebars_context * ctx
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL;

/**
 * @brief Free a parser and it's resources.
 * @param[in] parser The parser to free
 * @return void
 */
void handlebars_parser_dtor(struct handlebars_parser * parser) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Convenience function for lexing to a token list
 * @param[in] parser The parser
 * @return the token list
 */
struct handlebars_token ** handlebars_lex(
    struct handlebars_parser * parser
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL;

/**
 * @brief Parser a template. The template is stored in handlebars_parser#tmpl and the resultant
 *        AST is stored in handlebars_parser#program
 * @param[in] parser The parser
 * @return true on success
 */
bool handlebars_parse(struct handlebars_parser * parser) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Throw an error. If the context has a `jmp_buf`, `longjmp(context->jmp, num)` will be called, otherwise
 *        `abort()` will be called. This function does not return.
 * @param[in] context The handlebars context
 * @param[in] num The error code from the `handlebars_error_type`
 * @param[in] msg The error message
 */
void handlebars_throw(
    struct handlebars_context * context,
    enum handlebars_error_type num,
    const char * msg,
    ...
) HBS_ATTR_NONNULL_ALL HBS_ATTR_PRINTF(3, 4) HBS_ATTR_NORETURN;

/**
 * @brief Throw an error. If the context has a `jmp_buf`, `longjmp(context->jmp, num)` will be called, otherwise
 *        `abort()` will be called. This function does not return. Includes a location in the executing template.
 * @param[in] context The handlebars context
 * @param[in] num The error code from the `handlebars_error_type`
 * @param[in] loc The location of the error
 * @param[in] msg The error message
 */
void handlebars_throw_ex(
    struct handlebars_context * context,
    enum handlebars_error_type num,
    struct handlebars_locinfo * loc,
    const char * msg,
    ...
) HBS_ATTR_NONNULL_ALL HBS_ATTR_PRINTF(4, 5) HBS_ATTR_NORETURN;

/**
 * @brief Fetch the error code from enum #handlebars_error_type for the specified context
 * @param[in] context The handlebars context
 * @return The error code, or #HANDLEBARS_SUCCESS if no error
 */
enum handlebars_error_type handlebars_error_num(struct handlebars_context * context) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Get the error location for parse errors
 * @param[in] context The handlebars context
 * @return The error location
 */
struct handlebars_locinfo handlebars_error_loc(struct handlebars_context * context) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Get the error message for the specified context
 * @param[in] context The handlebars context
 * @return The error message, or NULL if no error has occurred.
 */
const char * handlebars_error_msg(struct handlebars_context * context) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Set the jump buffer for the specified context.
 * @see longjmp(3)
 * @param[in] context The handlebars context
 * @param[in] buf The jump buffer to use, or NULL to unset
 * @return The previous jump buffer, or NULL if none was set
 */
jmp_buf * handlebars_error_jmp(struct handlebars_context * context, jmp_buf * buf);

/**
 * @brief Get a copy the error message from a context, or NULL.
 * @param[in] context The handlebars context
 * @return The error message
 */
char * handlebars_error_message(struct handlebars_context * context) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Get a copy the error message from a context, or NULL (compatibility for handlebars specification)
 * @param[in] context
 * @return the error message
 */
char * handlebars_error_message_js(struct handlebars_context * context) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Used to check for memory allocation failure. If `ptr` is `NULL`, handlebars_throw() will be called.
 *        This function does not return if `ptr` is `NULL`.
 * @param[in] context The handlebars context
 * @param[in] ptr The pointer to check
 * @param[in] msg The error message (file location)
 * @return The value of `ptr`
 */
static inline void * handlebars_check(struct handlebars_context * context, void * ptr, const char * msg)
{
    if( handlebars_unlikely(ptr == NULL) ) {
        handlebars_throw(context, HANDLEBARS_NOMEM, "%s", msg);
    }
    return ptr;
}

/**
 * @brief Check if the specified pointer is a valid handlebars context using `talloc_get_type`. This function
 *        is used internally when NDEBUG is undefined. If the check fails, the program will abort.
 * @param[in] ctx
 * @param[in] loc
 * @return The context
 */
struct handlebars_context * handlebars_get_context(void * ctx, const char * loc);

#ifndef NDEBUG
#define HBSCTX(ctx) handlebars_get_context(ctx, HBS_S2(__FILE__) ":" HBS_S2(__LINE__))
#else
#define HBSCTX(ctx) ((struct handlebars_context *)ctx)
#endif

// Flex/Bison prototypes
int handlebars_yy_get_column(void * yyscanner);
void handlebars_yy_set_column(int column_no, void * yyscanner);
int handlebars_yy_parse(struct handlebars_parser * parser);

#ifdef	__cplusplus
}
#endif

#endif
