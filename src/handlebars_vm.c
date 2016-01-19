
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "handlebars_compiler.h"
#include "handlebars_data.h"
#include "handlebars_memory.h"
#include "handlebars_opcodes.h"

#include "handlebars_vm.h"


#define FRAME vm->frameStack.v[vm->frameStack.s - 1]


struct handlebars_vm * handlebars_vm_ctor(void * ctx)
{
	return handlebars_talloc_zero(ctx, struct handlebars_vm);
}

void handlebars_vm_accept(struct handlebars_vm * vm, struct handlebars_compiler * compiler)
{
	size_t i;

	for( i = 0; i < compiler->opcodes_length; i++ ) {
		struct handlebars_opcode * opcode = compiler->opcodes[i];
	}
}

void handlebars_vm_execute_program(struct handlebars_vm * vm, int program, struct handlebars_value * context)
{
	struct handlebars_compiler * compiler = vm->opcodes->children[program];
	struct handlebars_vm_frame * frame = &vm->frameStack.v[vm->frameStack.s++];

	frame->context = context;

	handlebars_vm_accept(vm, compiler);
}

void handlebars_vm_execute(
		struct handlebars_vm * vm, struct handlebars_compiler * compiler,
		struct handlebars_value * context)
{
	vm->opcodes = compiler;
	vm->context = context;

	handlebars_vm_execute_program(vm, 0, context);
}
