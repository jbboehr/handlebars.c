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

#ifndef HANDLEBARS_STACK_H
#define HANDLEBARS_STACK_H

#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif

#include "handlebars.h"

HBS_EXTERN_C_START

struct handlebars_context;
struct handlebars_value;
struct handlebars_stack;

struct handlebars_stack_save_buf {
    size_t protect;
    size_t count;
};

#ifdef HANDLEBARS_HAVE_STATEMENT_EXPRESSIONS
#define handlebars_stack_alloca(ctx, capacity) ({ \
        void * HANDLEBARS_STACK_ALLOC_PTR = alloca(handlebars_stack_size(capacity)); \
        handlebars_stack_init(ctx, HANDLEBARS_STACK_ALLOC_PTR, capacity); \
        HANDLEBARS_STACK_ALLOC_PTR; \
    })
#else
// @TODO can we do better than this?
extern void *HANDLEBARS_STACK_ALLOC_PTR;
#define handlebars_stack_alloca(ctx, capacity) ( \
        HANDLEBARS_STACK_ALLOC_PTR = alloca(handlebars_stack_size(capacity)), \
        handlebars_stack_init(ctx, HANDLEBARS_STACK_ALLOC_PTR, capacity), \
        HANDLEBARS_STACK_ALLOC_PTR \
    )
#endif

/**
 * @brief Calculate the size of a stack for a given #capacity
 * @param[in] capacity The number of desired elements
 * @return The size in bytes
 */
size_t handlebars_stack_size(size_t capacity)
    HBS_ATTR_CONST;

// {{{ Constructors and Destructors

/**
 * @brief Initialize a new stack allocated stack
 * @param[in] ctx The handlebars context
 * @param[in] stack The previously allocated memory
 * @param[in] capacity The desired number of elements
 * @return The previously allocated stack
 */
struct handlebars_stack * handlebars_stack_init(
    struct handlebars_context * ctx,
    struct handlebars_stack * stack,
    size_t capacity
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL;

/**
 * @brief Construct a new stack
 * @param[in] ctx The handlebars context
 * @param[in] capacity The desired number of elements
 * @return The new stack
 */
struct handlebars_stack * handlebars_stack_ctor(
    struct handlebars_context * ctx,
    size_t capacity
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

struct handlebars_stack * handlebars_stack_copy_ctor(
    struct handlebars_stack * stack,
    size_t capacity
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

/**
 * @brief Destruct a stack
 * @param[in] stack
 * @return void
 */
void handlebars_stack_dtor(
    struct handlebars_stack * stack
) HBS_ATTR_NONNULL_ALL;

// }}} Constructors and Destructors

// {{{ Reference Counting

void handlebars_stack_addref(struct handlebars_stack * stack)
    HBS_ATTR_NONNULL_ALL;

void handlebars_stack_delref(struct handlebars_stack * stack)
    HBS_ATTR_NONNULL_ALL;

// }}} Reference Counting

/**
 * @brief Get the number of elements in a stack
 * @param[in] stack
 * @return The number of elements
 */
size_t handlebars_stack_count(
    struct handlebars_stack * stack
) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Push a value onto the top of a stack
 * @param[in] stack
 * @param[in] value
 * @return The stack, possibly reallocated
 */
struct handlebars_stack * handlebars_stack_push(
    struct handlebars_stack * stack,
    struct handlebars_value * value
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

/**
 * @brief Pop a value from the top of a stack
 * @param[in] stack
 * @return The popped value
 */
struct handlebars_value * handlebars_stack_pop(
    struct handlebars_stack * stack,
    struct handlebars_value * rv
) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Get the value on the top of the stack
 * @param[in] stack
 * @return The value on the top of the stack
 */
struct handlebars_value * handlebars_stack_top(
    struct handlebars_stack * stack
) HBS_ATTR_NONNULL_ALL HBS_ATTR_WARN_UNUSED_RESULT;

/**
 * @brief Get the value at the specified offset from the bottom of the stack
 * @param[in] stack
 * @param[in] offset
 * @return The value at the specified offset
 */
struct handlebars_value * handlebars_stack_get(
    struct handlebars_stack * stack,
    size_t offset
) HBS_ATTR_NONNULL_ALL HBS_ATTR_WARN_UNUSED_RESULT;

/**
 * @brief Set the value at the specified offset from the bottom of the stack
 * @param[in] stack
 * @param[in] offset
 * @return
 */
struct handlebars_stack * handlebars_stack_set(
    struct handlebars_stack * stack,
    size_t offset,
    struct handlebars_value * value
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

/**
 * @brief Prevent the bottom #num elements from being popped or set. In the
 *        future, may have no effect in non-debug mode (when NDEBUG is defined).
 * @param[in] stack
 * @param[in] num
 * @return The previous protect value
 */
size_t handlebars_stack_protect(
    struct handlebars_stack * stack,
    size_t num
) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Truncate the stack to the bottom #num elements
 * @param[in] stack
 * @param[in] num
 * @return void
 */
void handlebars_stack_truncate(
    struct handlebars_stack * stack,
    size_t num
) HBS_ATTR_NONNULL_ALL;

struct handlebars_stack_save_buf handlebars_stack_save(
    struct handlebars_stack * stack
) HBS_ATTR_NONNULL_ALL;

void handlebars_stack_restore(
    struct handlebars_stack * stack,
    struct handlebars_stack_save_buf buf
) HBS_ATTR_NONNULL_ALL;

#endif /* HANDLEBARS_STACK_H */
