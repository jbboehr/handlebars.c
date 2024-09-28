/**
 * Copyright (c) anno Domini nostri Jesu Christi MMXVI-MMXXIV John Boehr & contributors
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <stdio.h>
#include <string.h>

#define HANDLEBARS_OPCODE_SERIALIZER_PRIVATE
#define HANDLEBARS_OPCODES_PRIVATE

#include "handlebars.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"
#include "handlebars_value_private.h"
#include "handlebars_vm_private.h"

#include "handlebars_cache.h"
#include "handlebars_closure.h"
#include "handlebars_compiler.h"
#include "handlebars_delimiters.h"
#include "handlebars_helpers.h"
#include "handlebars_map.h"
#include "handlebars_parser.h"
#include "handlebars_ptr.h"
#include "handlebars_opcodes.h"
#include "handlebars_opcode_printer.h"
#include "handlebars_opcode_serializer.h"
#include "handlebars_stack.h"
#include "handlebars_string.h"
#include "handlebars_value.h"
#include "handlebars_value_handlers.h"
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

#undef CONTEXT
#define CONTEXT HBSCTX(vm)

// }}} Macros

// {{{ Prototypes & Variables

ACCEPT_FUNCTION(push_context);

const size_t HANDLEBARS_VM_SIZE = sizeof(struct handlebars_vm);

// }}} Prototypes & Variables

// {{{ Macros

static inline struct handlebars_value * _get(struct handlebars_stack * stack, size_t pos) {
    if (handlebars_stack_count(stack) < pos + 1) {
        return NULL;
    }
    return handlebars_stack_get(stack, handlebars_stack_count(stack) - pos - 1);
}

#define LEN(stack) handlebars_stack_count(stack)
#define TOP(stack) handlebars_stack_top(stack)
#define GET(stack, pos) _get(stack, pos)

#if 0
static inline struct handlebars_stack * push(struct handlebars_stack * stack, struct handlebars_value * value, struct handlebars_vm * vm, int line)
{
    fprintf(stderr, "V[%ld] L[%d] PUSH %s\n", vm->depth, line, handlebars_value_dump(value, HBSCTX(vm), 0));
    return handlebars_stack_push(stack, value);
}
#define PUSH(stack, value) (stack = push(stack, value, vm, __LINE__))
static inline struct handlebars_value * pop(struct handlebars_stack * stack, struct handlebars_value * rv, struct handlebars_vm * vm, int line)
{
    rv = handlebars_stack_pop(stack, rv);
    fprintf(stderr, "V[%ld] L[%d] POP %s\n", vm->depth, line, rv ? handlebars_value_dump(rv, HBSCTX(vm), 0) : "(nil)");
    return rv;
}
#define POP(stack, rv) pop(stack, rv, vm, __LINE__)
#else
#define POP(stack, rv) handlebars_stack_pop(stack, rv)
#define PUSH(stack, value) (stack = handlebars_stack_push(stack, value))
#endif

// }}} Macros

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
    handlebars_value_dtor(&vm->helpers);
    handlebars_value_dtor(&vm->partials);
    handlebars_value_dtor(&vm->data);
    if (vm->delim_open) {
        handlebars_string_delref(vm->delim_open);
    }
    if (vm->delim_close) {
        handlebars_string_delref(vm->delim_close);
    }
    handlebars_talloc_free(vm);
}

// }}} Constructors & Destructors

// {{{ Getters & Setters

void handlebars_vm_set_flags(struct handlebars_vm * vm, unsigned long flags)
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

void handlebars_vm_set_logger(struct handlebars_vm * vm, handlebars_func log_func, void * log_ctx)
{
    vm->log_func = log_func;
    vm->log_ctx = log_ctx;
}

handlebars_func handlebars_vm_get_log_func(struct handlebars_vm * vm)
{
    return vm->log_func;
}

void * handlebars_vm_get_log_ctx(struct handlebars_vm * vm)
{
    return vm->log_ctx;
}

// }}} Getters & Setters

HBS_ATTR_NONNULL_ALL
static inline struct handlebars_value * lookup_helper(
    struct handlebars_vm * vm,
    struct handlebars_string * string,
    struct handlebars_value * rv
) {
    HANDLEBARS_VALUE_DECL(rv2);
    struct handlebars_value * helper;
    handlebars_helper_func fn;
    if( NULL != (helper = handlebars_value_map_find(&vm->helpers, string, rv2)) ) {
        handlebars_value_value(rv, helper);
    } else if( NULL != (fn = handlebars_builtins_find(hbs_str_val(string), hbs_str_len(string))) ) {
        handlebars_value_helper(rv, fn);
    } else {
        rv = NULL;
    }
    HANDLEBARS_VALUE_UNDECL(rv2);
    return rv;
}

HBS_ATTR_NONNULL(1, 4, 5)
struct handlebars_value * handlebars_vm_call_helper_str(const char * name, unsigned int len, HANDLEBARS_HELPER_ARGS)
{
    HANDLEBARS_VALUE_DECL(rv2);
    struct handlebars_value * helper;
    handlebars_helper_func fn;
    if( NULL != (helper = handlebars_value_map_str_find(&vm->helpers, name, len, rv2)) ) {
        rv = handlebars_value_call(helper, HANDLEBARS_HELPER_ARGS_PASSTHRU);
    } else if( NULL != (fn = handlebars_builtins_find(name, len)) ) {
        rv = fn(HANDLEBARS_HELPER_ARGS_PASSTHRU);
    } else {
        rv = NULL;
    }
    HANDLEBARS_VALUE_UNDECL(rv2);
    return rv;
}

static inline void setup_options(struct handlebars_vm * vm, int argc, struct handlebars_value * argv, struct handlebars_options * options, struct handlebars_value * mem)
{
    struct handlebars_value * inverse;
    struct handlebars_value * program;
    int i;

    //options->name = ctx->name ? MC(handlebars_talloc_strndup(options, ctx->name->val, ctx->name->len)) : NULL;
    options->hash = POP(vm->stack, mem++);
    options->scope = mem++;
    handlebars_value_value(options->scope, TOP(vm->contextStack));
    options->data = mem++;
    handlebars_value_value(options->data, &vm->data);

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
    while( i-- ) {
        POP(vm->stack, &argv[i]);
    }
}

#define VM_SETUP_OPTIONS(argc) \
    struct handlebars_options options = {0}; \
    HANDLEBARS_VALUE_ARRAY_DECL(argv, argc); \
    HANDLEBARS_VALUE_ARRAY_DECL(extra, 5); \
    setup_options(vm, argc, argv, &options, extra)

#define VM_TEARDOWN_OPTIONS(argc) \
    HANDLEBARS_VALUE_ARRAY_UNDECL(extra, 5); \
    HANDLEBARS_VALUE_ARRAY_UNDECL(argv, argc); \
    handlebars_options_deinit(&options)

HBS_ATTR_NONNULL_ALL
static inline void append_to_buffer(struct handlebars_vm * vm, struct handlebars_value * result, bool escape)
{
    vm->buffer = handlebars_value_expression_append(CONTEXT, result, vm->buffer, escape);
}

HBS_ATTR_NONNULL_ALL
static inline void depthed_lookup(struct handlebars_vm * vm, struct handlebars_string * key)
{
    size_t i;
    size_t l;
    HANDLEBARS_VALUE_DECL(rv);
    HANDLEBARS_VALUE_DECL(empty_value);
    struct handlebars_value * value = empty_value;
    struct handlebars_value * tmp;

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

HBS_ATTR_NONNULL(1, 2)
static inline struct handlebars_value * merge_hash(struct handlebars_context * context, struct handlebars_value * input, struct handlebars_value * hash)
{
    if( handlebars_value_get_type(input) == HANDLEBARS_VALUE_TYPE_MAP && handlebars_value_count(input) > 0 &&
            hash && handlebars_value_get_type(hash) == HANDLEBARS_VALUE_TYPE_MAP && handlebars_value_count(hash) > 0 ) {
        struct handlebars_map * new_map = handlebars_map_ctor(context, handlebars_value_count(input) + handlebars_value_count(hash));
        HANDLEBARS_VALUE_FOREACH_KV(input, key, child) {
            new_map = handlebars_map_update(new_map, key, child);
        } HANDLEBARS_VALUE_FOREACH_END();
        HANDLEBARS_VALUE_FOREACH_KV(hash, key, child) {
            new_map = handlebars_map_update(new_map, key, child);
        } HANDLEBARS_VALUE_FOREACH_END();
        handlebars_value_map(input, new_map);
    } else if( handlebars_value_get_type(input) == HANDLEBARS_VALUE_TYPE_NULL && hash ) {
        handlebars_value_value(input, hash);
    }
    return input;
}

HBS_ATTR_NONNULL(1, 2)
static struct handlebars_string * execute_template(
    struct handlebars_vm * vm,
    struct handlebars_string * volatile tmpl,
    struct handlebars_value * input,
    struct handlebars_string * indent,
    int escape,
    bool use_delimiters
) {
    struct handlebars_context * context = handlebars_context_ctor_ex(vm);
    struct handlebars_string * volatile retval = NULL;
    struct handlebars_module * volatile module = vm->cache ? handlebars_cache_find(vm->cache, tmpl) : NULL;
    bool const from_cache = module != NULL;
    long prev_depth = vm->depth;
    jmp_buf * prev_jmp = HBSCTX(vm)->e->jmp;
    jmp_buf buf;

    handlebars_string_addref(tmpl);

    // Get template
    if (!hbs_str_len(tmpl)) {
        goto done;
    }

    // Save jmp buf
    if( handlebars_setjmp_ex(vm, &buf) ) {
        goto done;
    }

    // Check for cached template, if available
    if( !from_cache ) {
        // Parse
        struct handlebars_parser * parser = handlebars_parser_ctor(context);
        if (vm->flags & handlebars_compiler_flag_compat) {
            tmpl = handlebars_preprocess_delimiters(
                HBSCTX(context),
                tmpl,
                use_delimiters ? vm->delim_open : NULL,
                use_delimiters ? vm->delim_close : NULL
            );
            if (indent) {
                tmpl = handlebars_string_indent(CONTEXT, tmpl, indent);
            }
        }
        struct handlebars_ast_node * ast = handlebars_parse_ex(parser, tmpl, vm->flags);

        // Compile
        struct handlebars_compiler * compiler = handlebars_compiler_ctor(context);
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

    vm->depth++;

    retval = handlebars_vm_execute(vm, module, input);
    assert(retval != NULL);

    if (indent && !(vm->flags & handlebars_compiler_flag_compat)) {
        retval = handlebars_string_indent(CONTEXT, retval, indent);
    }

done:
    HBSCTX(vm)->e->jmp = prev_jmp;
    vm->depth = prev_depth;
    if( from_cache ) {
        handlebars_cache_release(vm->cache, tmpl, module);
    }
    handlebars_string_delref(tmpl);
    handlebars_context_dtor(context);
    if (retval) {
        return retval;
    } else {
        return handlebars_string_ctor(CONTEXT, HBS_STRL(""));
    }
}

HANDLEBARS_CLOSURE_ATTRS
static struct handlebars_value * invoke_partial_block_closure(HANDLEBARS_CLOSURE_ARGS)
{
    assert(localc >= 3);
    assert(HANDLEBARS_LOCAL_AT(0)->type == HANDLEBARS_VALUE_TYPE_PTR);
    assert(HANDLEBARS_LOCAL_AT(1)->type == HANDLEBARS_VALUE_TYPE_INTEGER);
    assert(HANDLEBARS_LOCAL_AT(2)->type == HANDLEBARS_VALUE_TYPE_INTEGER);

    struct handlebars_module * module = handlebars_value_get_ptr(HANDLEBARS_LOCAL_AT(0), struct handlebars_module);
    long program = handlebars_value_get_intval(HANDLEBARS_LOCAL_AT(1));
    long partial_block_depth = handlebars_value_get_intval(HANDLEBARS_LOCAL_AT(2));
    bool pushed_partial_block = false;

    // Push partial block
    if (partial_block_depth > 0) {
        pushed_partial_block = true;
        PUSH(vm->partialBlockStack, handlebars_stack_get(vm->partialBlockStack, partial_block_depth - 1));
    }

    struct handlebars_value * input = argc > 0 ? &argv[0] : TOP(vm->contextStack);
    struct handlebars_string * buffer;
    if (vm->module == module) {
        buffer = handlebars_vm_execute_program_ex(vm, program, input, NULL, TOP(vm->blockParamStack));
    } else {
        buffer = handlebars_vm_execute_ex(vm, module, input, program, NULL, TOP(vm->blockParamStack));
    }
    if (buffer) {
        handlebars_value_str(rv, buffer);
    }

    // Pop partial block
    if (pushed_partial_block) {
        // This should never happen, but we need to convince scan-build of it
        assert(vm->partialBlockStack != NULL);
        if (!vm->partialBlockStack) return rv;

        HANDLEBARS_VALUE_DECL(closure_value);
        POP(vm->partialBlockStack, closure_value);
        HANDLEBARS_VALUE_UNDECL(closure_value);
    }

    return rv;
}

HANDLEBARS_CLOSURE_ATTRS
static struct handlebars_value * invoke_partial_string_closure(HANDLEBARS_CLOSURE_ARGS)
{
    assert(localc >= 2);
    assert(HANDLEBARS_LOCAL_AT(0)->type == HANDLEBARS_VALUE_TYPE_STRING);
    assert(HANDLEBARS_LOCAL_AT(1)->type == HANDLEBARS_VALUE_TYPE_STRING || HANDLEBARS_LOCAL_AT(1)->type == HANDLEBARS_VALUE_TYPE_NULL);

    struct handlebars_string * tmpl = handlebars_value_get_string(HANDLEBARS_LOCAL_AT(0));
    struct handlebars_string * indent = HANDLEBARS_LOCAL_AT(1)->type == HANDLEBARS_VALUE_TYPE_STRING ? handlebars_value_get_string(HANDLEBARS_LOCAL_AT(1)) : NULL;
    struct handlebars_string * buffer = execute_template(
        vm,
        tmpl,
        &argv[0],
        indent,
        0,
        0
    );
    if (buffer) {
        handlebars_value_str(rv, buffer);
    }

    return rv;
}

HANDLEBARS_CLOSURE_ATTRS
static struct handlebars_value * invoke_mustache_style_lambda_closure(HANDLEBARS_CLOSURE_ARGS)
{
    assert(localc == 3);
    assert(handlebars_value_is_callable(HANDLEBARS_LOCAL_AT(0)));
    assert(HANDLEBARS_LOCAL_AT(1)->type == HANDLEBARS_VALUE_TYPE_STRING);
    assert(HANDLEBARS_LOCAL_AT(2)->type == HANDLEBARS_VALUE_TYPE_TRUE || HANDLEBARS_LOCAL_AT(2)->type == HANDLEBARS_VALUE_TYPE_FALSE);

    HANDLEBARS_VALUE_DECL(lambda_result);
    HANDLEBARS_VALUE_ARRAY_DECL(lambda_argv, 1);
    struct handlebars_value *callable = HANDLEBARS_LOCAL_AT(0);
    struct handlebars_string *lambda_tmpl = handlebars_value_get_string(HANDLEBARS_LOCAL_AT(1));
    bool use_delimiters = handlebars_value_get_boolval(HANDLEBARS_LOCAL_AT(2));

    handlebars_value_str(&lambda_argv[0], lambda_tmpl);

    handlebars_value_call(callable, 1, lambda_argv, options, vm, lambda_result);

    if (!handlebars_value_is_empty(lambda_result)) {
        struct handlebars_string * tmpl = handlebars_value_to_string(lambda_result, CONTEXT);
        struct handlebars_string * rv_str = execute_template(vm, tmpl, callable, NULL, 0, use_delimiters);
        handlebars_value_str(rv, rv_str);
    }

    HANDLEBARS_VALUE_ARRAY_UNDECL(lambda_argv, 1);
    HANDLEBARS_VALUE_UNDECL(lambda_result);

    return rv;
}






ACCEPT_FUNCTION(ambiguous_block_value)
{
    HANDLEBARS_VALUE_DECL(rv);

    if( vm->last_helper == NULL ) {
        VM_SETUP_OPTIONS(1);
        struct handlebars_value * result = handlebars_vm_call_helper_str(HBS_STRL("blockHelperMissing"), 1, argv, &options, vm, rv);
        assert(result != NULL);
        PUSH(vm->stack, result);
        VM_TEARDOWN_OPTIONS(1);
    } else if (hbs_str_eq_strl(vm->last_helper, HBS_STRL("lambda"))) {
        VM_SETUP_OPTIONS(0);
        handlebars_string_delref(vm->last_helper);
        vm->last_helper = NULL;
        VM_TEARDOWN_OPTIONS(0);
    } else {
        VM_SETUP_OPTIONS(0);
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

    struct handlebars_value * result = handlebars_vm_call_helper_str(HBS_STRL("blockHelperMissing"), argc, argv, &options, vm, rv);
    if (likely(result != NULL)) {
        append_to_buffer(vm, result, 0);
    }

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
        handlebars_value_null(vm->last_context);
    } else if( depth == 0 ) {
        handlebars_value_value(vm->last_context, TOP(vm->contextStack));
    } else {
        handlebars_value_value(vm->last_context, GET(vm->contextStack, depth));
    }
}

ACCEPT_FUNCTION(invoke_ambiguous)
{
    HANDLEBARS_VALUE_DECL(rv);
    HANDLEBARS_VALUE_DECL(value);
    HANDLEBARS_VALUE_DECL(fnv);
    struct handlebars_value * result;
    struct handlebars_value * fn;
    struct handlebars_string * last_helper = NULL;

    HBS_ASSERT(POP(vm->stack, value));
    const int argc = 0;
    bool is_callable = handlebars_value_is_callable(value);

    ACCEPT_FN(empty_hash)(vm, opcode);

    assert(opcode->op1.type == handlebars_operand_type_string);
    assert(opcode->op2.type == handlebars_operand_type_boolean);

    VM_SETUP_OPTIONS(argc);
    options.name = opcode->op1.data.string.string;
    vm->last_helper = NULL;

    if (vm->flags & handlebars_compiler_flag_mustache_style_lambdas && is_callable) {
        assert(opcode->op3.type == handlebars_operand_type_string);

        // Construct a closure of the lambda
        const int closure_localc = 3;
        HANDLEBARS_VALUE_ARRAY_DECL(closure_localv, closure_localc);

        handlebars_value_value(&closure_localv[0], value);
        handlebars_value_str(&closure_localv[1], opcode->op3.data.string.string);
        handlebars_value_boolean(&closure_localv[2], opcode->op2.data.boolval);

        struct handlebars_closure * closure = handlebars_closure_ctor(vm, invoke_mustache_style_lambda_closure, closure_localc, closure_localv);
        handlebars_value_closure(value, closure);
        fn = value;

        last_helper = handlebars_string_ctor(CONTEXT, HBS_STRL("lambda")); // hackey but it works
        handlebars_string_addref(last_helper);

        HANDLEBARS_VALUE_ARRAY_UNDECL(closure_localv, closure_localc);
    } else if( NULL != (fn = lookup_helper(vm, options.name, fnv)) ) {
        last_helper = options.name;
        handlebars_string_addref(last_helper);
    } else if (value && is_callable) {
        fn = value;
    } else {
        struct handlebars_string * tmp_str = handlebars_string_ctor(CONTEXT, HBS_STRL("helperMissing"));
        fn = lookup_helper(vm, tmp_str, fnv);
        handlebars_string_delref(tmp_str);
    }

    result = handlebars_value_call(fn, argc, argv, &options, vm, rv);

    // Before, the null case was only done for helperMissing
    if (result->type != HANDLEBARS_VALUE_TYPE_NULL) {
        PUSH(vm->stack, result);
    } else {
        PUSH(vm->stack, value);
    }

    vm->last_helper = last_helper;

    VM_TEARDOWN_OPTIONS(argc);
    HANDLEBARS_VALUE_UNDECL(fnv);
    HANDLEBARS_VALUE_UNDECL(value);
    HANDLEBARS_VALUE_UNDECL(rv);
}

ACCEPT_FUNCTION(invoke_helper)
{
    HANDLEBARS_VALUE_DECL(value);
    HANDLEBARS_VALUE_DECL(rv);
    HANDLEBARS_VALUE_DECL(fnv);
    struct handlebars_value * fn;

    HBS_ASSERT(POP(vm->stack, value));

    assert(opcode->op1.type == handlebars_operand_type_long);
    assert(opcode->op2.type == handlebars_operand_type_string);
    assert(opcode->op3.type == handlebars_operand_type_boolean);

    int argc = (int) opcode->op1.data.longval;
    VM_SETUP_OPTIONS(argc);
    options.name = opcode->op2.data.string.string;

    if (opcode->op3.data.boolval && NULL != (fn = lookup_helper(vm, options.name, fnv))) { // isSimple
        // fallthrough
    } else if (value && handlebars_value_is_callable(value)) {
        fn = value;
    } else {
        struct handlebars_string * tmp_str = handlebars_string_ctor(CONTEXT, HBS_STRL("helperMissing"));
        fn = lookup_helper(vm, tmp_str, fnv);
        handlebars_string_delref(tmp_str);
    }

    PUSH(vm->stack, handlebars_value_call(fn, argc, argv, &options, vm, rv));

    VM_TEARDOWN_OPTIONS(argc);
    HANDLEBARS_VALUE_UNDECL(fnv);
    HANDLEBARS_VALUE_UNDECL(rv);
    HANDLEBARS_VALUE_UNDECL(value);
}

ACCEPT_FUNCTION(invoke_known_helper)
{
    HANDLEBARS_VALUE_DECL(rv);
    HANDLEBARS_VALUE_DECL(fnv);

    assert(opcode->op1.type == handlebars_operand_type_long);
    assert(opcode->op2.type == handlebars_operand_type_string);

    int argc = (int) opcode->op1.data.longval;
    VM_SETUP_OPTIONS(argc);
    options.name = opcode->op2.data.string.string;

    struct handlebars_value * fn = lookup_helper(vm, options.name, fnv);

    if (unlikely(fn == NULL)) {
        handlebars_throw_ex(
            CONTEXT,
            HANDLEBARS_ERROR,
            &opcode->loc,
            "Invalid known helper: %.*s",
            (int) hbs_str_len(options.name),
            hbs_str_val(options.name)
        );
    }

    PUSH(vm->stack, handlebars_value_call(fn, argc, argv, &options, vm, rv));

    VM_TEARDOWN_OPTIONS(argc);
    HANDLEBARS_VALUE_UNDECL(fnv);
    HANDLEBARS_VALUE_UNDECL(rv);
}

ACCEPT_FUNCTION(invoke_partial)
{
    const int argc = 1;
    struct handlebars_string * name = NULL;
    struct handlebars_value * partial = NULL;
    HANDLEBARS_VALUE_DECL(tmp);
    HANDLEBARS_VALUE_DECL(partial_rv);
    HANDLEBARS_VALUE_DECL(rv);
    HANDLEBARS_VALUE_DECL(partial_block);
    struct handlebars_string * buffer = NULL;
    bool pushed_partial_block = false;

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

    if (name) {
        partial = handlebars_value_map_find(&vm->partials, name, partial_rv);
    }

    // Try to look up partial block
    if (!partial && name && hbs_str_eq_strl(name, HBS_STRL("@partial-block")) && LEN(vm->partialBlockStack) > 0) {
        partial = TOP(vm->partialBlockStack);
    }

    // Push partial block
    if (options.program > 0) {
        const int closure_localc = 3;
        HANDLEBARS_VALUE_ARRAY_DECL(closure_localv, closure_localc);
        handlebars_value_ptr(&closure_localv[0], handlebars_ptr_ctor(CONTEXT, struct handlebars_module, vm->module, true));
        handlebars_value_integer(&closure_localv[1], options.program);
        handlebars_value_integer(&closure_localv[2], LEN(vm->partialBlockStack));
        handlebars_value_closure(partial_block, handlebars_closure_ctor(vm, invoke_partial_block_closure, closure_localc, closure_localv));
        pushed_partial_block = true;
        PUSH(vm->partialBlockStack, partial_block);
        HANDLEBARS_VALUE_ARRAY_UNDECL(closure_localv, closure_localc);
    }

    // Merge hashes
    merge_hash(HBSCTX(vm), &argv[0], options.hash);

    if (!partial) {
        if (options.program >= 0) {
            partial = partial_block;
        } else if (vm->flags & handlebars_compiler_flag_compat) {
            goto done;
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

    // Wrap partial string in a closure to execute_template
    if (partial->type == HANDLEBARS_VALUE_TYPE_STRING) {
        const int closure_localc = 2;
        HANDLEBARS_VALUE_ARRAY_DECL(closure_localv, closure_localc);
        handlebars_value_str(&closure_localv[0], handlebars_value_get_string(partial));
        if (vm->flags & handlebars_compiler_flag_compat) {
            handlebars_value_str(&closure_localv[1], opcode->op3.data.string.string);
        }
        struct handlebars_closure * closure = handlebars_closure_ctor(vm, invoke_partial_string_closure, closure_localc, closure_localv);
        handlebars_value_closure(partial, closure);
        HANDLEBARS_VALUE_ARRAY_UNDECL(closure_localv, closure_localc);
    }

    // Throw if the partial is not callable
    if (!handlebars_value_is_callable(partial)) {
        handlebars_throw(
            CONTEXT,
            HANDLEBARS_ERROR,
            "The partial %s was not a string, was %s",
            name ? hbs_str_val(name) : "(nil)",
            partial ? handlebars_value_type_readable(partial->type) : "(nil)"
        );
    }

    // Finally, call the partial
    do {
        buffer = handlebars_value_expression(
            CONTEXT,
            handlebars_value_call(partial, argc, argv, &options, vm, rv),
            false
        );

        if (vm->flags & handlebars_compiler_flag_compat) {
            vm->buffer = handlebars_string_append_str(CONTEXT, vm->buffer, buffer);
        } else {
            vm->buffer = handlebars_string_indent_append(HBSCTX(vm), vm->buffer, buffer, opcode->op3.data.string.string);
        }
    } while (0);

done:
    // Pop partial block
    if (pushed_partial_block) {
        HANDLEBARS_VALUE_DECL(closure_value);
        POP(vm->partialBlockStack, closure_value);
        HANDLEBARS_VALUE_UNDECL(closure_value);
    }

    VM_TEARDOWN_OPTIONS(argc);
    HANDLEBARS_VALUE_UNDECL(partial_block);
    HANDLEBARS_VALUE_UNDECL(rv);
    HANDLEBARS_VALUE_UNDECL(partial_rv);
    HANDLEBARS_VALUE_UNDECL(tmp);
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
    } else if (hbs_str_eq_strl(first->string, HBS_STRL("partial-block"))) {
        handlebars_value_value(val, TOP(vm->partialBlockStack));
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
            handlebars_throw_ex(
                CONTEXT,
                HANDLEBARS_ERROR,
                &opcode->loc,
                "\"%.*s\" not defined in object",
                (int) hbs_str_len(arr->string), hbs_str_val(arr->string)
            );
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
    HANDLEBARS_VALUE_DECL(rv2);
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

    value = HBS_ASSERT(POP(vm->stack, rv));

    do {
        bool is_last = arr == arr_end - 1;
        if( handlebars_value_get_type(value) == HANDLEBARS_VALUE_TYPE_MAP ) {
            value = handlebars_value_map_find(value, arr->string, rv2);
        } else if( handlebars_value_get_type(value) == HANDLEBARS_VALUE_TYPE_ARRAY ) {
            if (sscanf(hbs_str_val(arr->string), "%ld", &index)) {
                value = handlebars_value_array_find(value, index, rv2);
            } else {
                value = NULL;
            }
        } else if( vm->flags & handlebars_compiler_flag_assume_objects && is_last ) {
            goto done_and_err;
        } else {
            goto done_and_null;
        }
        if( !value ) {
            if( is_strict && !is_last ) {
                goto done_and_err;
            }
            goto done_and_null;
        }
    } while( ++arr < arr_end );

    if( value == NULL ) {
        done_and_null:
        if( require_terminal ) {
            done_and_err:
            handlebars_throw_ex(
                CONTEXT,
                HANDLEBARS_ERROR,
                &opcode->loc,
                "\"%.*s\" not defined in object",
                (int) hbs_str_len(arr->string),
                hbs_str_val(arr->string)
            );
        } else {
            value = empty_value;
        }
    }

    PUSH(vm->stack, value);

    HANDLEBARS_VALUE_UNDECL(rv2);
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
    HANDLEBARS_VALUE_DECL(hash);

    handlebars_value_map(hash, handlebars_map_ctor(CONTEXT, 4)); // number of items might be available somewhere
    PUSH(vm->hashStack, hash);

    HANDLEBARS_VALUE_UNDECL(hash);
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
    HANDLEBARS_VALUE_DECL(value);

    assert(opcode->op1.type == handlebars_operand_type_string);

    handlebars_value_str(value, opcode->op1.data.string.string);
    PUSH(vm->stack, value);

    HANDLEBARS_VALUE_UNDECL(value);
}

ACCEPT_FUNCTION(resolve_possible_lambda)
{
    HANDLEBARS_VALUE_DECL(value);

    HBS_ASSERT(POP(vm->stack, value));

    if( handlebars_value_is_callable(value) ) {
        HANDLEBARS_VALUE_DECL(rv);
        // This should really use the same options object as invoke*
        struct handlebars_options options = {0};
        const int argc = 1;
        HANDLEBARS_VALUE_ARRAY_DECL(argv, argc);
        handlebars_value_value(&argv[0], TOP(vm->contextStack));
        options.scope = &argv[0];
        PUSH(vm->stack, handlebars_value_call(value, argc, argv, &options, vm, rv));
        HANDLEBARS_VALUE_ARRAY_UNDECL(argv, argc);
        handlebars_options_deinit(&options);
        HANDLEBARS_VALUE_UNDECL(rv);
    } else {
        PUSH(vm->stack, value);
    }

    HANDLEBARS_VALUE_UNDECL(value);
}

static void handlebars_vm_accept(struct handlebars_vm * vm, struct handlebars_module_table_entry * entry)
{
#if 0
#define ACCEPT_DEBUG() \
    do { \
        struct handlebars_string * tmp = handlebars_opcode_print(HBSCTX(vm), opcode, 0); \
        fprintf(stdout, "V[%ld] P[%ld] OPCODE: %.*s\n", vm->depth, entry->guid, (int) hbs_str_len(tmp), hbs_str_val(tmp)); \
        talloc_free(tmp); \
    } while (0)
#else
#define ACCEPT_DEBUG()
#endif
#define ACCEPT_ERROR handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "Unhandled opcode: %s\n", handlebars_opcode_readable_type(opcode->type));
#if HAVE_COMPUTED_GOTOS
#define DISPATCH() goto *dispatch_table[opcode->type]
#define ACCEPT_LABEL(name) do_ ## name
#define ACCEPT_CASE(name) ACCEPT_LABEL(name):
#define ACCEPT(name) ACCEPT_LABEL(name): ACCEPT_DEBUG(); ACCEPT_FN(name)(vm, opcode); opcode++; DISPATCH();
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

    struct handlebars_opcode * opcode = &vm->module->opcodes[entry->opcode_offset];
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
    struct handlebars_string * prev_buffer = vm->buffer;
    vm->buffer = handlebars_string_init(CONTEXT, HANDLEBARS_VM_BUFFER_INIT_SIZE);

    // Check stacks
    assert(vm->stack != NULL);
    assert(vm->contextStack != NULL);
    assert(vm->hashStack != NULL);
    assert(vm->blockParamStack != NULL);
    assert(vm->partialBlockStack != NULL);

    // Save stacks
    struct handlebars_stack_save_buf st = handlebars_stack_save(vm->stack);
    struct handlebars_stack_save_buf hst = handlebars_stack_save(vm->hashStack);
    struct handlebars_stack_save_buf cst = handlebars_stack_save(vm->contextStack);
    struct handlebars_stack_save_buf bst = handlebars_stack_save(vm->blockParamStack);
    struct handlebars_stack_save_buf pst = handlebars_stack_save(vm->partialBlockStack);

    // Push the context stack
    // if (LEN(vm->contextStack) <= 0 || TOP(vm->contextStack) != context) {
    if (LEN(vm->contextStack) <= 0 || !handlebars_value_eq(TOP(vm->contextStack), context)) {
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
    handlebars_stack_restore(vm->partialBlockStack, pst);

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
    vm->buffer = prev_buffer;

    return buffer;
}

struct handlebars_string * handlebars_vm_execute_program(struct handlebars_vm * vm, long program, struct handlebars_value * context)
{
    return handlebars_vm_execute_program_ex(vm, program, context, NULL, NULL);
}

struct handlebars_string * handlebars_vm_execute_ex(
    struct handlebars_vm * vm,
    struct handlebars_module * module,
    struct handlebars_value * context,
    long program,
    struct handlebars_value * data,
    struct handlebars_value * block_params
) {
    jmp_buf * prev = HBSCTX(vm)->e->jmp;
    struct handlebars_module * prev_module = vm->module;
    unsigned long prev_flags = vm->flags;
    struct handlebars_value * prev_last_context = vm->last_context;
    struct handlebars_string * prev_delim_open = vm->delim_open;
    struct handlebars_string * prev_delim_close = vm->delim_close;

    struct handlebars_string * buffer = NULL;
    bool volatile setup_stacks = false;
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
        vm->contextStack = handlebars_stack_alloca(HBSCTX(vm), HANDLEBARS_VM_STACK_SIZE);
        vm->hashStack = handlebars_stack_alloca(HBSCTX(vm), HANDLEBARS_VM_STACK_SIZE);
        vm->blockParamStack = handlebars_stack_alloca(HBSCTX(vm), HANDLEBARS_VM_STACK_SIZE);
        vm->partialBlockStack = handlebars_stack_alloca(HBSCTX(vm), HANDLEBARS_VM_STACK_SIZE);
        setup_stacks = true;
    }

    if (vm->last_context == NULL) {
        vm->last_context = alloca(HANDLEBARS_VALUE_SIZE);
        handlebars_value_init(vm->last_context);
    }

    vm->module = module;
    vm->flags |= module->flags;

    // Execute
    buffer = handlebars_vm_execute_program_ex(vm, program, context, data, block_params);

done:
    HBSCTX(vm)->e->jmp = prev;

    // Reset stacks
    if (setup_stacks) {
        vm->stack = NULL;
        vm->contextStack = NULL;
        vm->hashStack = NULL;
        vm->blockParamStack = NULL;
        vm->partialBlockStack = NULL;
    }

    // Reset
    vm->delim_open = prev_delim_open;
    vm->delim_close = prev_delim_close;
    vm->last_context = prev_last_context;
    vm->module = prev_module;
    vm->flags = prev_flags;

    return buffer;
}

struct handlebars_string * handlebars_vm_execute(
    struct handlebars_vm * vm,
    struct handlebars_module * module,
    struct handlebars_value * context
) {
    return handlebars_vm_execute_ex(vm, module, context, 0, NULL, NULL);
}
