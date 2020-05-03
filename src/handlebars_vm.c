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
#define HANDLEBARS_VM_PRIVATE

#include "handlebars.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"

#include "handlebars_cache.h"
#include "handlebars_compiler.h"
#include "handlebars_delimiters.h"
#include "handlebars_map.h"
#include "handlebars_parser.h"
#include "handlebars_opcodes.h"
#include "handlebars_opcode_printer.h"
#include "handlebars_opcode_serializer.h"
#include "handlebars_stack.h"
#include "handlebars_utils.h"
#include "handlebars_value.h"
#include "handlebars_vm.h"



#if INTELLIJ
#undef HAVE_COMPUTED_GOTOS
#endif

#define inline

#define OPCODE_NAME(name) handlebars_opcode_type_ ## name
#define ACCEPT_FN(name) accept_ ## name
#define ACCEPT_NAMED_FUNCTION(name) static inline void name (struct handlebars_vm * vm, struct handlebars_opcode * opcode)
#define ACCEPT_FUNCTION(name) ACCEPT_NAMED_FUNCTION(ACCEPT_FN(name))

#define LEN(stack) stack.i
#define BOTTOM(stack) stack.v[0]

#ifndef NDEBUG
#else
#endif

static inline struct handlebars_value *_top(struct handlebars_vm_stack *stack) {
    struct handlebars_value *rv;
    if (stack->i > 0) {
        rv = stack->v[stack->i - 1];
        handlebars_value_addref2(rv);
        return rv;
    }
    return NULL;
}

#define GET(stack, pos) handlebars_value_addref2(stack.v[stack.i - pos - 1])
#define TOP(stack) _top(&stack)
#define POP(stack) (stack.i > 0 ? stack.v[--stack.i] : NULL)
#define PUSH(stack, value) do { \
        assert(value != NULL); \
        if( stack.i < HANDLEBARS_VM_STACK_SIZE ) { \
            stack.v[stack.i++] = value; \
        } else { \
            handlebars_throw(HBSCTX(vm), HANDLEBARS_STACK_OVERFLOW, "Stack overflow in %s", #stack); \
        } \
    } while(0)

#define TOPCONTEXT TOP(vm->contextStack)

ACCEPT_FUNCTION(push_context);

size_t HANDLEBARS_VM_SIZE = sizeof(struct handlebars_vm);

extern inline void handlebars_vm_set_flags(struct handlebars_vm * vm, unsigned flags) HBS_ATTR_NONNULL_ALL;
extern inline void handlebars_vm_set_helpers(struct handlebars_vm * vm, struct handlebars_value * helpers) HBS_ATTR_NONNULL_ALL;
extern inline void handlebars_vm_set_partials(struct handlebars_vm * vm, struct handlebars_value * partials) HBS_ATTR_NONNULL_ALL;
extern inline void handlebars_vm_set_data(struct handlebars_vm * vm, struct handlebars_value * data) HBS_ATTR_NONNULL_ALL;
extern inline void handlebars_vm_set_cache(struct handlebars_vm * vm, struct handlebars_cache * cache) HBS_ATTR_NONNULL_ALL;


#undef CONTEXT
#define CONTEXT ctx

struct handlebars_vm * handlebars_vm_ctor(struct handlebars_context * ctx)
{
    struct handlebars_vm * vm =  MC(handlebars_talloc_zero(ctx, struct handlebars_vm));
    handlebars_context_bind(ctx, HBSCTX(vm));
    return vm;
}

#undef CONTEXT
#define CONTEXT HBSCTX(vm)


void handlebars_vm_dtor(struct handlebars_vm * vm)
{
    // @TODO FIXME
    // handlebars_value_try_delref(vm->helpers);
    // handlebars_value_try_delref(vm->partials);
    // handlebars_value_try_delref(vm->data);
    handlebars_talloc_free(vm);
}

HBS_ATTR_NONNULL(1, 3, 4)
static inline struct handlebars_value * call_helper(struct handlebars_string * string, int argc, struct handlebars_value * argv[], struct handlebars_options * options)
{
    struct handlebars_value * helper;
    struct handlebars_value * result;
    handlebars_helper_func fn;
    if( NULL != (helper = handlebars_value_map_find(options->vm->helpers, string)) ) {
        result = handlebars_value_call(helper, argc, argv, options);
        handlebars_value_delref(helper);
        return result;
    } else if( NULL != (fn = handlebars_builtins_find(string->val, string->len)) ) {
        return fn(argc, argv, options);
    }
    return NULL;
}

