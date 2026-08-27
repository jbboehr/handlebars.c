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
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
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
#define ACCEPT_NOINLINE_FUNCTION(name) static void ACCEPT_FN(name) (struct handlebars_vm * vm, struct handlebars_opcode * opcode)

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
    handlebars_value_map(&vm->helpers, handlebars_map_ctor(HBSCTX(vm), 0));
    handlebars_value_map(&vm->partials, handlebars_map_ctor(HBSCTX(vm), 0));
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

HBS_LOCAL void handlebars_vm_call_checkpoint_begin(
    struct handlebars_vm * vm,
    struct handlebars_vm_call_checkpoint * checkpoint
)
{
    checkpoint->open = false;
    if( vm->stack == NULL ) {
        assert(vm->hashStack == NULL);
        assert(vm->contextStack == NULL);
        assert(vm->blockParamStack == NULL);
        assert(vm->partialBlockStack == NULL);
        assert(vm->partialScopeStack == NULL);
        return;
    }

    assert(vm->hashStack != NULL);
    assert(vm->contextStack != NULL);
    assert(vm->blockParamStack != NULL);
    assert(vm->partialBlockStack != NULL);
    assert(vm->partialScopeStack != NULL);
    checkpoint->stack = handlebars_stack_save(vm->stack);
    checkpoint->hash_stack = handlebars_stack_save(vm->hashStack);
    checkpoint->context_stack = handlebars_stack_save(vm->contextStack);
    checkpoint->block_param_stack = handlebars_stack_save(vm->blockParamStack);
    checkpoint->partial_block_stack = handlebars_stack_save(vm->partialBlockStack);
    checkpoint->partial_scope_stack = handlebars_stack_save(vm->partialScopeStack);
    checkpoint->buffer = vm->buffer;
    checkpoint->depth = vm->depth;
    checkpoint->open = true;
}

HBS_LOCAL void handlebars_vm_call_checkpoint_commit(
    struct handlebars_vm * vm,
    struct handlebars_vm_call_checkpoint * checkpoint
)
{
    if( !checkpoint->open ) {
        return;
    }

    handlebars_stack_protect(vm->stack, checkpoint->stack.protect);
    handlebars_stack_protect(vm->hashStack, checkpoint->hash_stack.protect);
    handlebars_stack_protect(vm->contextStack, checkpoint->context_stack.protect);
    handlebars_stack_protect(vm->blockParamStack, checkpoint->block_param_stack.protect);
    handlebars_stack_protect(vm->partialBlockStack, checkpoint->partial_block_stack.protect);
    handlebars_stack_protect(vm->partialScopeStack, checkpoint->partial_scope_stack.protect);
    checkpoint->open = false;
}

HBS_LOCAL void handlebars_vm_call_checkpoint_rollback(
    struct handlebars_vm * vm,
    struct handlebars_vm_call_checkpoint * checkpoint
)
{
    if( !checkpoint->open ) {
        return;
    }

    handlebars_stack_restore(vm->stack, checkpoint->stack);
    handlebars_stack_restore(vm->hashStack, checkpoint->hash_stack);
    handlebars_stack_restore(vm->contextStack, checkpoint->context_stack);
    handlebars_stack_restore(vm->blockParamStack, checkpoint->block_param_stack);
    handlebars_stack_restore(vm->partialBlockStack, checkpoint->partial_block_stack);
    handlebars_stack_restore(vm->partialScopeStack, checkpoint->partial_scope_stack);
    if( vm->buffer != checkpoint->buffer ) {
        if( vm->buffer != NULL ) {
            handlebars_string_delref(vm->buffer);
        }
        vm->buffer = checkpoint->buffer;
    }
    vm->depth = checkpoint->depth;
    checkpoint->open = false;
}

HBS_LOCAL void handlebars_vm_call_checkpoint_finish(
    struct handlebars_vm * vm,
    struct handlebars_vm_call_checkpoint * checkpoint,
    enum handlebars_error_type result
)
{
    if( result == HANDLEBARS_SUCCESS ) {
        handlebars_vm_call_checkpoint_commit(vm, checkpoint);
    } else {
        handlebars_vm_call_checkpoint_rollback(vm, checkpoint);
    }
}

HBS_LOCAL HBS_ATTR_NORETURN void handlebars_vm_rethrow_caught(
    struct handlebars_vm * vm,
    jmp_buf * previous,
    enum handlebars_error_type caught
)
{
    assert(caught != HANDLEBARS_SUCCESS);
    if( likely(previous != NULL) ) {
        handlebars_longjmp(HBSCTX(vm), previous, caught);
    }
    fprintf(
        stderr,
        "Throw with invalid jmp_buf: %s\n",
        handlebars_error_msg(HBSCTX(vm))
    );
    abort();
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

HBS_ATTR_NONNULL_ALL
static inline struct handlebars_value * lookup_partial(
    struct handlebars_vm * vm,
    struct handlebars_string * name,
    struct handlebars_value * rv
) {
    if( vm->partialScopeStack != NULL ) {
        size_t count = LEN(vm->partialScopeStack);
        for( size_t i = 0; i < count; i++ ) {
            struct handlebars_value * scope = GET(vm->partialScopeStack, i);
            struct handlebars_value * partial;

            if( unlikely(scope == NULL
                    || handlebars_value_get_type(scope) != HANDLEBARS_VALUE_TYPE_MAP) ) {
                handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "Invalid inline partial scope");
            }
            partial = handlebars_value_map_find(scope, name, rv);
            if( partial != NULL ) {
                return partial;
            }
        }
    }
    return handlebars_value_map_find(&vm->partials, name, rv);
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

static inline size_t program_block_params(struct handlebars_vm * vm, long program)
{
    struct handlebars_module_table_entry * entry;

    if( program < 0 ) {
        return 0;
    }
    if( unlikely((size_t) program >= vm->module->program_count) ) {
        handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "Invalid program: %ld", program);
    }
    entry = &vm->module->programs[program];
    if( unlikely(entry->guid != (size_t) program || entry->block_params > 2) ) {
        handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "Invalid program metadata: %ld", program);
    }
    return entry->block_params;
}

static inline bool helper_context_is_truthy(struct handlebars_value * value)
{
    switch( value->type ) {
        case HANDLEBARS_VALUE_TYPE_NULL:
        case HANDLEBARS_VALUE_TYPE_FALSE:
            return false;
        case HANDLEBARS_VALUE_TYPE_INTEGER:
            return value->v.lval != 0;
        case HANDLEBARS_VALUE_TYPE_FLOAT:
            return value->v.dval != 0.0 && !isnan(value->v.dval);
        case HANDLEBARS_VALUE_TYPE_STRING:
            return hbs_str_len(value->v.string) != 0;
        default:
            return true;
    }
}

