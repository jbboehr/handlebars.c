
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "handlebars_compiler.h"
#include "handlebars_data.h"
#include "handlebars_memory.h"
#include "handlebars_opcodes.h"
#include "handlebars_opcode_printer.h"
#include "handlebars_stack.h"

#include "handlebars_vm.h"


#define OPCODE_NAME(name) handlebars_opcode_type_ ## name
#define ACCEPT(name) case OPCODE_NAME(name) : ACCEPT_FN(name)(vm, opcode); break;
#define ACCEPT_FN(name) accept_ ## name
#define ACCEPT_NAMED_FUNCTION(name) static void name (struct handlebars_vm * vm, struct handlebars_opcode * opcode)
#define ACCEPT_FUNCTION(name) ACCEPT_NAMED_FUNCTION(ACCEPT_FN(name))

struct literal {
    char * value;
};

ACCEPT_FUNCTION(push_context);


struct handlebars_vm * handlebars_vm_ctor(void * ctx)
{
    TALLOC_CTX * tmp;
	struct handlebars_vm * vm = handlebars_talloc_zero(tmp, struct handlebars_vm);
    if( !vm ) {
        return NULL;
    }

    vm->frameStack = handlebars_stack_ctor(vm);
    vm->depths = handlebars_stack_ctor(vm);
    vm->stack = handlebars_stack_ctor(vm);
    vm->hashStack = handlebars_stack_ctor(vm);
    if( !vm->frameStack || !vm->depths || !vm->stack || !vm->hashStack ) {
        goto error;
    }

    talloc_steal(ctx, vm);
    return vm;
error:
    handlebars_talloc_free(tmp);
    return NULL;
}

ACCEPT_FUNCTION(append) {
    struct handlebars_vm_frame * frame = handlebars_stack_top_type(vm->frameStack, struct handlebars_vm_frame);
    struct handlebars_value * value = handlebars_stack_pop_type(vm->stack, struct handlebars_value);

    if( value ) {
        frame->buffer = handlebars_talloc_strdup_append_buffer(frame->buffer, handlebars_value_get_strval(value));
    }
}

ACCEPT_FUNCTION(append_escaped) {
    struct handlebars_vm_frame * frame = handlebars_stack_top_type(vm->frameStack, struct handlebars_vm_frame);
    struct handlebars_value * value = handlebars_stack_pop_type(vm->stack, struct handlebars_value);

    if( value ) {
        // @todo escape
        frame->buffer = handlebars_talloc_strdup_append_buffer(frame->buffer, handlebars_value_get_strval(value));
    }
}

ACCEPT_FUNCTION(append_content) {
    struct handlebars_vm_frame * frame = handlebars_stack_top_type(vm->frameStack, struct handlebars_vm_frame);

    assert(opcode->type == handlebars_opcode_type_append_content);
    assert(opcode->op1.type == handlebars_operand_type_string);

    frame->buffer = handlebars_talloc_strdup_append_buffer(frame->buffer, opcode->op1.data.stringval);
}

ACCEPT_FUNCTION(get_context) {
    struct handlebars_vm_frame * frame = handlebars_stack_top_type(vm->frameStack, struct handlebars_vm_frame);

    assert(opcode->type == handlebars_opcode_type_get_context);
    assert(opcode->op1.type == handlebars_operand_type_long);

    long depth = opcode->op1.data.longval;

    size_t length = handlebars_stack_length(vm->depths);
    if( depth >= length ) {
        // error
        // @todo
        assert(0);
    } else if( depth == 0 ) {
        vm->last_context = handlebars_stack_top_type(vm->depths, struct handlebars_value);
    } else {
        vm->last_context = handlebars_stack_get_type(vm->depths, length - depth, struct handlebars_value);
    }
}

