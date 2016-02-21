
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
struct handlebars_token_list;

// Attributes
#if (__GNUC__ >= 3)
#define HBS_ATTR_NORETURN __attribute__ ((noreturn))
#define HBS_ATTR_PRINTF(a1, a2) __attribute__ ((format (__printf__, a1, a2)))
#define HBS_ATTR_UNUSED  __attribute__((__unused__))
#define HBS_ATTR_NONNULL(...) __attribute__((nonnull (__VA_ARGS__)))
#else
#define HBS_ATTR_NORETURN
#define HBS_ATTR_PRINTF(a1, a2)
#define HBS_ATTR_UNUSED
#define HBS_ATTR_NONNULL
#endif

// returns_nonnull
#if (__GNUC__ >= 5) || ((__GNUC__ >= 4) && (__GNUC_MINOR__ >= 9))
#define HBS_ATTR_RETURNS_NONNULL  __attribute__((returns_nonnull))
#else
#define HBS_ATTR_RETURNS_NONNULL
#endif
#define HBSARN HBS_ATTR_RETURNS_NONNULL

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

#define HANDLEBARS_MEMCHECK_MSG "Out of memory  [" HBS_LOC "]"
#define HANDLEBARS_MEMCHECK(cond, ctx) \
    do { \
        if( handlebars_unlikely(!cond) ) { \
            handlebars_context_throw(ctx, HANDLEBARS_NOMEM, HANDLEBARS_MEMCHECK_MSG); \
        } \
    } while(0)
#define HBS_MEMCHK(cond) MEMCHKEX(cond, CONTEXT)

#define handlebars_setjmp(ctx) setjmp(*(HBSCTX(ctx)->jmp))
#define handlebars_setjmp_cp(ctx, ctx2) setjmp(*(HBSCTX(ctx)->jmp = ctx2->jmp))
#define handlebars_setjmp_ex(ctx, buf) setjmp(*(HBSCTX(ctx)->jmp = (buf)))

/**
 * @brief Enumeration of error types
 */
enum handlebars_error_type
{
    /**
     * @brief Indicates that no error has occurred
     */
    HANDLEBARS_SUCCESS = 0,
    
    /**
     * @brief Indicates that a generic error has occurred 
     */
    HANDLEBARS_ERROR = 1,
    
    /**
     * @brief Indicates that failure was due to an allocation failure
     */
    HANDLEBARS_NOMEM = 2,
    
    /**
     * @brief Indicates that failure was due to a null pointer argument
     */
    HANDLEBARS_NULLARG = 3,
    
    /**
     * @brief Indicates that failure was due to a parse error
     */
    HANDLEBARS_PARSEERR = 4,

    /**
     * @brief The compiler encountered an unknown helper in known helpers only mode
     */
    HANDLEBARS_UNKNOWN_HELPER = 5,

    /**
     * @brief An unsupported number of partial arguments were given
     */
    HANDLEBARS_UNSUPPORTED_PARTIAL_ARGS = 6,

    /**
     * @brief A stack was overflown
     */
    HANDLEBARS_STACK_OVERFLOW = 7
};

/**
 * @brief Location type
 */
struct handlebars_locinfo
{
    int first_line;
    int first_column;
    int last_line;
    int last_column;
};
#define YYLTYPE handlebars_locinfo

/**
 * @brief
 */
struct handlebars_context
{
    long num;
    char * msg;
    struct handlebars_locinfo loc;
    jmp_buf * jmp;
};

struct handlebars_parser
{
    struct handlebars_context ctx;

    const char * tmpl;
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
const char * handlebars_version_string(void) HBSARN;

const char * handlebars_spec_version_string(void) HBSARN;

const char * handlebars_mustache_spec_version_string(void) HBSARN;

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

struct handlebars_parser * handlebars_parser_ctor(struct handlebars_context * ctx);

void handlebars_parser_dtor(struct handlebars_parser * parser);

/**
 * @brief Convenience function for lexing to a token list
 * 
 * @param[in] ctx
 * @return the token list
 */
struct handlebars_token_list * handlebars_lex(struct handlebars_parser * parser) HBSARN;

bool handlebars_parse(struct handlebars_parser * parser);

void handlebars_throw(struct handlebars_context * context, enum handlebars_error_type num, const char * msg, ...) HBS_ATTR_NORETURN HBS_ATTR_PRINTF(3, 4);
void handlebars_throw_ex(struct handlebars_context * context, enum handlebars_error_type num, struct handlebars_locinfo * loc, const char * msg, ...) HBS_ATTR_NORETURN  HBS_ATTR_PRINTF(4, 5);
#define handlebars_context_throw handlebars_throw
#define handlebars_context_throw_ex handlebars_throw_ex

/**
 * @brief Get the error message from a context, or NULL.
 *
 * @param[in] context
 * @return the error message
 */
char * handlebars_error_message(struct handlebars_context * context);
#define handlebars_context_get_errmsg handlebars_error_message

/**
 * @brief Get the error message from a context, or NULL (compatibility for handlebars specification)
 *
 * @param[in] context
 * @return the error message
 */
char * handlebars_error_message_js(struct handlebars_context * context);
#define handlebars_context_get_errmsg_js handlebars_error_message_js

// Flex/Bison prototypes
int handlebars_yy_get_column(void * yyscanner);
void handlebars_yy_set_column(int column_no, void * yyscanner);
int handlebars_yy_parse(struct handlebars_parser * parser);

static inline void * handlebars_check(struct handlebars_context * context, void * ptr, const char * msg)
{
    if( handlebars_unlikely(ptr == NULL) ) {
        handlebars_context_throw(context, HANDLEBARS_NOMEM, "%s", msg);
    }
    return ptr;
}

#ifndef NDEBUG
struct handlebars_context * _HBSCTX(void * ctx, const char * loc);
#define HBSCTX(ctx) _HBSCTX(ctx, HBS_S2(__FILE__) ":" HBS_S2(__LINE__))
#else
#define HBSCTX(ctx) ((struct handlebars_context *)ctx)
#endif

#ifdef	__cplusplus
}
#endif

#endif
