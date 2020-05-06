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



enum handlebars_stack_flags {
    HANDLEBARS_STACK_TALLOCATED = 1,
};

struct handlebars_stack {
    //! Handlebars context
    struct handlebars_context * ctx;
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

struct handlebars_stack * handlebars_stack_init(struct handlebars_context * ctx, struct handlebars_stack * stack, size_t elem)
{
    memset(stack, 0, handlebars_stack_size(elem));
    stack->ctx = ctx;
    stack->capacity = elem;
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

size_t handlebars_stack_length(struct handlebars_stack * stack)
{
    return stack->i;
}

static void handlebars_stack_dump(struct handlebars_stack * stack) {
    size_t i;
    for (i = 0; i < stack->i; i++) {
        fprintf(stderr, "STACK[%lu]: %s\n", i, handlebars_value_dump(stack->v[i], 0));
    }
}

struct handlebars_stack * handlebars_stack_push(struct handlebars_stack * stack, struct handlebars_value * value)
{
    size_t capacity;

    assert(stack != NULL);
    assert(value != NULL);

    capacity = stack->capacity;

    // Resize array if necessary
    if( capacity <= stack->i ) {
        // Cannot resize if this stack was stack allocated
        if (!(stack->flags & HANDLEBARS_STACK_TALLOCATED)) {
            handlebars_throw(stack->ctx, HANDLEBARS_STACK_OVERFLOW, "Stack overflow");
        }
        // @TODO this is unsafe if the refcount of the stack is greater than one - but we need refcounting for that
        capacity += 32;
        stack = handlebars_talloc_realloc_size(NULL, stack, handlebars_stack_size(capacity));
        HANDLEBARS_MEMCHECK(stack, stack->ctx);
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

    if (stack->i < stack->protect) {
        handlebars_throw(stack->ctx, HANDLEBARS_STACK_OVERFLOW, "Attempting to pop protected stack segment i=%lu protect=%lu", stack->i, stack->protect);
    }

    struct handlebars_value * value = stack->v[--stack->i];
    stack->v[stack->i] = NULL; // we don't *really* need to null-terminate
    return value;
}

struct handlebars_value * handlebars_stack_top(struct handlebars_stack * stack)
{
    struct handlebars_value * value;

    if( stack->i <= 0 ) {
        return NULL;
    }

    return stack->v[stack->i - 1];
}

struct handlebars_value * handlebars_stack_get(struct handlebars_stack * stack, size_t offset)
{
    struct handlebars_value * value;

    if( offset >= stack->i ) {
        return NULL;
    }

    return stack->v[offset];
}

struct handlebars_value * handlebars_stack_set(struct handlebars_stack * stack, size_t offset, struct handlebars_value * value)
{
    struct handlebars_value * old;

    assert(value != NULL);

    // As a special case, push if it does not require a reallocation
    if( offset == stack->i && offset < stack->capacity ) {
        // @TODO should allow realloc here?
        handlebars_stack_push(stack, value);
        return value;
    }

    // Out-of-bounds
    if( offset >= stack->i ) {
        handlebars_throw(stack->ctx, HANDLEBARS_STACK_OVERFLOW, "Out-of-bounds %lu/%lu", offset, stack->i);
    }

    // Protect
    if (offset < stack->protect) {
        handlebars_throw(stack->ctx, HANDLEBARS_STACK_OVERFLOW, "Attempting to set protected stack segment offset=%lu protect=%lu", offset, stack->protect);
    }

    // As a special case, ignore
    if( value == stack->v[offset] ) {
        return value;
    }

    old = stack->v[offset];
    stack->v[offset] = value;

    handlebars_value_addref(value);
    handlebars_value_delref(old);

    return value;
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
        handlebars_stack_pop(stack);
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