ACCEPT_FUNCTION(invoke_ambiguous) {
    struct handlebars_vm_frame * frame = handlebars_stack_top_type(vm->frameStack, struct handlebars_vm_frame);
    struct handlebars_value * value = handlebars_stack_pop_type(vm->stack, struct handlebars_value);

    // @todo helper >.>

    //frame->buffer = handlebars_talloc_strdup_append_buffer(frame->buffer, handlebars_value_get_strval(value));
    handlebars_stack_push(vm->stack, value);
}

ACCEPT_FUNCTION(lookup_on_context) {
    assert(opcode->op1.type == handlebars_operand_type_array);
    assert(opcode->op2.type == handlebars_operand_type_boolean || opcode->op2.type == handlebars_operand_type_null);
    assert(opcode->op3.type == handlebars_operand_type_boolean || opcode->op3.type == handlebars_operand_type_null);
    assert(opcode->op4.type == handlebars_operand_type_boolean || opcode->op4.type == handlebars_operand_type_null);

    ACCEPT_FN(push_context)(vm, opcode);

    struct handlebars_value * value = handlebars_stack_top_type(vm->stack, struct handlebars_value);
    fprintf(stdout, "ARRGGGG %d %p\n", value ? handlebars_value_get_type(value) : -1, value);

    if( value ) {
        char **arr = opcode->op1.data.arrayval;
        do {
            value = handlebars_value_map_find(value, *arr, strlen(*arr));
            if (!value) {
                break;
            }
        } while (*++arr);
    }

    // meh
    if( !value ) {
        value = handlebars_value_ctor(vm);
    }

    fprintf(stdout, "ARRGGGG %s\n", handlebars_value_get_strval(value));

    handlebars_stack_pop(vm->stack);
    handlebars_stack_push(vm->stack, value);
}

ACCEPT_FUNCTION(push_context) {
    struct handlebars_vm_frame * frame = handlebars_stack_top_type(vm->frameStack, struct handlebars_vm_frame);

    assert(vm->last_context != NULL);

    handlebars_stack_push(vm->stack, vm->last_context);
}

ACCEPT_FUNCTION(push_program) {
    struct handlebars_vm_frame * frame = handlebars_stack_top_type(vm->frameStack, struct handlebars_vm_frame);

    assert(opcode->type == handlebars_opcode_type_push_program);

    long * program = handlebars_talloc(vm, long);
    if( opcode->op1.type == handlebars_operand_type_long ) {
        *program = opcode->op1.data.longval;
    } else {
        *program = -1;
    }

    handlebars_stack_push(vm->stack, program);
}

ACCEPT_FUNCTION(push_literal) {
    struct literal * lit = handlebars_talloc(vm, struct literal);
    handlebars_stack_push(vm->stack, lit);
}

void handlebars_vm_accept(struct handlebars_vm * vm, struct handlebars_compiler * compiler)
{
	size_t i;

	for( i = 0; i < compiler->opcodes_length; i++ ) {
		struct handlebars_opcode * opcode = compiler->opcodes[i];

        // Print opcode?
        char * tmp = handlebars_opcode_print(vm, opcode);
        fprintf(stdout, "OPCODE: %s\n", tmp);
        talloc_free(tmp);

		switch( opcode->type ) {
            ACCEPT(append)
            ACCEPT(append_escaped)
            ACCEPT(append_content);
            ACCEPT(get_context);
            ACCEPT(push_context);
            ACCEPT(push_program);
            ACCEPT(push_literal);
            ACCEPT(lookup_on_context);
            ACCEPT(invoke_ambiguous);

//            case handlebars_opcode_type_append:
//
//            case handlebars_opcode_type_ambiguous_block_value:
//            case handlebars_opcode_type_append_escaped:
//            case handlebars_opcode_type_empty_hash:
//            case handlebars_opcode_type_pop_hash:
//            case handlebars_opcode_type_push_context:
//            case handlebars_opcode_type_push_hash:
//            case handlebars_opcode_type_resolve_possible_lambda:
//            case handlebars_opcode_type_get_context:
//            case handlebars_opcode_type_push_program:
//            case handlebars_opcode_type_append_content:
//            case handlebars_opcode_type_assign_to_hash:
//            case handlebars_opcode_type_block_value:
//            case handlebars_opcode_type_push:
//            case handlebars_opcode_type_push_literal:
//            case handlebars_opcode_type_push_string:
//            case handlebars_opcode_type_invoke_partial:
//            case handlebars_opcode_type_push_id:
//            case handlebars_opcode_type_push_string_param:
//            case handlebars_opcode_type_invoke_ambiguous:
//            case handlebars_opcode_type_invoke_known_helper:
//            case handlebars_opcode_type_invoke_helper:
//            case handlebars_opcode_type_lookup_on_context:
//            case handlebars_opcode_type_lookup_data:
//            case handlebars_opcode_type_lookup_block_param:
//            case handlebars_opcode_type_register_decorator:
//                fprintf(stderr, "Unhandled opcode: %s\n", handlebars_opcode_readable_type(opcode->type));
//                break;
//
//            case handlebars_opcode_type_invalid:
//            case handlebars_opcode_type_nil:
            default:
                fprintf(stdout, "Unhandled opcode: %s\n", handlebars_opcode_readable_type(opcode->type));
                //assert(0);
                break;
        }
	}
}

