
#include <stdlib.h>
#include <talloc.h>

#include "handlebars_memory.h"
#include "handlebars_private.h"
#include "handlebars_stack.h"
#include "handlebars_value.h"

struct handlebars_stack * handlebars_stack_ctor(void * ctx)
{
    struct handlebars_stack * stack = handlebars_talloc(ctx, struct handlebars_stack);
    if( stack ) {
        stack->ctx = ctx;
        stack->i = 0;
        stack->v = handlebars_talloc_array(stack, void *, 32);
    }
    return stack;
}

void handlebars_stack_dtor(struct handlebars_stack * stack)
{
    handlebars_talloc_free(stack);
}

size_t handlebars_stack_length(struct handlebars_stack * stack)
{
    return stack->i;
}

struct handlebars_value * handlebars_stack_push(struct handlebars_stack * stack, struct handlebars_value * value)
{
    size_t s = talloc_array_length(stack->v);

    // Resize array if necessary
    if( stack->i <= s ) {
        do {
            s += 32;
        } while( s <= stack->i );
        struct handlebars_value ** nv = handlebars_talloc_realloc(stack, stack->v, struct handlebars_value *, s);
        if( !nv ) {
            return NULL;
        }
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

    if( offset >= stack->i || offset < 0 ) {
        return NULL;
    }

    value = stack->v[offset];
    handlebars_value_addref(value);
    return value;
}

void * handlebars_stack_push_ptr(struct handlebars_stack * stack, void * ptr)
{
    struct handlebars_value * value = handlebars_value_ctor(stack->ctx);

    if( unlikely(value == NULL) ) {
        return NULL;
    }

    value->type = HANDLEBARS_VALUE_TYPE_PTR;
    value->v.ptr = ptr;

    value = handlebars_stack_push(stack, value);

    if( unlikely(value == NULL) ) {
        ptr = NULL;
    }

    return ptr;
}

void * handlebars_stack_pop_ptr(struct handlebars_stack * stack)
{
    struct handlebars_value * value = handlebars_stack_pop(stack);

    if( unlikely(value == NULL || value->type != HANDLEBARS_VALUE_TYPE_PTR) ) {
        return NULL;
    }

    return value->v.ptr;
}

void * handlebars_stack_top_ptr(struct handlebars_stack * stack)
{
    struct handlebars_value * value = handlebars_stack_top(stack);

    if( unlikely(value == NULL || value->type != HANDLEBARS_VALUE_TYPE_PTR) ) {
        return NULL;
    }

    return value->v.ptr;
}

void * handlebars_stack_get_ptr(struct handlebars_stack * stack, size_t offset)
{
    struct handlebars_value * value = handlebars_stack_get(stack, offset);

    if( unlikely(value == NULL || value->type != HANDLEBARS_VALUE_TYPE_PTR) ) {
        return NULL;
    }

    return value->v.ptr;
}
