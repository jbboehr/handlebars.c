
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <stdlib.h>
#include <talloc.h>

#include "handlebars_context.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"
#include "handlebars_stack.h"
#include "handlebars_value.h"



#define CONTEXT ctx

struct handlebars_stack * handlebars_stack_ctor(struct handlebars_context * ctx)
{
    struct handlebars_stack * stack = MC(handlebars_talloc_zero(ctx, struct handlebars_stack));
    stack->ctx = ctx;
    stack->i = 0;
    stack->v = MC(handlebars_talloc_array(stack, struct handlebars_value *, 32));
    return stack;
}

#undef CONTEXT
#define CONTEXT stack->ctx

void handlebars_stack_dtor(struct handlebars_stack * stack)
{
    size_t i;
    for( i = 0; i < stack->i; i++ ) {
        struct handlebars_value * value = stack->v[i];
        handlebars_value_delref(value);
    }
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

    s = talloc_array_length(stack->v);

    // Resize array if necessary
    if( stack->i <= s ) {
        do {
            s += 32;
        } while( s <= stack->i );
        nv = MC(handlebars_talloc_realloc(stack, stack->v, struct handlebars_value *, s));
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
        handlebars_context_throw(stack->ctx, HANDLEBARS_STACK_OVERFLOW, "Out-of-bounds");
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


void * handlebars_stack_push_ptr(struct handlebars_stack * stack, void * ptr)
{
    struct handlebars_value * value = handlebars_value_ctor(stack->ctx);

    assert(value != NULL);
    assert(ptr != NULL);

    value->type = HANDLEBARS_VALUE_TYPE_PTR;
    value->v.ptr = ptr;

    handlebars_stack_push(stack, value);

    return ptr;
}

void * handlebars_stack_pop_ptr(struct handlebars_stack * stack)
{
    struct handlebars_value * value = handlebars_stack_pop(stack);
    void * ptr;

    if( unlikely(value == NULL || value->type != HANDLEBARS_VALUE_TYPE_PTR) ) {
        return NULL;
    }

    // @todo this may not be safe if ptr is owned by value - steal?
    ptr = value->v.ptr;
    handlebars_value_delref(value);
    return ptr;
}

void * handlebars_stack_top_ptr(struct handlebars_stack * stack)
{
    struct handlebars_value * value = handlebars_stack_top(stack);
    void * ptr;

    if( unlikely(value == NULL || value->type != HANDLEBARS_VALUE_TYPE_PTR) ) {
        return NULL;
    }

    // @todo this may not be safe if ptr is owned by value - steal?
    ptr = value->v.ptr;
    handlebars_value_delref(value);
    return ptr;
}

void * handlebars_stack_get_ptr(struct handlebars_stack * stack, size_t offset)
{
    struct handlebars_value * value = handlebars_stack_get(stack, offset);
    void * ptr;

    if( unlikely(value == NULL || value->type != HANDLEBARS_VALUE_TYPE_PTR) ) {
        return NULL;
    }

    // @todo this may not be safe if ptr is owned by value - steal?
    ptr = value->v.ptr;
    handlebars_value_delref(value);
    return ptr;
}