HBS_ATTR_NONNULL(1, 4, 5)
struct handlebars_value * handlebars_vm_call_helper_str(const char * name, unsigned int len, int argc, struct handlebars_value * argv[], struct handlebars_options * options)
{
    struct handlebars_value * helper;
    struct handlebars_value * result;
    handlebars_helper_func fn;
    assert(options->vm->helpers != NULL);
    if( NULL != (helper = handlebars_value_map_str_find(options->vm->helpers, name, len)) ) {
        result = handlebars_value_call(helper, argc, argv, options);
        handlebars_value_delref(helper);
        return result;
    } else if( NULL != (fn = handlebars_builtins_find(name, len)) ) {
        return fn(argc, argv, options);
    }
    return NULL;
}

static inline void setup_options(struct handlebars_vm * vm, int argc, struct handlebars_value * argv[], struct handlebars_options * options)
{
    struct handlebars_value * inverse;
    struct handlebars_value * program;
    int i;

    //options->name = ctx->name ? MC(handlebars_talloc_strndup(options, ctx->name->val, ctx->name->len)) : NULL;
    options->hash = POP(vm->stack);
    options->scope = TOPCONTEXT;
    options->vm = vm;

    // programs
    inverse = POP(vm->stack);
    program = POP(vm->stack);
    options->inverse = inverse ? handlebars_value_get_intval(inverse) : -1;
    options->program = program ? handlebars_value_get_intval(program) : -1;
    handlebars_value_try_delref(inverse);
    handlebars_value_try_delref(program);

    i = argc;
    while( i-- ) {
        argv[i] = POP(vm->stack);
    }

    // Data
    // @todo check useData
    if( vm->data ) {
        options->data = vm->data;
        handlebars_value_addref(vm->data);
    //} else if( vm->flags & handlebars_compiler_flag_use_data ) {
    } else {
        options->data = handlebars_value_ctor(CONTEXT);
    }
}

static inline void append_to_buffer(struct handlebars_vm * vm, struct handlebars_value * result, bool escape)
{
    if( result ) {
        vm->buffer = handlebars_value_expression_append(vm->buffer, result, escape);
        handlebars_value_delref(result);
    }
}

