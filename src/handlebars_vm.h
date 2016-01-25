
#ifndef HANDLEBARS_VM_H
#define HANDLEBARS_VM_H

struct handlebars_compiler;
struct handlebars_map;

struct handlebars_vm_frame {
	struct handlebars_value * context;
	int program;
	char * buffer;
    struct handlebars_value * last_context;
};

struct handlebars_vm {
	//struct handlebars_compiler * opcodes;
    struct handlebars_compiler ** programs;
    size_t guid_index;

	struct handlebars_map * helpers;

    struct handlebars_value * last_context;
	struct handlebars_value * context;
	char * buffer;
    struct handlebars_stack * frameStack;
    struct handlebars_stack * depths;
    struct handlebars_stack * stack;
    struct handlebars_stack * hashStack;
};

struct handlebars_vm * handlebars_vm_ctor(void * ctx);

void handlebars_vm_execute(
		struct handlebars_vm * vm, struct handlebars_compiler * compiler,
		struct handlebars_value * context);

#endif
