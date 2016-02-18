
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "handlebars.h"
#include "handlebars_private.h"
#include "handlebars_compiler.h"
#include "handlebars_value.h"
#include "handlebars_map.h"
#include "handlebars_memory.h"
#include "handlebars_opcodes.h"
#include "handlebars_opcode_printer.h"
#include "handlebars_stack.h"
#include "handlebars_utils.h"
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
    struct handlebars_options * options;
    struct handlebars_value * helper;
    bool is_block_helper;
    short use_register;
};

ACCEPT_FUNCTION(push_context);




#undef CONTEXT
#define CONTEXT ctx

struct handlebars_vm * handlebars_vm_ctor(struct handlebars_context * ctx)
{
    struct handlebars_vm * vm = MC(handlebars_talloc_zero(ctx, struct handlebars_vm));
    vm->frameStack = talloc_steal(vm, handlebars_stack_ctor(&vm->ctx));
    vm->depths = talloc_steal(vm, handlebars_stack_ctor(&vm->ctx));
    vm->stack = talloc_steal(vm, handlebars_stack_ctor(&vm->ctx));
    vm->hashStack = talloc_steal(vm, handlebars_stack_ctor(&vm->ctx));
    vm->blockParamStack = talloc_steal(vm, handlebars_stack_ctor(&vm->ctx));
    return vm;
}

#undef CONTEXT
#define CONTEXT ((struct handlebars_context *) vm)


void handlebars_vm_dtor(struct handlebars_vm * vm)
{
    handlebars_stack_dtor(vm->frameStack);
    if( vm->depths ) {
        handlebars_stack_dtor(vm->depths);
    }
    handlebars_stack_dtor(vm->stack);
    handlebars_stack_dtor(vm->hashStack);
    if( vm->blockParamStack ) {
        handlebars_stack_dtor(vm->blockParamStack);
    }
    handlebars_value_try_delref(vm->builtins);
    handlebars_value_try_delref(vm->last_context);
    handlebars_talloc_free(vm);
}

static inline struct handlebars_value * get_helper(struct handlebars_vm * vm, const char * name)
{
    struct handlebars_value * helper;
    helper = handlebars_value_map_find(vm->helpers, name);
    if( !helper || helper->type == HANDLEBARS_VALUE_TYPE_NULL ) {
        if( !vm->builtins ) {
            vm->builtins = handlebars_builtins(CONTEXT);
        }
        helper = handlebars_value_map_find(vm->builtins, name);
    }
    return helper;
}

static inline void setup_options(struct handlebars_vm * vm, struct setup_ctx * ctx)
{
    struct handlebars_vm_frame * frame = handlebars_stack_top_type(vm->frameStack, struct handlebars_vm_frame);
    struct handlebars_options * options = MC(handlebars_talloc_zero(vm, struct handlebars_options));
    long * inverse;
    long * program;
    size_t i, j;
    struct handlebars_value * placeholder;

    ctx->options = options;

    options->name = ctx->name ? MC(handlebars_talloc_strdup(options, ctx->name)) : NULL;
    options->hash = handlebars_stack_pop(vm->stack);
    options->scope = frame->context;
    options->vm = vm;

    // programs
    inverse = handlebars_stack_pop_type(vm->stack, long);
    program = handlebars_stack_pop_type(vm->stack, long);

    options->program = program ? *program : -1;
    options->inverse = inverse ? *inverse : -1;

    // params
    i = ctx->param_size;
    placeholder = handlebars_value_ctor(CONTEXT);
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
    options->params = talloc_steal(options, ctx->params);

    // Data
    // @todo check useData
    if( frame->data ) {
        options->data = frame->data;
        handlebars_value_addref(frame->data);
    //} else if( vm->flags & handlebars_compiler_flag_use_data ) {
    } else {
        options->data = handlebars_value_ctor(CONTEXT);
    }
}

static inline void setup_params(struct handlebars_vm * vm, struct setup_ctx * ctx)
{
    setup_options(vm, ctx);

    /* Disable this for now
    if( ctx->use_register ) {
        struct handlebars_vm_frame * frame = handlebars_stack_top_type(vm->frameStack, struct handlebars_vm_frame);
        if( ctx->use_register == 2 ) {
            assert(frame->options_register != NULL);
            handlebars_stack_push(ctx->params, frame->options_register);
        } else {
            frame->options_register = ctx->options;
            handlebars_stack_push(ctx->params, frame->options_register);
        }
    }
    */

    // Don't include options in oarams for now
    /*
    if( ctx->params != NULL ) {
        struct handlebars_value * value = handlebars_value_ctor(CONTEXT);
        value->type = HANDLEBARS_VALUE_TYPE_OPTIONS;
        value->v.options = ctx->options;
        handlebars_stack_push(ctx->params, value);
    }
    */
}

