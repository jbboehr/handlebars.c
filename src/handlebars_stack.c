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

#include <assert.h>
#include <stdlib.h>
#include <talloc.h>

#include "handlebars.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"
#include "handlebars_value_private.h"

#include "handlebars_stack.h"
#include "handlebars_value.h"

#ifndef HANDLEBARS_NO_REFCOUNT
#include "handlebars_rc.h"
#endif



enum handlebars_stack_flags {
    HANDLEBARS_STACK_TALLOCATED = 1,
};

struct handlebars_stack {
    //! Handlebars context
    struct handlebars_context * ctx;
#ifndef HANDLEBARS_NO_REFCOUNT
    struct handlebars_rc rc;
#endif
    //! Number of elements in the stack
    size_t i;
    //! Currently available number of elements (size of the buffer)
    size_t capacity;
    //! Flags from enum #handlebars_stack_flags
    unsigned flags;
    //! Number of elements to protect from popping or setting
    size_t protect;
    //! Data
    struct handlebars_value v[];
};

void * HANDLEBARS_STACK_ALLOC_PTR = NULL;

size_t handlebars_stack_size(size_t capacity) {
    return (sizeof(struct handlebars_stack) + sizeof(struct handlebars_value) * (capacity + 1));
}

// {{{ Reference Counting

#ifndef HANDLEBARS_NO_REFCOUNT
static void stack_rc_dtor(struct handlebars_rc * rc)
{
    struct handlebars_stack * stack = talloc_get_type_abort(hbs_container_of(rc, struct handlebars_stack, rc), struct handlebars_stack);
    handlebars_stack_dtor(stack);
}
#endif

void handlebars_stack_addref(struct handlebars_stack * stack)
{
#ifndef HANDLEBARS_NO_REFCOUNT
    handlebars_rc_addref(&stack->rc);
#endif
}

void handlebars_stack_delref(struct handlebars_stack * stack)
{
#ifndef HANDLEBARS_NO_REFCOUNT
    handlebars_rc_delref(&stack->rc, stack_rc_dtor);
#endif
}

static inline struct handlebars_stack * stack_separate(struct handlebars_stack * stack) {
#ifndef HANDLEBARS_NO_REFCOUNT
    if (handlebars_rc_refcount(&stack->rc) > 1) {
        // Cannot resize if this stack was stack allocated
        if (!(stack->flags & HANDLEBARS_STACK_TALLOCATED)) {
            handlebars_throw(stack->ctx, HANDLEBARS_STACK_OVERFLOW, "Stack overflow");
        }
        struct handlebars_stack * prev_stack = stack;
        stack = handlebars_stack_copy_ctor(stack, prev_stack->capacity);
        handlebars_stack_delref(prev_stack);
        handlebars_stack_addref(stack);
    }
#endif

    return stack;
}

// }}} Reference Counting

// {{{ Constructors and Destructors

struct handlebars_stack * handlebars_stack_init(struct handlebars_context * ctx, struct handlebars_stack * stack, size_t elem)
{
    memset(stack, 0, sizeof(struct handlebars_stack));
    stack->ctx = ctx;
    stack->capacity = elem;
#ifndef HANDLEBARS_NO_REFCOUNT
    handlebars_rc_init(&stack->rc);
#endif
    return stack;
}

struct handlebars_stack * handlebars_stack_ctor(struct handlebars_context * ctx, size_t capacity)
{
    struct handlebars_stack * stack = handlebars_talloc_size(ctx, handlebars_stack_size(capacity));
    HANDLEBARS_MEMCHECK(stack, ctx);
    talloc_set_type(stack, struct handlebars_stack);
    memset(stack, 0, sizeof(struct handlebars_stack));
    stack->ctx = ctx;
    stack->capacity = capacity;
    stack->flags = HANDLEBARS_STACK_TALLOCATED;
#ifndef HANDLEBARS_NO_REFCOUNT
    handlebars_rc_init(&stack->rc);
#endif
    return stack;
}

struct handlebars_stack * handlebars_stack_copy_ctor(struct handlebars_stack * prev_stack, size_t new_capacity)
{
    if (new_capacity < prev_stack->i) {
        new_capacity = prev_stack->i;
    }

    struct handlebars_stack * stack = handlebars_stack_ctor(prev_stack->ctx, new_capacity);
    for ( size_t i = 0; i < prev_stack->i; i++ ) {
        handlebars_value_init(&stack->v[i]);
        handlebars_value_value(&stack->v[i], &prev_stack->v[i]);
    }
    stack->i = prev_stack->i;
    return stack;
}

void handlebars_stack_dtor(struct handlebars_stack * stack)
{
#ifndef HANDLEBARS_NO_REFCOUNT
    size_t i;
    for( i = 0; i < stack->i; i++ ) {
        handlebars_value_dtor(&stack->v[i]);
    }
#endif
    if (stack->flags & HANDLEBARS_STACK_TALLOCATED) {
        handlebars_talloc_free(stack);
    }
}

