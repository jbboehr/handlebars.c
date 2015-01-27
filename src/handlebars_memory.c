
#include <stdlib.h>
#include <talloc.h>

#define IN_HANDLEBARS_MEMORY_C
#include "handlebars_memory.h"
#undef IN_HANDLEBARS_MEMORY_C

static int _handlebars_memfail_enabled = 0;

// Setup default memory function pointers
handlebars_talloc_free_func _handlebars_talloc_free = &_talloc_free;
handlebars_talloc_named_const_func _handlebars_talloc_named_const = &talloc_named_const;
handlebars_talloc_realloc_array_func _handlebars_talloc_realloc_array = &_talloc_realloc_array;
handlebars_talloc_strdup_func _handlebars_talloc_strdup = &talloc_strdup;
handlebars_talloc_strdup_append_func _handlebars_talloc_strdup_append = &talloc_strdup_append;
handlebars_talloc_strdup_append_buffer_func _handlebars_talloc_strdup_append_buffer = &talloc_strdup_append_buffer;
handlebars_talloc_strndup_func _handlebars_talloc_strndup = &talloc_strndup;
handlebars_talloc_strndup_append_buffer_func _handlebars_talloc_strndup_append_buffer = &talloc_strndup_append_buffer;
handlebars_talloc_zero_func _handlebars_talloc_zero = &_talloc_zero;

// Overrides for memory functions
static int _handlebars_memfail_talloc_free(void *ptr, const char *location)
{
    return -1;
}

static void * _handlebars_memfail_talloc_named_const(const void * context, size_t size, const char * name)
{
    return NULL;
}

static void * _handlebars_memfail_talloc_realloc_array(const void *ctx, void *ptr, size_t el_size, unsigned count, const char *name)
{
    return NULL;
}

static char * _handlebars_memfail_talloc_strdup(const void *t, const char *p)
{
    return NULL;
}

static char * _handlebars_memfail_talloc_strdup_append(char *s, const char *a)
{
    return NULL;
}

static char * _handlebars_memfail_talloc_strdup_append_buffer(char *s, const char *a)
{
    return NULL;
}

static char * _handlebars_memfail_talloc_strndup(const void * t, const char * p, size_t n)
{
   return NULL;
}

static char * _handlebars_memfail_talloc_strndup_append_buffer(char *s, const char *a, size_t n)
{
    return NULL;
}

static void * _handlebars_memfail_talloc_zero(const void * ctx, size_t size, const char * name)
{
    return NULL;
}

// Allocators for a reentrant scanner (flex)
struct handlebars_context * _handlebars_context_tmp;

void * handlebars_yy_alloc(size_t bytes, void * yyscanner)
{
    // Note: it looks like the yyscanner is allocated before we can pass in
    // a handlebars context...
    // Also look into the performance hit for doing this
    struct handlebars_context * ctx = (yyscanner ? handlebars_yy_get_extra(yyscanner) : _handlebars_context_tmp);
    return (void *) handlebars_talloc_size(ctx, bytes);
}

void * handlebars_yy_realloc(void * ptr, size_t bytes, void * yyscanner)
{
    struct handlebars_context * ctx = (yyscanner ? handlebars_yy_get_extra(yyscanner) : _handlebars_context_tmp);
    return (void *) handlebars_talloc_realloc(ctx, ptr, char, bytes);
}

void handlebars_yy_free(void * ptr, void * yyscanner)
{
    handlebars_talloc_free(ptr);
}

// Functions to manipulate memory allocation failures
void handlebars_memory_fail_enable(void)
{
    if( !_handlebars_memfail_enabled ) {
        _handlebars_talloc_free = &_handlebars_memfail_talloc_free;
        _handlebars_talloc_named_const = &_handlebars_memfail_talloc_named_const;
        _handlebars_talloc_realloc_array = &_handlebars_memfail_talloc_realloc_array;
        _handlebars_talloc_strdup = &_handlebars_memfail_talloc_strdup;
        _handlebars_talloc_strdup_append = &_handlebars_memfail_talloc_strdup_append;
        _handlebars_talloc_strdup_append_buffer = &_handlebars_memfail_talloc_strdup_append_buffer;
        _handlebars_talloc_strndup = &_handlebars_memfail_talloc_strndup;
        _handlebars_talloc_strndup_append_buffer = &_handlebars_memfail_talloc_strndup_append_buffer;
        _handlebars_talloc_zero = &_handlebars_memfail_talloc_zero;
    }
}

void handlebars_memory_fail_disable(void)
{
    if( _handlebars_memfail_enabled ) {
        _handlebars_talloc_free = &_talloc_free;
        _handlebars_talloc_named_const = &talloc_named_const;
        _handlebars_talloc_realloc_array = &_talloc_realloc_array;
        _handlebars_talloc_strdup = &talloc_strdup;
        _handlebars_talloc_strdup_append = &talloc_strdup_append;
        _handlebars_talloc_strdup_append_buffer = &talloc_strdup_append_buffer;
        _handlebars_talloc_strndup = &talloc_strndup;
        _handlebars_talloc_strndup_append_buffer = &talloc_strndup_append_buffer;
        _handlebars_talloc_zero = &_talloc_zero;
    }
}