static inline void setup_helper(struct handlebars_vm * vm, struct setup_ctx * ctx)
{
    ctx->params = handlebars_stack_ctor(CONTEXT);
    setup_params(vm, ctx);
    ctx->helper = get_helper(vm, ctx->name);
}

static inline void append_to_buffer(struct handlebars_vm * vm, struct handlebars_value * result, bool escape)
{
    struct handlebars_vm_frame * frame;
    char * tmp;
    if( !result ) {
        return;
    }
    if( NULL != (tmp = handlebars_value_expression(result, escape)) ) {
#ifndef NDEBUG
        if( getenv("DEBUG") ) {
            fprintf(stderr, "APPEND TO BUFFER: %s\n", tmp);
        }
#endif
        frame = handlebars_stack_top_type(vm->frameStack, struct handlebars_vm_frame);
        frame->buffer = MC(handlebars_talloc_strdup_append_buffer(frame->buffer, tmp));
        handlebars_talloc_free(tmp);
    }
    handlebars_value_delref(result);
}

static inline void depthed_lookup(struct handlebars_vm * vm, const char * key)
{
    size_t i;
    struct handlebars_value * value = NULL;
    struct handlebars_value * tmp;

    for( i = handlebars_stack_length(vm->depths); i > 0; i-- ) {
        value = handlebars_stack_get(vm->depths, i - 1);
        if( !value ) continue;
        if( handlebars_value_get_type(value) == HANDLEBARS_VALUE_TYPE_MAP ) {
            tmp = handlebars_value_map_find(value, key);
            if( tmp != NULL ) {
                handlebars_value_delref(tmp);
                break;
            }
        }
        handlebars_value_delref(value);
    }

    if( !value ) {
        value = handlebars_value_ctor(CONTEXT);
    }

    handlebars_stack_push(vm->stack, value);
    handlebars_value_delref(value);
}

static inline char * dump_stack(struct handlebars_stack * stack)
{
    struct handlebars_value * tmp = handlebars_value_ctor(stack->ctx);
    char * str;
    tmp->type = HANDLEBARS_VALUE_TYPE_ARRAY;
    tmp->v.stack = stack;
    str = handlebars_value_dump(tmp, 0);
    talloc_steal(stack, str);
    handlebars_talloc_free(tmp);
    return str;
}










ACCEPT_FUNCTION(ambiguous_block_value)
{
    struct handlebars_vm_frame * frame = handlebars_stack_top_type(vm->frameStack, struct handlebars_vm_frame);
    struct handlebars_value * current;
    struct handlebars_value * result;
    struct handlebars_value * helper;
    struct setup_ctx ctx = {0};

    ctx.name = vm->last_helper;
    ctx.params = handlebars_stack_ctor(CONTEXT);
    handlebars_stack_push(ctx.params, frame->context);
    setup_params(vm, &ctx);

    current = handlebars_stack_pop(vm->stack);
    if( !current ) { // @todo I don't think this should happen
        current = handlebars_value_ctor(CONTEXT);
    }
    handlebars_stack_set(ctx.params, 0, current);

    if( vm->last_helper == NULL ) {
        helper = get_helper(vm, "blockHelperMissing");

        assert(helper != NULL);
        assert(handlebars_value_is_callable(helper));
        assert(ctx.options != NULL);

        result = handlebars_value_call(helper, ctx.options);
        append_to_buffer(vm, result, 0);

        handlebars_value_delref(helper);
    }

    handlebars_options_dtor(ctx.options);
    handlebars_value_delref(current);
}

ACCEPT_FUNCTION(append)
{
    struct handlebars_value * value = handlebars_stack_pop(vm->stack);
    append_to_buffer(vm, value, 0);
}

ACCEPT_FUNCTION(append_escaped)
{
    struct handlebars_value * value = handlebars_stack_pop(vm->stack);
    append_to_buffer(vm, value, 1);
}

ACCEPT_FUNCTION(append_content)
{
    struct handlebars_vm_frame * frame = handlebars_stack_top_type(vm->frameStack, struct handlebars_vm_frame);

    assert(opcode->type == handlebars_opcode_type_append_content);
    assert(opcode->op1.type == handlebars_operand_type_string);

    frame->buffer = MC(handlebars_talloc_strdup_append(frame->buffer, opcode->op1.data.stringval));
}

