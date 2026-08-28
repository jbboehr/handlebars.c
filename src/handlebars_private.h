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

#ifndef HANDLEBARS_PRIVATE_H
#define HANDLEBARS_PRIVATE_H

#include <assert.h>

#include "handlebars.h"

HBS_EXTERN_C_START

struct handlebars_context;
struct handlebars_error;
struct handlebars_map;
struct handlebars_stack;

HBS_LOCAL struct handlebars_context * handlebars_map_get_context(
    struct handlebars_map * map
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL;

HBS_LOCAL struct handlebars_context * handlebars_stack_get_context(
    struct handlebars_stack * stack
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL;

HBS_LOCAL void handlebars_value_iterator_unwind(
    struct handlebars_error * error,
    jmp_buf * target
) HBS_ATTR_NONNULL_ALL;

HBS_LOCAL HBS_ATTR_NORETURN void handlebars_longjmp(
    struct handlebars_context * context,
    jmp_buf * target,
    int num
) HBS_ATTR_NONNULL_ALL;

HBS_LOCAL void handlebars_error_set(
    struct handlebars_context * context,
    enum handlebars_error_type num,
    const char * msg,
    ...
) HBS_ATTR_NONNULL_ALL HBS_ATTR_PRINTF(3, 4);

#define likely handlebars_likely
#define unlikely handlebars_unlikely

#define CONTEXT context
#define MEMCHK_MSG HANDLEBARS_MEMCHECK_MSG
#define MEMCHKEX(cond, ctx) HANDLEBARS_MEMCHECK(cond, ctx)
#define MEMCHK(cond) MEMCHKEX(cond, CONTEXT)
#define MEMCHKF(ptr) (HBS_TYPEOF(ptr)) handlebars_check(CONTEXT, (void *) (ptr), MEMCHK_MSG)
#define MC(ptr) MEMCHKF(ptr)

// Assert wrapper with side effects, yielding the expression
#if !defined(NDEBUG) && defined(HANDLEBARS_HAVE_STATEMENT_EXPRESSIONS)
#define HBS_ASSERT(expr) ({ __typeof__ (expr) _expr = expr; assert(((void) #expr, _expr)); _expr; })
#else
#define HBS_ASSERT(expr) (expr)
#endif

#define YY_NO_UNISTD_H 1
#define YYLTYPE handlebars_locinfo

HBS_ATTR_CONST
static inline size_t handlebars_align_size(size_t size, size_t alignment)
{
    size_t rem = size % alignment;
    if (rem == 0) {
        return size;
    } else {
        return size + alignment - rem;
    }
}

HBS_EXTERN_C_END

#endif
