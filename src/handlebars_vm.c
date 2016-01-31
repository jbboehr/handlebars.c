
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "handlebars_compiler.h"
#include "handlebars_value.h"
#include "handlebars_map.h"
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
struct setup_ctx {
    const char * name;
    size_t param_size;
    struct handlebars_stack * params;
    short is_block_helper;
    struct handlebars_options * options;
    struct handlebars_value * helper;
};

ACCEPT_FUNCTION(push_context);


static inline void setup_options(struct handlebars_vm * vm, struct setup_ctx * ctx)
{
    struct handlebars_vm_frame * frame = handlebars_stack_top_type(vm->frameStack, struct handlebars_vm_frame);
    struct handlebars_options * options = handlebars_talloc(vm, struct handlebars_options);
    ctx->options = options;

    options->name = ctx->name ? handlebars_talloc_strdup(options, ctx->name) : NULL;
    options->hash = handlebars_stack_pop(vm->stack);
    options->scope = frame->context;
    options->vm = vm;

    // programs
    long * inverse = handlebars_stack_pop_type(vm->stack, long);
    long * program = handlebars_stack_pop_type(vm->stack, long);

    options->program = program ? *program : -1;
    options->inverse = inverse ? *inverse : -1;

    // params
    size_t i = ctx->param_size;

    size_t j;
    struct handlebars_value * placeholder = handlebars_value_ctor(vm);
    for( j = 0; j < i; j++ ) {
        handlebars_stack_set(ctx->params, j, placeholder);
    }

    while( i-- ) {
        struct handlebars_value * param = handlebars_stack_pop(vm->stack);
        handlebars_stack_set(ctx->params, i, param);
        handlebars_value_delref(param);
    }

    handlebars_value_delref(placeholder);

    // Params
    options->params = ctx->params;

    // Data
    // @todo
    options->data = NULL;


    //options->scope =
    //options->inverse = handlebars_stack_pop_type();
}

static inline void setup_params(struct handlebars_vm * vm, struct setup_ctx * ctx)
{
    setup_options(vm, ctx);
    if( ctx->params != NULL ) {
        struct handlebars_value * value = handlebars_value_ctor(vm);
        value->type = HANDLEBARS_VALUE_TYPE_OPTIONS;
        value->v.options = ctx->options;
        handlebars_stack_push(ctx->params, value);
    }

}

static inline void setup_helper(struct handlebars_vm * vm, struct setup_ctx * ctx)
{
    ctx->params = handlebars_stack_ctor(vm);
    setup_params(vm, ctx);
    ctx->helper = handlebars_map_find(vm->helpers, ctx->name);
}







struct handlebars_vm * handlebars_vm_ctor(void * ctx)
{
    TALLOC_CTX * tmp = talloc_init(ctx);
	struct handlebars_vm * vm = handlebars_talloc_zero(tmp, struct handlebars_vm);
    if( !vm ) {
        return NULL;
    }

    vm->frameStack = handlebars_stack_ctor(vm);
    vm->depths = handlebars_stack_ctor(vm);
    vm->stack = handlebars_stack_ctor(vm);
    vm->hashStack = handlebars_stack_ctor(vm);
    if( !vm->frameStack || !vm->depths || !vm->stack || !vm->hashStack ) {
        vm = NULL;
        goto error;
    }

    talloc_steal(ctx, vm);
error:
    handlebars_talloc_free(tmp);
    return vm;
}


ACCEPT_FUNCTION(ambiguous_block_value) {
    struct handlebars_vm_frame * frame = handlebars_stack_top_type(vm->frameStack, struct handlebars_vm_frame);
    struct setup_ctx ctx = {0};

    ctx.name = vm->last_helper;
    ctx.params = handlebars_stack_ctor(vm);
    handlebars_stack_push(ctx.params, frame->context);
    setup_params(vm, &ctx);

    struct handlebars_value * current = handlebars_stack_pop(vm->stack);
    handlebars_stack_set(ctx.params, 0, current);

    if( vm->last_helper == NULL ) {
        struct handlebars_value * helper = handlebars_map_find(vm->helpers, "blockHelperMissing");

        assert(helper != NULL);
        assert(helper->type == HANDLEBARS_VALUE_TYPE_HELPER);
        assert(ctx.options != NULL);

        struct handlebars_value * result = helper->v.helper(ctx.options);
        if( result ) {
            char * tmp = handlebars_value_expression(vm, result, 0);
            frame->buffer = handlebars_talloc_strdup_append(frame->buffer, tmp);
            handlebars_talloc_free(tmp);
        }
    }
}

