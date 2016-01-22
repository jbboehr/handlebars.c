
#ifndef HANDLEBARS_STACK_H
#define HANDLEBARS_STACK_H

#include <stddef.h>

#define handlebars_stack_pop_type(stack, type) talloc_get_type(handlebars_stack_pop(stack), type)
#define handlebars_stack_top_type(stack, type) talloc_get_type(handlebars_stack_top(stack), type)
#define handlebars_stack_get_type(stack, index, type) talloc_get_type(handlebars_stack_get(stack, index), type)

struct handlebars_stack {
    size_t i;
    void ** v;
};

struct handlebars_stack * handlebars_stack_ctor(void * ctx);
void handlebars_stack_dtor(struct handlebars_stack * stack);
void * handlebars_stack_push(struct handlebars_stack * stack, void * value);
void * handlebars_stack_pop(struct handlebars_stack * stack);
void * handlebars_stack_top(struct handlebars_stack * stack);
void * handlebars_stack_get(struct handlebars_stack * stack, size_t offset);
size_t handlebars_stack_length(struct handlebars_stack * stack);

#endif