char * handlebars_vm_execute_program(struct handlebars_vm * vm, int program, struct handlebars_value * context)
{
    //assert(program < vm->opcodes->children_length);
    if( program >= vm->guid_index ) {
        assert(0);
        // @todo error
        return NULL;
    }

	struct handlebars_compiler * compiler = vm->programs[program];

    // Push the frame stack
	struct handlebars_vm_frame * frame = handlebars_talloc_zero(vm, struct handlebars_vm_frame);
    handlebars_stack_push(vm->frameStack, frame);
    frame->buffer = handlebars_talloc_strdup(vm, "");

    // Set program
    frame->program = program;

    // Set context
	frame->context = context;

    // Push depths
    handlebars_stack_push(vm->depths, context);

    // Set data
    // @todo

    // Set block params
    // @todo

    // Execute the program
	handlebars_vm_accept(vm, compiler);

    // Pop depths
    handlebars_stack_pop(vm->depths);

    return frame->buffer;
}

static void preprocess_opcode(struct handlebars_vm * vm, struct handlebars_opcode * opcode, struct handlebars_compiler * compiler)
{
    if( opcode->type == handlebars_opcode_type_push_program ) {
        if( opcode->op1.type == handlebars_operand_type_long ) {
            long program = opcode->op1.data.longval;
            assert(program < compiler->children_length);
            struct handlebars_compiler * child = compiler->children[program];
            long guid = child->guid;
            opcode->op1.data.longval = guid;
        }
    }
}

static void preprocess_program(struct handlebars_vm * vm, struct handlebars_compiler * compiler) {
    size_t i;

    compiler->guid = vm->guid_index++;

    // Realloc
    if( compiler->guid >= talloc_array_length(vm->programs) ) {
        vm->programs = handlebars_talloc_realloc(vm, vm->programs, struct handlebars_compiler *, talloc_array_length(vm->programs) * 2);
    }

    vm->programs[compiler->guid] = compiler;

    for( i = 0; i < compiler->children_length; i++ ) {
        preprocess_program(vm, compiler->children[i]);
    }

    for( i = 0; i < compiler->opcodes_length; i++ ) {
        preprocess_opcode(vm, compiler->opcodes[i], compiler);
    }
}


void handlebars_vm_execute(
		struct handlebars_vm * vm, struct handlebars_compiler * compiler,
		struct handlebars_value * context)
{
    // Preprocess
    vm->programs = handlebars_talloc_array(vm, struct handlebars_compiler *, 32);
    preprocess_program(vm, compiler);

    // Save context
	vm->context = context;

    // Execute
	vm->buffer = handlebars_vm_execute_program(vm, 0, context);
}