ACCEPT_FUNCTION(assign_to_hash)
{
    struct handlebars_value * hash = handlebars_stack_top(vm->hashStack);
    struct handlebars_value * value = handlebars_stack_pop(vm->stack);

    assert(opcode->op1.type == handlebars_operand_type_string);
    assert(hash->type == HANDLEBARS_VALUE_TYPE_MAP);

    handlebars_map_update(hash->v.map, opcode->op1.data.stringval, value);

    handlebars_value_delref(hash);
    handlebars_value_delref(value);
}

ACCEPT_FUNCTION(block_value)
{
    struct handlebars_vm_frame * frame = handlebars_stack_top_type(vm->frameStack, struct handlebars_vm_frame);
    struct handlebars_value * current;
    struct handlebars_value * helper;
    struct handlebars_value * result;
    struct setup_ctx ctx = {0};

    assert(opcode->op1.type == handlebars_operand_type_string);

    ctx.params = handlebars_stack_ctor(CONTEXT);
    //handlebars_stack_push(ctx.params, frame->context);
    ctx.name = opcode->op1.data.stringval;
    setup_params(vm, &ctx);

    current = handlebars_stack_pop(vm->stack);
    assert(current != NULL);
    handlebars_stack_set(ctx.params, 0, current);

    helper = get_helper(vm, "blockHelperMissing");

    assert(helper != NULL);
    assert(handlebars_value_is_callable(helper));

    result = handlebars_value_call(helper, ctx.options);
    append_to_buffer(vm, result, 0);

    handlebars_value_delref(helper);
    handlebars_value_delref(current);
    handlebars_options_dtor(ctx.options);
}

ACCEPT_FUNCTION(empty_hash)
{
    struct handlebars_value * value = handlebars_value_ctor(CONTEXT);
    handlebars_value_map_init(value);
    handlebars_stack_push(vm->stack, value);
    handlebars_value_delref(value);
}

ACCEPT_FUNCTION(get_context)
{
    assert(opcode->type == handlebars_opcode_type_get_context);
    assert(opcode->op1.type == handlebars_operand_type_long);

    size_t depth = (size_t) opcode->op1.data.longval;
    size_t length = handlebars_stack_length(vm->depths);

    if( depth >= length ) {
        // @todo should we throw?
        vm->last_context = NULL;
    } else if( depth == 0 ) {
        vm->last_context = handlebars_stack_top(vm->depths);
    } else {
        vm->last_context = handlebars_stack_get(vm->depths, length - depth - 1);
    }
}

ACCEPT_FUNCTION(invoke_ambiguous)
{
    struct handlebars_value * value = handlebars_stack_pop(vm->stack);
    struct setup_ctx ctx = {0};
    struct handlebars_value * fn;

    ACCEPT_FN(empty_hash)(vm, opcode);

    assert(opcode->op1.type == handlebars_operand_type_string);
    assert(opcode->op2.type == handlebars_operand_type_boolean);

    ctx.name = opcode->op1.data.stringval;
    ctx.is_block_helper = opcode->op2.data.boolval;
    setup_helper(vm, &ctx);
    vm->last_helper = ctx.helper ? ctx.name : NULL;

    if( ctx.helper != NULL && ctx.helper->type != HANDLEBARS_VALUE_TYPE_NULL ) {
        assert(handlebars_value_is_callable(ctx.helper));
        struct handlebars_value *result = handlebars_value_call(ctx.helper, ctx.options);
        append_to_buffer(vm, result, 0);
    } else if( value->type == HANDLEBARS_VALUE_TYPE_HELPER ) {
        struct handlebars_value * result = handlebars_value_call(value, ctx.options);
        assert(result != NULL);
        handlebars_stack_push(vm->stack, result);
        handlebars_value_delref(result);
    } else if( value->type == HANDLEBARS_VALUE_TYPE_USER ) {
        // @todo improve this
        struct handlebars_value * result = handlebars_value_call(value, ctx.options);
        if( result == NULL ) {
            goto missing;
        }
        handlebars_stack_push(vm->stack, result);
        handlebars_value_delref(result);
    } else {
missing:
        fn = get_helper(vm, "helperMissing");
        assert(fn != NULL);
        assert(handlebars_value_is_callable(fn));
        struct handlebars_value * result = handlebars_value_call(fn, ctx.options);
        append_to_buffer(vm, result, 0);
        handlebars_stack_push(vm->stack, value);
    }

    handlebars_options_dtor(ctx.options);
}

