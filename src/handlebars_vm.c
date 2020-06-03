/**
 * Copyright (C) 2016 John Boehr
 *
 * This file is part of handlebars.c.
 *
 * handlebars.c is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * handlebars.c is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with handlebars.c.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <stdio.h>
#include <string.h>

#define HANDLEBARS_HELPERS_PRIVATE
#define HANDLEBARS_OPCODE_SERIALIZER_PRIVATE
#define HANDLEBARS_OPCODES_PRIVATE

#include "handlebars.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"
#include "handlebars_value_private.h"

#include "handlebars_cache.h"
#include "handlebars_compiler.h"
#include "handlebars_delimiters.h"
#include "handlebars_helpers.h"
#include "handlebars_map.h"
#include "handlebars_parser.h"
#include "handlebars_opcodes.h"
#include "handlebars_opcode_printer.h"
#include "handlebars_opcode_serializer.h"
#include "handlebars_stack.h"
#include "handlebars_string.h"
#include "handlebars_value.h"
#include "handlebars_vm.h"

// @TODO fix these?
#pragma GCC diagnostic warning "-Winline"

// {{{ Macros

#if defined(INTELLIJ)
#undef HAVE_COMPUTED_GOTOS
#endif

#define OPCODE_NAME(name) handlebars_opcode_type_ ## name
#define ACCEPT_FN(name) accept_ ## name
#define ACCEPT_NAMED_FUNCTION(name) static inline void name (struct handlebars_vm * vm, struct handlebars_opcode * opcode)
#define ACCEPT_FUNCTION(name) ACCEPT_NAMED_FUNCTION(ACCEPT_FN(name))

static inline struct handlebars_value * _get(struct handlebars_stack * stack, size_t pos) {
    if (handlebars_stack_count(stack) < pos + 1) {
        return NULL;
    }
    return handlebars_stack_get(stack, handlebars_stack_count(stack) - pos - 1);
}

static inline struct handlebars_value * _pop(struct handlebars_stack * stack, struct handlebars_value * rv)
{
    return handlebars_stack_pop(stack, rv);
}

#define LEN(stack) handlebars_stack_count(stack)
#define TOP(stack) handlebars_stack_top(stack)
#define POP(stack, rv) _pop(stack, rv)
#define GET(stack, pos) _get(stack, pos)
#define PUSH(stack, value) (stack = handlebars_stack_push(stack, value))

#undef CONTEXT
#define CONTEXT HBSCTX(vm)

// }}} Macros

// {{{ Prototypes & Variables

ACCEPT_FUNCTION(push_context);

struct handlebars_vm {
    struct handlebars_context ctx;
    struct handlebars_cache * cache;

    struct handlebars_module * module;

    long depth;
    long flags;

    struct handlebars_string * buffer;

    struct handlebars_value data;
    struct handlebars_value helpers;
    struct handlebars_value partials;

    struct handlebars_string * last_helper;
    struct handlebars_value * last_context;

    struct handlebars_stack * stack;
    struct handlebars_stack * contextStack;
    struct handlebars_stack * hashStack;
    struct handlebars_stack * blockParamStack;
};

const size_t HANDLEBARS_VM_SIZE = sizeof(struct handlebars_vm);

// }}} Prototypes & Variables

// {{{ Constructors & Destructors

struct handlebars_vm * handlebars_vm_ctor(struct handlebars_context * ctx)
{
    struct handlebars_vm * vm = handlebars_talloc_zero(ctx, struct handlebars_vm);
    HANDLEBARS_MEMCHECK(vm, ctx);
    handlebars_context_bind(ctx, HBSCTX(vm));
    handlebars_value_map(&vm->helpers, handlebars_map_ctor(ctx, 0));
    handlebars_value_map(&vm->partials, handlebars_map_ctor(ctx, 0));
    return vm;
}


void handlebars_vm_dtor(struct handlebars_vm * vm)
{
    // @TODO FIXME
    handlebars_value_dtor(&vm->helpers);
    handlebars_value_dtor(&vm->partials);
    handlebars_value_dtor(&vm->data);
    handlebars_talloc_free(vm);
}

// }}} Constructors & Destructors

// {{{ Getters & Setters

void handlebars_vm_set_flags(struct handlebars_vm * vm, unsigned flags)
{
    vm->flags = flags;
}

void handlebars_vm_set_helpers(struct handlebars_vm * vm, struct handlebars_value * helpers)
{
    handlebars_value_value(&vm->helpers, helpers);
}

void handlebars_vm_set_partials(struct handlebars_vm * vm, struct handlebars_value * partials)
{
    handlebars_value_value(&vm->partials, partials);
}

void handlebars_vm_set_data(struct handlebars_vm * vm, struct handlebars_value * data)
{
    handlebars_value_value(&vm->data, data);
}

void handlebars_vm_set_cache(struct handlebars_vm * vm, struct handlebars_cache * cache)
{
    vm->cache = cache;
}

// }}} Getters & Setters

HBS_ATTR_NONNULL(1, 3, 4)
static inline struct handlebars_value * call_helper(struct handlebars_string * string, int argc, struct handlebars_value * argv[], struct handlebars_options * options, struct handlebars_value * rv)
{
    HANDLEBARS_VALUE_DECL(rv2);
    struct handlebars_value * helper;
    handlebars_helper_func fn;
    if( NULL != (helper = handlebars_value_map_find(&options->vm->helpers, string, rv2)) ) {
        rv = handlebars_value_call(helper, argc, argv, options, rv);
    } else if( NULL != (fn = handlebars_builtins_find(hbs_str_val(string), hbs_str_len(string))) ) {
        rv = fn(argc, argv, options, rv);
    } else {
        rv = NULL;
    }
    HANDLEBARS_VALUE_UNDECL(rv2);
    return rv;
}

HBS_ATTR_NONNULL(1, 4, 5)
struct handlebars_value * handlebars_vm_call_helper_str(const char * name, unsigned int len, int argc, struct handlebars_value * argv[], struct handlebars_options * options, struct handlebars_value * rv)
{
    HANDLEBARS_VALUE_DECL(rv2);
    struct handlebars_value * helper;
    handlebars_helper_func fn;
    if( NULL != (helper = handlebars_value_map_str_find(&options->vm->helpers, name, len, rv2)) ) {
        rv = handlebars_value_call(helper, argc, argv, options, rv);
    } else if( NULL != (fn = handlebars_builtins_find(name, len)) ) {
        rv = fn(argc, argv, options, rv);
    } else {
        rv = NULL;
    }
    HANDLEBARS_VALUE_UNDECL(rv2);
    return rv;
}

static inline void setup_options(struct handlebars_vm * vm, int argc, struct handlebars_value * argv[], struct handlebars_options * options, struct handlebars_value * mem)
{
    struct handlebars_value * inverse;
    struct handlebars_value * program;
    int i;

    //options->name = ctx->name ? MC(handlebars_talloc_strndup(options, ctx->name->val, ctx->name->len)) : NULL;
    options->hash = POP(vm->stack, mem++);
    options->scope = mem++;
    handlebars_value_value(options->scope, TOP(vm->contextStack));
    options->vm = vm;

    // programs
    inverse = POP(vm->stack, mem++);
    program = POP(vm->stack, mem++);
    if (inverse) {
        options->inverse = handlebars_value_get_intval(inverse);
        handlebars_value_dtor(inverse);
    } else {
        options->inverse = -1;
    }
    if (program) {
        options->program = handlebars_value_get_intval(program);
        handlebars_value_dtor(program);
    } else {
        options->program = -1;
    }

    i = argc;
    mem += argc;
    while( i-- ) {
        argv[i] = POP(vm->stack, --mem);
    }

    // Data
    // @todo check useData
    options->data = &vm->data;
    // @TODO fixme this isn't working
    // options->data = mem++;
    // handlebars_value_value(options->data, &vm->data);
}

#define VM_SETUP_OPTIONS(argc) \
    struct handlebars_options options = {0}; \
    struct handlebars_value argv_mem[argc + 4]; \
    struct handlebars_value * argv[argc]; \
    memset(&argv_mem, 0, sizeof(argv_mem)); \
    setup_options(vm, argc, argv, &options, argv_mem)

static inline void teardown_options(struct handlebars_vm * vm, int argc, struct handlebars_value * argv[], struct handlebars_options * options)
{
    int i;
    i = argc;
    while( i-- ) {
        handlebars_value_dtor(argv[i]);
    }
    handlebars_options_deinit(options);
}

#define VM_TEARDOWN_OPTIONS(argc) \
    teardown_options(vm, argc, argv, &options)

static inline void append_to_buffer(struct handlebars_vm * vm, struct handlebars_value * result, bool escape)
{
    if (likely(result != NULL)) {
        vm->buffer = handlebars_value_expression_append(CONTEXT, result, vm->buffer, escape);
    }
}

static inline void depthed_lookup(struct handlebars_vm * vm, struct handlebars_string * key)
{
    size_t i;
    size_t l;
    struct handlebars_value * value = NULL;
    struct handlebars_value * tmp;
    HANDLEBARS_VALUE_DECL(rv);
    HANDLEBARS_VALUE_DECL(empty_value);

    for( i = 0, l = LEN(vm->contextStack); i < l; i++ ) {
        value = GET(vm->contextStack, i);
        assert(value != NULL);
        if( handlebars_value_get_type(value) == HANDLEBARS_VALUE_TYPE_MAP ) {
            tmp = handlebars_value_map_find(value, key, rv);
            if( tmp != NULL ) {
                break;
            }
        }
        value = empty_value;
    }

    PUSH(vm->stack, value);

    HANDLEBARS_VALUE_UNDECL(empty_value);
    HANDLEBARS_VALUE_UNDECL(rv);
}

static inline struct handlebars_value * merge_hash(struct handlebars_context * context, struct handlebars_value * hash, struct handlebars_value * context1, struct handlebars_value * rv)
{
    if( context1 && handlebars_value_get_type(context1) == HANDLEBARS_VALUE_TYPE_MAP &&
            hash && handlebars_value_get_type(hash) == HANDLEBARS_VALUE_TYPE_MAP ) {
        struct handlebars_map * new_map = handlebars_map_ctor(context, handlebars_value_count(context1) + handlebars_value_count(hash));
        HANDLEBARS_VALUE_FOREACH_KV(context1, key, child) {
            new_map = handlebars_map_update(new_map, key, child);
        } HANDLEBARS_VALUE_FOREACH_END();
        HANDLEBARS_VALUE_FOREACH_KV(hash, key, child) {
            new_map = handlebars_map_update(new_map, key, child);
        } HANDLEBARS_VALUE_FOREACH_END();
        handlebars_value_map(rv, new_map);
    } else if( (!context1 || handlebars_value_get_type(context1) == HANDLEBARS_VALUE_TYPE_NULL) && hash ) {
        handlebars_value_value(rv, hash);
    } else if (context1) {
        handlebars_value_value(rv, context1);
    }
    return rv;
}

static inline struct handlebars_string * execute_template(
    struct handlebars_context * context,
    struct handlebars_vm * vm,
    struct handlebars_string * tmpl,
    struct handlebars_value * data,
    struct handlebars_string * indent,
    int escape
) {
    // jmp_buf buf;

    // @TODO FIXME
    // setjmp/long handling here is messed up

    // Save jump buffer
    // if( setjmp(buf) ) {
    //     handlebars_rethrow(CONTEXT, context);
    // }

    // Get template
    if( !tmpl || !hbs_str_len(tmpl) ) {
        return handlebars_string_ctor(HBSCTX(context), HBS_STRL(""));
    }

    // Check for cached template, if available
    struct handlebars_compiler * compiler;
    struct handlebars_module * module = vm->cache ? handlebars_cache_find(vm->cache, tmpl) : NULL;
    volatile bool from_cache = module != NULL;
    if( !from_cache ) {
        // Recompile

        // Parse
        struct handlebars_parser * parser = handlebars_parser_ctor(context);
        if( vm->flags & handlebars_compiler_flag_compat ) {
            // @TODO free old template
            tmpl = handlebars_preprocess_delimiters(HBSCTX(parser), tmpl, NULL, NULL);
        }
        struct handlebars_ast_node * ast = handlebars_parse_ex(parser, tmpl, vm->flags); // @todo fix setjmp

        // Compile
        compiler = handlebars_compiler_ctor(context);
        handlebars_compiler_set_flags(compiler, vm->flags);
        struct handlebars_program * program = handlebars_compiler_compile_ex(compiler, ast);

        // Serialize
        module = handlebars_program_serialize(context, program);

        // Save cache entry
        if( vm->cache ) {
            handlebars_cache_add(vm->cache, tmpl, module);
        }

        // Cleanup parser
        handlebars_parser_dtor(parser);
    }

    // Construct child VM
    struct handlebars_vm * vm2 = handlebars_vm_ctor(context);

    // Save jump buffer
    // if( setjmp(buf) ) {
    //     if( from_cache ) {
    //         handlebars_cache_release(vm->cache, tmpl, module);
    //     }
    //     handlebars_rethrow(CONTEXT, HBSCTX(vm2));
    // }

    // Setup new VM
    vm2->depth = vm->depth + 1;
    vm2->flags = vm->flags;
    handlebars_value_value(&vm2->helpers, &vm->helpers);
    handlebars_value_value(&vm2->partials, &vm->partials);
    handlebars_value_value(&vm2->data, &vm->data);

    // Copy stack pointers
    vm2->stack = vm->stack;
    vm2->hashStack = vm->hashStack;
    vm2->contextStack = vm->contextStack;
    vm2->blockParamStack = vm->blockParamStack;

    struct handlebars_string * retval = handlebars_vm_execute(vm2, module, data);
    assert(retval != NULL);

    if (indent) {
        retval = handlebars_string_indent(HBSCTX(vm), retval, indent);
    }

    talloc_steal(vm, retval);

    if( from_cache ) {
        handlebars_cache_release(vm->cache, tmpl, module);
    }

    handlebars_vm_dtor(vm2);

// done:
    return retval;
}










ACCEPT_FUNCTION(ambiguous_block_value)
{
    HANDLEBARS_VALUE_DECL(rv);

    if( vm->last_helper == NULL ) {
        VM_SETUP_OPTIONS(1);
        options.name = vm->last_helper; // @todo dupe?
        struct handlebars_value * result = handlebars_vm_call_helper_str(HBS_STRL("blockHelperMissing"), 1, argv, &options, rv);
        assert(result != NULL);
        PUSH(vm->stack, result);
        VM_TEARDOWN_OPTIONS(1);
    } else if (hbs_str_eq_strl(vm->last_helper, HBS_STRL("lambda"))) {
        VM_SETUP_OPTIONS(0);
        options.name = vm->last_helper; // @todo dupe?
        handlebars_string_delref(vm->last_helper);
        vm->last_helper = NULL;
        VM_TEARDOWN_OPTIONS(0);
    }

    HANDLEBARS_VALUE_UNDECL(rv);
}

ACCEPT_FUNCTION(append)
{
    HANDLEBARS_VALUE_DECL(value);

    if (likely(NULL != POP(vm->stack, value))) {
        append_to_buffer(vm, value, 0);
    }

    HANDLEBARS_VALUE_UNDECL(value);
}

ACCEPT_FUNCTION(append_escaped)
{
    HANDLEBARS_VALUE_DECL(value);

    if (likely(NULL != POP(vm->stack, value))) {
        append_to_buffer(vm, value, 1);
    }

    HANDLEBARS_VALUE_UNDECL(value);
}

ACCEPT_FUNCTION(append_content)
{
    assert(opcode->type == handlebars_opcode_type_append_content);
    assert(opcode->op1.type == handlebars_operand_type_string);

    vm->buffer = handlebars_string_append(CONTEXT, vm->buffer, HBS_STR_STRL(opcode->op1.data.string.string));
}

ACCEPT_FUNCTION(assign_to_hash)
{
    HANDLEBARS_VALUE_DECL(hash);
    HANDLEBARS_VALUE_DECL(value);

    HBS_ASSERT(POP(vm->hashStack, hash));
    HBS_ASSERT(POP(vm->stack, value));

    assert(hash != NULL);
    assert(value != NULL);
    assert(opcode->op1.type == handlebars_operand_type_string);
    assert(handlebars_value_get_type(hash) == HANDLEBARS_VALUE_TYPE_MAP);

    struct handlebars_map * map = handlebars_value_get_map(hash);
    map = handlebars_map_update(map, opcode->op1.data.string.string, value);
    handlebars_value_map(hash, map);

    PUSH(vm->hashStack, hash);

    HANDLEBARS_VALUE_UNDECL(value);
    HANDLEBARS_VALUE_UNDECL(hash);
}

ACCEPT_FUNCTION(block_value)
{
    const int argc = 1;
    HANDLEBARS_VALUE_DECL(rv);

    assert(opcode->op1.type == handlebars_operand_type_string);

    VM_SETUP_OPTIONS(argc);
    options.name = opcode->op1.data.string.string;

    append_to_buffer(
        vm,
        handlebars_vm_call_helper_str(HBS_STRL("blockHelperMissing"), argc, argv, &options, rv),
        0
    );

    VM_TEARDOWN_OPTIONS(argc);
    HANDLEBARS_VALUE_UNDECL(rv);
}

ACCEPT_FUNCTION(empty_hash)
{
    HANDLEBARS_VALUE_DECL(value);

    handlebars_value_map(value, handlebars_map_ctor(CONTEXT, 0));
    PUSH(vm->stack, value);

    HANDLEBARS_VALUE_UNDECL(value);
}

ACCEPT_FUNCTION(get_context)
{
    assert(opcode->type == handlebars_opcode_type_get_context);
    assert(opcode->op1.type == handlebars_operand_type_long);

    size_t depth = (size_t) opcode->op1.data.longval;
    size_t length = LEN(vm->contextStack);

    if( depth >= length ) {
        // @todo should we throw?
        handlebars_value_null(vm->last_context);
    } else if( depth == 0 ) {
        handlebars_value_value(vm->last_context, TOP(vm->contextStack));
    } else {
        handlebars_value_value(vm->last_context, GET(vm->contextStack, depth));
    }
}

static inline struct handlebars_value * invoke_mustache_style_lambda(
    struct handlebars_vm *vm,
    struct handlebars_opcode *opcode,
    struct handlebars_value *value,
    struct handlebars_options *options,
    struct handlebars_value * rv
) {
    struct handlebars_value * argv[1];
    struct handlebars_value arg = {0};
    HANDLEBARS_VALUE_DECL(lambda_result);

    assert(opcode->op3.type == handlebars_operand_type_string);

    argv[0] = &arg;
    handlebars_value_str(argv[0], opcode->op3.data.string.string);

    if (NULL != handlebars_value_call(value, 1, argv, options, lambda_result) && !handlebars_value_is_empty(lambda_result)) {
        struct handlebars_string * tmpl = handlebars_value_to_string(lambda_result, CONTEXT);
        struct handlebars_context * context = handlebars_context_ctor_ex(vm);
        struct handlebars_string * rv_str = execute_template(context, vm, tmpl, value, NULL, 0);
        handlebars_value_str(rv, rv_str);
        handlebars_context_dtor(context);
    }

    HANDLEBARS_VALUE_UNDECL(lambda_result);

    return rv;
}

ACCEPT_FUNCTION(invoke_ambiguous)
{
    HANDLEBARS_VALUE_DECL(rv);
    HANDLEBARS_VALUE_DECL(value);
    struct handlebars_value * result;

    HBS_ASSERT(POP(vm->stack, value));
    const int argc = 0;

    ACCEPT_FN(empty_hash)(vm, opcode);

    assert(opcode->op1.type == handlebars_operand_type_string);
    assert(opcode->op2.type == handlebars_operand_type_boolean);

    VM_SETUP_OPTIONS(argc);
    options.name = opcode->op1.data.string.string;
    vm->last_helper = NULL;

    if (vm->flags & handlebars_compiler_flag_mustache_style_lambdas && handlebars_value_is_callable(value)) {
        result = invoke_mustache_style_lambda(vm, opcode, value, &options, rv);
        PUSH(vm->stack, result);
        vm->last_helper = handlebars_string_ctor(CONTEXT, HBS_STRL("lambda")); // hackey but it works
        handlebars_string_addref(vm->last_helper);
    } else if( NULL != (result = call_helper(options.name, argc, argv, &options, rv)) ) {
        append_to_buffer(vm, result, 0);
        vm->last_helper = options.name;
        handlebars_string_addref(vm->last_helper);
    } else if( value && handlebars_value_is_callable(value) ) {
        result = handlebars_value_call(value, argc, argv, &options, rv);
        PUSH(vm->stack, result);
    } else {
        result = handlebars_vm_call_helper_str(HBS_STRL("helperMissing"), argc, argv, &options, rv);
        append_to_buffer(vm, result, 0);
        PUSH(vm->stack, value);
    }

    VM_TEARDOWN_OPTIONS(argc);
    HANDLEBARS_VALUE_UNDECL(value);
    HANDLEBARS_VALUE_UNDECL(rv);
}

ACCEPT_FUNCTION(invoke_helper)
{
    HANDLEBARS_VALUE_DECL(value);
    HANDLEBARS_VALUE_DECL(rv);
    struct handlebars_value * result;

    HBS_ASSERT(POP(vm->stack, value));

    assert(opcode->op1.type == handlebars_operand_type_long);
    assert(opcode->op2.type == handlebars_operand_type_string);
    assert(opcode->op3.type == handlebars_operand_type_boolean);

    int argc = (int) opcode->op1.data.longval;
    VM_SETUP_OPTIONS(argc);
    options.name = opcode->op2.data.string.string;

    if( opcode->op3.data.boolval ) { // isSimple
        if( NULL != (result = call_helper(options.name, argc, argv, &options, rv)) ) {
            goto done;
        }
    }

    if( value && handlebars_value_is_callable(value) ) {
        result = handlebars_value_call(value, argc, argv, &options, rv);
    } else {
        result = handlebars_vm_call_helper_str(HBS_STRL("helperMissing"), argc, argv, &options, rv);
    }

done:
    PUSH(vm->stack, result);

    // @TODO this one isn't working
    // VM_TEARDOWN_OPTIONS(argc, &options);
    handlebars_options_deinit(&options);

    HANDLEBARS_VALUE_UNDECL(rv);
    HANDLEBARS_VALUE_UNDECL(value);
}

ACCEPT_FUNCTION(invoke_known_helper)
{
    HANDLEBARS_VALUE_DECL(rv);

    assert(opcode->op1.type == handlebars_operand_type_long);
    assert(opcode->op2.type == handlebars_operand_type_string);

    int argc = (int) opcode->op1.data.longval;
    VM_SETUP_OPTIONS(argc);
    options.name = opcode->op2.data.string.string;

    struct handlebars_value * result = call_helper(options.name, argc, argv, &options, rv);

    if( result == NULL ) {
        handlebars_throw(
            CONTEXT,
            HANDLEBARS_ERROR,
            "Invalid known helper: %.*s",
            (int) hbs_str_len(options.name), hbs_str_val(options.name)
        );
    }

    PUSH(vm->stack, result);

    VM_TEARDOWN_OPTIONS(argc);
    HANDLEBARS_VALUE_UNDECL(rv);
}

ACCEPT_FUNCTION(invoke_partial)
{
    const int argc = 1;
    struct handlebars_string * name = NULL;
    struct handlebars_value * partial = NULL;
    HANDLEBARS_VALUE_DECL(tmp);
    HANDLEBARS_VALUE_DECL(partial_rv);

    assert(opcode->op1.type == handlebars_operand_type_boolean);
    assert(opcode->op2.type == handlebars_operand_type_string || opcode->op2.type == handlebars_operand_type_null || opcode->op2.type == handlebars_operand_type_long);
    assert(opcode->op3.type == handlebars_operand_type_string);

    VM_SETUP_OPTIONS(argc);

    if( opcode->op1.data.boolval ) {
        // Dynamic partial
        HBS_ASSERT(POP(vm->stack, tmp));
        name = handlebars_value_get_string(tmp);
        options.name = NULL; // fear
    } else {
        if( opcode->op2.type == handlebars_operand_type_long ) {
            char tmp_str[32];
            size_t tmp_str_len = snprintf(tmp_str, 32, "%ld", opcode->op2.data.longval);
            name = handlebars_string_ctor(HBSCTX(vm), tmp_str, tmp_str_len);
            //name = MC(handlebars_talloc_asprintf(vm, "%ld", opcode->op2.data.longval));
        } else if( opcode->op2.type == handlebars_operand_type_string ) {
            name = opcode->op2.data.string.string;
        }
    }

    if( name ) {
        partial = handlebars_value_map_find(&vm->partials, name, partial_rv);
    }
    if( !partial ) {
        if( vm->flags & handlebars_compiler_flag_compat ) {
            return;
        } else {
            if (!name) {
                name = handlebars_string_ctor(CONTEXT, HBS_STRL("(NULL)"));
            }
            handlebars_throw(
                CONTEXT,
                HANDLEBARS_ERROR,
                "The partial %.*s could not be found",
                (int) hbs_str_len(name), hbs_str_val(name)
            );
        }
    }

    HANDLEBARS_VALUE_UNDECL(tmp);

    // Construct new context
    struct handlebars_context * context = handlebars_context_ctor_ex(vm);
    context->e = vm->ctx.e;

    // Merge hashes
    HANDLEBARS_VALUE_DECL(input_rv);
    struct handlebars_value * input = merge_hash(HBSCTX(vm), options.hash, argv[0], input_rv);

    // If partial is a function?
    if( handlebars_value_is_callable(partial) ) {
        HANDLEBARS_VALUE_DECL(rv);

        argv[0] = input;

        struct handlebars_value * ret = handlebars_value_call(partial, argc, argv, &options, rv);
        if (ret != NULL) {
            vm->buffer = handlebars_string_indent_append(HBSCTX(vm), vm->buffer, handlebars_value_expression(CONTEXT, ret, 0), opcode->op3.data.string.string);
        }

        HANDLEBARS_VALUE_UNDECL(rv);
    } else {

        if( !partial || handlebars_value_get_type(partial) != HANDLEBARS_VALUE_TYPE_STRING ) {
            handlebars_throw(
                CONTEXT,
                HANDLEBARS_ERROR,
                "The partial %s was not a string, was %u",
                name ? hbs_str_val(name) : "(NULL)",
                partial ? handlebars_value_get_type(partial) : 0
            );
        }

        struct handlebars_string *rv_str = execute_template(context, vm, handlebars_value_get_string(partial), input, opcode->op3.data.string.string, 0);
        assert(rv_str != NULL);
        vm->buffer = handlebars_string_append_str(CONTEXT, vm->buffer, rv_str);
        handlebars_string_delref(rv_str);

    }

    HANDLEBARS_VALUE_UNDECL(input_rv);
    handlebars_context_dtor(context);
    VM_TEARDOWN_OPTIONS(argc);
    HANDLEBARS_VALUE_UNDECL(partial_rv);
}

ACCEPT_FUNCTION(lookup_block_param)
{
    long blockParam1 = -1;
    long blockParam2 = -1;
    struct handlebars_value * v1 = NULL;
    size_t arr_len;
    struct handlebars_operand_string * arr;
    size_t i;
    HANDLEBARS_VALUE_DECL(empty_value);
    HANDLEBARS_VALUE_DECL(v2_rv);
    HANDLEBARS_VALUE_DECL(rv);
    HANDLEBARS_VALUE_DECL(value);
    struct handlebars_value * v2 = NULL;

    assert(opcode->op1.type == handlebars_operand_type_array);
    assert(opcode->op2.type == handlebars_operand_type_array);

    sscanf(hbs_str_val(opcode->op1.data.array.array[0].string), "%ld", &blockParam1);
    sscanf(hbs_str_val(opcode->op1.data.array.array[1].string), "%ld", &blockParam2);

    if( blockParam1 >= (long) LEN(vm->blockParamStack) ) goto done;

    v1 = GET(vm->blockParamStack, blockParam1);
    if( !v1 || handlebars_value_get_type(v1) != HANDLEBARS_VALUE_TYPE_ARRAY ) goto done;

    v2 = handlebars_value_array_find(v1, blockParam2, v2_rv);
    if( !v2 ) goto done;

    arr_len = opcode->op2.data.array.count;
    arr = opcode->op2.data.array.array;

    if( arr_len > 1 ) {
        struct handlebars_value * tmp = v2;
        struct handlebars_value * tmp2;
        for( i = 1; i < arr_len; i++ ) {
            tmp2 = handlebars_value_map_find(tmp, arr[i].string, rv);
            if( tmp2 ) {
                tmp = tmp2;
            } else {
                break;
            }
        }
        if (tmp) {
            handlebars_value_value(value, tmp);
        }
    } else {
        handlebars_value_value(value, v2);
    }

done:
    PUSH(vm->stack, value);

    HANDLEBARS_VALUE_UNDECL(value);
    HANDLEBARS_VALUE_UNDECL(rv);
    HANDLEBARS_VALUE_UNDECL(v2_rv);
    HANDLEBARS_VALUE_UNDECL(empty_value);
}

ACCEPT_FUNCTION(lookup_data)
{
    HANDLEBARS_VALUE_DECL(rv);
    HANDLEBARS_VALUE_DECL(data);
    HANDLEBARS_VALUE_DECL(val);
    struct handlebars_value * tmp;

    assert(opcode->op1.type == handlebars_operand_type_long);
    assert(opcode->op2.type == handlebars_operand_type_array);
    assert(opcode->op3.type == handlebars_operand_type_boolean || opcode->op3.type == handlebars_operand_type_null);

    handlebars_value_value(data, &vm->data);

    bool is_strict = (vm->flags & handlebars_compiler_flag_strict) || (vm->flags & handlebars_compiler_flag_assume_objects);
    bool require_terminal = (vm->flags & handlebars_compiler_flag_strict) && opcode->op3.data.boolval;

    long depth = opcode->op1.data.longval;
    size_t arr_len = opcode->op2.data.array.count;
    size_t i;
    struct handlebars_operand_string * arr = opcode->op2.data.array.array;
    struct handlebars_operand_string * first = arr;

    if( depth && data ) {
        while( data && depth-- ) {
            tmp = handlebars_value_map_str_find(data, HBS_STRL("_parent"), rv);
            if (tmp != NULL) {
                handlebars_value_value(data, tmp);
            }
        }
    }

    if( data && (tmp = handlebars_value_map_find(data, first->string, rv)) ) {
        handlebars_value_value(val, tmp);
    } else if (hbs_str_eq_strl(first->string, HBS_STRL("root"))) {
        handlebars_value_value(val, TOP(vm->contextStack));
    } else if( vm->flags & handlebars_compiler_flag_assume_objects ) {
        goto done_and_err;
    } else {
        goto done_and_null;
    }

    for( i = 1 ; i < arr_len; i++ ) {
        struct handlebars_operand_string * part = arr + i;
        if( handlebars_value_get_type(val) == HANDLEBARS_VALUE_TYPE_MAP &&
                NULL != (tmp = handlebars_value_map_find(val, part->string, rv)) ) {
            handlebars_value_value(val, tmp);
        } else if( is_strict || require_terminal ) {
            goto done_and_err;
        }
    }

    if( val->type == HANDLEBARS_VALUE_TYPE_NULL ) {
        done_and_null:
        if( require_terminal ) {
            done_and_err:
            handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "\"%.*s\" not defined in object", (int) hbs_str_len(arr->string), hbs_str_val(arr->string));
        }
    }

    PUSH(vm->stack, val);

    HANDLEBARS_VALUE_UNDECL(val);
    HANDLEBARS_VALUE_UNDECL(data);
    HANDLEBARS_VALUE_UNDECL(rv);
}

ACCEPT_FUNCTION(lookup_on_context)
{
    HANDLEBARS_VALUE_DECL(empty_value);
    HANDLEBARS_VALUE_DECL(rv);
    struct handlebars_value * value;

    assert(opcode->op1.type == handlebars_operand_type_array);
    assert(opcode->op2.type == handlebars_operand_type_boolean || opcode->op2.type == handlebars_operand_type_null);
    assert(opcode->op3.type == handlebars_operand_type_boolean || opcode->op3.type == handlebars_operand_type_null);
    assert(opcode->op4.type == handlebars_operand_type_boolean || opcode->op4.type == handlebars_operand_type_null);

    size_t arr_len = opcode->op1.data.array.count;
    struct handlebars_operand_string * arr = opcode->op1.data.array.array;
    struct handlebars_operand_string * arr_end = arr + arr_len;
    long index = -1;
    bool is_strict = (vm->flags & handlebars_compiler_flag_strict) || (vm->flags & handlebars_compiler_flag_assume_objects);
    bool require_terminal = (vm->flags & handlebars_compiler_flag_strict) && opcode->op3.data.boolval;

    if( !opcode->op4.data.boolval && (vm->flags & handlebars_compiler_flag_compat) ) {
        depthed_lookup(vm, arr->string);
    } else {
        ACCEPT_FN(push_context)(vm, opcode);
    }

    HBS_ASSERT((value = POP(vm->stack, rv)));

    do {
        bool is_last = arr == arr_end - 1;
        if( handlebars_value_get_type(value) == HANDLEBARS_VALUE_TYPE_MAP ) {
            value = handlebars_value_map_find(value, arr->string, rv);
        } else if( handlebars_value_get_type(value) == HANDLEBARS_VALUE_TYPE_ARRAY ) {
            if (sscanf(hbs_str_val(arr->string), "%ld", &index)) {
                value = handlebars_value_array_find(value, index, rv);
            } else {
                value = NULL;
            }
        } else if( vm->flags & handlebars_compiler_flag_assume_objects && is_last ) {
            handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "\"%.*s\" not defined in object", (int) hbs_str_len(arr->string), hbs_str_val(arr->string));
        } else {
            goto done_and_null;
        }
        if( !value ) {
            if( is_strict && !is_last ) {
                handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "\"%.*s\" not defined in object", (int) hbs_str_len(arr->string), hbs_str_val(arr->string));
            }
            goto done_and_null;
        }
    } while( ++arr < arr_end );

    if( value == NULL ) {
        done_and_null:
        if( require_terminal ) {
            handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "\"%.*s\" not defined in object", (int) hbs_str_len(arr->string), hbs_str_val(arr->string));
        } else {
            value = empty_value;
        }
    }

    PUSH(vm->stack, value);

    HANDLEBARS_VALUE_UNDECL(rv);
    HANDLEBARS_VALUE_UNDECL(empty_value);
}

ACCEPT_FUNCTION(pop_hash)
{
    HANDLEBARS_VALUE_DECL(hash);

    HBS_ASSERT(POP(vm->hashStack, hash));
    PUSH(vm->stack, hash);

    HANDLEBARS_VALUE_UNDECL(hash);
}

ACCEPT_FUNCTION(push_context)
{
    PUSH(vm->stack, vm->last_context);
}

ACCEPT_FUNCTION(push_hash)
{
    // @TODO not sure why decl/undecl is not working here
    struct handlebars_value hash = {0};
    handlebars_value_map(&hash, handlebars_map_ctor(CONTEXT, 4)); // number of items might be available somewhere
    PUSH(vm->hashStack, &hash);
}

ACCEPT_FUNCTION(push_program)
{
    HANDLEBARS_VALUE_DECL(value);

    if( opcode->op1.type == handlebars_operand_type_long ) {
        handlebars_value_integer(value, opcode->op1.data.longval);
    } else {
        handlebars_value_integer(value, -1);
    }

    PUSH(vm->stack, value);

    HANDLEBARS_VALUE_UNDECL(value);
}

ACCEPT_FUNCTION(push_literal)
{
    HANDLEBARS_VALUE_DECL(value);

    switch( opcode->op1.type ) {
        case handlebars_operand_type_string:
            // @todo should we move this to the parser?
            if (hbs_str_eq_strl(opcode->op1.data.string.string, HBS_STRL("undefined"))) {
                break;
            } else if (hbs_str_eq_strl(opcode->op1.data.string.string, HBS_STRL("null"))) {
                break;
            }
            handlebars_value_str(value, opcode->op1.data.string.string);
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

    PUSH(vm->stack, value);

    HANDLEBARS_VALUE_UNDECL(value);
}

ACCEPT_FUNCTION(push_string)
{
    // @TODO not sure why decl/undecl is not working here
    struct handlebars_value value = {0};

    assert(opcode->op1.type == handlebars_operand_type_string);

    handlebars_value_str(&value, opcode->op1.data.string.string);
    PUSH(vm->stack, &value);
}

ACCEPT_FUNCTION(resolve_possible_lambda)
{
    HANDLEBARS_VALUE_DECL(rv);
    HANDLEBARS_VALUE_DECL(value);

    HBS_ASSERT(POP(vm->stack, value));

    if( handlebars_value_is_callable(value) ) {
        struct handlebars_options options = {0};
        int argc = 1;
        struct handlebars_value * argv[1];
        argv[0] = alloca(HANDLEBARS_VALUE_SIZE);
        handlebars_value_init(argv[0]);
        handlebars_value_value(argv[0], TOP(vm->contextStack));
        options.vm = vm;
        options.scope = argv[0];
        struct handlebars_value * result = handlebars_value_call(value, argc, argv, &options, rv);
        HBS_ASSERT(result);
        PUSH(vm->stack, result);
        handlebars_options_deinit(&options);
    } else {
        PUSH(vm->stack, value);
    }

    HANDLEBARS_VALUE_UNDECL(value);
    HANDLEBARS_VALUE_UNDECL(rv);
}

static void handlebars_vm_accept(struct handlebars_vm * vm, struct handlebars_module_table_entry * entry)
{
#define ACCEPT_ERROR handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "Unhandled opcode: %s\n", handlebars_opcode_readable_type(opcode->type));
#if HAVE_COMPUTED_GOTOS
#define DISPATCH() goto *dispatch_table[opcode->type]
#define ACCEPT_LABEL(name) do_ ## name
#define ACCEPT_CASE(name) ACCEPT_LABEL(name):
#define ACCEPT(name) ACCEPT_LABEL(name): ACCEPT_FN(name)(vm, opcode); opcode++; DISPATCH();
    static void * dispatch_table[] = {
            &&do_nil, &&do_ambiguous_block_value, &&do_append, &&do_append_escaped, &&do_empty_hash,
            &&do_pop_hash, &&do_push_context, &&do_push_hash, &&do_resolve_possible_lambda, &&do_get_context,
            &&do_push_program, &&do_append_content, &&do_assign_to_hash, &&do_block_value, &&do_push,
            &&do_push_literal, &&do_push_string, &&do_invoke_partial, &&do_push_id, &&do_push_string_param,
            &&do_invoke_ambiguous, &&do_invoke_known_helper, &&do_invoke_helper, &&do_lookup_on_context, &&do_lookup_data,
            &&do_lookup_block_param, &&do_register_decorator, &&do_return
    };
#define ACCEPT_DEFAULT
#define START_ACCEPT DISPATCH();
#define END_ACCEPT
#else
#define ACCEPT_CASE(name) case OPCODE_NAME(name):
#define ACCEPT(name) case OPCODE_NAME(name) : ACCEPT_FN(name)(vm, opcode); opcode++; break;
#define ACCEPT_DEFAULT default: ACCEPT_ERROR
#define START_ACCEPT start: switch( opcode->type ) {
#define END_ACCEPT } goto start;
#endif

/*
#ifndef NDEBUG
    if( getenv("DEBUG") ) {
        struct handlebars_string * tmp = handlebars_opcode_print(HBSCTX(vm), opcode, 0);
        fprintf(stdout, "V[%ld] P[%ld] OPCODE: %.*s\n", vm->depth, entry->guid, (int) tmp->len, tmp->val);
        talloc_free(tmp);
    }
#endif
*/

    struct handlebars_opcode * opcode = entry->opcodes;
    START_ACCEPT
        ACCEPT(ambiguous_block_value)
        ACCEPT(append)
        ACCEPT(append_escaped)
        ACCEPT(append_content)
        ACCEPT(assign_to_hash)
        ACCEPT(block_value)
        ACCEPT(get_context)
        ACCEPT(empty_hash)
        ACCEPT(invoke_ambiguous)
        ACCEPT(invoke_helper)
        ACCEPT(invoke_known_helper)
        ACCEPT(invoke_partial)
        ACCEPT(lookup_block_param)
        ACCEPT(lookup_data)
        ACCEPT(lookup_on_context)
        ACCEPT(pop_hash)
        ACCEPT(push_context)
        ACCEPT(push_hash)
        ACCEPT(push_program)
        ACCEPT(push_literal)
        ACCEPT(push_string)
        ACCEPT(resolve_possible_lambda)

        // Special return opcode
        ACCEPT_CASE(return) return;

        // Unhandled opcodes
        ACCEPT_CASE(nil)
        ACCEPT_CASE(push)
        ACCEPT_CASE(push_id)
        ACCEPT_CASE(push_string_param)
        ACCEPT_CASE(register_decorator)
        ACCEPT_DEFAULT
            ACCEPT_ERROR
    END_ACCEPT
}

