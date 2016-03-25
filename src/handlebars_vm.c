
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "handlebars.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"

#include "handlebars_cache.h"
#include "handlebars_compiler.h"
#include "handlebars_value.h"
#include "handlebars_map.h"
#include "handlebars_opcodes.h"
#include "handlebars_opcode_printer.h"
#include "handlebars_stack.h"
#include "handlebars_utils.h"
#include "handlebars_vm.h"



#define OPCODE_NAME(name) handlebars_opcode_type_ ## name
#define ACCEPT(name) case OPCODE_NAME(name) : ACCEPT_FN(name)(vm, opcode); break;
#define ACCEPT_FN(name) accept_ ## name
#define ACCEPT_NAMED_FUNCTION(name) static inline void name (struct handlebars_vm * vm, struct handlebars_opcode * opcode)
#define ACCEPT_FUNCTION(name) ACCEPT_NAMED_FUNCTION(ACCEPT_FN(name))

#define LEN(stack) stack.i
#define BOTTOM(stack) stack.v[0]
#define GET(stack, pos) handlebars_value_addref2(stack.v[stack.i - pos - 1])
#define TOP(stack) handlebars_value_addref2(stack.i > 0 ? stack.v[stack.i - 1] : NULL)
#define POP(stack) (stack.i > 0 ? stack.v[--stack.i] : NULL)
#define PUSH(stack, value) do { \
        if( stack.i < HANDLEBARS_VM_STACK_SIZE ) { \
            stack.v[stack.i++] = value; \
        } else { \
            handlebars_throw(HBSCTX(vm), HANDLEBARS_STACK_OVERFLOW, "Stack overflow in %s", #stack); \
        } \
    } while(0)

#define TOPCONTEXT TOP(vm->contextStack)

ACCEPT_FUNCTION(push_context);




#undef CONTEXT
#define CONTEXT ctx

struct handlebars_vm * handlebars_vm_ctor(struct handlebars_context * ctx)
{
    return MC(handlebars_talloc_zero(ctx, struct handlebars_vm));
}

#undef CONTEXT
#define CONTEXT HBSCTX(vm)


void handlebars_vm_dtor(struct handlebars_vm * vm)
{
    handlebars_talloc_free(vm);
}

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

static inline struct handlebars_value * call_helper_str(const char * name, unsigned int len, int argc, struct handlebars_value * argv[], struct handlebars_options * options)
{
    struct handlebars_value * helper;
    struct handlebars_value * result;
    handlebars_helper_func fn;
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
        result = call_helper_str("blockHelperMissing", sizeof("blockHelperMissing") - 1, 1, argv, &options);
        append_to_buffer(vm, result, 0);
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

    vm->buffer = handlebars_string_append(CONTEXT, vm->buffer, HBS_STR_STRL(opcode->op1.data.string));
}