ACCEPT_FUNCTION(invoke_helper)
{
    struct handlebars_value * value = handlebars_stack_pop(vm->stack);
    struct handlebars_value * fn = value;
    struct handlebars_value * fn2 = NULL;
    struct handlebars_value * result;
    struct setup_ctx ctx = {0};

    assert(opcode->op1.type == handlebars_operand_type_long);
    assert(opcode->op2.type == handlebars_operand_type_string);
    assert(opcode->op3.type == handlebars_operand_type_boolean);

    ctx.name = opcode->op2.data.stringval;
    ctx.param_size = opcode->op1.data.longval;
    setup_helper(vm, &ctx);

    if( opcode->op3.data.boolval ) { // isSimple
        if( NULL != (fn2 = get_helper(vm, ctx.name)) ) {
            fn = fn2;
        }
    }

    if( !fn || !handlebars_value_is_callable(fn) ) {
        fn = value;
    }

    if( !fn || !handlebars_value_is_callable(fn) ) {
        fn = get_helper(vm, "helperMissing");
    }

    if( !fn || !handlebars_value_is_callable(fn) ) {
        handlebars_context_throw(CONTEXT, HANDLEBARS_ERROR, "Helper missing: %s", ctx.name);
    }

    result = handlebars_value_call(fn, ctx.options);
    if( !result ) {
        result = handlebars_value_ctor(CONTEXT);
    }

    handlebars_stack_push(vm->stack, result);

    handlebars_options_dtor(ctx.options);
    handlebars_value_delref(fn);
    // @todo make sure value and result are delref'd
}

ACCEPT_FUNCTION(invoke_known_helper)
{
    struct setup_ctx ctx = {0};

    assert(opcode->op1.type == handlebars_operand_type_long);
    assert(opcode->op2.type == handlebars_operand_type_string);

    ctx.param_size = opcode->op1.data.longval;
    ctx.name = opcode->op2.data.stringval;
    setup_helper(vm, &ctx);

    if( !ctx.helper || !handlebars_value_is_callable(ctx.helper) ) {
        handlebars_context_throw(CONTEXT, HANDLEBARS_ERROR, "Invalid known helper: %s", ctx.name);
    }

    struct handlebars_value * result = handlebars_value_call(ctx.helper, ctx.options);
    handlebars_stack_push(vm->stack, result);
    //append_to_buffer(vm, result, 0);
    handlebars_value_delref(result);
    handlebars_options_dtor(ctx.options);
}