static inline void depthed_lookup(struct handlebars_vm * vm, struct handlebars_string * key)
{
    size_t i;
    size_t l;
    struct handlebars_value * value = NULL;
    struct handlebars_value * tmp;

    for( i = 0, l = LEN(vm->contextStack); i < l; i++ ) {
        value = GET(vm->contextStack, i);
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

    PUSH(vm->stack, value);
}

// static inline char * dump_stack(struct handlebars_stack * stack)
// {
//     struct handlebars_value * tmp = handlebars_value_ctor(stack->ctx);
//     char * str;
//     tmp->type = HANDLEBARS_VALUE_TYPE_ARRAY;
//     tmp->v.stack = stack;
//     str = handlebars_value_dump(tmp, 0);
//     talloc_steal(stack, str);
//     handlebars_talloc_free(tmp);
//     return str;
// }

// static inline void dump_vm_stack(struct handlebars_vm *vm)
// {
//     size_t i;
//     fprintf(stdout, "STACK[%lu]\n", (unsigned long) vm->stack.i);
//     for ( i = 0; i < vm->stack.i; i++ ) {
//         fprintf(stdout, "STACK[%lu]: %s\n", (unsigned long) i, handlebars_value_dump(vm->stack.v[i], 0));
//     }
//     fflush(stdout);
// }

static inline struct handlebars_value * merge_hash(struct handlebars_context * context, struct handlebars_value * hash, struct handlebars_value * context1)
{
    struct handlebars_value * context2;
    struct handlebars_value_iterator it;
    if( context1 && handlebars_value_get_type(context1) == HANDLEBARS_VALUE_TYPE_MAP &&
        hash && hash->type == HANDLEBARS_VALUE_TYPE_MAP ) {
        context2 = handlebars_value_ctor(context);
        handlebars_value_map_init(context2);
        handlebars_value_iterator_init(&it, context1);
        for( ; it.current ; it.next(&it) ) {
            handlebars_map_update(context2->v.map, it.key, it.current);
        }
        handlebars_value_iterator_init(&it, hash);
        for( ; it.current ; it.next(&it) ) {
            handlebars_map_update(context2->v.map, it.key, it.current);
        }
    } else if( !context1 || context1->type == HANDLEBARS_VALUE_TYPE_NULL ) {
        context2 = hash;
        if( context2 ) {
            handlebars_value_addref(context2);
        }
    } else {
        context2 = context1;
        handlebars_value_addref(context1);
    }
    return context2;
}

static inline struct handlebars_value * execute_template(
    struct handlebars_context * context,
    struct handlebars_vm * vm,
    struct handlebars_string * tmpl,
    struct handlebars_value * data,
    struct handlebars_string * indent,
    int escape
) {
    jmp_buf buf;
    struct handlebars_value *retval = handlebars_value_ctor(CONTEXT);

    // Save jump buffer
    if( setjmp(buf) ) {
        handlebars_rethrow(CONTEXT, context);
    }

    // Get template
    if( !tmpl || !tmpl->len ) {
        return retval;
    }

    // Check for cached template, if available
    struct handlebars_compiler * compiler;
    struct handlebars_module * module = vm->cache ? handlebars_cache_find(vm->cache, tmpl) : NULL;
    bool from_cache = module != NULL;
    if( !from_cache ) {
        // Recompile

        // Parse
        struct handlebars_parser * parser = handlebars_parser_ctor(context);
        if( vm->flags & handlebars_compiler_flag_compat ) {
            // @TODO free old template
            tmpl = handlebars_preprocess_delimiters(HBSCTX(parser), tmpl, NULL, NULL);
        }
        struct handlebars_ast_node * program = handlebars_parse_ex(parser, tmpl, vm->flags); // @todo fix setjmp

        // Compile
        compiler = handlebars_compiler_ctor(context);
        handlebars_compiler_set_flags(compiler, vm->flags);
        handlebars_compiler_compile(compiler, program);

        // Serialize
        module = handlebars_program_serialize(context, handlebars_compiler_get_program(compiler));

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
    if( setjmp(buf) ) {
        if( from_cache ) {
            handlebars_cache_release(vm->cache, tmpl, module);
        }
        handlebars_rethrow(CONTEXT, HBSCTX(vm2));
    }

    // Setup new VM
    vm2->depth = vm->depth + 1;
    vm2->flags = vm->flags;
    vm2->helpers = vm->helpers;
    vm2->partials = vm->partials;
    vm2->data = vm->data; // need to pass data along to partials

    // Copy stacks
    memcpy(&vm2->contextStack, &vm->contextStack, offsetof(struct handlebars_vm_stack, v) + (sizeof(struct handlebars_value *) * LEN(vm->contextStack)));
    memcpy(&vm2->blockParamStack, &vm->blockParamStack, offsetof(struct handlebars_vm_stack, v) + (sizeof(struct handlebars_value *) * LEN(vm->blockParamStack)));

    handlebars_vm_execute(vm2, module, data);

    if( vm2->buffer ) {
        struct handlebars_string * tmp2;
        if (indent) {
            tmp2 = handlebars_string_indent(HBSCTX(vm2), HBS_STR_STRL(vm2->buffer), HBS_STR_STRL(indent));
        } else {
            tmp2 = vm2->buffer;
        }
        handlebars_value_str_steal(retval, tmp2);
    }

    if( from_cache ) {
        handlebars_cache_release(vm->cache, tmpl, module);
    }

    vm2->data = NULL;
    handlebars_vm_dtor(vm2);

// done:
    return retval;
}










ACCEPT_FUNCTION(ambiguous_block_value)
{
    struct handlebars_value * current;
    struct handlebars_value * result;
    struct handlebars_options options = {0};
    struct handlebars_value * argv[1];

    options.name = vm->last_helper; // @todo dupe?
    setup_options(vm, 0, argv, &options);

    current = POP(vm->stack);
    if( !current ) { // @todo I don't think this should happen
        current = handlebars_value_ctor(CONTEXT);
    }
    argv[0] = current;

    if( vm->last_helper == NULL ) {
        result = handlebars_vm_call_helper_str(HBS_STRL("blockHelperMissing"), 1, argv, &options);
        PUSH(vm->stack, result);
    } else if (0 == strcmp(vm->last_helper->val, "lambda")) {
        PUSH(vm->stack, current);
        talloc_free(vm->last_helper);
        vm->last_helper = NULL;
    }

    handlebars_options_deinit(&options);
    //handlebars_value_delref(current); // @todo double-check
}

ACCEPT_FUNCTION(append)
{
    struct handlebars_value * value = POP(vm->stack);
    append_to_buffer(vm, value, 0);
}

ACCEPT_FUNCTION(append_escaped)
{
    struct handlebars_value * value = POP(vm->stack);
    append_to_buffer(vm, value, 1);
}

ACCEPT_FUNCTION(append_content)
{
    assert(opcode->type == handlebars_opcode_type_append_content);
    assert(opcode->op1.type == handlebars_operand_type_string);

    vm->buffer = handlebars_string_append(CONTEXT, vm->buffer, HBS_STR_STRL(opcode->op1.data.string.string));
}

ACCEPT_FUNCTION(assign_to_hash)
{
    struct handlebars_value * hash = TOP(vm->hashStack);
    struct handlebars_value * value = POP(vm->stack);

    assert(hash != NULL);
    assert(value != NULL);
    assert(opcode->op1.type == handlebars_operand_type_string);
    assert(hash->type == HANDLEBARS_VALUE_TYPE_MAP);

    handlebars_map_update(hash->v.map, opcode->op1.data.string.string, value);

    handlebars_value_delref(hash);
    handlebars_value_delref(value);
}

ACCEPT_FUNCTION(block_value)
{
    struct handlebars_value * current;
    struct handlebars_value * result;
    struct handlebars_options options = {0};
    struct handlebars_value * argv[1];

    assert(opcode->op1.type == handlebars_operand_type_string);

    options.name = opcode->op1.data.string.string;
    setup_options(vm, 0, argv, &options);

    current = POP(vm->stack);
    assert(current != NULL);
    argv[0] = current;

    result = handlebars_vm_call_helper_str(HBS_STRL("blockHelperMissing"), 1, argv, &options);
    append_to_buffer(vm, result, 0);

    //handlebars_value_delref(current); // @todo double-check
    handlebars_options_deinit(&options);
}

ACCEPT_FUNCTION(empty_hash)
{
    struct handlebars_value * value = handlebars_value_ctor(CONTEXT);
    handlebars_value_map_init(value);
    PUSH(vm->stack, value);
}

ACCEPT_FUNCTION(get_context)
{
    assert(opcode->type == handlebars_opcode_type_get_context);
    assert(opcode->op1.type == handlebars_operand_type_long);

    size_t depth = (size_t) opcode->op1.data.longval;
    size_t length = LEN(vm->contextStack);

    if( depth >= length ) {
        // @todo should we throw?
        vm->last_context = NULL;
    } else if( depth == 0 ) {
        vm->last_context = TOP(vm->contextStack);
    } else {
        vm->last_context = GET(vm->contextStack, depth);
    }
}

static inline struct handlebars_value * invoke_mustache_style_lambda(
    struct handlebars_vm *vm,
    struct handlebars_opcode *opcode,
    struct handlebars_value *value,
    struct handlebars_options *options
) {
    struct handlebars_value * argv[1];
    struct handlebars_value * rv;

    assert(opcode->op3.type == handlebars_operand_type_string);

    argv[0] = handlebars_value_ctor(CONTEXT);
    handlebars_value_str(argv[0], opcode->op3.data.string.string);

    struct handlebars_value * lambda_result = handlebars_value_call(value, 1, argv, options);
    if (handlebars_value_is_empty(lambda_result)) {
        rv = handlebars_value_ctor(CONTEXT);
        handlebars_value_try_delref(lambda_result);
        return rv;
    }

    struct handlebars_string * tmpl = handlebars_value_to_string(lambda_result);
    struct handlebars_context * context = handlebars_context_ctor_ex(vm);
    rv = execute_template(context, vm, tmpl, value, NULL, 0);
    rv = talloc_steal(CONTEXT, rv);
    handlebars_context_dtor(context);

    handlebars_value_try_delref(lambda_result);
    return rv;
}

ACCEPT_FUNCTION(invoke_ambiguous)
{
    struct handlebars_value * value = POP(vm->stack);
    struct handlebars_value * result;
    struct handlebars_options options = {0};
    struct handlebars_value * argv[1];

    ACCEPT_FN(empty_hash)(vm, opcode);

    assert(opcode->op1.type == handlebars_operand_type_string);
    assert(opcode->op2.type == handlebars_operand_type_boolean);

    options.name = opcode->op1.data.string.string;
    setup_options(vm, 0, NULL, &options);
    vm->last_helper = NULL;

    if (vm->flags & handlebars_compiler_flag_mustache_style_lambdas && handlebars_value_is_callable(value)) {
        result = invoke_mustache_style_lambda(vm, opcode, value, &options);
        PUSH(vm->stack, result);
        vm->last_helper = handlebars_string_ctor(CONTEXT, HBS_STRL("lambda")); // hackey but it works
    } else if( NULL != (result = call_helper(options.name, 0, argv, &options)) ) {
        append_to_buffer(vm, result, 0);
        vm->last_helper = options.name;
    } else if( value && handlebars_value_is_callable(value) ) {
        result = handlebars_value_call(value, 0, argv, &options);
        PUSH(vm->stack, result);
    } else {
        result = handlebars_vm_call_helper_str(HBS_STRL("helperMissing"), 0, argv, &options);
        append_to_buffer(vm, result, 0);
        PUSH(vm->stack, value);
    }

    handlebars_options_deinit(&options);
}

ACCEPT_FUNCTION(invoke_helper)
{
    struct handlebars_value * value = POP(vm->stack);
    struct handlebars_value * result;
    struct handlebars_options options = {0};

    assert(opcode->op1.type == handlebars_operand_type_long);
    assert(opcode->op2.type == handlebars_operand_type_string);
    assert(opcode->op3.type == handlebars_operand_type_boolean);

    int argc = (int) opcode->op1.data.longval;
    //struct handlebars_value * argv[argc];
    struct handlebars_value ** argv = alloca(sizeof(struct handlebars_value *) * argc);
    options.name = opcode->op2.data.string.string;
    setup_options(vm, argc, argv, &options);

    if( opcode->op3.data.boolval ) { // isSimple
        if( NULL != (result = call_helper(options.name, argc, argv, &options)) ) {
            goto done;
        }
    }

    if( value && handlebars_value_is_callable(value) ) {
        result = handlebars_value_call(value, argc, argv, &options);
    } else {
        result = handlebars_vm_call_helper_str(HBS_STRL("helperMissing"), argc, argv, &options);
        if( !result ) {
            result = handlebars_value_ctor(CONTEXT);
        }
    }

done:
    PUSH(vm->stack, result);

    handlebars_options_deinit(&options);
    handlebars_value_delref(value);
}

ACCEPT_FUNCTION(invoke_known_helper)
{
    struct handlebars_options options = {0};
    struct handlebars_value * result;

    assert(opcode->op1.type == handlebars_operand_type_long);
    assert(opcode->op2.type == handlebars_operand_type_string);

    int argc = (int) opcode->op1.data.longval;
    //struct handlebars_value * argv[argc];
    struct handlebars_value ** argv = alloca(sizeof(struct handlebars_value *) * argc);
    options.name = opcode->op2.data.string.string;
    setup_options(vm, argc, argv, &options);

    result = call_helper(options.name, argc, argv, &options);

    if( result == NULL ) {
        handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "Invalid known helper: %s", options.name->val);
    }

    PUSH(vm->stack, result);
    handlebars_options_deinit(&options);
}

ACCEPT_FUNCTION(invoke_partial)
{
    struct handlebars_options options = {0};
    struct handlebars_value * tmp = NULL;
    struct handlebars_string * name = NULL;
    struct handlebars_value * partial = NULL;
    int argc = 1;
    struct handlebars_value * argv[1];

    assert(opcode->op1.type == handlebars_operand_type_boolean);
    assert(opcode->op2.type == handlebars_operand_type_string || opcode->op2.type == handlebars_operand_type_null || opcode->op2.type == handlebars_operand_type_long);
    assert(opcode->op3.type == handlebars_operand_type_string);

    setup_options(vm, argc, argv, &options);

    if( opcode->op1.data.boolval ) {
        // Dynamic partial
        tmp = POP(vm->stack);
        assert(tmp != NULL);
        name = tmp->v.string;
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
        partial = handlebars_value_map_find(vm->partials, name);
    }
    if( !partial ) {
        if( vm->flags & handlebars_compiler_flag_compat ) {
            return;
        } else {
            if (!name) {
                name = handlebars_string_ctor(CONTEXT, HBS_STRL("(NULL)"));
            }
            handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "The partial %.*s could not be found", (int) name->len, name->val);
        }
    }

    // Construct new context
    struct handlebars_context * context = handlebars_context_ctor_ex(vm);
    context->e = vm->ctx.e;

    // If partial is a function?
    if( handlebars_value_is_callable(partial) ) {
        struct handlebars_value * context1 = argv[0];
        struct handlebars_value * context2 = merge_hash(HBSCTX(vm), options.hash, context1);
        argv[0] = context2;

        struct handlebars_value * ret = handlebars_value_call(partial, argc, argv, &options);
        struct handlebars_string * tmp2 = handlebars_value_expression(ret, 0);
        struct handlebars_string * tmp3 = handlebars_string_indent(HBSCTX(vm), tmp2->val, tmp2->len, HBS_STR_STRL(opcode->op3.data.string.string));
        vm->buffer = handlebars_string_append(CONTEXT, vm->buffer, HBS_STR_STRL(tmp3));
        handlebars_talloc_free(tmp3);
        handlebars_talloc_free(tmp2);
        handlebars_value_try_delref(ret);
    } else {

        if( !partial || partial->type != HANDLEBARS_VALUE_TYPE_STRING ) {
            handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "The partial %s was not a string, was %d", name ? name->val : "(NULL)", partial ? partial->type : -1);
        }

        struct handlebars_value * context1 = argv[0];
        struct handlebars_value * context2 = merge_hash(HBSCTX(context), options.hash, context1);
        struct handlebars_value *rv = execute_template(context, vm, partial->v.string, context2, opcode->op3.data.string.string, 0);
        vm->buffer = handlebars_string_append(CONTEXT, vm->buffer, HBS_STR_STRL(handlebars_value_to_string(rv)));
        handlebars_value_try_delref(rv);

    }

    handlebars_context_dtor(context);
    handlebars_value_try_delref(partial);
    handlebars_options_deinit(&options);
}

