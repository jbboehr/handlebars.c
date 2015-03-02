
#include <stdarg.h>
#include <stdlib.h>
#include <talloc.h>

#include "handlebars_memory.h"

// LCOV_EXCL_START

static int _handlebars_exit_fail_enabled = 0;
static int _handlebars_memory_fail_enabled = 0;
static int _handlebars_memory_fail_counter = -1;
static int _handlebars_memory_last_exit_code = -1;
static int _handlebars_memory_call_counter = 0;

static void _handlebars_exit(int exit_code);

// Setup default memory function pointers
handlebars_talloc_array_func _handlebars_talloc_array = &_talloc_array;
handlebars_talloc_asprintf_func _handlebars_talloc_asprintf = &talloc_asprintf;
handlebars_talloc_asprintf_append_func _handlebars_talloc_asprintf_append = &talloc_asprintf_append;
handlebars_talloc_asprintf_append_buffer_func _handlebars_talloc_asprintf_append_buffer = &talloc_asprintf_append_buffer;
handlebars_talloc_free_func _handlebars_talloc_free = &_talloc_free;
handlebars_talloc_named_const_func _handlebars_talloc_named_const = &talloc_named_const;
handlebars_talloc_realloc_array_func _handlebars_talloc_realloc_array = &_talloc_realloc_array;
handlebars_talloc_strdup_func _handlebars_talloc_strdup = &talloc_strdup;
handlebars_talloc_strdup_append_func _handlebars_talloc_strdup_append = &talloc_strdup_append;
handlebars_talloc_strdup_append_buffer_func _handlebars_talloc_strdup_append_buffer = &talloc_strdup_append_buffer;
handlebars_talloc_strndup_func _handlebars_talloc_strndup = &talloc_strndup;
handlebars_talloc_strndup_append_buffer_func _handlebars_talloc_strndup_append_buffer = &talloc_strndup_append_buffer;
handlebars_talloc_zero_func _handlebars_talloc_zero = &_talloc_zero;
handlebars_exit_func handlebars_exit = &_handlebars_exit;

// Overrides for memory functions
static void * _handlebars_memfail_talloc_array(const void * ctx, size_t el_size, unsigned count, const char * name)
{
    // Increment call counter
    _handlebars_memory_call_counter++;
    
    // Do counter, succeed unless last count
    if( _handlebars_memory_fail_counter > -1 ) {
        if( --_handlebars_memory_fail_counter == 0 ) {
            handlebars_memory_fail_disable();
            // fall through
        } else {
            return _talloc_array(ctx, el_size, count, name);
        }
    }
    
    return NULL;
}

static char * _handlebars_memfail_talloc_asprintf(const void * t, const char * fmt, ...)
{
    // Increment call counter
    _handlebars_memory_call_counter++;
    
    // Do counter, succeed unless last count
    if( _handlebars_memory_fail_counter > -1 ) {
        if( --_handlebars_memory_fail_counter == 0 ) {
            handlebars_memory_fail_disable();
            // fall through
        } else {
            va_list ap;
            char * str;
            va_start(ap, fmt);
            str = talloc_vasprintf(t, fmt, ap);
            va_end(ap);
            return str;
        }
    }
    
    return NULL;
}

static char * _handlebars_memfail_talloc_asprintf_append(char * s, const char * fmt, ...)
{
    // Increment call counter
    _handlebars_memory_call_counter++;
    
    // Do counter, succeed unless last count
    if( _handlebars_memory_fail_counter > -1 ) {
        if( --_handlebars_memory_fail_counter == 0 ) {
            handlebars_memory_fail_disable();
            // fall through
        } else {
            va_list ap;
            char * str;
            va_start(ap, fmt);
            str = talloc_vasprintf_append(s, fmt, ap);
            va_end(ap);
            return str;
        }
    }
    
    return NULL;
}

