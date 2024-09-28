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

/**
 * @file
 * @brief Memory allocation and manipulation functions
 */

#ifndef HANDLEBARS_MEMORY_H
#define HANDLEBARS_MEMORY_H

#include <talloc.h>
#include "handlebars.h"

HBS_EXTERN_C_START

#ifndef HANDLEBARS_MEMORY

#define handlebars_talloc talloc
#define handlebars_talloc_array talloc_array
#define handlebars_talloc_asprintf talloc_asprintf
#define handlebars_talloc_asprintf_append talloc_asprintf_append
#define handlebars_talloc_asprintf_append_buffer talloc_asprintf_append_buffer
#define handlebars_talloc_free talloc_free
#define handlebars_talloc_named_const talloc_named_const
#define handlebars_talloc_realloc talloc_realloc
#define handlebars_talloc_realloc_size talloc_realloc_size
#define handlebars_talloc_size talloc_size
#define handlebars_talloc_strdup talloc_strdup
#define handlebars_talloc_strdup_append talloc_strdup_append
#define handlebars_talloc_strdup_append_buffer talloc_strdup_append_buffer
#define handlebars_talloc_strndup talloc_strndup
#define handlebars_talloc_strndup_append_buffer talloc_strndup_append_buffer
#define handlebars_talloc_zero talloc_zero
#define handlebars_talloc_zero_size talloc_zero_size

#define handlebars_exit exit

#else /* HANDLEBARS_MEMORY */