static void setup_options(struct handlebars_vm * vm, int argc, struct handlebars_value * argv, struct handlebars_options * options, struct handlebars_value * mem)
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
        options->program_block_params = program_block_params(vm, options->program);
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
    struct handlebars_module * volatile module = NULL;
    bool volatile from_cache = false;
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

    // Check for cached template, if available
    module = vm->cache ? handlebars_cache_find(vm->cache, tmpl) : NULL;
    from_cache = module != NULL;
    if( !from_cache ) {
        // Parse
        struct handlebars_parser * parser = handlebars_parser_ctor(context);
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
    assert(localc >= 4);
    assert(HANDLEBARS_LOCAL_AT(0)->type == HANDLEBARS_VALUE_TYPE_PTR);
    assert(HANDLEBARS_LOCAL_AT(1)->type == HANDLEBARS_VALUE_TYPE_INTEGER);
    assert(HANDLEBARS_LOCAL_AT(2)->type == HANDLEBARS_VALUE_TYPE_INTEGER);
    assert(HANDLEBARS_LOCAL_AT(3)->type == HANDLEBARS_VALUE_TYPE_ARRAY);

    struct handlebars_module * module = handlebars_value_get_ptr(HANDLEBARS_LOCAL_AT(0), struct handlebars_module);
    long program = handlebars_value_get_intval(HANDLEBARS_LOCAL_AT(1));
    long partial_block_depth = handlebars_value_get_intval(HANDLEBARS_LOCAL_AT(2));
    struct handlebars_stack * captured_block_params = handlebars_value_get_stack(HANDLEBARS_LOCAL_AT(3));
    struct handlebars_stack * previous_block_params = vm->blockParamStack;
    struct handlebars_stack_save_buf captured_block_params_save = handlebars_stack_save(captured_block_params);
    struct handlebars_stack_save_buf stack_save = handlebars_stack_save(vm->stack);
    struct handlebars_stack_save_buf hash_stack_save = handlebars_stack_save(vm->hashStack);
    struct handlebars_stack_save_buf context_stack_save = handlebars_stack_save(vm->contextStack);
    struct handlebars_stack_save_buf partial_block_stack_save = handlebars_stack_save(vm->partialBlockStack);
    struct handlebars_stack_save_buf partial_scope_stack_save = handlebars_stack_save(vm->partialScopeStack);
    struct handlebars_string * previous_buffer = vm->buffer;
    long previous_depth = vm->depth;
    struct handlebars_error * error = HBSCTX(vm)->e;
    jmp_buf * volatile prev_jmp = error->jmp;
    enum handlebars_error_type volatile caught = HANDLEBARS_SUCCESS;
    struct handlebars_stack * current_block_params;
    struct handlebars_value * input;
    struct handlebars_string * buffer;
    jmp_buf buf;

    /* Keep the closure's snapshot alive when PUSH transfers or separates the
     * VM's working reference. */
    handlebars_stack_addref(captured_block_params);
    vm->blockParamStack = captured_block_params;

    if( handlebars_setjmp_ex(vm, &buf) ) {
        caught = error->num;
        goto done;
    }

    // Push partial block
    if (partial_block_depth > 0) {
        struct handlebars_value * partial_block = handlebars_stack_get(vm->partialBlockStack, partial_block_depth - 1);
        if( unlikely(partial_block == NULL) ) {
            handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "Invalid partial block depth: %ld", partial_block_depth);
        }
        PUSH(vm->partialBlockStack, partial_block);
    }

    input = argc > 0 ? &argv[0] : TOP(vm->contextStack);
    if (vm->module == module) {
        buffer = handlebars_vm_execute_program_ex(vm, program, input, NULL, NULL);
    } else {
        buffer = handlebars_vm_execute_ex(vm, module, input, program, NULL, NULL);
    }
    if (buffer) {
        handlebars_value_str(rv, buffer);
    }

done:
    current_block_params = vm->blockParamStack;
    error->jmp = prev_jmp;
    if( vm->buffer != previous_buffer ) {
        if( vm->buffer != NULL ) {
            handlebars_string_delref(vm->buffer);
        }
        vm->buffer = previous_buffer;
    }
    handlebars_stack_restore(vm->stack, stack_save);
    handlebars_stack_restore(vm->hashStack, hash_stack_save);
    handlebars_stack_restore(vm->contextStack, context_stack_save);
    handlebars_stack_restore(current_block_params, captured_block_params_save);
    if( current_block_params != captured_block_params ) {
        handlebars_stack_restore(captured_block_params, captured_block_params_save);
    }
    vm->blockParamStack = previous_block_params;
    handlebars_stack_delref(current_block_params);
    handlebars_stack_restore(vm->partialBlockStack, partial_block_stack_save);
    handlebars_stack_restore(vm->partialScopeStack, partial_scope_stack_save);
    vm->depth = previous_depth;

    if( caught != HANDLEBARS_SUCCESS && prev_jmp != NULL ) {
        handlebars_longjmp(HBSCTX(vm), prev_jmp, caught);
    }

    return rv;
}

HANDLEBARS_CLOSURE_ATTRS
static struct handlebars_value * invoke_inline_partial_closure(HANDLEBARS_CLOSURE_ARGS)
{
    assert(localc >= 4);
    assert(HANDLEBARS_LOCAL_AT(0)->type == HANDLEBARS_VALUE_TYPE_PTR);
    assert(HANDLEBARS_LOCAL_AT(1)->type == HANDLEBARS_VALUE_TYPE_INTEGER);
    assert(HANDLEBARS_LOCAL_AT(2)->type == HANDLEBARS_VALUE_TYPE_ARRAY);
    assert(HANDLEBARS_LOCAL_AT(3)->type == HANDLEBARS_VALUE_TYPE_ARRAY);

    struct handlebars_module * module = handlebars_value_get_ptr(
        HANDLEBARS_LOCAL_AT(0),
        struct handlebars_module
    );
    long program = handlebars_value_get_intval(HANDLEBARS_LOCAL_AT(1));
    struct handlebars_stack * captured_contexts = handlebars_value_get_stack(HANDLEBARS_LOCAL_AT(2));
    struct handlebars_stack * captured_block_params = handlebars_value_get_stack(HANDLEBARS_LOCAL_AT(3));
    struct handlebars_stack * previous_contexts = vm->contextStack;
    struct handlebars_stack * previous_block_params = vm->blockParamStack;
    struct handlebars_stack_save_buf captured_contexts_save = handlebars_stack_save(captured_contexts);
    struct handlebars_stack_save_buf captured_block_params_save = handlebars_stack_save(captured_block_params);
    struct handlebars_stack_save_buf stack_save = handlebars_stack_save(vm->stack);
    struct handlebars_stack_save_buf hash_stack_save = handlebars_stack_save(vm->hashStack);
    struct handlebars_stack_save_buf partial_block_stack_save = handlebars_stack_save(vm->partialBlockStack);
    struct handlebars_stack_save_buf partial_scope_stack_save = handlebars_stack_save(vm->partialScopeStack);
    struct handlebars_string * previous_buffer = vm->buffer;
    long previous_depth = vm->depth;
    struct handlebars_error * error = HBSCTX(vm)->e;
    jmp_buf * volatile prev_jmp = error->jmp;
    enum handlebars_error_type volatile caught = HANDLEBARS_SUCCESS;
    struct handlebars_stack * current_contexts;
    struct handlebars_stack * current_block_params;
    struct handlebars_value * input;
    struct handlebars_string * buffer = NULL;
    jmp_buf buf;

    /* Keep the closure's snapshots alive while the VM uses copy-on-write
     * working references. */
    handlebars_stack_addref(captured_contexts);
    handlebars_stack_addref(captured_block_params);
    vm->contextStack = captured_contexts;
    vm->blockParamStack = captured_block_params;

    if( handlebars_setjmp_ex(vm, &buf) ) {
        caught = error->num;
        goto done;
    }

    input = argc > 0 ? &argv[0] : TOP(captured_contexts);
    if( vm->module == module ) {
        buffer = handlebars_vm_execute_program_ex(
            vm,
            program,
            input,
            options->data,
            NULL
        );
    } else {
        buffer = handlebars_vm_execute_ex(
            vm,
            module,
            input,
            program,
            options->data,
            NULL
        );
    }
    if( buffer != NULL ) {
        handlebars_value_str(rv, buffer);
    }

done:
    current_contexts = vm->contextStack;
    current_block_params = vm->blockParamStack;
    error->jmp = prev_jmp;
    if( vm->buffer != previous_buffer ) {
        if( vm->buffer != NULL ) {
            handlebars_string_delref(vm->buffer);
        }
        vm->buffer = previous_buffer;
    }
    handlebars_stack_restore(vm->stack, stack_save);
    handlebars_stack_restore(vm->hashStack, hash_stack_save);
    handlebars_stack_restore(current_contexts, captured_contexts_save);
    if( current_contexts != captured_contexts ) {
        handlebars_stack_restore(captured_contexts, captured_contexts_save);
    }
    handlebars_stack_restore(current_block_params, captured_block_params_save);
    if( current_block_params != captured_block_params ) {
        handlebars_stack_restore(captured_block_params, captured_block_params_save);
    }
    vm->contextStack = previous_contexts;
    vm->blockParamStack = previous_block_params;
    handlebars_stack_delref(current_contexts);
    handlebars_stack_delref(current_block_params);
    handlebars_stack_restore(vm->partialBlockStack, partial_block_stack_save);
    handlebars_stack_restore(vm->partialScopeStack, partial_scope_stack_save);
    vm->depth = previous_depth;