ACCEPT_FUNCTION(invoke_partial)
{
    struct handlebars_vm_frame * frame = handlebars_stack_top_type(vm->frameStack, struct handlebars_vm_frame);
    struct setup_ctx ctx = {0};
    struct handlebars_value * tmp;
    char * name = NULL;
    jmp_buf buf;

    assert(opcode->op1.type == handlebars_operand_type_boolean);
    assert(opcode->op2.type == handlebars_operand_type_string || opcode->op2.type == handlebars_operand_type_null || opcode->op2.type == handlebars_operand_type_long);
    assert(opcode->op3.type == handlebars_operand_type_string);

    ctx.params = handlebars_stack_ctor(CONTEXT);
    ctx.param_size = 1;
    setup_options(vm, &ctx);

    if( opcode->op2.type == handlebars_operand_type_long ) {
        name = MC(handlebars_talloc_asprintf(vm, "%ld", opcode->op2.data.longval));
    } else if( opcode->op2.type == handlebars_operand_type_string ) {
        name = opcode->op2.data.stringval;
    }
    if( opcode->op1.data.boolval ) {
        tmp = handlebars_stack_pop(vm->stack);
        if( tmp ) { // @todo why is this null?
            name = handlebars_value_get_strval(tmp);
            handlebars_value_delref(tmp);
        }
        ctx.options->name = NULL; // fear
    }

    struct handlebars_value * partial = NULL;
    if( /*!opcode->op1.data.boolval*/ name ) {
        partial = handlebars_value_map_find(vm->partials, name);
    } /* else {
        partial = handlebars_value_ctor(CONTEXT);
        handlebars_value_string(partial, name);
    } */

    if( !partial ) {
        if( vm->flags & handlebars_compiler_flag_compat ) {
            return;
        } else {
            handlebars_context_throw(CONTEXT, HANDLEBARS_ERROR, "The partial %s could not be found", name);
        }
    }

    // If partial is a function?
    if( handlebars_value_is_callable(partial) ) {
        struct handlebars_value * tmp = partial;
        partial = handlebars_value_call(tmp, ctx.options);
        handlebars_value_delref(tmp);
    }

    // Construct new context
    struct handlebars_context * context = handlebars_context_ctor_ex(vm);

    // Save jump buffer
    context->jmp = &buf;
    if( setjmp(buf) ) {
        handlebars_context_throw_ex(CONTEXT, context->num, &context->loc, context->msg);
    }

    // Construct parser
    struct handlebars_parser * parser = handlebars_parser_ctor(context);
    parser->tmpl = handlebars_value_get_strval(partial);
    if( !*parser->tmpl ) {
        goto done;
    }

    // Construct intermediate compiler and VM
    struct handlebars_compiler * compiler = handlebars_compiler_ctor(context, parser);
    struct handlebars_vm * vm2 = handlebars_vm_ctor(context);

    // Parse
    handlebars_parse(parser);

    // Compile
    handlebars_compiler_set_flags(compiler, vm->flags);
    handlebars_compiler_compile(compiler, parser->program);

    // Get context
    // @todo change parent to new vm?
    struct handlebars_value * context1 = handlebars_stack_get(ctx.options->params, 0);
    struct handlebars_value * context2 = NULL;
    struct handlebars_value_iterator * it;
    if( context1 && handlebars_value_get_type(context1) == HANDLEBARS_VALUE_TYPE_MAP ) {
        context2 = handlebars_value_ctor(&vm2->ctx);
        handlebars_value_map_init(context2);
        it = handlebars_value_iterator_ctor(context1);
        for( ; it->current ; handlebars_value_iterator_next(it) ) {
            handlebars_map_update(context2->v.map, it->key, it->current);
        }
        handlebars_talloc_free(it);
        if( ctx.options->hash && ctx.options->hash->type == HANDLEBARS_VALUE_TYPE_MAP ) {
            it = handlebars_value_iterator_ctor(ctx.options->hash);
            for( ; it->current ; handlebars_value_iterator_next(it) ) {
                handlebars_map_update(context2->v.map, it->key, it->current);
            }
            handlebars_talloc_free(it);
        }
    } else if( !context1 || context1->type == HANDLEBARS_VALUE_TYPE_NULL ) {
        context2 = ctx.options->hash;
        if( context2 ) {
            handlebars_value_addref(context2);
        }
    } else {
        context2 = context1;
        context1 = NULL;
    }

    // Setup new VM
    vm2->depth = vm->depth + 1;
    vm2->flags = vm->flags;
    vm2->helpers = vm->helpers;
    vm2->partials = vm->partials;
    vm2->depths = vm->depths;
    vm2->blockParamStack = vm->blockParamStack;
    // @todo block params

    handlebars_vm_execute(vm2, compiler, context2);

    if( vm2->buffer ) {
        char *tmp2 = handlebars_indent(vm2, vm2->buffer, opcode->op3.data.stringval);
        frame->buffer = MC(handlebars_talloc_strdup_append_buffer(frame->buffer, tmp2));
        handlebars_talloc_free(tmp2);
    }

    handlebars_value_try_delref(context1);
    handlebars_value_try_delref(context2);
    vm2->blockParamStack = NULL;
    vm2->depths = NULL;
    handlebars_vm_dtor(vm2);
    handlebars_compiler_dtor(compiler);

done:
    handlebars_context_dtor(context);
    handlebars_value_try_delref(partial);
    handlebars_options_dtor(ctx.options);
}

ACCEPT_FUNCTION(lookup_block_param)
{
    long blockParam1 = -1;
    long blockParam2 = -1;
    struct handlebars_value * value = NULL;

    assert(opcode->op1.type == handlebars_operand_type_array);
    assert(opcode->op2.type == handlebars_operand_type_array);

    sscanf(*(opcode->op1.data.arrayval), "%ld", &blockParam1);
    sscanf(*(opcode->op1.data.arrayval + 1), "%ld", &blockParam2);

    struct handlebars_value * v1 = handlebars_stack_get(vm->blockParamStack, handlebars_stack_length(vm->blockParamStack) - blockParam1 - 1);
    if( !v1 || handlebars_value_get_type(v1) != HANDLEBARS_VALUE_TYPE_ARRAY ) goto done;

    struct handlebars_value * v2 = handlebars_value_array_find(v1, blockParam2);
    if( !v2 ) goto done;

    char ** arr = opcode->op2.data.arrayval;
    arr++;
    if( *arr ) {
        struct handlebars_value * tmp = v2;
        struct handlebars_value * tmp2;
        for(  ; *arr != NULL; arr++ ) {
            tmp2 = handlebars_value_map_find(tmp, *arr);
            if( !tmp2 ) {
                break;
            } else {
                handlebars_value_delref(tmp);
                tmp = tmp2;
            }
        }
        value = tmp;
    } else {
        value = v2;
    }

done:
    if( !value ) {
        value = handlebars_value_ctor(CONTEXT);
    }
    handlebars_stack_push(vm->stack, value);
    handlebars_value_delref(value);
}

