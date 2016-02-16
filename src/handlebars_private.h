
#ifndef HANDLEBARS_PRIVATE_H
#define HANDLEBARS_PRIVATE_H

#include "handlebars.h"

#ifdef	__cplusplus
extern "C" {
#endif

struct handlebars_context;

#define HANDLEBARS_PACKI(i, n) ((i & (0xff << n)) >> n)
#define HANDLEBARS_UNPACKI(i, n) (i << n)

// Builtin expect
#ifdef HAVE___BUILTIN_EXPECT
#  define likely(x)   __builtin_expect(!!(x), 1)
#  define unlikely(x) __builtin_expect(!!(x), 0)
#else
#  define likely(x) (x)
#  define unlikely(x) (x)
#endif

// Unused attribute
#ifdef HAVE_VAR_ATTRIBUTE_UNUSED
#  define HANDLEBARS_ATTR_UNUSED __attribute__((__unused__))
#else
#  define HANDLEBARS_ATTR_UNUSED
#endif

#if (__GNUC__ >= 3)
#define HBS_TYPEOF(ptr) __typeof__(ptr)
#else
#define HBS_TYPEOF(ptr) void *
#endif

#define CONTEXT context
#define MEMCHK_MSG "Out of memory  [" HBS_S2(__FILE__) ":" HBS_S2(__LINE__) "]"
#define MEMCHK(cond) \
    do { \
        if( unlikely(!cond) ) { \
            handlebars_context_throw(CONTEXT, HANDLEBARS_NOMEM, MEMCHK_MSG); \
        } \
    } while(0)
#define MEMCHKF(ptr) (HBS_TYPEOF(ptr)) handlebars_check(CONTEXT, (void *) (ptr), MEMCHK_MSG)
#define MC(ptr) MEMCHKF(ptr)

static inline void * handlebars_check(struct handlebars_context * context, void * ptr, const char * msg)
{
    if( unlikely(ptr == NULL) ) {
        handlebars_context_throw(context, HANDLEBARS_NOMEM, msg);
    }
    return ptr;
}

#ifdef	__cplusplus
}
#endif

#endif