    if( caught != HANDLEBARS_SUCCESS && prev_jmp != NULL ) {
        handlebars_longjmp(HBSCTX(vm), prev_jmp, caught);
    }
    return rv;
}

#define INLINE_PARTIAL_MIN_OPCODE_COUNT 5
#define INLINE_PARTIAL_CLOSURE_LOCAL_COUNT 4

struct handlebars_inline_partial_install_state {
    struct handlebars_value closure_localv[INLINE_PARTIAL_CLOSURE_LOCAL_COUNT];
    struct handlebars_value closure_value;
    struct handlebars_value scope_value;
    struct handlebars_string * temporary_name;
    struct handlebars_closure ** closures;
    size_t closure_count;
    size_t opcode_count;
    size_t definition_count;
};

static bool handlebars_vm_inline_partial_name_opcode_is_valid(
    const struct handlebars_opcode * opcode
)
{
    if( opcode->type == handlebars_opcode_type_push_string ) {
        return opcode->op1.type == handlebars_operand_type_string;
    }
    if( opcode->type != handlebars_opcode_type_push_literal ) {
        return false;
    }
    return opcode->op1.type == handlebars_operand_type_boolean
        || opcode->op1.type == handlebars_operand_type_long
        || opcode->op1.type == handlebars_operand_type_string;
}

static struct handlebars_string * handlebars_vm_long_to_string(
    struct handlebars_vm * vm,
    long value
)
{
    char buffer[sizeof(long) * CHAR_BIT + 2];
    int printed = snprintf(buffer, sizeof(buffer), "%ld", value);

    if( unlikely(printed < 0 || (size_t) printed >= sizeof(buffer)) ) {
        handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "Failed to format partial name");
    }
    return handlebars_string_ctor(CONTEXT, buffer, (size_t) printed);
}

static size_t handlebars_vm_inline_partial_opcode_range_length(
    struct handlebars_opcode * opcodes,
    size_t opcode_count,
    size_t * program_opcode_offset
)
{
    if( opcode_count < INLINE_PARTIAL_MIN_OPCODE_COUNT
            || !handlebars_vm_inline_partial_name_opcode_is_valid(&opcodes[0]) ) {
        return 0;
    }

    for( size_t candidate_offset = 1;
            candidate_offset <= opcode_count - 4;
            candidate_offset++ ) {
        size_t hash_offset;
        size_t registration_offset;

        if( opcodes[candidate_offset].type != handlebars_opcode_type_push_program
                || opcodes[candidate_offset].op1.type != handlebars_operand_type_long
                || opcodes[candidate_offset].op1.data.longval < 0
                || !opcodes[candidate_offset].op4.data.boolval
                || opcodes[candidate_offset + 1].type != handlebars_opcode_type_push_program
                || opcodes[candidate_offset + 1].op1.type != handlebars_operand_type_null ) {
            continue;
        }

        hash_offset = candidate_offset + 2;
        if( opcodes[hash_offset].type == handlebars_opcode_type_empty_hash ) {
            registration_offset = hash_offset + 1;
        } else if( opcodes[hash_offset].type == handlebars_opcode_type_push_hash ) {
            size_t hash_depth = 1;
            bool found_pop_hash = false;

            for( size_t i = hash_offset + 1; i < opcode_count; i++ ) {
                if( opcodes[i].type == handlebars_opcode_type_push_hash ) {
                    hash_depth++;
                } else if( opcodes[i].type == handlebars_opcode_type_pop_hash ) {
                    if( hash_depth == 1 ) {
                        registration_offset = i + 1;
                        found_pop_hash = true;
                        break;
                    }
                    hash_depth--;
                }
            }
            if( !found_pop_hash ) {
                continue;
            }
        } else {
            continue;
        }

        if( registration_offset >= opcode_count ) {
            continue;
        }

        if( opcodes[registration_offset].type == handlebars_opcode_type_register_decorator
                && opcodes[registration_offset].op1.type == handlebars_operand_type_long
                && opcodes[registration_offset].op1.data.longval >= 1
                && opcodes[registration_offset].op2.type == handlebars_operand_type_string
                && hbs_str_eq_strl(
                    opcodes[registration_offset].op2.data.string.string,
                    HBS_STRL("inline")
                )
                && opcodes[registration_offset].op3.type == handlebars_operand_type_boolean
                && opcodes[registration_offset].op3.data.boolval ) {
            if( program_opcode_offset != NULL ) {
                *program_opcode_offset = candidate_offset;
            }
            return registration_offset + 1;
        }
    }

    return 0;
}

static struct handlebars_string * handlebars_vm_inline_partial_name(
    struct handlebars_vm * vm,
    struct handlebars_inline_partial_install_state * state,
    const struct handlebars_opcode * opcode
)
{
    const char * value;
    size_t length;

    assert(handlebars_vm_inline_partial_name_opcode_is_valid(opcode));
    assert(state->temporary_name == NULL);

    if( opcode->op1.type == handlebars_operand_type_string ) {
        return opcode->op1.data.string.string;
    }
    if( opcode->op1.type == handlebars_operand_type_boolean ) {
        if( opcode->op1.data.boolval ) {
            value = "true";
            length = sizeof("true") - 1;
        } else {
            value = "false";
            length = sizeof("false") - 1;
        }
    } else {
        assert(opcode->op1.type == handlebars_operand_type_long);
        state->temporary_name = handlebars_vm_long_to_string(
            vm,
            opcode->op1.data.longval
        );
        handlebars_string_addref(state->temporary_name);
        return state->temporary_name;
    }

    state->temporary_name = handlebars_string_ctor(CONTEXT, value, length);
    handlebars_string_addref(state->temporary_name);
    return state->temporary_name;
}

static void handlebars_vm_release_inline_partial_name(
    struct handlebars_inline_partial_install_state * state
)
{
    if( state->temporary_name == NULL ) {
        return;
    }
#ifdef HANDLEBARS_NO_REFCOUNT
    handlebars_talloc_free(state->temporary_name);
#else
    handlebars_string_delref(state->temporary_name);
#endif
    state->temporary_name = NULL;
}

static void handlebars_vm_clear_inline_partial_install_values(
    struct handlebars_inline_partial_install_state * state
)
{
    for( int i = 0; i < INLINE_PARTIAL_CLOSURE_LOCAL_COUNT; i++ ) {
        handlebars_value_dtor(&state->closure_localv[i]);
    }
    handlebars_value_dtor(&state->closure_value);
    handlebars_vm_release_inline_partial_name(state);
}

