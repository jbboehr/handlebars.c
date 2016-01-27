
#ifndef HANDLEBARS_STACK_H
#define HANDLEBARS_STACK_H

#include <stddef.h>

struct handlebars_value;

#define handlebars_stack_pop_type(stack, type) talloc_get_type(handlebars_stack_pop_ptr(stack), type)
#define handlebars_stack_top_type(stack, type) talloc_get_type(handlebars_stack_top_ptr(stack), type)
#define handlebars_stack_get_type(stack, index, type) talloc_get_type(handlebars_stack_get_ptr(stack, index), type)

struct handlebars_stack {
    void * ctx;
    size_t i;
    struct handlebars_value ** v;
};

#define handlebars_stack_foreach(stack, el, i) \
    for( i = 0, el = stack->v[0]; i < stack->i; i++, el = stack->v[i] )

struct handlebars_stack * handlebars_stack_ctor(void * ctx);
void handlebars_stack_dtor(struct handlebars_stack * stack);
size_t handlebars_stack_length(struct handlebars_stack * stack);

struct handlebars_value * handlebars_stack_push(struct handlebars_stack * stack, struct handlebars_value * value);
struct handlebars_value * handlebars_stack_pop(struct handlebars_stack * stack);
struct handlebars_value * handlebars_stack_top(struct handlebars_stack * stack);
struct handlebars_value * handlebars_stack_get(struct handlebars_stack * stack, size_t offset);
struct handlebars_value * handlebars_stack_set(struct handlebars_stack * stack, size_t offset, struct handlebars_value * value);
void handlebars_stack_reverse(struct handlebars_stack * stack);

void * handlebars_stack_push_ptr(struct handlebars_stack * stack, void * value);
void * handlebars_stack_pop_ptr(struct handlebars_stack * stack);
void * handlebars_stack_top_ptr(struct handlebars_stack * stack);
void * handlebars_stack_get_ptr(struct handlebars_stack * stack, size_t offset);

#endif