ACCEPT_FUNCTION(lookup_block_param)
{
    long blockParam1 = -1;
    long blockParam2 = -1;
    struct handlebars_value * value = NULL;
    struct handlebars_value * v1;
    struct handlebars_value * v2;
    size_t arr_len;
    struct handlebars_operand_string * arr;
    size_t i;

    assert(opcode->op1.type == handlebars_operand_type_array);
    assert(opcode->op2.type == handlebars_operand_type_array);

    sscanf(opcode->op1.data.array.array[0].string->val, "%ld", &blockParam1);
    sscanf(opcode->op1.data.array.array[1].string->val, "%ld", &blockParam2);

    if( blockParam1 >= LEN(vm->blockParamStack) ) goto done;

    v1 = GET(vm->blockParamStack, blockParam1);
    if( !v1 || handlebars_value_get_type(v1) != HANDLEBARS_VALUE_TYPE_ARRAY ) goto done;

    v2 = handlebars_value_array_find(v1, blockParam2);
    if( !v2 ) goto done;

    arr_len = opcode->op2.data.array.count;
    arr = opcode->op2.data.array.array;

    if( arr_len > 1 ) {
        struct handlebars_value * tmp = v2;
        struct handlebars_value * tmp2;
        for( i = 1; i < arr_len; i++ ) {
            tmp2 = handlebars_value_map_find(tmp, arr[i].string);
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
    PUSH(vm->stack, value);
}

ACCEPT_FUNCTION(lookup_data)
{
    struct handlebars_value * data = vm->data;
    struct handlebars_value * tmp;
    struct handlebars_value * val = NULL;

    assert(opcode->op1.type == handlebars_operand_type_long);
    assert(opcode->op2.type == handlebars_operand_type_array);
    assert(opcode->op3.type == handlebars_operand_type_boolean || opcode->op3.type == handlebars_operand_type_null);
    bool is_strict = (vm->flags & handlebars_compiler_flag_strict) || (vm->flags & handlebars_compiler_flag_assume_objects);
    bool require_terminal = (vm->flags & handlebars_compiler_flag_strict) && opcode->op3.data.boolval;

    long depth = opcode->op1.data.longval;
    size_t arr_len = opcode->op2.data.array.count;
    size_t i;
    struct handlebars_operand_string * arr = opcode->op2.data.array.array;
    struct handlebars_operand_string * first = arr;

    if( depth && data ) {
        handlebars_value_addref(data);
        while( data && depth-- ) {
            tmp = handlebars_value_map_str_find(data, HBS_STRL("_parent"));
            handlebars_value_delref(data);
            data = tmp;
        }
    }

    if( data && (tmp = handlebars_value_map_find(data, first->string)) ) {
        val = tmp;
    } else if( 0 == strcmp(first->string->val, "root") ) {
        val = BOTTOM(vm->contextStack);
    } else if( vm->flags & handlebars_compiler_flag_assume_objects ) {
        handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "\"%.*s\" not defined in object", (int) arr->string->len, arr->string->val);
    } else {
        goto done_and_null;
    }

    assert(val != NULL);

    for( i = 1 ; i < arr_len; i++ ) {
        struct handlebars_operand_string * part = arr + i;
        if( val && handlebars_value_get_type(val) == HANDLEBARS_VALUE_TYPE_MAP ) {
            tmp = handlebars_value_map_find(val, part->string);
            handlebars_value_delref(val);
            val = tmp;
        } else if( is_strict ) {
            handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "\"%.*s\" not defined in object", (int) arr->string->len, arr->string->val);
        } else {
            goto done_and_null;
        }
    }

    if( !val ) {
        done_and_null:
        if( require_terminal ) {
            handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "\"%.*s\" not defined in object", (int) arr->string->len, arr->string->val);
        } else {
            val = handlebars_value_ctor(CONTEXT);
        }
    }

    PUSH(vm->stack, val);
}

