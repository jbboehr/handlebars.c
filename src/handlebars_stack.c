
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



#undef CONTEXT
#define CONTEXT HBSCTX(ctx)

struct handlebars_stack * handlebars_stack_ctor(struct handlebars_context * ctx)
{
    struct handlebars_stack * stack = MC(handlebars_talloc_zero(ctx, struct handlebars_stack));
    stack->ctx = CONTEXT;
    stack->i = 0;
    stack->s = 32;
    stack->v = MC(handlebars_talloc_array(stack, struct handlebars_value *, stack->s));
    return stack;
}

#undef CONTEXT
#define CONTEXT HBSCTX(stack->ctx)

void handlebars_stack_dtor(struct handlebars_stack * stack)
{
#ifndef HANDLEBARS_NO_REFCOUNT
    size_t i;
    for( i = 0; i < stack->i; i++ ) {
        struct handlebars_value * value = stack->v[i];
        handlebars_value_delref(value);
    }
#endif
    handlebars_talloc_free(stack);
}

size_t handlebars_stack_length(struct handlebars_stack * stack)
{
    return stack->i;
}

struct handlebars_value * handlebars_stack_push(struct handlebars_stack * stack, struct handlebars_value * value)
{
    size_t s;
    struct handlebars_value ** nv;

    assert(stack != NULL);
    assert(value != NULL);

    s = stack->s;

    // Resize array if necessary
    if( stack->i <= s ) {
        do {
            s += 32;
        } while( s <= stack->i );
        nv = MC(handlebars_talloc_realloc(stack, stack->v, struct handlebars_value *, s));
        stack->s = s;
        stack->v = nv;
    }

    stack->v[stack->i++] = value;

    handlebars_value_addref(value);

    return value;

}

struct handlebars_value * handlebars_stack_pop(struct handlebars_stack * stack)
{
    if( stack->i <= 0 ) {
        return NULL;
    }

    return stack->v[--stack->i];
}

struct handlebars_value * handlebars_stack_top(struct handlebars_stack * stack)
{
    struct handlebars_value * value;

    if( stack->i <= 0 ) {
        return NULL;
    }

    value = stack->v[stack->i - 1];
    handlebars_value_addref(value);
    return value;
}

struct handlebars_value * handlebars_stack_get(struct handlebars_stack * stack, size_t offset)
{
    struct handlebars_value * value;

    if( offset >= stack->i ) {
        return NULL;
    }

    value = stack->v[offset];
    handlebars_value_addref(value);
    return value;
}

struct handlebars_value * handlebars_stack_set(struct handlebars_stack * stack, size_t offset, struct handlebars_value * value)
{
    struct handlebars_value * old;

    assert(value != NULL);

    // As a special case, push
    if( offset == stack->i ) {
        return handlebars_stack_push(stack, value);
    }

    // Out-of-bounds
    if( offset >= stack->i ) {
        handlebars_throw(CONTEXT, HANDLEBARS_STACK_OVERFLOW, "Out-of-bounds");
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

void handlebars_stack_reverse(struct handlebars_stack * stack)
{
    size_t start = 0;
    size_t end = stack->i - 1;
    struct handlebars_value * tmp;

    while( start < end ) {
        tmp = stack->v[start];
        stack->v[start] = stack->v[end];
        stack->v[end] = tmp;
        start++;
        end--;
    }
}