struct handlebars_string * handlebars_vm_execute_program_ex(
    struct handlebars_vm * vm,
    long program_num,
    struct handlebars_value * context,
    struct handlebars_value * data,
    struct handlebars_value * block_params
) {
    if( program_num < 0 ) {
        return handlebars_string_init(CONTEXT, 0);
    } else if( program_num >= (long) vm->module->program_count ) {
        handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "Invalid program: %ld", program_num);
    }

    // Get program
	struct handlebars_module_table_entry * entry = &vm->module->programs[program_num];

    // Save and set buffer
    struct handlebars_string * prevBuffer = vm->buffer;
    vm->buffer = handlebars_string_init(CONTEXT, HANDLEBARS_VM_BUFFER_INIT_SIZE);

    // Check stacks
    assert(vm->stack != NULL);
    assert(vm->contextStack != NULL);
    assert(vm->hashStack != NULL);
    assert(vm->blockParamStack != NULL);

    // Save stacks
    struct handlebars_stack_save_buf st = handlebars_stack_save(vm->stack);
    struct handlebars_stack_save_buf hst = handlebars_stack_save(vm->hashStack);
    struct handlebars_stack_save_buf cst = handlebars_stack_save(vm->contextStack);
    struct handlebars_stack_save_buf bst = handlebars_stack_save(vm->blockParamStack);

    // Push the context stack
    if( /*context &&*/ (LEN(vm->contextStack) <= 0 || !handlebars_value_eq(TOP(vm->contextStack), context)) ) {
        PUSH(vm->contextStack, context);
    }

    // Save and set data
    HANDLEBARS_VALUE_DECL(prev_data);
    if( data ) {
        handlebars_value_value(prev_data, &vm->data);
        handlebars_value_value(&vm->data, data);
    }

    // Set block params
    if( block_params ) {
        PUSH(vm->blockParamStack, block_params);
    }

    // Execute the program
	handlebars_vm_accept(vm, entry);

    // Restore stacks
    handlebars_stack_restore(vm->stack, st);
    handlebars_stack_restore(vm->hashStack, hst);
    handlebars_stack_restore(vm->contextStack, cst);
    handlebars_stack_restore(vm->blockParamStack, bst);

    // Clear last context
    if (vm->last_context) {
        handlebars_value_null(vm->last_context);
    }

    // Restore data
    if (data) {
        handlebars_value_value(&vm->data, prev_data);
    }
    HANDLEBARS_VALUE_UNDECL(prev_data);

    // Restore buffer
    struct handlebars_string * buffer = vm->buffer;
    vm->buffer = prevBuffer;

    return buffer;
}

