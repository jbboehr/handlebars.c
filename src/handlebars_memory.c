/**
 * Copyright (c) anno Domini nostri Jesu Christi MMXVI-MMXXIV John Boehr & contributors
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdarg.h>
#include <stdlib.h>
#include <talloc.h>

#include "handlebars_memory.h"

// LCOV_EXCL_START

static int _handlebars_memory_fail_enabled = 0;
static int _handlebars_memory_fail_counter = -1;
static int _handlebars_memory_fail_flags = handlebars_memory_fail_flag_all;
static int _handlebars_memory_last_exit_code = -1;
static int _handlebars_memory_call_counter = 0;

static void _handlebars_exit(int exit_code) HBS_ATTR_NORETURN;

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

// Setup memory function pointers for scanner
handlebars_talloc_named_const_func _handlebars_yy_alloc = &talloc_named_const;
handlebars_talloc_realloc_array_func _handlebars_yy_realloc = &_talloc_realloc_array;
handlebars_talloc_free_func _handlebars_yy_free = &_talloc_free;

// Setup other function pointers
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

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic warning "-Wformat-nonliteral"
#endif

HBS_ATTR_PRINTF(2,3)
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

HBS_ATTR_PRINTF(2,3)
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

HBS_ATTR_PRINTF(2,3)
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

#ifdef __clang__
#pragma clang diagnostic pop
#endif

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

HBS_ATTR_NORETURN
static void _handlebars_exit(int exit_code)
{
    exit(exit_code);
}


// Functions to manipulate memory allocation failures
void handlebars_memory_fail_enable(void)
{
    //if( _handlebars_memory_fail_enabled ) {
    //    return;
    //}

    _handlebars_memory_fail_enabled = 1;
    _handlebars_memory_fail_counter = -1;
    _handlebars_memory_last_exit_code = 0;
    _handlebars_memory_call_counter = 0;
    if( _handlebars_memory_fail_flags & handlebars_memory_fail_flag_alloc ) {
        _handlebars_talloc_array = &_handlebars_memfail_talloc_array;
        _handlebars_talloc_asprintf = &_handlebars_memfail_talloc_asprintf;
        _handlebars_talloc_asprintf_append = &_handlebars_memfail_talloc_asprintf_append;
        _handlebars_talloc_asprintf_append_buffer = &_handlebars_memfail_talloc_asprintf_append_buffer;
        _handlebars_talloc_named_const = &_handlebars_memfail_talloc_named_const;
        _handlebars_talloc_realloc_array = &_handlebars_memfail_talloc_realloc_array;
        _handlebars_talloc_strdup = &_handlebars_memfail_talloc_strdup;
        _handlebars_talloc_strdup_append = &_handlebars_memfail_talloc_strdup_append;
        _handlebars_talloc_strdup_append_buffer = &_handlebars_memfail_talloc_strdup_append_buffer;
        _handlebars_talloc_strndup = &_handlebars_memfail_talloc_strndup;
        _handlebars_talloc_strndup_append_buffer = &_handlebars_memfail_talloc_strndup_append_buffer;
        _handlebars_talloc_zero = &_handlebars_memfail_talloc_zero;
    }
    if( _handlebars_memory_fail_flags & handlebars_memory_fail_flag_free ) {
        _handlebars_talloc_free = &_handlebars_memfail_talloc_free;
    }
    if( _handlebars_memory_fail_flags & handlebars_memory_fail_flag_yy ) {
        _handlebars_yy_alloc = &_handlebars_memfail_talloc_named_const;
        _handlebars_yy_realloc = &_handlebars_memfail_talloc_realloc_array;
        _handlebars_yy_free = &_handlebars_memfail_talloc_free;
    }
}

void handlebars_memory_fail_disable(void)
{
    //if( !_handlebars_memory_fail_enabled ) {
    //    return;
    //}

    _handlebars_memory_fail_enabled = 0;
    _handlebars_memory_fail_flags = handlebars_memory_fail_flag_all;
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
    _handlebars_yy_alloc = &talloc_named_const;
    _handlebars_yy_realloc = &_talloc_realloc_array;
    _handlebars_yy_free = &_talloc_free;
    handlebars_exit = &_handlebars_exit;
}

int handlebars_memory_fail_get_state(void)
{
    return _handlebars_memory_fail_enabled;
}

void handlebars_memory_fail_set_flags(int flags)
{
    _handlebars_memory_fail_flags = flags;
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

int handlebars_memory_fail_counter_incr(void)
{
    if( _handlebars_memory_fail_enabled && _handlebars_memory_fail_counter > -1 ) {
        return ++_handlebars_memory_fail_counter;
    } else {
        return -1;
    }
}

int handlebars_memory_get_last_exit_code(void)
{
    return _handlebars_memory_last_exit_code;
}

int handlebars_memory_get_call_counter(void)
{
    return _handlebars_memory_call_counter;
}


// LCOV_EXCL_STOP