ACCEPT_FUNCTION(append) {
    struct handlebars_vm_frame * frame = handlebars_stack_top_type(vm->frameStack, struct handlebars_vm_frame);
    struct handlebars_value * value = handlebars_stack_pop(vm->stack);

    if( value ) {
        char * tmp = handlebars_value_expression(vm, value, 0);
        frame->buffer = handlebars_talloc_strdup_append(frame->buffer, tmp);
        handlebars_talloc_free(tmp);
    }
}

ACCEPT_FUNCTION(append_escaped) {
    struct handlebars_vm_frame * frame = handlebars_stack_top_type(vm->frameStack, struct handlebars_vm_frame);
    struct handlebars_value * value = handlebars_stack_pop(vm->stack);

    if( value ) {
        char * tmp = handlebars_value_expression(vm, value, 1);
        frame->buffer = handlebars_talloc_strdup_append(frame->buffer, tmp);
        handlebars_talloc_free(tmp);
    }
}

ACCEPT_FUNCTION(append_content) {
    struct handlebars_vm_frame * frame = handlebars_stack_top_type(vm->frameStack, struct handlebars_vm_frame);

    assert(opcode->type == handlebars_opcode_type_append_content);
    assert(opcode->op1.type == handlebars_operand_type_string);

    frame->buffer = handlebars_talloc_strdup_append(frame->buffer, opcode->op1.data.stringval);
}

ACCEPT_FUNCTION(empty_hash) {
    struct handlebars_value * value = handlebars_value_ctor(vm);
    value->type = HANDLEBARS_VALUE_TYPE_MAP;
    value->v.map = handlebars_map_ctor(value);
    handlebars_stack_push(vm->stack, value);
    handlebars_value_delref(value);
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
        vm->last_context = NULL;
        assert(0);
    } else if( depth == 0 ) {
        vm->last_context = handlebars_stack_top(vm->depths);
    } else {
        vm->last_context = handlebars_stack_get(vm->depths, length - depth);
    }
}

ACCEPT_FUNCTION(invoke_ambiguous) {
    struct handlebars_vm_frame * frame = handlebars_stack_top_type(vm->frameStack, struct handlebars_vm_frame);
    struct handlebars_value * value = handlebars_stack_pop(vm->stack);
    struct setup_ctx ctx = {0};

    ACCEPT_FN(empty_hash)(vm, opcode);

    assert(opcode->op1.type == handlebars_operand_type_string);
    assert(opcode->op2.type == handlebars_operand_type_boolean);

    ctx.name = opcode->op1.data.stringval;
    ctx.is_block_helper = opcode->op2.data.boolval;
    setup_helper(vm, &ctx);
    vm->last_helper = ctx.helper ? ctx.name : NULL;

    if( ctx.helper != NULL ) {
        fprintf(stderr, "CALLLED %d\n", __LINE__);
        assert(ctx.helper->type == HANDLEBARS_VALUE_TYPE_HELPER);

        struct handlebars_value *result = ctx.helper->v.helper(ctx.options);
        if (result) {
            char *tmp = handlebars_value_expression(vm, result, 0);
            frame->buffer = handlebars_talloc_strdup_append(frame->buffer, tmp);
            handlebars_talloc_free(tmp);
        }
    } else if( value->type == HANDLEBARS_VALUE_TYPE_HELPER ) {
        struct handlebars_value * result = value->v.helper(ctx.options);
        assert(result != NULL);
        handlebars_stack_push(vm->stack, result);
    } else {
        //frame->buffer = handlebars_talloc_strdup_append(frame->buffer, handlebars_value_get_strval(value));
        handlebars_stack_push(vm->stack, value);
    }
}

ACCEPT_FUNCTION(invoke_helper)
{
    struct handlebars_vm_frame * frame = handlebars_stack_top_type(vm->frameStack, struct handlebars_vm_frame);
    struct handlebars_value * value = handlebars_stack_pop(vm->stack);
    struct handlebars_value * fn = NULL;
    struct handlebars_value * result;
    struct setup_ctx ctx = {0};

    assert(opcode->op1.type == handlebars_operand_type_long);
    assert(opcode->op2.type == handlebars_operand_type_string);
    assert(opcode->op3.type == handlebars_operand_type_boolean);

    ctx.name = opcode->op2.data.stringval;
    ctx.param_size = opcode->op1.data.longval;
    setup_helper(vm, &ctx);

    if( opcode->op3.data.boolval ) { // isSimple
        fn = handlebars_map_find(vm->helpers, ctx.name);
    }
    if( !fn || fn->type != HANDLEBARS_VALUE_TYPE_HELPER ) {
        fn = value;
    }

    if( !fn || fn->type != HANDLEBARS_VALUE_TYPE_HELPER ) {
        fprintf(stderr, "LE SIGH %d %d\n", fn->type, 12345);
        fn = handlebars_map_find(vm->helpers, "helperMissing");
    }

    // @todo runtime error
    assert(fn != NULL);
    assert(fn->type == HANDLEBARS_VALUE_TYPE_HELPER);

    result = fn->v.helper(ctx.options);
    if( !result ) {
        result = handlebars_value_ctor(vm);
    }
    handlebars_stack_push(vm->stack, result);
}