ACCEPT_FUNCTION(lookup_on_context)
{
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

    struct handlebars_value * value = TOP(vm->stack);
    struct handlebars_value * tmp;

    assert(value != NULL);

    do {
        bool is_last = arr == arr_end - 1;
        if( handlebars_value_get_type(value) == HANDLEBARS_VALUE_TYPE_MAP ) {
            tmp = handlebars_value_map_find(value, arr->string);
            handlebars_value_try_delref(value);
            value = tmp;
        } else if( handlebars_value_get_type(value) == HANDLEBARS_VALUE_TYPE_ARRAY ) {
            if (sscanf(arr->string->val, "%ld", &index)) {
                tmp = handlebars_value_array_find(value, index);
            } else {
                tmp = NULL;
            }
            handlebars_value_try_delref(value);
            value = tmp;
        } else if( vm->flags & handlebars_compiler_flag_assume_objects && is_last ) {
            handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "\"%.*s\" not defined in object", (int) arr->string->len, arr->string->val);
        } else {
            goto done_and_null;
        }
        if( !value ) {
            if( is_strict && !is_last ) {
                handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "\"%.*s\" not defined in object", (int) arr->string->len, arr->string->val);
            }
            goto done_and_null;
        }
    } while( ++arr < arr_end );

    if( value == NULL ) {
        done_and_null:
        if( require_terminal ) {
            handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "\"%.*s\" not defined in object", (int) arr->string->len, arr->string->val);
        } else {
            value = handlebars_value_ctor(CONTEXT);
        }
    }

    struct handlebars_value *t = POP(vm->stack);
    if (t) {
        handlebars_value_delref(t);
    }
    PUSH(vm->stack, value);
}