static char * _handlebars_memfail_talloc_asprintf_append_buffer(char * s, const char * fmt, ...)
{
    // Increment call counter
    _handlebars_memory_call_counter++;
    
    // Do counter, succeed unless last count
    if( _handlebars_memory_fail_counter > -1 ) {
        if( --_handlebars_memory_fail_counter == 0 ) {
            handlebars_memory_fail_disable();
            // fall through
        } else {
            va_list ap;
            char * str;
            va_start(ap, fmt);
            str = talloc_vasprintf_append_buffer(s, fmt, ap);
            va_end(ap);
            return str;
        }
    }
    
    return NULL;
}

static int _handlebars_memfail_talloc_free(void *ptr, const char *location)
{
    // Increment call counter
    _handlebars_memory_call_counter++;
    
    // Do counter, succeed unless last count
    if( _handlebars_memory_fail_counter > -1 ) {
        if( --_handlebars_memory_fail_counter == 0 ) {
            handlebars_memory_fail_disable();
            // fall through
        } else {
            return _talloc_free(ptr, location);
        }
    }
    
    return -1;
}

static void * _handlebars_memfail_talloc_named_const(const void * context, size_t size, const char * name)
{
    // Increment call counter
    _handlebars_memory_call_counter++;
    
    // Do counter, succeed unless last count
    if( _handlebars_memory_fail_counter > -1 ) {
        if( --_handlebars_memory_fail_counter == 0 ) {
            handlebars_memory_fail_disable();
            // fall through
        } else {
            return talloc_named_const(context, size, name);
        }
    }
    
    return NULL;
}

static void * _handlebars_memfail_talloc_realloc_array(const void *ctx, void *ptr, size_t el_size, unsigned count, const char *name)
{
    // Increment call counter
    _handlebars_memory_call_counter++;
    
    // Do counter, succeed unless last count
    if( _handlebars_memory_fail_counter > -1 ) {
        if( --_handlebars_memory_fail_counter == 0 ) {
            handlebars_memory_fail_disable();
            // fall through
        } else {
            return _talloc_realloc_array(ctx, ptr, el_size, count, name);
        }
    }
    
    return NULL;
}

static char * _handlebars_memfail_talloc_strdup(const void *t, const char *p)
{
    // Increment call counter
    _handlebars_memory_call_counter++;
    
    // Do counter, succeed unless last count
    if( _handlebars_memory_fail_counter > -1 ) {
        if( --_handlebars_memory_fail_counter == 0 ) {
            handlebars_memory_fail_disable();
            // fall through
        } else {
            return talloc_strdup(t, p);
        }
    }
    
    return NULL;
}

static char * _handlebars_memfail_talloc_strdup_append(char *s, const char *a)
{
    // Increment call counter
    _handlebars_memory_call_counter++;
    
    // Do counter, succeed unless last count
    if( _handlebars_memory_fail_counter > -1 ) {
        if( --_handlebars_memory_fail_counter == 0 ) {
            handlebars_memory_fail_disable();
            // fall through
        } else {
            return talloc_strdup_append(s, a);
        }
    }
    
    return NULL;
}

static char * _handlebars_memfail_talloc_strdup_append_buffer(char *s, const char *a)
{
    // Increment call counter
    _handlebars_memory_call_counter++;
    
    // Do counter, succeed unless last count
    if( _handlebars_memory_fail_counter > -1 ) {
        if( --_handlebars_memory_fail_counter == 0 ) {
            handlebars_memory_fail_disable();
            // fall through
        } else {
            return talloc_strdup_append_buffer(s, a);
        }
    }
    
    return NULL;
}

static char * _handlebars_memfail_talloc_strndup(const void * t, const char * p, size_t n)
{
    // Increment call counter
    _handlebars_memory_call_counter++;
    
    // Do counter, succeed unless last count
    if( _handlebars_memory_fail_counter > -1 ) {
        if( --_handlebars_memory_fail_counter == 0 ) {
            handlebars_memory_fail_disable();
            // fall through
        } else {
            return talloc_strndup(t, p, n);
        }
    }
    
    return NULL;
}