static void handlebars_vm_install_inline_partial_scope(
    struct handlebars_vm * vm,
    struct handlebars_module_table_entry * entry,
    size_t opcode_count,
    size_t definition_count,
    bool capture_current_context
)
{
    struct handlebars_error * error;
    jmp_buf * volatile prev_jmp;
    enum handlebars_error_type volatile caught = HANDLEBARS_SUCCESS;
    struct handlebars_inline_partial_install_state * state;
    jmp_buf buf;

    state = handlebars_talloc_zero(vm, struct handlebars_inline_partial_install_state);
    HANDLEBARS_MEMCHECK(state, CONTEXT);
    state->opcode_count = opcode_count;
    state->definition_count = definition_count;
    error = HBSCTX(vm)->e;
    prev_jmp = error->jmp;
    if( handlebars_setjmp_ex(vm, &buf) ) {
        caught = error->num;
        goto done;
    }

    state->closures = handlebars_talloc_array(
        state,
        struct handlebars_closure *,
        state->definition_count
    );
    HANDLEBARS_MEMCHECK(state->closures, CONTEXT);
    handlebars_value_map(
        &state->scope_value,
        handlebars_map_ctor(CONTEXT, state->definition_count)
    );
    talloc_steal(state, handlebars_value_get_map(&state->scope_value));

    for( size_t offset = 0; offset < state->opcode_count; ) {
        struct handlebars_opcode * opcodes = &vm->module->opcodes[
            entry->opcode_offset + offset
        ];
        size_t program_opcode_offset;
        size_t range_length = handlebars_vm_inline_partial_opcode_range_length(
            opcodes,
            state->opcode_count - offset,
            &program_opcode_offset
        );
        struct handlebars_value * previous;
        size_t previous_closure_index = SIZE_MAX;
        struct handlebars_string * name;
        struct handlebars_ptr * module_ptr;
        struct handlebars_stack * captured_contexts;
        struct handlebars_stack * captured_block_params;
        struct handlebars_closure * closure;

        assert(range_length > 0);
        name = handlebars_vm_inline_partial_name(vm, state, &opcodes[0]);
        module_ptr = handlebars_ptr_ctor(
            CONTEXT,
            struct handlebars_module,
            vm->module,
            true
        );
        talloc_steal(state, module_ptr);
        handlebars_value_ptr(&state->closure_localv[0], module_ptr);
        handlebars_value_integer(
            &state->closure_localv[1],
            opcodes[program_opcode_offset].op1.data.longval
        );

        captured_contexts = handlebars_stack_copy_ctor(
            vm->contextStack,
            HANDLEBARS_VM_STACK_SIZE
        );
        if( !capture_current_context
                && handlebars_stack_count(captured_contexts) > 0 ) {
            size_t parent_count = handlebars_stack_count(captured_contexts) - 1;
            handlebars_stack_protect(captured_contexts, 0);
            handlebars_stack_truncate(captured_contexts, parent_count);
            handlebars_stack_protect(captured_contexts, parent_count);
        }
        talloc_steal(state, captured_contexts);
        handlebars_value_array(&state->closure_localv[2], captured_contexts);

        captured_block_params = handlebars_stack_copy_ctor(
            vm->blockParamStack,
            HANDLEBARS_VM_STACK_SIZE
        );
        talloc_steal(state, captured_block_params);
        handlebars_value_array(&state->closure_localv[3], captured_block_params);

        closure = handlebars_closure_ctor(
            vm,
            invoke_inline_partial_closure,
            INLINE_PARTIAL_CLOSURE_LOCAL_COUNT,
            state->closure_localv
        );
        talloc_steal(state, closure);
        talloc_steal(closure, module_ptr);
        talloc_steal(closure, captured_contexts);
        talloc_steal(closure, captured_block_params);
        handlebars_value_closure(&state->closure_value, closure);

        previous = handlebars_map_find(
            handlebars_value_get_map(&state->scope_value),
            name
        );
        if( previous != NULL
                && handlebars_value_get_type(previous) == HANDLEBARS_VALUE_TYPE_CLOSURE ) {
            struct handlebars_closure * previous_closure = handlebars_value_get_closure(
                previous
            );
            for( size_t i = 0; i < state->closure_count; i++ ) {
                if( state->closures[i] == previous_closure ) {
                    previous_closure_index = i;
                    break;
                }
            }
        }
        handlebars_value_map_update(
            &state->scope_value,
            name,
            &state->closure_value
        );
        talloc_steal(state, handlebars_value_get_map(&state->scope_value));
        if( previous_closure_index != SIZE_MAX ) {
            state->closures[previous_closure_index] = NULL;
        }
        state->closures[state->closure_count++] = closure;
        handlebars_vm_clear_inline_partial_install_values(state);
        offset += range_length;
    }

    PUSH(vm->partialScopeStack, &state->scope_value);
    talloc_steal(vm, handlebars_value_get_map(&state->scope_value));
    for( size_t i = 0; i < state->closure_count; i++ ) {
        if( state->closures[i] != NULL ) {
            talloc_steal(vm, state->closures[i]);
        }
    }

done:
    error->jmp = prev_jmp;
    handlebars_vm_clear_inline_partial_install_values(state);
    handlebars_value_dtor(&state->scope_value);
    handlebars_talloc_free(state);

    if( caught != HANDLEBARS_SUCCESS && prev_jmp != NULL ) {
        handlebars_longjmp(HBSCTX(vm), prev_jmp, caught);
    }
}

