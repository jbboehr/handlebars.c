
#ifndef HANDLEBARS_VM_H
#define HANDLEBARS_VM_H

struct handlebars_compiler;

struct handlebars_vm_frame {
	struct handlebars_value * context;
	char * buffer;
};

struct handlebars_vm {
	struct handlebars_compiler * opcodes;
	struct handlebars_value * context;
	struct {
		size_t s;
		struct handlebars_vm_frame v[128];
	} frameStack;
};

struct handlebars_vm * handlebars_vm_ctor(void * ctx);

void handlebars_vm_execute(
		struct handlebars_vm * vm, struct handlebars_compiler * compiler,
		struct handlebars_value * context);

#endif