ACCEPT_FUNCTION(assign_to_hash)
{
    struct handlebars_value * hash = TOP(vm->hashStack);
    struct handlebars_value * value = POP(vm->stack);

    assert(opcode->op1.type == handlebars_operand_type_string);
    assert(hash->type == HANDLEBARS_VALUE_TYPE_MAP);

    handlebars_map_update(hash->v.map, opcode->op1.data.string, value);

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

    options.name = opcode->op1.data.string;
    setup_options(vm, 0, argv, &options);

    current = POP(vm->stack);
    assert(current != NULL);
    argv[0] = current;

    result = call_helper_str("blockHelperMissing", sizeof("blockHelperMissing") - 1, 1, argv, &options);
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

ACCEPT_FUNCTION(invoke_ambiguous)
{
    struct handlebars_value * value = POP(vm->stack);
    struct handlebars_value * result;
    struct handlebars_options options = {0};
    struct handlebars_value * argv[0];

    ACCEPT_FN(empty_hash)(vm, opcode);

    assert(opcode->op1.type == handlebars_operand_type_string);
    assert(opcode->op2.type == handlebars_operand_type_boolean);

    options.name = opcode->op1.data.string;
    setup_options(vm, 0, NULL, &options);
    vm->last_helper = NULL;

    if( NULL != (result = call_helper(options.name, 0, NULL, &options)) ) {
        append_to_buffer(vm, result, 0);
        vm->last_helper = options.name;
    } else if( value && handlebars_value_is_callable(value) ) {
        result = handlebars_value_call(value, 0, argv, &options);
        assert(result != NULL);
        PUSH(vm->stack, result);
    } else {
        result = call_helper_str("helperMissing", sizeof("helperMissing") - 1, 0, NULL, &options);
        append_to_buffer(vm, result, 0);
        PUSH(vm->stack, value);
    }

    handlebars_options_deinit(&options);
}

ACCEPT_FUNCTION(invoke_helper)
{
    struct handlebars_value * value = POP(vm->stack);
    struct handlebars_value * result = NULL;
    struct handlebars_options options = {0};

    assert(opcode->op1.type == handlebars_operand_type_long);
    assert(opcode->op2.type == handlebars_operand_type_string);
    assert(opcode->op3.type == handlebars_operand_type_boolean);

    int argc = (int) opcode->op1.data.longval;
    struct handlebars_value * argv[argc];
    options.name = opcode->op2.data.string;
    setup_options(vm, argc, argv, &options);

    if( opcode->op3.data.boolval ) { // isSimple
        if( NULL != (result = call_helper(options.name, argc, argv, &options)) ) {
            goto done;
        }
    }

    if( value && handlebars_value_is_callable(value) ) {
        result = handlebars_value_call(value, argc, argv, &options);
    } else {
        result = call_helper_str("helperMissing", sizeof("helperMissing") - 1, argc, argv, &options);
    }

done:
    if( !result ) {
        result = handlebars_value_ctor(CONTEXT);
    }

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
    struct handlebars_value * argv[argc];
    options.name = opcode->op2.data.string;
    setup_options(vm, argc, argv, &options);

    result = call_helper(options.name, argc, argv, &options);

    if( result == NULL ) {
        handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "Invalid known helper: %s", options.name->val);
    }

    PUSH(vm->stack, result);
    handlebars_options_deinit(&options);
}

