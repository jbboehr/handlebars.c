
#ifndef HANDLEBARS_H
#define HANDLEBARS_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdlib.h>

// Memory macros
#define handlebars_calloc calloc
#define handlebars_malloc malloc
#define handlebars_realloc realloc
#define handlebars_free free
#define handlebars_talloc talloc
#define handlebars_talloc_free talloc_free
#define handlebars_talloc_zero talloc_zero

// Error types
#define HANDLEBARS_SUCCESS 0
#define HANDLEBARS_ERROR 1
#define HANDLEBARS_NOMEM 2
#define HANDLEBARS_NULLARG 3

#define handlebars_stringify(x) #x

#ifdef	__cplusplus
}
#endif

#endif