// }}} Constructors and Destructors

size_t handlebars_stack_count(struct handlebars_stack * stack)
{
    return stack->i;
}

HBS_ATTR_UNUSED
static void handlebars_stack_dump(struct handlebars_stack * stack) {
    size_t i;
    for (i = 0; i < stack->i; i++) {
        fprintf(stderr, "STACK[%zu]: %s\n", i, handlebars_value_dump(&stack->v[i], stack->ctx, 0));
    }
}

struct handlebars_stack * handlebars_stack_push(struct handlebars_stack * stack, struct handlebars_value * value)
{
    assert(stack != NULL);
    assert(value != NULL);

    if( stack->capacity > stack->i ) {
        // Separate if refcount > 1
        stack = stack_separate(stack);
    } else {
        // Resize array if necessary

        // Cannot resize if this stack was stack allocated
        if (!(stack->flags & HANDLEBARS_STACK_TALLOCATED)) {
            handlebars_throw(stack->ctx, HANDLEBARS_STACK_OVERFLOW, "Stack overflow");
        }

        size_t capacity = (stack->capacity | 3) * 3 / 2;
        struct handlebars_stack * prev_stack = stack;
        stack = handlebars_stack_copy_ctor(prev_stack, capacity);
        handlebars_stack_delref(prev_stack);
        handlebars_stack_addref(stack);
    }

    handlebars_value_init(&stack->v[stack->i]);
    handlebars_value_value(&stack->v[stack->i], value);
    stack->i++;

    return stack;
}

struct handlebars_value * handlebars_stack_pop(struct handlebars_stack * stack, struct handlebars_value * rv)
{
    if( stack->i <= 0 ) {
        return NULL;
    }

    // we need to change the API of pop to support separation, but we're not really using it anywhere
#ifndef HANDLEBARS_NO_REFCOUNT
    if (handlebars_rc_refcount(&stack->rc) > 1) {
        handlebars_throw(stack->ctx, HANDLEBARS_ERROR, "Attempting to pop protected stack with refcount > 1");
    }
#endif

    if (stack->i < stack->protect) {
        handlebars_throw(stack->ctx, HANDLEBARS_STACK_OVERFLOW, "Attempting to pop protected stack segment i=%zu protect=%zu", stack->i, stack->protect);
    }

    --stack->i;
    struct handlebars_value * value = &stack->v[stack->i];
    handlebars_value_value(rv, value);
    handlebars_value_null(value);
    return rv;
}

struct handlebars_value * handlebars_stack_top(struct handlebars_stack * stack)
{
    if( stack->i <= 0 ) {
        return NULL;
    }

    return &stack->v[stack->i - 1];
}

struct handlebars_value * handlebars_stack_get(struct handlebars_stack * stack, size_t offset)
{
    if( offset >= stack->i ) {
        return NULL;
    }

    return &stack->v[offset];
}

struct handlebars_stack * handlebars_stack_set(struct handlebars_stack * stack, size_t offset, struct handlebars_value * value)
{
    assert(value != NULL);

    // As a special case, push
    if( offset == stack->i ) {
        return handlebars_stack_push(stack, value);
    }

    // Separate if refcount > 1
    stack = stack_separate(stack);

    // Out-of-bounds
    if( offset >= stack->i ) {
        handlebars_throw(stack->ctx, HANDLEBARS_STACK_OVERFLOW, "Out-of-bounds %zu/%zu", offset, stack->i);
    }

    // Protect
    if (offset < stack->protect) {
        handlebars_throw(stack->ctx, HANDLEBARS_STACK_OVERFLOW, "Attempting to set protected stack segment offset=%zu protect=%zu", offset, stack->protect);
    }

    // As a special case, ignore
    if( value == &stack->v[offset] ) {
        return stack;
    }

    handlebars_value_value(&stack->v[offset], value);

    return stack;
}

size_t handlebars_stack_protect(
    struct handlebars_stack * stack,
    size_t num
) {
    size_t old = stack->protect;
    stack->protect = num;
    return old;
};

void handlebars_stack_truncate(
    struct handlebars_stack * stack,
    size_t num
) {
    HANDLEBARS_VALUE_DECL(rv);
    while (handlebars_stack_count(stack) > num) {
        struct handlebars_value * value = handlebars_stack_pop(stack, rv);
        handlebars_value_dtor(value);
    }
    HANDLEBARS_VALUE_UNDECL(rv);
}

struct handlebars_stack_save_buf handlebars_stack_save(struct handlebars_stack * stack)
{
    struct handlebars_stack_save_buf buf;
    buf.count = handlebars_stack_count(stack);
    buf.protect = handlebars_stack_protect(stack, buf.count);
    return buf;
}

void handlebars_stack_restore(struct handlebars_stack * stack, struct handlebars_stack_save_buf buf)
{
    handlebars_stack_protect(stack, buf.protect);
    handlebars_stack_truncate(stack, buf.count);
}
