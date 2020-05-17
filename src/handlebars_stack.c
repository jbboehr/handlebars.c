/**
 * Copyright (C) 2016 John Boehr
 *
 * This file is part of handlebars.c.
 *
 * handlebars.c is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * handlebars.c is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with handlebars.c.  If not, see <http://www.gnu.org/licenses/>.
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
    struct handlebars_value * v[];
};

void * HANDLEBARS_STACK_ALLOC_PTR = NULL;

size_t handlebars_stack_size(size_t capacity) {
    return (sizeof(struct handlebars_stack) + sizeof(struct handlebars_value *) * (capacity + 1));
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
    handlebars_rc_delref(&stack->rc);
#endif
}

static inline struct handlebars_stack * stack_separate(struct handlebars_stack * stack) {
#ifndef HANDLEBARS_NO_REFCOUNT
    if (handlebars_rc_refcount(&stack->rc) > 1) {
        struct handlebars_stack * prev_stack = stack;
        stack = handlebars_stack_copy_ctor(stack);
        handlebars_stack_delref(prev_stack);
    }
#endif

    return stack;
}

// }}} Reference Counting

// {{{ Constructors and Destructors

struct handlebars_stack * handlebars_stack_init(struct handlebars_context * ctx, struct handlebars_stack * stack, size_t elem)
{
    memset(stack, 0, handlebars_stack_size(elem));
    stack->ctx = ctx;
    stack->capacity = elem;
#ifndef HANDLEBARS_NO_REFCOUNT
    handlebars_rc_init(&stack->rc, stack_rc_dtor);
#endif
    return stack;
}

struct handlebars_stack * handlebars_stack_ctor(struct handlebars_context * ctx, size_t capacity)
{
    struct handlebars_stack * stack = handlebars_talloc_zero_size(ctx, handlebars_stack_size(capacity));
    HANDLEBARS_MEMCHECK(stack, ctx);
    talloc_set_type(stack, struct handlebars_stack);
    stack->ctx = ctx;
    stack->capacity = capacity;
    stack->flags = HANDLEBARS_STACK_TALLOCATED;
#ifndef HANDLEBARS_NO_REFCOUNT
    handlebars_rc_init(&stack->rc, stack_rc_dtor);
#endif
    return stack;
}

struct handlebars_stack * handlebars_stack_copy_ctor(struct handlebars_stack * prev_stack)
{
    struct handlebars_stack * stack = handlebars_stack_ctor(prev_stack->ctx, prev_stack->i);
    for ( size_t i = 0; i < prev_stack->i; i++ ) {
        stack->v[i] = prev_stack->v[i];
        handlebars_value_addref(stack->v[i]);
    }
    stack->i = prev_stack->i;
    stack->v[stack->i] = NULL;
    return stack;
}

void handlebars_stack_dtor(struct handlebars_stack * stack)
{
#ifndef HANDLEBARS_NO_REFCOUNT
    size_t i;
    for( i = 0; i < stack->i; i++ ) {
        handlebars_value_delref(stack->v[i]);
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
        fprintf(stderr, "STACK[%zu]: %s\n", i, handlebars_value_dump(stack->v[i], 0));
    }
}

struct handlebars_stack * handlebars_stack_push(struct handlebars_stack * stack, struct handlebars_value * value)
{
    assert(stack != NULL);
    assert(value != NULL);

    // Separate if refcount > 1
    stack = stack_separate(stack);

    // Resize array if necessary
    if( stack->capacity <= stack->i ) {
        // Cannot resize if this stack was stack allocated
        if (!(stack->flags & HANDLEBARS_STACK_TALLOCATED)) {
            handlebars_throw(stack->ctx, HANDLEBARS_STACK_OVERFLOW, "Stack overflow");
        }

        size_t capacity = (stack->capacity | 3) * 3 / 2;
        struct handlebars_context * ctx = stack->ctx;
#ifndef HANDLEBARS_NO_REFCOUNT
        stack = handlebars_talloc_realloc_size(NULL, stack, handlebars_stack_size(capacity));
#else
        // We're going to be hemorrhaging memory when refcounting is disabled
        struct handlebars_stack * prev_stack = stack;
        stack = handlebars_talloc_size(ctx, handlebars_stack_size(capacity));
        memcpy(stack, prev_stack, handlebars_stack_size(prev_stack->capacity));
#endif
        HANDLEBARS_MEMCHECK(stack, ctx);
        talloc_set_type(stack, struct handlebars_stack);
        stack->capacity = capacity;
    }

    handlebars_value_addref(value);

    stack->v[stack->i++] = value;
    stack->v[stack->i] = NULL; // we don't *really* need to null-terminate

    return stack;
}

struct handlebars_value * handlebars_stack_pop(struct handlebars_stack * stack)
{
    if( stack->i <= 0 ) {
        return NULL;
    }

    // @TODO we need to change the API of pop to support separation
#ifndef HANDLEBARS_NO_REFCOUNT
    if (handlebars_rc_refcount(&stack->rc) > 1) {
        handlebars_throw(stack->ctx, HANDLEBARS_ERROR, "Attempting to pop protected stack with refcout > 1");
    }
#endif

    if (stack->i < stack->protect) {
        handlebars_throw(stack->ctx, HANDLEBARS_STACK_OVERFLOW, "Attempting to pop protected stack segment i=%zu protect=%zu", stack->i, stack->protect);
    }

    struct handlebars_value * value = stack->v[--stack->i];
    stack->v[stack->i] = NULL; // we don't *really* need to null-terminate
    return value;
}

struct handlebars_value * handlebars_stack_top(struct handlebars_stack * stack)
{
    if( stack->i <= 0 ) {
        return NULL;
    }

    return stack->v[stack->i - 1];
}

struct handlebars_value * handlebars_stack_get(struct handlebars_stack * stack, size_t offset)
{
    if( offset >= stack->i ) {
        return NULL;
    }

    return stack->v[offset];
}

struct handlebars_stack * handlebars_stack_set(struct handlebars_stack * stack, size_t offset, struct handlebars_value * value)
{
    struct handlebars_value * old;

    assert(value != NULL);

    // As a special case, push
    if( offset == stack->i ) {
        return handlebars_stack_push(stack, value);
    }

    // Out-of-bounds
    if( offset >= stack->i ) {
        handlebars_throw(stack->ctx, HANDLEBARS_STACK_OVERFLOW, "Out-of-bounds %zu/%zu", offset, stack->i);
    }

    // Protect
    if (offset < stack->protect) {
        handlebars_throw(stack->ctx, HANDLEBARS_STACK_OVERFLOW, "Attempting to set protected stack segment offset=%zu protect=%zu", offset, stack->protect);
    }

    // As a special case, ignore
    if( value == stack->v[offset] ) {
        return stack;
    }

    old = stack->v[offset];
    stack->v[offset] = value;

    handlebars_value_addref(value);
    handlebars_value_delref(old);

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
    while (handlebars_stack_count(stack) > num) {
        struct handlebars_value * value = handlebars_stack_pop(stack);
        handlebars_value_delref(value);
    }
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
