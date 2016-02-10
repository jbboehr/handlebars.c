
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

struct handlebars_vm_frame {
    struct handlebars_value * context;
    struct handlebars_value * data;
    int program;
    char * buffer;
    struct handlebars_value * last_context;
    struct handlebars_options * options_register;
};

struct handlebars_vm {
    struct handlebars_context * ctx;
    struct handlebars_compiler ** programs;
    size_t guid_index;
    long depth;

    struct handlebars_value * context;
    struct handlebars_value * data;
    struct handlebars_value * helpers;
	struct handlebars_value * partials;
    long flags;

    const char * last_helper;
    struct handlebars_value * last_context;
    char * buffer;
    struct handlebars_stack * frameStack;
    struct handlebars_stack * depths;
    struct handlebars_stack * stack;
    struct handlebars_stack * hashStack;
    struct handlebars_stack * blockParamStack;
};

struct handlebars_vm * handlebars_vm_ctor(struct handlebars_context * ctx);

void handlebars_vm_execute(
		struct handlebars_vm * vm, struct handlebars_compiler * compiler,
		struct handlebars_value * context);

char * handlebars_vm_execute_program(
        struct handlebars_vm * vm, int program, struct handlebars_value * context);

char * handlebars_vm_execute_program_ex(
        struct handlebars_vm * vm, int program, struct handlebars_value * context,
        struct handlebars_value * data, struct handlebars_value * block_params);

void handlebars_vm_throw(struct handlebars_vm * vm, long num, const char * msg) HBS_NORETURN;

#ifdef	__cplusplus
}
#endif

#endif
