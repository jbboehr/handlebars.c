
#ifndef HANDLEBARS_PRIVATE_H
#define HANDLEBARS_PRIVATE_H

#include "handlebars.h"

#ifdef	__cplusplus
extern "C" {
#endif

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

#ifdef	__cplusplus
}
#endif

#endif