static inline struct handlebars_value * merge_hash(struct handlebars_context * context, struct handlebars_value * hash, struct handlebars_value * context1)
{
    struct handlebars_value * context2 = NULL;
    struct handlebars_value_iterator it;
    if( context1 && handlebars_value_get_type(context1) == HANDLEBARS_VALUE_TYPE_MAP &&
        hash && hash->type == HANDLEBARS_VALUE_TYPE_MAP ) {
        context2 = handlebars_value_ctor(context);
        handlebars_value_map_init(context2);
        handlebars_value_iterator_init(&it, context1);
        for( ; it.current ; handlebars_value_iterator_next(&it) ) {
            handlebars_map_update(context2->v.map, it.key, it.current);
        }
        handlebars_value_iterator_init(&it, hash);
        for( ; it.current ; handlebars_value_iterator_next(&it) ) {
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

ACCEPT_FUNCTION(invoke_partial)
{
    struct handlebars_options options = {0};
    struct handlebars_value * tmp = NULL;
    struct handlebars_string * name = NULL;
    struct handlebars_value * partial = NULL;
    jmp_buf buf;
    int argc = 1;
    struct handlebars_value * argv[argc];

    assert(opcode->op1.type == handlebars_operand_type_boolean);
    assert(opcode->op2.type == handlebars_operand_type_string || opcode->op2.type == handlebars_operand_type_null || opcode->op2.type == handlebars_operand_type_long);
    assert(opcode->op3.type == handlebars_operand_type_string);

    setup_options(vm, argc, argv, &options);

    if( opcode->op1.data.boolval ) {
        // Dynamic partial
        tmp = POP(vm->stack);
        if( tmp ) {
            name = tmp->v.string;
        }
        options.name = NULL; // fear
    } else {
        if( opcode->op2.type == handlebars_operand_type_long ) {
            char tmp_str[32];
            size_t tmp_str_len = snprintf(tmp_str, 32, "%ld", opcode->op2.data.longval);
            name = handlebars_string_ctor(HBSCTX(vm), tmp_str, tmp_str_len);
            //name = MC(handlebars_talloc_asprintf(vm, "%ld", opcode->op2.data.longval));
        } else if( opcode->op2.type == handlebars_operand_type_string ) {
            name = opcode->op2.data.string;
        }
    }

    if( name ) {
        partial = handlebars_value_map_find(vm->partials, name);
    }
    if( !partial ) {
        if( vm->flags & handlebars_compiler_flag_compat ) {
            return;
        } else {
            handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "The partial %s could not be found", name ? name->val : "(NULL)");
        }
    }

    // Construct new context
    struct handlebars_context * context = handlebars_context_ctor_ex(vm);

    // If partial is a function?
    if( handlebars_value_is_callable(partial) ) {
        struct handlebars_value * context1 = argv[0];
        struct handlebars_value * context2 = merge_hash(HBSCTX(vm), options.hash, context1);
        argv[0] = context2;

        struct handlebars_value * ret = handlebars_value_call(partial, argc, argv, &options);
        struct handlebars_string * tmp2 = handlebars_value_expression(ret, 0);
        struct handlebars_string * tmp3 = handlebars_string_indent(HBSCTX(vm), tmp2->val, tmp2->len, HBS_STR_STRL(opcode->op3.data.string));
        vm->buffer = handlebars_string_append(CONTEXT, vm->buffer, HBS_STR_STRL(tmp3));
        handlebars_talloc_free(tmp3);
        handlebars_talloc_free(tmp2);
        handlebars_value_try_delref(ret);
        goto done;
    }

    if( !partial || partial->type != HANDLEBARS_VALUE_TYPE_STRING ) {
        handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "The partial %s was not a string, was %d", name ? name->val : "(NULL)", partial ? partial->type : -1);
    }

    // Save jump buffer
    context->jmp = &buf;
    if( setjmp(buf) ) {
        handlebars_throw_ex(CONTEXT, context->num, &context->loc, "%s", context->msg);
    }

    // Get template
    struct handlebars_string * tmpl = partial->v.string;
    if( !tmpl || !tmpl->len ) {
        goto done;
    }

    // Check for cached template, if available
    struct handlebars_compiler * compiler = NULL;
    struct handlebars_cache_entry * cache_entry;
    if( vm->cache && (cache_entry = handlebars_cache_find(vm->cache, tmpl)) ) {
        // Use cached
        compiler = cache_entry->compiler;
    } else {
        // Recompile

        // Parse
        struct handlebars_parser * parser = handlebars_parser_ctor(context);
        parser->tmpl = tmpl;
        handlebars_parse(parser); // @todo fix setjmp

        // Compile
        compiler = handlebars_compiler_ctor(context);
        handlebars_compiler_set_flags(compiler, vm->flags);
        handlebars_compiler_compile(compiler, parser->program);

        // Save cache entry
        if( vm->cache ) {
            handlebars_cache_add(vm->cache, tmpl, compiler);
        }

        // Cleanup parser
        handlebars_parser_dtor(parser);
    }

    // Construct VM
    struct handlebars_vm * vm2 = handlebars_vm_ctor(context);

    // Get context
    struct handlebars_value * context1 = argv[0];
    struct handlebars_value * context2 = merge_hash(HBSCTX(vm2), options.hash, context1);

    // Setup new VM
    vm2->depth = vm->depth + 1;
    vm2->flags = vm->flags;
    vm2->helpers = vm->helpers;
    vm2->partials = vm->partials;

    // Copy stacks
    memcpy(&vm2->contextStack, &vm->contextStack, offsetof(struct handlebars_vm_stack, v) + (sizeof(struct handlebars_value *) * LEN(vm->contextStack)));
    memcpy(&vm2->blockParamStack, &vm->blockParamStack, offsetof(struct handlebars_vm_stack, v) + (sizeof(struct handlebars_value *) * LEN(vm->blockParamStack)));

    handlebars_vm_execute(vm2, compiler, context2);

    if( vm2->buffer ) {
        struct handlebars_string * tmp2 = handlebars_string_indent(
                HBSCTX(vm2), HBS_STR_STRL(vm2->buffer), HBS_STR_STRL(opcode->op3.data.string));
        vm->buffer = handlebars_string_append(CONTEXT, vm->buffer, HBS_STR_STRL(tmp2));
        handlebars_talloc_free(tmp2);
    }

    //handlebars_value_try_delref(context1); // @todo double-check
    //handlebars_value_try_delref(context2); // @todo double-check
    handlebars_value_try_delref(tmp);
    handlebars_vm_dtor(vm2);

done:
    handlebars_context_dtor(context);
    handlebars_value_try_delref(partial);
    handlebars_options_deinit(&options);
}

ACCEPT_FUNCTION(lookup_block_param)
{
    long blockParam1 = -1;
    long blockParam2 = -1;
    struct handlebars_value * value = NULL;

    assert(opcode->op1.type == handlebars_operand_type_array);
    assert(opcode->op2.type == handlebars_operand_type_array);

    sscanf(opcode->op1.data.array[0]->val, "%ld", &blockParam1);
    sscanf(opcode->op1.data.array[1]->val, "%ld", &blockParam2);

    if( blockParam1 >= LEN(vm->blockParamStack) ) goto done;

    struct handlebars_value * v1 = GET(vm->blockParamStack, blockParam1);
    if( !v1 || handlebars_value_get_type(v1) != HANDLEBARS_VALUE_TYPE_ARRAY ) goto done;

    struct handlebars_value * v2 = handlebars_value_array_find(v1, blockParam2);
    if( !v2 ) goto done;

    struct handlebars_string ** arr = opcode->op2.data.array;
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

    size_t depth = opcode->op1.data.longval;
    struct handlebars_string ** arr = opcode->op2.data.array;
    struct handlebars_string * first = *arr++;

    if( depth && data ) {
        handlebars_value_addref(data);
        while( data && depth-- ) {
            tmp = handlebars_value_map_str_find(data, HBS_STRL("_parent"));
            handlebars_value_delref(data);
            data = tmp;
        }
    }

    if( data && (tmp = handlebars_value_map_find(data, first)) ) {
        val = tmp;
    } else if( 0 == strcmp(first->val, "root") ) {
        val = BOTTOM(vm->contextStack);
    }

    if( val ) {
        for (; *arr != NULL; arr++) {
            struct handlebars_string * part = *arr;
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

    PUSH(vm->stack, val);
}

ACCEPT_FUNCTION(lookup_on_context)
{
    assert(opcode->op1.type == handlebars_operand_type_array);
    assert(opcode->op2.type == handlebars_operand_type_boolean || opcode->op2.type == handlebars_operand_type_null);
    assert(opcode->op3.type == handlebars_operand_type_boolean || opcode->op3.type == handlebars_operand_type_null);
    assert(opcode->op4.type == handlebars_operand_type_boolean || opcode->op4.type == handlebars_operand_type_null);

    struct handlebars_string ** arr = opcode->op1.data.array;
    long index = -1;

    if( !opcode->op4.data.boolval && (vm->flags & handlebars_compiler_flag_compat) ) {
        depthed_lookup(vm, *arr);
    } else {
        ACCEPT_FN(push_context)(vm, opcode);
    }

    struct handlebars_value * value = TOP(vm->stack);
    struct handlebars_value * tmp;

    if( value ) {
        do {
            if( handlebars_value_get_type(value) == HANDLEBARS_VALUE_TYPE_MAP ) {
                tmp = handlebars_value_map_find(value, (*arr));
                handlebars_value_try_delref(value);
                value = tmp;
            } else if( handlebars_value_get_type(value) == HANDLEBARS_VALUE_TYPE_ARRAY ) {
                if( sscanf((*arr)->val, "%ld", &index) ) {
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

    handlebars_value_delref(POP(vm->stack));
    PUSH(vm->stack, value);
}

ACCEPT_FUNCTION(pop_hash)
{
    struct handlebars_value * hash = POP(vm->hashStack);
    if( !hash ) {
        hash = handlebars_value_ctor(CONTEXT);
    }
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
            // @todo we should move this to the parser
            if( 0 == strcmp(opcode->op1.data.string->val, "undefined") ) {
                break;
            } else if( 0 == strcmp(opcode->op1.data.string->val, "null") ) {
                break;
            }
            handlebars_value_str(value, opcode->op1.data.string);
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

    handlebars_value_str(value, opcode->op1.data.string);
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
        struct handlebars_value * argv[argc];
        argv[0] = TOPCONTEXT;
        options.vm = vm;
        options.scope = TOPCONTEXT;
        result = handlebars_value_call(top, argc, argv, &options);
        if( !result ) {
            result = handlebars_value_ctor(CONTEXT);
        }
        handlebars_value_delref(POP(vm->stack));
        PUSH(vm->stack, result);
        handlebars_options_deinit(&options);
    }
    handlebars_value_delref(top);
}

static inline void handlebars_vm_accept(struct handlebars_vm * vm, struct handlebars_compiler * compiler)
{
	size_t i = compiler->opcodes_length;
    struct handlebars_opcode ** opcodes = compiler->opcodes;

	for( ; i > 0; i-- ) {
		struct handlebars_opcode * opcode = *opcodes++;

        // Print opcode?
#ifndef NDEBUG
        if( getenv("DEBUG") ) {
            struct handlebars_string * tmp = handlebars_opcode_print(HBSCTX(vm), opcode, 0);
            fprintf(stdout, "V[%ld] P[%ld] OPCODE: %.*s\n", vm->depth, compiler->guid, (int) tmp->len, tmp->val);
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
                handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "Unhandled opcode: %s\n", handlebars_opcode_readable_type(opcode->type));
                break;
        }
	}
}

struct handlebars_string * handlebars_vm_execute_program_ex(
    struct handlebars_vm * vm,
    long program,
    struct handlebars_value * context,
    struct handlebars_value * data,
    struct handlebars_value * block_params
) {
    bool pushed_context = false;
    bool pushed_block_param = false;

    if( program < 0 ) {
        return handlebars_string_init(CONTEXT, 0);
    }
    if( program >= vm->guid_index ) {
        handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "Invalid program: %ld", program);
    }

    // Get compiler
	struct handlebars_compiler * compiler = vm->programs[program];

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
	handlebars_vm_accept(vm, compiler);

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


struct handlebars_string * handlebars_vm_execute(
		struct handlebars_vm * vm, struct handlebars_compiler * compiler,
		struct handlebars_value * context)
{
    jmp_buf * prev = HBSCTX(vm)->jmp;
    jmp_buf buf;

    // Save jump buffer
    if( !prev ) {
        if( handlebars_setjmp_ex(vm, &buf) ) {
            goto done;
        }
    }

    // Preprocess
    if( compiler->programs ) {
        vm->programs = compiler->programs;
        vm->guid_index = compiler->programs_index;
    } else {
        vm->programs = compiler->programs = MC(handlebars_talloc_array(compiler, struct handlebars_compiler *, 32));
        preprocess_program(vm, compiler);
        compiler->programs_index = vm->guid_index;
        compiler->programs = talloc_steal(compiler, vm->programs);
        compiler->programs_index = vm->guid_index;
    }

    // Save context
    handlebars_value_addref(context);
    vm->context = context;

    // Execute
    vm->buffer = handlebars_vm_execute_program_ex(vm, 0, context, vm->data, NULL);

done:
    // Release context
    handlebars_value_delref(context);

    // Reset
    vm->programs = NULL;
    vm->guid_index = 0;
    HBSCTX(vm)->jmp = prev;

    return vm->buffer;
}
