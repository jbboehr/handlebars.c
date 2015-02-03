
#include <stdlib.h>
#include <talloc.h>

#define IN_HANDLEBARS_MEMORY_C
#include "handlebars_memory.h"
#undef IN_HANDLEBARS_MEMORY_C

static int _handlebars_memfail_enabled = 0;
static int _handlebars_memfail_counter = -1;

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

static void _handlebars_memfail_internal()
{
}

// Overrides for memory functions
static int _handlebars_memfail_talloc_free(void *ptr, const char *location)
{
    // Do counter, succeed unless last count
    if( _handlebars_memfail_counter > -1 ) {
        if( --_handlebars_memfail_counter == 0 ) {
            handlebars_memory_fail_disable();
            // fall through
        } else {
            return _talloc_free(ptr, location);
        }
    }
    
    // Suppress unused parameter errors
    ptr = ptr;
    location = location;
    return -1;
}

static void * _handlebars_memfail_talloc_named_const(const void * context, size_t size, const char * name)
{
    // Do counter, succeed unless last count
    if( _handlebars_memfail_counter > -1 ) {
        if( --_handlebars_memfail_counter == 0 ) {
            handlebars_memory_fail_disable();
            // fall through
        } else {
            return talloc_named_const(context, size, name);
        }
    }
    
    // Suppress unused parameter errors
    context = context;
    size = size;
    name = name;
    return NULL;
}

static void * _handlebars_memfail_talloc_realloc_array(const void *ctx, void *ptr, size_t el_size, unsigned count, const char *name)
{
    // Do counter, succeed unless last count
    if( _handlebars_memfail_counter > -1 ) {
        if( --_handlebars_memfail_counter == 0 ) {
            handlebars_memory_fail_disable();
            // fall through
        } else {
            return _talloc_realloc_array(ctx, ptr, el_size, count, name);
        }
    }
    
    // Suppress unused parameter errors
    ctx = ctx;
    ptr = ptr;
    el_size = el_size;
    count = count;
    name = name;
    return NULL;
}

static char * _handlebars_memfail_talloc_strdup(const void *t, const char *p)
{
    // Do counter, succeed unless last count
    if( _handlebars_memfail_counter > -1 ) {
        if( --_handlebars_memfail_counter == 0 ) {
            handlebars_memory_fail_disable();
            // fall through
        } else {
            return talloc_strdup(t, p);
        }
    }
    
    // Suppress unused parameter errors
    t = t;
    p = p;
    return NULL;
}

static char * _handlebars_memfail_talloc_strdup_append(char *s, const char *a)
{
    // Do counter, succeed unless last count
    if( _handlebars_memfail_counter > -1 ) {
        if( --_handlebars_memfail_counter == 0 ) {
            handlebars_memory_fail_disable();
            // fall through
        } else {
            return talloc_strdup_append(s, a);
        }
    }
    
    // Suppress unused parameter errors
    s = s;
    a = a;
    return NULL;
}

static char * _handlebars_memfail_talloc_strdup_append_buffer(char *s, const char *a)
{
    // Do counter, succeed unless last count
    if( _handlebars_memfail_counter > -1 ) {
        if( --_handlebars_memfail_counter == 0 ) {
            handlebars_memory_fail_disable();
            // fall through
        } else {
            return talloc_strdup_append_buffer(s, a);
        }
    }
    
    // Suppress unused parameter errors
    s = s;
    a = a;
    return NULL;
}

static char * _handlebars_memfail_talloc_strndup(const void * t, const char * p, size_t n)
{
    // Do counter, succeed unless last count
    if( _handlebars_memfail_counter > -1 ) {
        if( --_handlebars_memfail_counter == 0 ) {
            handlebars_memory_fail_disable();
            // fall through
        } else {
            return talloc_strndup(t, p, n);
        }
    }
    
    // Suppress unused parameter errors
    t = t;
    p = p;
    n = n;
    return NULL;
}

static char * _handlebars_memfail_talloc_strndup_append_buffer(char *s, const char *a, size_t n)
{
    // Do counter, succeed unless last count
    if( _handlebars_memfail_counter > -1 ) {
        if( --_handlebars_memfail_counter == 0 ) {
            handlebars_memory_fail_disable();
            // fall through
        } else {
            return talloc_strndup(s, a, n);;
        }
    }
    
    // Suppress unused parameter errors
    s = s;
    a = a;
    n = n;
    return NULL;
}

static void * _handlebars_memfail_talloc_zero(const void * ctx, size_t size, const char * name)
{
    // Do counter, succeed unless last count
    if( _handlebars_memfail_counter > -1 ) {
        if( --_handlebars_memfail_counter == 0 ) {
            handlebars_memory_fail_disable();
            // fall through
        } else {
            return _talloc_zero(ctx, size, name);
        }
    }
    
    
    // Suppress unused parameter errors
    ctx = ctx;
    size = size;
    name = name;
    return NULL;
}

// Functions to manipulate memory allocation failures
void handlebars_memory_fail_enable(void)
{
    if( !_handlebars_memfail_enabled ) {
        _handlebars_memfail_enabled = 1;
        _handlebars_memfail_counter = -1;
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
        _handlebars_memfail_enabled = 0;
        _handlebars_memfail_counter = -1;
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

int handlebars_memory_fail_get_state(void)
{
    return _handlebars_memfail_enabled;
}

void handlebars_memory_fail_counter(int count)
{
    handlebars_memory_fail_enable();
    _handlebars_memfail_counter = count;
}

int handlebars_memory_fail_get_counter(void)
{
    return _handlebars_memfail_counter;
}
