
#ifndef HANDLEBARS_VM_H
#define HANDLEBARS_VM_H

#include <setjmp.h>
#include "handlebars.h"

#ifdef	__cplusplus
extern "C" {
#endif

struct handlebars_compiler;
struct handlebars_context;
struct handlebars_map;
struct handlebars_options;

#ifndef HANDLEBARS_VM_STACK_SIZE
#define HANDLEBARS_VM_STACK_SIZE 96
#endif

typedef void (*handlebars_log_func)(
    int argc,
    struct handlebars_value * argv[],
    struct handlebars_options * options
);

struct handlebars_vm_stack {
    size_t i;
    struct handlebars_value * v[HANDLEBARS_VM_STACK_SIZE];
};

struct handlebars_vm {
    struct handlebars_context ctx;
    struct handlebars_cache * cache;

    struct handlebars_compiler ** programs;
    size_t guid_index;
    long depth;
    long flags;
    handlebars_log_func log_func;
    void * log_ctx;

    char * buffer;
    struct handlebars_value * context;
    struct handlebars_value * data;
    struct handlebars_value * helpers;
    struct handlebars_value * partials;

    struct handlebars_string * last_helper;
    struct handlebars_value * last_context;

    struct handlebars_vm_stack stack;
    struct handlebars_vm_stack contextStack;
    struct handlebars_vm_stack hashStack;
    struct handlebars_vm_stack blockParamStack;
};

struct handlebars_vm * handlebars_vm_ctor(struct handlebars_context * ctx) HBS_ATTR_RETURNS_NONNULL;
void handlebars_vm_dtor(struct handlebars_vm * vm);

void handlebars_vm_execute(
		struct handlebars_vm * vm, struct handlebars_compiler * compiler,
		struct handlebars_value * context);

char * handlebars_vm_execute_program(
        struct handlebars_vm * vm, long program, struct handlebars_value * context);

char * handlebars_vm_execute_program_ex(
        struct handlebars_vm * vm, long program, struct handlebars_value * context,
        struct handlebars_value * data, struct handlebars_value * block_params);

#ifdef	__cplusplus
}
#endif

#endif