ACCEPT_FUNCTION(pop_hash)
{
    struct handlebars_value * hash = POP(vm->hashStack);
    assert(hash != NULL);
    PUSH(vm->stack, hash);
}

ACCEPT_FUNCTION(push_context)
{
    struct handlebars_value * value = vm->last_context;

    if( !value ) {
        value = handlebars_value_ctor(CONTEXT);
    } else {
        handlebars_value_addref(value);
    }

    PUSH(vm->stack, value);
}

ACCEPT_FUNCTION(push_hash)
{
    struct handlebars_value * hash = handlebars_value_ctor(CONTEXT);
    handlebars_value_map_init(hash);
    PUSH(vm->hashStack, hash);
}

ACCEPT_FUNCTION(push_program)
{
    struct handlebars_value * value = handlebars_value_ctor(CONTEXT);

    if( opcode->op1.type == handlebars_operand_type_long ) {
        handlebars_value_integer(value, opcode->op1.data.longval);
    } else {
        handlebars_value_integer(value, -1);
    }

    PUSH(vm->stack, value);
}

ACCEPT_FUNCTION(push_literal)
{
    struct handlebars_value * value = handlebars_value_ctor(CONTEXT);

    switch( opcode->op1.type ) {
        case handlebars_operand_type_string:
            // @todo should we move this to the parser?
            if( 0 == strcmp(opcode->op1.data.string.string->val, "undefined") ) {
                break;
            } else if( 0 == strcmp(opcode->op1.data.string.string->val, "null") ) {
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
}

ACCEPT_FUNCTION(push_string)
{
    struct handlebars_value * value = handlebars_value_ctor(CONTEXT);

    assert(opcode->op1.type == handlebars_operand_type_string);

    handlebars_value_str(value, opcode->op1.data.string.string);
    PUSH(vm->stack, value);
}

ACCEPT_FUNCTION(resolve_possible_lambda)
{
    struct handlebars_value * top = TOP(vm->stack);
    struct handlebars_value * result;

    assert(top != NULL);

    if( handlebars_value_is_callable(top) ) {
        struct handlebars_options options = {0};
        int argc = 1;
        struct handlebars_value * argv[1/*argc*/];
        argv[0] = TOPCONTEXT;
        options.vm = vm;
        options.scope = TOPCONTEXT;
        result = handlebars_value_call(top, argc, argv, &options);
        struct handlebars_value *t = POP(vm->stack);
        if (t) {
            handlebars_value_try_delref(t);
        }
        PUSH(vm->stack, result);
        handlebars_options_deinit(&options);
    }
    handlebars_value_delref(top);
}

static inline void handlebars_vm_accept(struct handlebars_vm * vm, struct handlebars_module_table_entry * entry)
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
    bool pushed_context = false;
    bool pushed_block_param = false;

    if( program_num < 0 ) {
        return handlebars_string_init(CONTEXT, 0);
    } else if( program_num >= vm->module->program_count ) {
        handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "Invalid program: %ld", program_num);
    }

    // Get program
	struct handlebars_module_table_entry * entry = &vm->module->programs[program_num];

    // Save and set buffer
    struct handlebars_string * prevBuffer = vm->buffer;
    vm->buffer = handlebars_string_init(CONTEXT, HANDLEBARS_VM_BUFFER_INIT_SIZE);

    // Push the context stack
    if( LEN(vm->contextStack) <= 0 || TOP(vm->contextStack) != context ) {
        PUSH(vm->contextStack, context);
        pushed_context = true;
    }

    // Save and set data
    struct handlebars_value * prevData = vm->data;
    if( data ) {
        vm->data = data;
    }

    // Set block params
    if( block_params ) {
        PUSH(vm->blockParamStack, block_params);
        pushed_block_param = true;
    }

    // Execute the program
	handlebars_vm_accept(vm, entry);

    // Pop context stack
    if( pushed_context ) {
        POP(vm->contextStack);
    }

    // Pop block params
    if( pushed_block_param ) {
        POP(vm->blockParamStack);
    }

    // Restore data
    vm->data = prevData;

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

    vm->module = module;

    // Save context
    handlebars_value_addref(context);
    vm->context = context;

    // Execute
    vm->buffer = handlebars_vm_execute_program_ex(vm, 0, context, vm->data, NULL);

done:
    // Release context
    handlebars_value_delref(context);

    // Reset
    vm->module = NULL;
    vm->guid_index = 0;
    e->jmp = prev;

    return vm->buffer;
}