static char * _handlebars_memfail_talloc_strndup_append_buffer(char *s, const char *a, size_t n)
{
    // Increment call counter
    _handlebars_memory_call_counter++;
    
    // Do counter, succeed unless last count
    if( _handlebars_memory_fail_counter > -1 ) {
        if( --_handlebars_memory_fail_counter == 0 ) {
            handlebars_memory_fail_disable();
            // fall through
        } else {
            return talloc_strndup(s, a, n);;
        }
    }
    
    return NULL;
}

static void * _handlebars_memfail_talloc_zero(const void * ctx, size_t size, const char * name)
{
    // Increment call counter
    _handlebars_memory_call_counter++;
    
    // Do counter, succeed unless last count
    if( _handlebars_memory_fail_counter > -1 ) {
        if( --_handlebars_memory_fail_counter == 0 ) {
            handlebars_memory_fail_disable();
            // fall through
        } else {
            return _talloc_zero(ctx, size, name);
        }
    }
    
    return NULL;
}

// Other function pointers
// Disabling this for now
static void _handlebars_memfail_exit(int exit_code)
{
    _handlebars_memory_call_counter++;
    _handlebars_memory_last_exit_code = exit_code;
}

static void _handlebars_exit(int exit_code)
{
    exit(exit_code);
}


// Functions to manipulate memory allocation failures
void handlebars_memory_fail_enable(void)
{
    if( !_handlebars_memory_fail_enabled ) {
        _handlebars_memory_fail_enabled = 1;
        _handlebars_memory_fail_counter = -1;
        _handlebars_memory_last_exit_code = 0;
        _handlebars_memory_call_counter = 0;
        _handlebars_talloc_array = &_handlebars_memfail_talloc_array;
        _handlebars_talloc_asprintf = &_handlebars_memfail_talloc_asprintf;
        _handlebars_talloc_asprintf_append = &_handlebars_memfail_talloc_asprintf_append;
        _handlebars_talloc_asprintf_append_buffer = &_handlebars_memfail_talloc_asprintf_append_buffer;
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
    if( _handlebars_memory_fail_enabled ) {
        _handlebars_memory_fail_enabled = 0;
        //_handlebars_memory_fail_counter = -1;
        //_handlebars_memory_last_exit_code = 0;
        //_handlebars_memory_call_counter = 0;
        _handlebars_talloc_array = &_talloc_array;
        _handlebars_talloc_asprintf = &talloc_asprintf;
        _handlebars_talloc_asprintf_append = &talloc_asprintf_append;
        _handlebars_talloc_asprintf_append_buffer = &talloc_asprintf_append_buffer;
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
    return _handlebars_memory_fail_enabled;
}

void handlebars_memory_fail_counter(int count)
{
    handlebars_memory_fail_enable();
    _handlebars_memory_fail_counter = count;
}

int handlebars_memory_fail_get_counter(void)
{
    return _handlebars_memory_fail_counter;
}

int handlebars_memory_get_last_exit_code(void)
{
    return _handlebars_memory_last_exit_code;
}

int handlebars_memory_get_call_counter(void)
{
    return _handlebars_memory_call_counter;
}

// Functions to manipulate other failures
void handlebars_exit_fail_enable(void)
{
    if( !_handlebars_exit_fail_enabled ) {
        _handlebars_exit_fail_enabled = 1;
        _handlebars_memory_fail_counter = -1;
        _handlebars_memory_last_exit_code = 0;
        _handlebars_memory_call_counter = 0;
        handlebars_exit = &_handlebars_memfail_exit;
    }
}

void handlebars_exit_fail_disable(void)
{
    if( _handlebars_exit_fail_enabled ) {
        _handlebars_exit_fail_enabled = 0;
        //_handlebars_memory_fail_counter = -1;
        //_handlebars_memory_last_exit_code = 0;
        //_handlebars_memory_call_counter = 0;
        handlebars_exit = &_handlebars_exit;
    }
}

// LCOV_EXCL_STOP
