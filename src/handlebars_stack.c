
#include <stdlib.h>
#include <talloc.h>

#include "handlebars_memory.h"
#include "handlebars_stack.h"

struct handlebars_stack * handlebars_stack_ctor(void * ctx)
{
    struct handlebars_stack * stack = handlebars_talloc(ctx, struct handlebars_stack);
    if( stack ) {
        stack->i = 0;
        stack->v = handlebars_talloc_array(stack, void *, 32);
    }
    return stack;
}

void handlebars_stack_dtor(struct handlebars_stack * stack)
{
    handlebars_talloc_free(stack);
}

void * handlebars_stack_push(struct handlebars_stack * stack, void * value)
{
    size_t s = talloc_array_length(stack->v);

    // Resize array if necessary
    if( stack->i <= s ) {
        do {
            s += 32;
        } while( s <= stack->i );
        stack->v = handlebars_talloc_realloc(stack, stack->v, void *, s);
    }

    stack->v[stack->i++] = value;

    return value;
}

void * handlebars_stack_pop(struct handlebars_stack * stack)
{
    if( stack->i <= 0 ) {
        return NULL;
    }

    return stack->v[--stack->i];
}

void * handlebars_stack_top(struct handlebars_stack * stack)
{
    if( stack->i <= 0 ) {
        return NULL;
    }

    return stack->v[stack->i - 1];
}

void * handlebars_stack_get(struct handlebars_stack * stack, size_t offset)
{
    if( offset >= stack->i ) {
        return NULL;
    }

    return stack->v[offset];
}

size_t handlebars_stack_length(struct handlebars_stack * stack)
{
    return stack->i;
}