ACCEPT_FUNCTION(lookup_data)
{
    struct handlebars_vm_frame * frame = handlebars_stack_top_type(vm->frameStack, struct handlebars_vm_frame);
    struct handlebars_value * data = frame->data;
    struct handlebars_value * tmp;
    struct handlebars_value * val = NULL;

    assert(opcode->op1.type == handlebars_operand_type_long);
    assert(opcode->op2.type == handlebars_operand_type_array);
    assert(opcode->op3.type == handlebars_operand_type_boolean || opcode->op3.type == handlebars_operand_type_null);

    size_t depth = opcode->op1.data.longval;
    char **arr = opcode->op2.data.arrayval;
    char * first = *arr++;

    if( depth && data ) {
        handlebars_value_addref(data);
        while( data && depth-- ) {
            tmp = handlebars_value_map_find(data, "_parent");
            handlebars_value_delref(data);
            data = tmp;
        }
    }

    if( data && (tmp = handlebars_value_map_find(data, first)) ) {
        val = tmp;
    } else if( 0 == strcmp(first, "root") ) {
        val = handlebars_stack_get(vm->depths, 0);
    }

    if( val ) {
        for (; *arr != NULL; arr++) {
            char *part = *arr;
            if (val == NULL || handlebars_value_get_type(val) != HANDLEBARS_VALUE_TYPE_MAP) {
                break;
            }
            tmp = handlebars_value_map_find(val, part);
            handlebars_value_delref(val);
            val = tmp;
        }
    }

    if( !val ) {
        val = handlebars_value_ctor(CONTEXT);
    }

    handlebars_stack_push(vm->stack, val);
    handlebars_value_delref(val);
}

ACCEPT_FUNCTION(lookup_on_context)
{
    assert(opcode->op1.type == handlebars_operand_type_array);
    assert(opcode->op2.type == handlebars_operand_type_boolean || opcode->op2.type == handlebars_operand_type_null);
    assert(opcode->op3.type == handlebars_operand_type_boolean || opcode->op3.type == handlebars_operand_type_null);
    assert(opcode->op4.type == handlebars_operand_type_boolean || opcode->op4.type == handlebars_operand_type_null);

    char **arr = opcode->op1.data.arrayval;
    long index = -1;

    if( !opcode->op4.data.boolval && (vm->flags & handlebars_compiler_flag_compat) ) {
        depthed_lookup(vm, *arr);
    } else {
        ACCEPT_FN(push_context)(vm, opcode);
    }

    struct handlebars_value * value = handlebars_stack_top(vm->stack);
    struct handlebars_value * tmp;

    if( value ) {
        do {
            if( handlebars_value_get_type(value) == HANDLEBARS_VALUE_TYPE_MAP ) {
                tmp = handlebars_value_map_find(value, *arr);
                handlebars_value_try_delref(value);
                value = tmp;
            } else if( handlebars_value_get_type(value) == HANDLEBARS_VALUE_TYPE_ARRAY ) {
                if( sscanf(*arr, "%ld", &index) ) {
                    tmp = handlebars_value_array_find(value, index);
                } else {
                    tmp = NULL;
                }
                handlebars_value_try_delref(value);
                value = tmp;
            } else {
                value = NULL;
            }
            if( !value ) {
                break;
            }
        } while( *++arr );
    }

    if( !value ) {
        value = handlebars_value_ctor(CONTEXT);
    }

    handlebars_value_delref(handlebars_stack_pop(vm->stack));
    handlebars_stack_push(vm->stack, value);
    handlebars_value_delref(value);
}

ACCEPT_FUNCTION(pop_hash)
{
    struct handlebars_value * hash = handlebars_stack_pop(vm->hashStack);
    if( !hash ) {
        hash = handlebars_value_ctor(CONTEXT);
    }
    handlebars_stack_push(vm->stack, hash);
    handlebars_value_delref(hash);
}

ACCEPT_FUNCTION(push_context)
{
    struct handlebars_value * value = vm->last_context;

    if( !value ) {
        value = handlebars_value_ctor(CONTEXT);
    } else {
        handlebars_value_addref(value);
    }

    handlebars_stack_push(vm->stack, value);
    handlebars_value_delref(value);
}

ACCEPT_FUNCTION(push_hash)
{
    struct handlebars_value * hash = handlebars_value_ctor(CONTEXT);
    handlebars_value_map_init(hash);
    handlebars_stack_push(vm->hashStack, hash);
    handlebars_value_delref(hash);
}