static size_t handlebars_vm_install_inline_partials(
    struct handlebars_vm * vm,
    struct handlebars_module_table_entry * entry,
    bool capture_current_context
)
{
    size_t consumed = 0;
    size_t definition_count = 0;

    while( consumed < entry->opcode_count ) {
        size_t range_length = handlebars_vm_inline_partial_opcode_range_length(
            &vm->module->opcodes[entry->opcode_offset + consumed],
            entry->opcode_count - consumed,
            NULL
        );

        if( range_length == 0 ) {
            break;
        }
        consumed += range_length;
        definition_count++;
    }
    if( definition_count > 0 ) {
        handlebars_vm_install_inline_partial_scope(
            vm,
            entry,
            consumed,
            definition_count,
            capture_current_context
        );
    }
    return consumed;
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

struct handlebars_helper_call_state {
    struct handlebars_options options;
    struct handlebars_value * argv;
    struct handlebars_value extra[5];
    struct handlebars_value rv;
    struct handlebars_value fnv;
    struct handlebars_value value;
    struct handlebars_string * temporary_name;
    struct handlebars_vm_call_checkpoint checkpoint;
    int argc;
};

static void handlebars_helper_call_state_init(
    struct handlebars_helper_call_state * state,
    struct handlebars_value * argv
)
{
    memset(state, 0, sizeof(*state));
    state->argv = argv;
}

static void handlebars_helper_call_state_deinit(
    struct handlebars_helper_call_state * state
)
{
    if( state->temporary_name != NULL ) {
        handlebars_string_delref(state->temporary_name);
    }
    for( int i = 0; i < 5; i++ ) {
        handlebars_value_dtor(&state->extra[i]);
    }
    for( int i = 0; i < state->argc; i++ ) {
        handlebars_value_dtor(&state->argv[i]);
    }
    handlebars_options_deinit(&state->options);
    handlebars_value_dtor(&state->value);
    handlebars_value_dtor(&state->fnv);
    handlebars_value_dtor(&state->rv);
}

HBS_ATTR_NOINLINE HBS_ATTR_NONNULL_ALL
static void accept_ambiguous_block_value_guarded(
    struct handlebars_vm * vm,
    struct handlebars_opcode * opcode,
    struct handlebars_helper_call_state * state
)
{
    struct handlebars_error * error = HBSCTX(vm)->e;
    jmp_buf * volatile prev_jmp = error->jmp;
    enum handlebars_error_type volatile caught = HANDLEBARS_SUCCESS;
    jmp_buf buf;

    if( handlebars_setjmp_ex(vm, &buf) ) {
        caught = error->num;
        goto done;
    }

    if( vm->last_helper == NULL ) {
        struct handlebars_value * result;

        state->argc = 1;
        setup_options(
            vm,
            state->argc,
            state->argv,
            &state->options,
            state->extra
        );
        handlebars_vm_call_checkpoint_begin(vm, &state->checkpoint);
        result = handlebars_vm_call_helper_str(
            HBS_STRL("blockHelperMissing"),
            state->argc,
            state->argv,
            &state->options,
            vm,
            &state->rv
        );
        assert(result != NULL);
        PUSH(vm->stack, result);
    } else if (hbs_str_eq_strl(vm->last_helper, HBS_STRL("lambda"))) {
        setup_options(
            vm,
            state->argc,
            state->argv,
            &state->options,
            state->extra
        );
        handlebars_string_delref(vm->last_helper);
        vm->last_helper = NULL;
    } else {
        setup_options(
            vm,
            state->argc,
            state->argv,
            &state->options,
            state->extra
        );
    }

done:
    error->jmp = prev_jmp;
    handlebars_vm_call_checkpoint_finish(vm, &state->checkpoint, caught);
    handlebars_helper_call_state_deinit(state);

    if( caught != HANDLEBARS_SUCCESS ) {
        handlebars_vm_rethrow_caught(vm, prev_jmp, caught);
    }
}

ACCEPT_NOINLINE_FUNCTION(ambiguous_block_value)
{
    HANDLEBARS_VALUE_ARRAY_DECL(argv, 1);
    struct handlebars_helper_call_state state;

    handlebars_helper_call_state_init(&state, argv);
    accept_ambiguous_block_value_guarded(vm, opcode, &state);
    HANDLEBARS_VALUE_ARRAY_UNDECL(argv, 0);
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

    handlebars_value_map_update(hash, opcode->op1.data.string.string, value);

    PUSH(vm->hashStack, hash);

    HANDLEBARS_VALUE_UNDECL(value);
    HANDLEBARS_VALUE_UNDECL(hash);
}

HBS_ATTR_NOINLINE HBS_ATTR_NONNULL_ALL
static void accept_block_value_guarded(
    struct handlebars_vm * vm,
    struct handlebars_opcode * opcode,
    struct handlebars_helper_call_state * state
)
{
    struct handlebars_error * error = HBSCTX(vm)->e;
    jmp_buf * volatile prev_jmp = error->jmp;
    enum handlebars_error_type volatile caught = HANDLEBARS_SUCCESS;
    struct handlebars_value * result;
    jmp_buf buf;

    if( handlebars_setjmp_ex(vm, &buf) ) {
        caught = error->num;
        goto done;
    }

    assert(opcode->op1.type == handlebars_operand_type_string);

    state->argc = 1;
    setup_options(
        vm,
        state->argc,
        state->argv,
        &state->options,
        state->extra
    );
    state->options.name = opcode->op1.data.string.string;
    handlebars_vm_call_checkpoint_begin(vm, &state->checkpoint);

    result = handlebars_vm_call_helper_str(
        HBS_STRL("blockHelperMissing"),
        state->argc,
        state->argv,
        &state->options,
        vm,
        &state->rv
    );
    /* append_to_buffer may replace the borrowed buffer snapshot. */
    handlebars_vm_call_checkpoint_commit(vm, &state->checkpoint);
    if (likely(result != NULL)) {
        append_to_buffer(vm, result, 0);
    }

done:
    error->jmp = prev_jmp;
    handlebars_vm_call_checkpoint_finish(vm, &state->checkpoint, caught);
    handlebars_helper_call_state_deinit(state);

    if( caught != HANDLEBARS_SUCCESS ) {
        handlebars_vm_rethrow_caught(vm, prev_jmp, caught);
    }
}

ACCEPT_NOINLINE_FUNCTION(block_value)
{
    HANDLEBARS_VALUE_ARRAY_DECL(argv, 1);
    struct handlebars_helper_call_state state;

    handlebars_helper_call_state_init(&state, argv);
    accept_block_value_guarded(vm, opcode, &state);
    HANDLEBARS_VALUE_ARRAY_UNDECL(argv, 0);
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

struct handlebars_ambiguous_call_state {
    struct handlebars_options options;
    struct handlebars_value argv[1];
    struct handlebars_value extra[5];
    struct handlebars_value closure_localv[3];
    struct handlebars_value rv;
    struct handlebars_value value;
    struct handlebars_value fnv;
    struct handlebars_string * last_helper;
};

HBS_ATTR_NOINLINE HBS_ATTR_NONNULL_ALL
static void accept_invoke_ambiguous_guarded(
    struct handlebars_vm * vm,
    struct handlebars_opcode * opcode,
    struct handlebars_ambiguous_call_state * state
)
{
    const int argc = 0;
    const int closure_localc = 3;
    struct handlebars_error * error = HBSCTX(vm)->e;
    jmp_buf * volatile prev_jmp = error->jmp;
    enum handlebars_error_type volatile caught = HANDLEBARS_SUCCESS;
    long previous_depth = vm->depth;
    struct handlebars_value * result;
    struct handlebars_value * fn;
    bool is_callable;
    jmp_buf buf;

    if( handlebars_setjmp_ex(vm, &buf) ) {
        caught = error->num;
        goto done;
    }

    HBS_ASSERT(POP(vm->stack, &state->value));
    is_callable = handlebars_value_is_callable(&state->value);

    ACCEPT_FN(empty_hash)(vm, opcode);

    assert(opcode->op1.type == handlebars_operand_type_string);
    assert(opcode->op2.type == handlebars_operand_type_boolean);

    setup_options(vm, argc, state->argv, &state->options, state->extra);
    state->options.name = opcode->op1.data.string.string;
    if( vm->last_helper != NULL ) {
        handlebars_string_delref(vm->last_helper);
    }
    vm->last_helper = NULL;

    if( vm->flags & handlebars_compiler_flag_mustache_style_lambdas
            && is_callable ) {
        struct handlebars_closure * closure;

        assert(opcode->op3.type == handlebars_operand_type_string);
        handlebars_value_value(&state->closure_localv[0], &state->value);
        handlebars_value_str(
            &state->closure_localv[1],
            opcode->op3.data.string.string
        );
        handlebars_value_boolean(
            &state->closure_localv[2],
            opcode->op2.data.boolval
        );

        closure = handlebars_closure_ctor(
            vm,
            invoke_mustache_style_lambda_closure,
            closure_localc,
            state->closure_localv
        );
        handlebars_value_closure(&state->value, closure);
        fn = &state->value;

        state->last_helper = handlebars_string_ctor(
            CONTEXT,
            HBS_STRL("lambda")
        );
        handlebars_string_addref(state->last_helper);
    } else if( NULL != (fn = lookup_helper(
            vm,
            state->options.name,
            &state->fnv
        )) ) {
        state->last_helper = state->options.name;
        handlebars_string_addref(state->last_helper);
    } else if( is_callable ) {
        fn = &state->value;
    } else {
        struct handlebars_string * tmp_str = handlebars_string_ctor(
            CONTEXT,
            HBS_STRL("helperMissing")
        );
        fn = lookup_helper(vm, tmp_str, &state->fnv);
        handlebars_string_delref(tmp_str);
    }

    result = handlebars_value_call(
        fn,
        argc,
        state->argv,
        &state->options,
        vm,
        &state->rv
    );

    // Before, the null case was only done for helperMissing
    if( result->type != HANDLEBARS_VALUE_TYPE_NULL ) {
        PUSH(vm->stack, result);
    } else {
        PUSH(vm->stack, &state->value);
    }

    vm->last_helper = state->last_helper;
    state->last_helper = NULL;

done:
    error->jmp = prev_jmp;
    if( state->last_helper != NULL ) {
        handlebars_string_delref(state->last_helper);
    }
    for( int i = 0; i < closure_localc; i++ ) {
        handlebars_value_dtor(&state->closure_localv[i]);
    }
    for( int i = 0; i < 5; i++ ) {
        handlebars_value_dtor(&state->extra[i]);
    }
    handlebars_value_dtor(&state->argv[0]);
    handlebars_options_deinit(&state->options);
    handlebars_value_dtor(&state->fnv);
    handlebars_value_dtor(&state->value);
    handlebars_value_dtor(&state->rv);
    vm->depth = previous_depth;

    if( caught != HANDLEBARS_SUCCESS && prev_jmp != NULL ) {
        handlebars_longjmp(HBSCTX(vm), prev_jmp, caught);
    }
}

ACCEPT_NOINLINE_FUNCTION(invoke_ambiguous)
{
    struct handlebars_ambiguous_call_state state;

    memset(&state.options, 0, sizeof(state.options));
    handlebars_value_init(&state.argv[0]);
    for( int i = 0; i < 5; i++ ) {
        handlebars_value_init(&state.extra[i]);
    }
    for( int i = 0; i < 3; i++ ) {
        handlebars_value_init(&state.closure_localv[i]);
    }
    handlebars_value_init(&state.rv);
    handlebars_value_init(&state.value);
    handlebars_value_init(&state.fnv);
    state.last_helper = NULL;

    accept_invoke_ambiguous_guarded(vm, opcode, &state);
}

HBS_ATTR_NOINLINE HBS_ATTR_NONNULL_ALL
static void accept_invoke_helper_guarded(
    struct handlebars_vm * vm,
    struct handlebars_opcode * opcode,
    struct handlebars_helper_call_state * state
)
{
    struct handlebars_error * error = HBSCTX(vm)->e;
    jmp_buf * volatile prev_jmp = error->jmp;
    enum handlebars_error_type volatile caught = HANDLEBARS_SUCCESS;
    struct handlebars_value * fn;
    jmp_buf buf;

    if( handlebars_setjmp_ex(vm, &buf) ) {
        caught = error->num;
        goto done;
    }

    HBS_ASSERT(POP(vm->stack, &state->value));

    assert(opcode->op1.type == handlebars_operand_type_long);
    assert(opcode->op2.type == handlebars_operand_type_string);
    assert(opcode->op3.type == handlebars_operand_type_boolean);

    setup_options(
        vm,
        state->argc,
        state->argv,
        &state->options,
        state->extra
    );
    state->options.name = opcode->op2.data.string.string;
    handlebars_vm_call_checkpoint_begin(vm, &state->checkpoint);

    if (opcode->op3.data.boolval && NULL != (fn = lookup_helper(
            vm,
            state->options.name,
            &state->fnv
        ))) { // isSimple
        // fallthrough
    } else if (handlebars_value_is_callable(&state->value)) {
        fn = &state->value;
    } else if (unlikely(helper_context_is_truthy(&state->value))) {
        handlebars_throw_ex(
            CONTEXT,
            HANDLEBARS_ERROR,
            &opcode->loc,
            "Value for helper \"%.*s\" is not callable",
            (int) hbs_str_len(state->options.name),
            hbs_str_val(state->options.name)
        );
    } else {
        state->temporary_name = handlebars_string_ctor(
            CONTEXT,
            HBS_STRL("helperMissing")
        );
        fn = lookup_helper(vm, state->temporary_name, &state->fnv);
        handlebars_string_delref(state->temporary_name);
        state->temporary_name = NULL;
    }

    PUSH(
        vm->stack,
        handlebars_value_call(
            fn,
            state->argc,
            state->argv,
            &state->options,
            vm,
            &state->rv
        )
    );

done:
    error->jmp = prev_jmp;
    handlebars_vm_call_checkpoint_finish(vm, &state->checkpoint, caught);
    handlebars_helper_call_state_deinit(state);

    if( caught != HANDLEBARS_SUCCESS ) {
        handlebars_vm_rethrow_caught(vm, prev_jmp, caught);
    }
}

ACCEPT_NOINLINE_FUNCTION(invoke_helper)
{
    int argc;
    struct handlebars_helper_call_state state;

    assert(opcode->op1.type == handlebars_operand_type_long);
    argc = (int) opcode->op1.data.longval;
    HANDLEBARS_VALUE_ARRAY_DECL(argv, argc);

    handlebars_helper_call_state_init(&state, argv);
    state.argc = argc;
    accept_invoke_helper_guarded(vm, opcode, &state);
    HANDLEBARS_VALUE_ARRAY_UNDECL(argv, 0);
}

HBS_ATTR_NOINLINE HBS_ATTR_NONNULL_ALL
static void accept_invoke_known_helper_guarded(
    struct handlebars_vm * vm,
    struct handlebars_opcode * opcode,
    struct handlebars_helper_call_state * state
)
{
    struct handlebars_error * error = HBSCTX(vm)->e;
    jmp_buf * volatile prev_jmp = error->jmp;
    enum handlebars_error_type volatile caught = HANDLEBARS_SUCCESS;
    struct handlebars_value * fn;
    jmp_buf buf;

    if( handlebars_setjmp_ex(vm, &buf) ) {
        caught = error->num;
        goto done;
    }

    assert(opcode->op1.type == handlebars_operand_type_long);
    assert(opcode->op2.type == handlebars_operand_type_string);

    setup_options(
        vm,
        state->argc,
        state->argv,
        &state->options,
        state->extra
    );
    state->options.name = opcode->op2.data.string.string;
    handlebars_vm_call_checkpoint_begin(vm, &state->checkpoint);

    fn = lookup_helper(vm, state->options.name, &state->fnv);

    if (unlikely(fn == NULL)) {
        handlebars_throw_ex(
            CONTEXT,
            HANDLEBARS_ERROR,
            &opcode->loc,
            "Invalid known helper: %.*s",
            (int) hbs_str_len(state->options.name),
            hbs_str_val(state->options.name)
        );
    }

    PUSH(
        vm->stack,
        handlebars_value_call(
            fn,
            state->argc,
            state->argv,
            &state->options,
            vm,
            &state->rv
        )
    );

done:
    error->jmp = prev_jmp;
    handlebars_vm_call_checkpoint_finish(vm, &state->checkpoint, caught);
    handlebars_helper_call_state_deinit(state);

    if( caught != HANDLEBARS_SUCCESS ) {
        handlebars_vm_rethrow_caught(vm, prev_jmp, caught);
    }
}

ACCEPT_NOINLINE_FUNCTION(invoke_known_helper)
{
    int argc;
    struct handlebars_helper_call_state state;

    assert(opcode->op1.type == handlebars_operand_type_long);
    argc = (int) opcode->op1.data.longval;
    HANDLEBARS_VALUE_ARRAY_DECL(argv, argc);

    handlebars_helper_call_state_init(&state, argv);
    state.argc = argc;
    accept_invoke_known_helper_guarded(vm, opcode, &state);
    HANDLEBARS_VALUE_ARRAY_UNDECL(argv, 0);
}

struct handlebars_partial_call_state {
    struct handlebars_options options;
    struct handlebars_value argv[1];
    struct handlebars_value extra[5];
    struct handlebars_value partial_block_localv[4];
    struct handlebars_value partial_string_localv[2];
    struct handlebars_value tmp;
    struct handlebars_value partial_rv;
    struct handlebars_value rv;
    struct handlebars_value partial_block;
    struct handlebars_string * temporary_name;
    struct handlebars_stack_save_buf inline_scope_save;
    bool inline_scope_saved;
    bool pushed_partial_block;
};

ACCEPT_NOINLINE_FUNCTION(invoke_partial)
{
    const int argc = 1;
    struct handlebars_error * error = HBSCTX(vm)->e;
    jmp_buf * volatile prev_jmp = error->jmp;
    enum handlebars_error_type volatile caught = HANDLEBARS_SUCCESS;
    long previous_depth = vm->depth;
    struct handlebars_partial_call_state * state = handlebars_talloc_zero(
        vm,
        struct handlebars_partial_call_state
    );
    struct handlebars_options * options;
    struct handlebars_value * argv;
    struct handlebars_value * extra;
    struct handlebars_value * tmp;
    struct handlebars_value * partial_rv;
    struct handlebars_value * rv;
    struct handlebars_value * partial_block;
    struct handlebars_string * name;
    struct handlebars_value * partial;
    struct handlebars_string * buffer;
    jmp_buf buf;

    HANDLEBARS_MEMCHECK(state, CONTEXT);
    options = &state->options;
    argv = state->argv;
    extra = state->extra;
    tmp = &state->tmp;
    partial_rv = &state->partial_rv;
    rv = &state->rv;
    partial_block = &state->partial_block;
    assert(opcode->op1.type == handlebars_operand_type_boolean);
    assert(opcode->op2.type == handlebars_operand_type_string || opcode->op2.type == handlebars_operand_type_null || opcode->op2.type == handlebars_operand_type_long);
    assert(opcode->op3.type == handlebars_operand_type_string);

    if( handlebars_setjmp_ex(vm, &buf) ) {
        caught = error->num;
        goto done;
    }

    setup_options(vm, argc, argv, options, extra);
    state->inline_scope_save = handlebars_stack_save(vm->partialScopeStack);
    state->inline_scope_saved = true;
    if( unlikely(options->program >= (long) vm->module->program_count) ) {
        handlebars_throw(
            CONTEXT,
            HANDLEBARS_ERROR,
            "Invalid program: %ld",
            options->program
        );
    }
    name = NULL;
    partial = NULL;
    buffer = NULL;

    if( opcode->op1.data.boolval ) {
        // Dynamic partial
        HBS_ASSERT(POP(vm->stack, tmp));
        name = handlebars_value_get_string(tmp);
        options->name = NULL; // fear
    } else {
        if( opcode->op2.type == handlebars_operand_type_long ) {
            state->temporary_name = handlebars_vm_long_to_string(
                vm,
                opcode->op2.data.longval
            );
            talloc_steal(state, state->temporary_name);
            name = state->temporary_name;
        } else if( opcode->op2.type == handlebars_operand_type_string ) {
            name = opcode->op2.data.string.string;
        }
    }

    if (name) {
        partial = lookup_partial(vm, name, partial_rv);
    }

    // Try to look up partial block
    if (!partial && name && hbs_str_eq_strl(name, HBS_STRL("@partial-block")) && LEN(vm->partialBlockStack) > 0) {
        partial = TOP(vm->partialBlockStack);
    }

    // Push partial block
    if (options->program > 0) {
        const int closure_localc = 4;
        struct handlebars_value * closure_localv = state->partial_block_localv;
        handlebars_value_ptr(&closure_localv[0], handlebars_ptr_ctor(CONTEXT, struct handlebars_module, vm->module, true));
        handlebars_value_integer(&closure_localv[1], options->program);
        handlebars_value_integer(&closure_localv[2], LEN(vm->partialBlockStack));
        handlebars_value_array(
            &closure_localv[3],
            handlebars_stack_copy_ctor(vm->blockParamStack, HANDLEBARS_VM_STACK_SIZE)
        );
        struct handlebars_closure * closure = handlebars_closure_ctor(
            vm,
            invoke_partial_block_closure,
            closure_localc,
            closure_localv
        );
        for( int i = 0; i < closure_localc; i++ ) {
            handlebars_value_dtor(&closure_localv[i]);
        }
        handlebars_value_closure(partial_block, closure);
        PUSH(vm->partialBlockStack, partial_block);
        state->pushed_partial_block = true;
    }

    // Inline partial declarations in a partial-block body are decorators on
    // the call itself. Install them before entering the selected partial even
    // when that body is never invoked as @partial-block.
    if( options->program >= 0 ) {
        struct handlebars_module_table_entry * program_entry = &vm->module->programs[
            options->program
        ];
        if( unlikely(program_entry->guid != (size_t) options->program
                || program_entry->opcode_count == 0
                || program_entry->opcode_offset > vm->module->opcode_count
                || program_entry->opcode_count
                    > vm->module->opcode_count - program_entry->opcode_offset) ) {
            handlebars_throw(
                CONTEXT,
                HANDLEBARS_ERROR,
                "Invalid opcode range for program: %ld",
                options->program
            );
        }
        handlebars_vm_install_inline_partials(vm, program_entry, true);
    }

    // Merge hashes
    merge_hash(HBSCTX(vm), &argv[0], options->hash);

    if (!partial) {
        if (options->program >= 0) {
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
        struct handlebars_value * closure_localv = state->partial_string_localv;
        handlebars_value_str(&closure_localv[0], handlebars_value_get_string(partial));
        if (vm->flags & handlebars_compiler_flag_compat) {
            handlebars_value_str(&closure_localv[1], opcode->op3.data.string.string);
        }
        struct handlebars_closure * closure = handlebars_closure_ctor(vm, invoke_partial_string_closure, closure_localc, closure_localv);
        for( int i = 0; i < closure_localc; i++ ) {
            handlebars_value_dtor(&closure_localv[i]);
        }
        handlebars_value_closure(partial, closure);
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
            handlebars_value_call(partial, argc, argv, options, vm, rv),
            false
        );

        if (vm->flags & handlebars_compiler_flag_compat) {
            vm->buffer = handlebars_string_append_str(CONTEXT, vm->buffer, buffer);
        } else {
            vm->buffer = handlebars_string_indent_append(HBSCTX(vm), vm->buffer, buffer, opcode->op3.data.string.string);
        }
    } while (0);

done:
    error->jmp = prev_jmp;
    if( state->inline_scope_saved ) {
        handlebars_stack_restore(vm->partialScopeStack, state->inline_scope_save);
    }

    // Pop partial block
    if (state->pushed_partial_block) {
        HANDLEBARS_VALUE_DECL(closure_value);
        POP(vm->partialBlockStack, closure_value);
        HANDLEBARS_VALUE_UNDECL(closure_value);
    }

    for( int i = 0; i < 5; i++ ) {
        handlebars_value_dtor(&state->extra[i]);
    }
    for( int i = 0; i < 4; i++ ) {
        handlebars_value_dtor(&state->partial_block_localv[i]);
    }
    for( int i = 0; i < 2; i++ ) {
        handlebars_value_dtor(&state->partial_string_localv[i]);
    }
    handlebars_value_dtor(&state->argv[0]);
    handlebars_options_deinit(&state->options);
    handlebars_value_dtor(&state->partial_block);
    handlebars_value_dtor(&state->rv);
    handlebars_value_dtor(&state->partial_rv);
    handlebars_value_dtor(&state->tmp);
    handlebars_talloc_free(state);
    vm->depth = previous_depth;

    if( caught != HANDLEBARS_SUCCESS && prev_jmp != NULL ) {
        handlebars_longjmp(HBSCTX(vm), prev_jmp, caught);
    }
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

    if( unlikely(opcode->op1.type != handlebars_operand_type_array
            || opcode->op2.type != handlebars_operand_type_array
            || opcode->op1.data.array.count < 2
            || opcode->op1.data.array.array == NULL
            || (opcode->op2.data.array.count > 0 && opcode->op2.data.array.array == NULL)) ) {
        handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "Invalid lookup_block_param operands");
    }

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

    if( unlikely(opcode->op1.type != handlebars_operand_type_long
            || opcode->op2.type != handlebars_operand_type_array
            || opcode->op2.data.array.count == 0
            || opcode->op2.data.array.array == NULL
            || (opcode->op3.type != handlebars_operand_type_boolean && opcode->op3.type != handlebars_operand_type_null)) ) {
        handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "Invalid lookup_data operands");
    }

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
        tmp = TOP(vm->partialBlockStack);
        if( tmp == NULL ) {
            goto done_and_null;
        }
        handlebars_value_value(val, tmp);
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

    if( unlikely(opcode->op1.type != handlebars_operand_type_array
            || opcode->op1.data.array.count == 0
            || opcode->op1.data.array.array == NULL
            || (opcode->op2.type != handlebars_operand_type_boolean && opcode->op2.type != handlebars_operand_type_null)
            || (opcode->op3.type != handlebars_operand_type_boolean && opcode->op3.type != handlebars_operand_type_null)
            || (opcode->op4.type != handlebars_operand_type_boolean && opcode->op4.type != handlebars_operand_type_null)) ) {
        handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "Invalid lookup_on_context operands");
    }

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
    } else if( unlikely(program_num >= (long) vm->module->program_count) ) {
        handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "Invalid program: %ld", program_num);
    }

    // Get program
    struct handlebars_module_table_entry entry_copy = vm->module->programs[program_num];
    struct handlebars_module_table_entry * entry = &entry_copy;
    if( unlikely(entry->guid != (size_t) program_num
            || entry->opcode_count == 0
            || entry->opcode_offset > vm->module->opcode_count
            || entry->opcode_count > vm->module->opcode_count - entry->opcode_offset) ) {
        handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "Invalid opcode range for program: %ld", program_num);
    }
    if( unlikely(vm->depth >= HANDLEBARS_VM_MAX_DEPTH) ) {
        handlebars_throw(
            CONTEXT,
            HANDLEBARS_STACK_OVERFLOW,
            "VM program stack overflow at depth %ld",
            vm->depth
        );
    }
    vm->depth++;
    HANDLEBARS_VALUE_DECL(empty_block_params);

    // Save and set buffer
    struct handlebars_string * prev_buffer = vm->buffer;
    vm->buffer = handlebars_string_init(CONTEXT, HANDLEBARS_VM_BUFFER_INIT_SIZE);

    // Check stacks
    assert(vm->stack != NULL);
    assert(vm->contextStack != NULL);
    assert(vm->hashStack != NULL);
    assert(vm->blockParamStack != NULL);
    assert(vm->partialBlockStack != NULL);
    assert(vm->partialScopeStack != NULL);

    // Save stacks
    struct handlebars_stack_save_buf st = handlebars_stack_save(vm->stack);
    struct handlebars_stack_save_buf hst = handlebars_stack_save(vm->hashStack);
    struct handlebars_stack_save_buf cst = handlebars_stack_save(vm->contextStack);
    struct handlebars_stack_save_buf bst = handlebars_stack_save(vm->blockParamStack);
    struct handlebars_stack_save_buf pst = handlebars_stack_save(vm->partialBlockStack);
    struct handlebars_stack_save_buf ist = handlebars_stack_save(vm->partialScopeStack);

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
    if( block_params || program_num != 0 ) {
        PUSH(vm->blockParamStack, block_params ? block_params : empty_block_params);
    }

    size_t inline_opcode_count = handlebars_vm_install_inline_partials(
        vm,
        entry,
        false
    );
    entry->opcode_offset += inline_opcode_count;
    entry->opcode_count -= inline_opcode_count;

    // Execute the program
	handlebars_vm_accept(vm, entry);

    // Restore stacks
    handlebars_stack_restore(vm->stack, st);
    handlebars_stack_restore(vm->hashStack, hst);
    handlebars_stack_restore(vm->contextStack, cst);
    handlebars_stack_restore(vm->blockParamStack, bst);
    handlebars_stack_restore(vm->partialBlockStack, pst);
    handlebars_stack_restore(vm->partialScopeStack, ist);

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

    HANDLEBARS_VALUE_UNDECL(empty_block_params);
    vm->depth--;

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
    struct handlebars_error * error = HBSCTX(vm)->e;
    jmp_buf * volatile prev = error->jmp;
    struct handlebars_module * prev_module = vm->module;
    struct handlebars_string * prev_buffer = vm->buffer;
    struct handlebars_string * prev_last_helper = vm->last_helper;
    unsigned long prev_flags = vm->flags;
    struct handlebars_value * prev_last_context = vm->last_context;
    struct handlebars_string * prev_delim_open = vm->delim_open;
    struct handlebars_string * prev_delim_close = vm->delim_close;
    long prev_depth = vm->depth;
    struct handlebars_stack_save_buf st;
    struct handlebars_stack_save_buf hst;
    struct handlebars_stack_save_buf cst;
    struct handlebars_stack_save_buf bst;
    struct handlebars_stack_save_buf pst;
    struct handlebars_stack_save_buf ist;
    HANDLEBARS_VALUE_DECL(prev_data);
    HANDLEBARS_VALUE_DECL(prev_last_context_value);

    struct handlebars_string * volatile buffer = NULL;
    enum handlebars_error_type volatile caught = HANDLEBARS_SUCCESS;
    bool volatile setup_last_context = false;
    bool volatile setup_stacks = false;
    jmp_buf buf;

    handlebars_value_value(prev_data, &vm->data);
    if( prev_delim_open != NULL ) {
        handlebars_string_addref(prev_delim_open);
    }
    if( prev_delim_close != NULL ) {
        handlebars_string_addref(prev_delim_close);
    }
    if( prev_last_helper != NULL ) {
        handlebars_string_addref(prev_last_helper);
    }

    // Allocate alloca-backed state before setjmp so it remains valid while
    // cleaning up after a longjmp from a helper or closure.
    if (vm->stack == NULL) {
        vm->stack = handlebars_stack_alloca(HBSCTX(vm), HANDLEBARS_VM_STACK_SIZE);
        vm->contextStack = handlebars_stack_alloca(HBSCTX(vm), HANDLEBARS_VM_STACK_SIZE);
        vm->hashStack = handlebars_stack_alloca(HBSCTX(vm), HANDLEBARS_VM_STACK_SIZE);
        vm->blockParamStack = handlebars_stack_alloca(HBSCTX(vm), HANDLEBARS_VM_STACK_SIZE);
        vm->partialBlockStack = handlebars_stack_alloca(HBSCTX(vm), HANDLEBARS_VM_STACK_SIZE);
        vm->partialScopeStack = handlebars_stack_alloca(HBSCTX(vm), HANDLEBARS_VM_STACK_SIZE);
        setup_stacks = true;
    }

    st = handlebars_stack_save(vm->stack);
    hst = handlebars_stack_save(vm->hashStack);
    cst = handlebars_stack_save(vm->contextStack);
    bst = handlebars_stack_save(vm->blockParamStack);
    pst = handlebars_stack_save(vm->partialBlockStack);
    ist = handlebars_stack_save(vm->partialScopeStack);

    if (vm->last_context == NULL) {
        vm->last_context = alloca(HANDLEBARS_VALUE_SIZE);
        handlebars_value_init(vm->last_context);
        setup_last_context = true;
    } else {
        handlebars_value_value(prev_last_context_value, vm->last_context);
    }

    // Always install a local boundary. Callers may catch the rethrown error,
    // so VM state must no longer refer to this frame's alloca-backed stacks.
    if( handlebars_setjmp_ex(vm, &buf) ) {
        caught = error->num;
        buffer = NULL;
        goto done;
    }

    vm->module = module;
    vm->flags |= module->flags;

    // Execute
    buffer = handlebars_vm_execute_program_ex(vm, program, context, data, block_params);