struct handlebars_string * handlebars_vm_execute_program(struct handlebars_vm * vm, long program, struct handlebars_value * context)
{
    return handlebars_vm_execute_program_ex(vm, program, context, NULL, NULL);
}

struct handlebars_string * handlebars_vm_execute(
    struct handlebars_vm * vm,
    struct handlebars_module * module,
    struct handlebars_value * context
) {
    struct handlebars_error * e = HBSCTX(vm)->e;
    jmp_buf * prev = e->jmp;
    jmp_buf buf;

    // Save jump buffer
    if( !prev ) {
        if( handlebars_setjmp_ex(vm, &buf) ) {
            goto done;
        }
    }

    // Setup stacks
    if (vm->stack == NULL) {
        vm->stack = handlebars_stack_alloca(HBSCTX(vm), HANDLEBARS_VM_STACK_SIZE);
    }
    if (vm->contextStack == NULL) {
        vm->contextStack = handlebars_stack_alloca(HBSCTX(vm), HANDLEBARS_VM_STACK_SIZE);
    }
    if (vm->hashStack == NULL) {
        vm->hashStack = handlebars_stack_alloca(HBSCTX(vm), HANDLEBARS_VM_STACK_SIZE);
    }
    if (vm->blockParamStack == NULL) {
        vm->blockParamStack = handlebars_stack_alloca(HBSCTX(vm), HANDLEBARS_VM_STACK_SIZE);
    }
    if (vm->last_context == NULL) {
        vm->last_context = alloca(HANDLEBARS_VALUE_SIZE);
        handlebars_value_init(vm->last_context);
    }

    vm->module = module;

    // Execute
    vm->buffer = handlebars_vm_execute_program_ex(vm, 0, context, NULL, NULL);

    // Reset stacks
    vm->stack = NULL;
    vm->contextStack = NULL;
    vm->hashStack = NULL;
    vm->blockParamStack = NULL;
    vm->last_context = NULL;

done:
    // Reset
    vm->module = NULL;
    e->jmp = prev;

    return vm->buffer;
}