ACCEPT_FUNCTION(push_program)
{
    long * program;

    assert(opcode->type == handlebars_opcode_type_push_program);

    program = MC(handlebars_talloc(vm, long));

    if( opcode->op1.type == handlebars_operand_type_long ) {
        *program = opcode->op1.data.longval;
    } else {
        *program = -1;
    }

    handlebars_stack_push_ptr(vm->stack, program);
}

ACCEPT_FUNCTION(push_literal)
{
    struct handlebars_value * value = handlebars_value_ctor(CONTEXT);

    switch( opcode->op1.type ) {
        case handlebars_operand_type_string:
            // @todo we should move this to the parser
            if( 0 == strcmp(opcode->op1.data.stringval, "undefined") ) {
                break;
            } else if( 0 == strcmp(opcode->op1.data.stringval, "null") ) {
                break;
            }
            handlebars_value_string(value, opcode->op1.data.stringval);
            break;
        case handlebars_operand_type_boolean:
            handlebars_value_boolean(value, opcode->op1.data.boolval);
            break;
        case handlebars_operand_type_long:
            handlebars_value_integer(value, opcode->op1.data.longval);
            break;
        case handlebars_operand_type_null:
            break;

        case handlebars_operand_type_array:
        default:
            assert(0);
            break;
    }

    handlebars_stack_push(vm->stack, value);
    handlebars_value_delref(value);
}

ACCEPT_FUNCTION(push_string)
{
    struct handlebars_value * value = handlebars_value_ctor(CONTEXT);

    assert(opcode->op1.type == handlebars_operand_type_string);

    handlebars_value_string(value, opcode->op1.data.stringval);
    handlebars_stack_push(vm->stack, value);
    handlebars_value_delref(value);
}

/*
ACCEPT_FUNCTION(push_string_param)
{
    assert(opcode->op1.type == handlebars_operand_type_string);
    assert(opcode->op2.type == handlebars_operand_type_string);

    ACCEPT_FN(push_context)(vm, opcode);

    do {
        struct handlebars_value * value = handlebars_value_ctor(CONTEXT);
        handlebars_value_string(value, opcode->op2.data.stringval);
        handlebars_stack_push(vm->stack, value);
        handlebars_value_delref(value);
    } while(0);

    if( 0 != strcmp(opcode->op2.data.stringval, "SubExpression") ) {
        struct handlebars_value * value = handlebars_value_ctor(CONTEXT);
        handlebars_value_string(value, opcode->op1.data.stringval);
        handlebars_stack_push(vm->stack, value);
        handlebars_value_delref(value);
    }
}
*/

ACCEPT_FUNCTION(resolve_possible_lambda)
{
    struct handlebars_vm_frame * frame = handlebars_stack_top_type(vm->frameStack, struct handlebars_vm_frame);
    struct handlebars_value * top = handlebars_stack_top(vm->stack);
    struct handlebars_value * result;

    assert(top != NULL);
    if( handlebars_value_is_callable(top) ) {
        struct handlebars_options * options = MC(handlebars_talloc_zero(vm, struct handlebars_options));
        options->params = handlebars_stack_ctor(CONTEXT);
        handlebars_stack_set(options->params, 0, frame->context);
        options->vm = vm;
        options->scope = frame->context;
        result = handlebars_value_call(top, options);
        if( !result ) {
            result = handlebars_value_ctor(CONTEXT);
        }
        handlebars_stack_pop(vm->stack);
        handlebars_stack_push(vm->stack, result);
        handlebars_options_dtor(options);
        handlebars_value_delref(result);
    }
    handlebars_value_delref(top);
}

void handlebars_vm_accept(struct handlebars_vm * vm, struct handlebars_compiler * compiler)
{
	size_t i;

	for( i = 0; i < compiler->opcodes_length; i++ ) {
		struct handlebars_opcode * opcode = compiler->opcodes[i];

        // Print opcode?
#ifndef NDEBUG
        if( getenv("DEBUG") ) {
            char *tmp = handlebars_opcode_print(vm, opcode);
            fprintf(stdout, "V[%d] P[%d] OPCODE: %s\n", vm->depth, compiler->guid, tmp);
            talloc_free(tmp);
        }
#endif

		switch( opcode->type ) {
            ACCEPT(ambiguous_block_value);
            ACCEPT(append)
            ACCEPT(append_escaped)
            ACCEPT(append_content);
            ACCEPT(assign_to_hash);
            ACCEPT(block_value);
            ACCEPT(get_context);
            ACCEPT(empty_hash);
            ACCEPT(invoke_ambiguous);
            ACCEPT(invoke_helper);
            ACCEPT(invoke_known_helper);
            ACCEPT(invoke_partial);
            ACCEPT(lookup_block_param);
            ACCEPT(lookup_data);
            ACCEPT(lookup_on_context);
            ACCEPT(pop_hash);
            ACCEPT(push_context);
            ACCEPT(push_hash);
            ACCEPT(push_program);
            ACCEPT(push_literal);
            ACCEPT(push_string);
            //ACCEPT(push_string_param);
            ACCEPT(resolve_possible_lambda);
            default:
                handlebars_context_throw(CONTEXT, HANDLEBARS_ERROR, "Unhandled opcode: %s\n", handlebars_opcode_readable_type(opcode->type));
                break;
        }
	}
}

