
#ifndef HANDLEBARS_MEMORY_H
#define HANDLEBARS_MEMORY_H

#include <stdlib.h>
#include <talloc.h>

#ifdef	__cplusplus
extern "C" {
#endif

// Note: a little concerned this naughty stuff may cause problems with
// different talloc versions, but let's see how it works for now
// @todo maybe just alias them all with macros when disabled

// Memory macros
#define handlebars_talloc(ctx, type) \
    (type *) handlebars_talloc_named_const(ctx, sizeof(type), #type)

#define handlebars_talloc_free(ctx) \
    _handlebars_talloc_free(ctx, __location__)

#define handlebars_talloc_named_const _handlebars_talloc_named_const

#define handlebars_talloc_size(ctx, size) \
    handlebars_talloc_named_const(ctx, size, __location__)

#define handlebars_talloc_strdup _handlebars_talloc_strdup

#define handlebars_talloc_strdup_append _handlebars_talloc_strdup_append

#define handlebars_talloc_strndup _handlebars_talloc_strndup

#define handlebars_talloc_realloc(ctx, p, type, count) \
    (type *) _handlebars_talloc_realloc_array(ctx, p, sizeof(type), count, #type)

#define handlebars_talloc_zero(ctx, type) \
    (type *) _handlebars_talloc_zero(ctx, sizeof(type), #type)

#define handlebars_talloc_zero_size(ctx, size) \
    _handlebars_talloc_zero(ctx, size, __location__)

// Typedefs for memory function pointers
typedef int (*handlebars_talloc_free_func)(void *ptr, const char *location);
typedef void * (*handlebars_talloc_named_const_func)(const void * context, size_t size, const char * name);
typedef void * (*handlebars_talloc_realloc_array_func)(const void *ctx, void *ptr, size_t el_size, unsigned count, const char *name);
typedef char * (*handlebars_talloc_strdup_func)(const void *t, const char *p);
typedef char * (*handlebars_talloc_strdup_append_func)(char *s, const char *a);
typedef char * (*handlebars_talloc_strndup_func)(const void * t, const char * p, size_t n);
typedef void * (*handlebars_talloc_zero_func)(const void * ctx, size_t size, const char * name);

// Memory function pointers
#ifndef IN_HANDLEBARS_MEMORY_C
extern handlebars_talloc_free_func _handlebars_talloc_free;
extern handlebars_talloc_named_const_func _handlebars_talloc_named_const;
extern handlebars_talloc_realloc_array_func _handlebars_talloc_realloc_array;
extern handlebars_talloc_strdup_func _handlebars_talloc_strdup;
extern handlebars_talloc_strdup_append_func _handlebars_talloc_strdup_append;
extern handlebars_talloc_strndup_func _handlebars_talloc_strndup;
extern handlebars_talloc_zero_func _handlebars_talloc_zero;
#endif

// Allocators for a reentrant scanner (flex)
// We use the scanner pointer as a talloc context
void * handlebars_yy_alloc(size_t bytes, void * yyscanner);
void * handlebars_yy_realloc(void * ptr, size_t bytes, void * yyscanner);
void handlebars_yy_free(void * ptr, void * yyscanner);

// Functions to manipulate memory allocation failures
void handlebars_memory_fail_enable(void);
void handlebars_memory_fail_disable(void);

#ifdef	__cplusplus
}
#endif

#endif