done:
    error->jmp = prev;

    handlebars_stack_restore(vm->stack, st);
    handlebars_stack_restore(vm->hashStack, hst);
    handlebars_stack_restore(vm->contextStack, cst);
    handlebars_stack_restore(vm->blockParamStack, bst);
    handlebars_stack_restore(vm->partialBlockStack, pst);
    handlebars_stack_restore(vm->partialScopeStack, ist);

    if( vm->buffer != prev_buffer ) {
        if( vm->buffer != NULL ) {
            handlebars_string_delref(vm->buffer);
        }
        vm->buffer = prev_buffer;
    }

    handlebars_value_value(&vm->data, prev_data);

    if( setup_last_context ) {
        handlebars_value_dtor(vm->last_context);
    } else {
        handlebars_value_value(prev_last_context, prev_last_context_value);
    }

    if( vm->delim_open != NULL ) {
        handlebars_string_delref(vm->delim_open);
    }
    if( vm->delim_close != NULL ) {
        handlebars_string_delref(vm->delim_close);
    }
    if( vm->last_helper != NULL ) {
        handlebars_string_delref(vm->last_helper);
    }
    vm->delim_open = prev_delim_open;
    vm->delim_close = prev_delim_close;
    vm->last_helper = prev_last_helper;

    // Reset stacks
    if (setup_stacks) {
        vm->stack = NULL;
        vm->contextStack = NULL;
        vm->hashStack = NULL;
        vm->blockParamStack = NULL;
        vm->partialBlockStack = NULL;
        vm->partialScopeStack = NULL;
    }

    // Reset
    vm->last_context = prev_last_context;
    vm->module = prev_module;
    vm->flags = prev_flags;
    vm->depth = prev_depth;

    HANDLEBARS_VALUE_UNDECL(prev_last_context_value);
    HANDLEBARS_VALUE_UNDECL(prev_data);

    if( caught != HANDLEBARS_SUCCESS && prev != NULL ) {
        handlebars_longjmp(HBSCTX(vm), prev, caught);
    }

    return (struct handlebars_string *) buffer;
}

struct handlebars_string * handlebars_vm_execute(
    struct handlebars_vm * vm,
    struct handlebars_module * module,
    struct handlebars_value * context
) {
    return handlebars_vm_execute_ex(vm, module, context, 0, NULL, NULL);
}