char * handlebars_vm_execute_program_ex(
        struct handlebars_vm * vm, int program, struct handlebars_value * context,
        struct handlebars_value * data, struct handlebars_value * block_params)
{
    bool pushed_depths = false;
    bool pushed_block_param = false;

    if( program < 0 ) {
        return NULL;
    }
    if( program >= vm->guid_index ) {
        assert(program < vm->guid_index);
        return NULL;
    }

    // Get compiler
	struct handlebars_compiler * compiler = vm->programs[program];

    // Get parent frame
    struct handlebars_vm_frame * parent_frame = handlebars_stack_top_type(vm->frameStack, struct handlebars_vm_frame);

    // Push the frame stack
	struct handlebars_vm_frame * frame = MC(handlebars_talloc_zero(vm, struct handlebars_vm_frame));

    handlebars_stack_push_ptr(vm->frameStack, frame);
    frame->buffer = MC(handlebars_talloc_strdup(vm, ""));

    // Set program
    frame->program = program;

    // Set context
	frame->context = context;

    // Push depths
    struct handlebars_value * depths_top = handlebars_stack_top(vm->depths);
    if( !depths_top || depths_top != context ) { // @todo compare by value?
        handlebars_stack_push(vm->depths, context);
        pushed_depths = 1;
    }
    if( depths_top ) {
        handlebars_value_delref(depths_top);
    }

    // Set data
    if( data ) {
        frame->data = data;
    } else if( parent_frame ) {
        frame->data = parent_frame->data;
    }

    // Set block params
    if( block_params ) {
        handlebars_stack_push(vm->blockParamStack, block_params);
        pushed_block_param = 1;
    }

    // Execute the program
	handlebars_vm_accept(vm, compiler);

    // Pop depths
    if( pushed_depths ) {
        handlebars_stack_pop(vm->depths);
    }

    // Pop block params
    if( pushed_block_param ) {
        handlebars_stack_pop(vm->blockParamStack);
    }

    // Pop frame
    char * buffer = talloc_steal(vm, frame->buffer);
    handlebars_stack_pop(vm->frameStack);
    handlebars_talloc_free(frame);

    return buffer;
}

char * handlebars_vm_execute_program(struct handlebars_vm * vm, int program, struct handlebars_value * context)
{
    return handlebars_vm_execute_program_ex(vm, program, context, NULL, NULL);
}

static void preprocess_opcode(struct handlebars_vm * vm, struct handlebars_opcode * opcode, struct handlebars_compiler * compiler)
{
    long program;
    struct handlebars_compiler * child;

    if( opcode->type == handlebars_opcode_type_push_program ) {
        if( opcode->op1.type == handlebars_operand_type_long && !opcode->op4.data.boolval ) {
            program = opcode->op1.data.longval;
            assert(program < compiler->children_length);
            child = compiler->children[program];
            opcode->op1.data.longval = child->guid;
            opcode->op4.data.boolval = 1;
        }
    }
}

static void preprocess_program(struct handlebars_vm * vm, struct handlebars_compiler * compiler) {
    size_t i;

    compiler->guid = vm->guid_index++;

    // Realloc
    if( compiler->guid >= talloc_array_length(vm->programs) ) {
        vm->programs = MC(handlebars_talloc_realloc(vm, vm->programs, struct handlebars_compiler *, talloc_array_length(vm->programs) * 2));
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
    jmp_buf * prev = vm->ctx.jmp;
    jmp_buf buf;

    // Save jump buffer
    if( !prev ) {
        vm->ctx.jmp = &buf;
        if( setjmp(buf) ) {
            goto done;
        }
    }

    // Preprocess
    vm->programs = MC(handlebars_talloc_array(vm, struct handlebars_compiler *, 32));
    preprocess_program(vm, compiler);

    // Save context
    handlebars_value_addref(context);
    vm->context = context;

    // Execute
    vm->buffer = handlebars_vm_execute_program_ex(vm, 0, context, vm->data, NULL);

done:
    // Release context
    handlebars_value_delref(context);

    vm->ctx.jmp = prev;
}