// Memory macros
#define handlebars_talloc(ctx, type) \
    (type *) handlebars_talloc_named_const(ctx, sizeof(type), #type)

#define handlebars_talloc_array(ctx, type, count) \
    (type *) _handlebars_talloc_array(ctx, sizeof(type), count, #type)

#define handlebars_talloc_asprintf _handlebars_talloc_asprintf
#define handlebars_talloc_asprintf_append _handlebars_talloc_asprintf_append
#define handlebars_talloc_asprintf_append_buffer _handlebars_talloc_asprintf_append_buffer

#define handlebars_talloc_free(ctx) \
    _handlebars_talloc_free(ctx, __location__)

#define handlebars_talloc_named_const _handlebars_talloc_named_const

#define handlebars_talloc_realloc(ctx, p, type, count) \
    (type *) _handlebars_talloc_realloc_array(ctx, p, sizeof(type), count, #type)

#define handlebars_talloc_realloc_size(ctx, p, s) \
    _handlebars_talloc_realloc_array(ctx, p, 1, s, __location__)

#define handlebars_talloc_size(ctx, size) \
    handlebars_talloc_named_const(ctx, size, __location__)

#define handlebars_talloc_strdup _handlebars_talloc_strdup
#define handlebars_talloc_strdup_append _handlebars_talloc_strdup_append
#define handlebars_talloc_strdup_append_buffer _handlebars_talloc_strdup_append_buffer
#define handlebars_talloc_strndup _handlebars_talloc_strndup
#define handlebars_talloc_strndup_append_buffer _handlebars_talloc_strndup_append_buffer

#define handlebars_talloc_zero(ctx, type) \
    (type *) _handlebars_talloc_zero(ctx, sizeof(type), #type)

#define handlebars_talloc_zero_size(ctx, size) \
    _handlebars_talloc_zero(ctx, size, __location__)

// Typedefs for memory function pointers
typedef void * (*handlebars_talloc_array_func)(const void * ctx, size_t el_size, unsigned count, const char * name);
typedef char * (*handlebars_talloc_asprintf_func)(const void *t, const char *fmt, ...) HBS_ATTR_PRINTF(2,3);
typedef char * (*handlebars_talloc_asprintf_append_func)(char *s, const char *fmt, ...) HBS_ATTR_PRINTF(2,3);
typedef char * (*handlebars_talloc_asprintf_append_buffer_func)(char *s, const char *fmt, ...) HBS_ATTR_PRINTF(2,3);
typedef int    (*handlebars_talloc_free_func)(void *ptr, const char *location);
typedef void * (*handlebars_talloc_named_const_func)(const void * context, size_t size, const char * name);
typedef void * (*handlebars_talloc_realloc_array_func)(const void *ctx, void *ptr, size_t el_size, unsigned count, const char *name);
typedef char * (*handlebars_talloc_strdup_func)(const void *t, const char *p);
typedef char * (*handlebars_talloc_strdup_append_func)(char *s, const char *a);
typedef char * (*handlebars_talloc_strdup_append_buffer_func)(char *s, const char *a);
typedef char * (*handlebars_talloc_strndup_func)(const void * t, const char * p, size_t n);
typedef char * (*handlebars_talloc_strndup_append_buffer_func)(char *s, const char *a, size_t n);
typedef void * (*handlebars_talloc_zero_func)(const void * ctx, size_t size, const char * name);

// Typedefs for other function pointers
typedef void (*handlebars_exit_func)(int exit_code) HBS_ATTR_NORETURN;

// Memory function pointers
extern handlebars_talloc_array_func _handlebars_talloc_array;
extern handlebars_talloc_asprintf_func _handlebars_talloc_asprintf;
extern handlebars_talloc_asprintf_append_func _handlebars_talloc_asprintf_append;
extern handlebars_talloc_asprintf_append_buffer_func _handlebars_talloc_asprintf_append_buffer;
extern handlebars_talloc_free_func _handlebars_talloc_free;
extern handlebars_talloc_named_const_func _handlebars_talloc_named_const;
extern handlebars_talloc_realloc_array_func _handlebars_talloc_realloc_array;
extern handlebars_talloc_strdup_func _handlebars_talloc_strdup;
extern handlebars_talloc_strdup_append_func _handlebars_talloc_strdup_append;
extern handlebars_talloc_strdup_append_buffer_func _handlebars_talloc_strdup_append_buffer;
extern handlebars_talloc_strndup_func _handlebars_talloc_strndup;
extern handlebars_talloc_strndup_append_buffer_func _handlebars_talloc_strndup_append_buffer;
extern handlebars_talloc_zero_func _handlebars_talloc_zero;

// Other function pointers
extern handlebars_exit_func handlebars_exit;

/**
 * @brief Flags to control the memory fail behaviour
 */
enum handlebars_memory_fail_flag {
    handlebars_memory_fail_flag_none = 0,
    handlebars_memory_fail_flag_alloc = (1 << 0),
    handlebars_memory_fail_flag_free = (1 << 1),
    handlebars_memory_fail_flag_exit = (1 << 2),
    handlebars_memory_fail_flag_yy = (1 << 3),
    handlebars_memory_fail_flag_all = (1 << 4) - 1,
};

// Functions to manipulate and mock memory allocations

/**
 * @brief Enable memory failure behaviour
 */
void handlebars_memory_fail_enable(void);

/**
 * @brief Disable memory failure behaviour
 */
void handlebars_memory_fail_disable(void);

/**
 * @brief Get whether memory failure is enabled
 */
int handlebars_memory_fail_get_state(void);

/**
 * @brief Set the memory fail flags. Must be called before enable, and disable resets these flags to all
 *
 * @param[in] flags
 */
void handlebars_memory_fail_set_flags(int flags);

/**
 * @brief Trigger memory allocation failures after specified number of allocations
 *
 * @param[in] count The number of allocations after which to fail
 */
void handlebars_memory_fail_counter(int count);

/**
 * @brief Get the number of memory allocations at which to start failure
 *
 * @return The number of allocations after which to fail
 */
int handlebars_memory_fail_get_counter(void);

int handlebars_memory_fail_counter_incr(void);

/**
 * @brief Get the last exit code when memory fail was enabled
 *
 * @return The exit code
 */
int handlebars_memory_get_last_exit_code(void);

/**
 * @brief Get the number of memory allocations since failure enabled
 *
 * @return The number of allocations
 */
int handlebars_memory_get_call_counter(void);

#endif /* HANDLEBARS_MEMORY */

HBS_EXTERN_C_END

#endif