ACCEPT_FUNCTION(invoke_known_helper) {
    struct handlebars_vm_frame * frame = handlebars_stack_top_type(vm->frameStack, struct handlebars_vm_frame);
    struct setup_ctx ctx = {0};

    assert(opcode->op1.type == handlebars_operand_type_long);
    assert(opcode->op2.type == handlebars_operand_type_string);

    ctx.param_size = opcode->op1.data.longval;
    ctx.name = opcode->op2.data.stringval;
    setup_helper(vm, &ctx);

    assert(ctx.helper != NULL);
    assert(ctx.options != NULL);

    struct handlebars_value * result = ctx.helper->v.helper(ctx.options);
    if( result != NULL ) {
        char *tmp = handlebars_value_expression(vm, result, 0);
        if( tmp ) {
            frame->buffer = handlebars_talloc_strdup_append(frame->buffer, tmp);
            handlebars_talloc_free(tmp);
        }
    }
}

ACCEPT_FUNCTION(lookup_on_context) {
    assert(opcode->op1.type == handlebars_operand_type_array);
    assert(opcode->op2.type == handlebars_operand_type_boolean || opcode->op2.type == handlebars_operand_type_null);
    assert(opcode->op3.type == handlebars_operand_type_boolean || opcode->op3.type == handlebars_operand_type_null);
    assert(opcode->op4.type == handlebars_operand_type_boolean || opcode->op4.type == handlebars_operand_type_null);

    ACCEPT_FN(push_context)(vm, opcode);

    struct handlebars_value * value = handlebars_stack_top(vm->stack);

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

    handlebars_stack_push_ptr(vm->stack, program);
}

ACCEPT_FUNCTION(push_literal) {
    struct literal * lit = handlebars_talloc(vm, struct literal);
    handlebars_stack_push_ptr(vm->stack, lit);
}

ACCEPT_FUNCTION(resolve_possible_lambda) {
    // @todo
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
            ACCEPT(ambiguous_block_value);
            ACCEPT(append)
            ACCEPT(append_escaped)
            ACCEPT(append_content);
            ACCEPT(get_context);
            ACCEPT(empty_hash);
            ACCEPT(invoke_ambiguous);
            ACCEPT(invoke_helper);
            ACCEPT(invoke_known_helper);
            ACCEPT(lookup_on_context);
            ACCEPT(push_context);
            ACCEPT(push_program);
            ACCEPT(push_literal);
            ACCEPT(resolve_possible_lambda);

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
    if( program < 0 ) {
        return NULL;
    }
    if( program >= vm->guid_index ) {
        assert(program < vm->guid_index);
        return NULL;
    }

	struct handlebars_compiler * compiler = vm->programs[program];

    // Push the frame stack
	struct handlebars_vm_frame * frame = handlebars_talloc_zero(vm, struct handlebars_vm_frame);
    handlebars_stack_push_ptr(vm->frameStack, frame);
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

    // Pop frame
    char * buffer = talloc_steal(vm, frame->buffer);
    handlebars_stack_pop(vm->frameStack);
    handlebars_talloc_free(frame);

    return buffer;
}

static void preprocess_opcode(struct handlebars_vm * vm, struct handlebars_opcode * opcode, struct handlebars_compiler * compiler)
{
    long program;
    struct handlebars_compiler * child;

    if( opcode->type == handlebars_opcode_type_push_program ) {
        if( opcode->op1.type == handlebars_operand_type_long ) {
            program = opcode->op1.data.longval;
            assert(program < compiler->children_length);
            child = compiler->children[program];
            opcode->op1.data.longval = child->guid;
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
    handlebars_value_addref(context);
    vm->context = context;

    // Execute
    vm->buffer = handlebars_vm_execute_program(vm, 0, context);

    // Release context
    handlebars_value_delref(context);
}
