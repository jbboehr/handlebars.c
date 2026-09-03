/**
 * Copyright (c) anno Domini nostri Jesu Christi MMXVI-MMXXIV John Boehr & contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <check.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <talloc.h>

#include "handlebars.h"
#include "handlebars_value_private.h"
#include "handlebars_closure.h"
#include "handlebars_compiler.h"
#include "handlebars_helpers.h"
#include "handlebars_memory.h"

#include "handlebars_map.h"
#include "handlebars_opcode_serializer.h"
#include "handlebars_parser.h"
#include "handlebars_ptr.h"
#include "handlebars_stack.h"
#include "handlebars_string.h"
#include "handlebars_value.h"
#include "handlebars_value_handlers.h"
#include "handlebars_vm.h"
#include "handlebars_vm_private.h"

#include "utils.h"

#if !IS_WIN
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif


static struct handlebars_value * test_closure_callback(
    int localc,
    struct handlebars_value * localv,
    int argc,
    struct handlebars_value * argv,
    struct handlebars_options * options,
    struct handlebars_vm * callback_vm,
    struct handlebars_value * rv
)
{
    (void) localc;
    (void) localv;
    (void) argc;
    (void) argv;
    (void) options;
    (void) callback_vm;
    return rv;
}

static struct handlebars_value * test_throwing_helper(
    int argc,
    struct handlebars_value * argv,
    struct handlebars_options * options,
    struct handlebars_vm * callback_vm,
    struct handlebars_value * rv
)
{
    (void) argc;
    (void) argv;
    (void) options;
    (void) rv;
    handlebars_throw(HBSCTX(callback_vm), HANDLEBARS_ERROR, "Intentional helper failure");
}

static struct handlebars_value * test_set_data_and_throw_helper(
    int argc,
    struct handlebars_value * argv,
    struct handlebars_options * options,
    struct handlebars_vm * callback_vm,
    struct handlebars_value * rv
)
{
    (void) argv;
    (void) options;
    ck_assert_int_eq(argc, 1);
    handlebars_value_integer(rv, 99);
    handlebars_vm_set_data(callback_vm, rv);
    handlebars_throw(
        HBSCTX(callback_vm),
        HANDLEBARS_ERROR,
        "Intentional callable condition failure"
    );
}

static struct handlebars_value * test_passthrough_helper(
    int argc,
    struct handlebars_value * argv,
    struct handlebars_options * options,
    struct handlebars_vm * callback_vm,
    struct handlebars_value * rv
)
{
    (void) options;
    (void) callback_vm;
    ck_assert_int_eq(argc, 1);
    handlebars_value_value(rv, &argv[0]);
    return rv;
}

static struct handlebars_value * test_context_helper(
    int argc,
    struct handlebars_value * argv,
    struct handlebars_options * options,
    struct handlebars_vm * callback_vm,
    struct handlebars_value * rv
)
{
    (void) argv;
    (void) options;
    ck_assert_int_eq(argc, 0);
    handlebars_value_str(rv, handlebars_string_ctor(HBSCTX(callback_vm), HBS_STRL("ok")));
    return rv;
}

static struct handlebars_value * test_catch_each_helper(
    int argc,
    struct handlebars_value * argv,
    struct handlebars_options * options,
    struct handlebars_vm * callback_vm,
    struct handlebars_value * rv
)
{
    struct handlebars_error * error = HBSCTX(callback_vm)->e;
    struct handlebars_value * unexpected_result;
    jmp_buf * volatile previous = error->jmp;
    jmp_buf buf;

    if( handlebars_setjmp_ex(callback_vm, &buf) ) {
        error->jmp = previous;
        ck_assert_int_eq(handlebars_error_num(HBSCTX(callback_vm)), HANDLEBARS_ERROR);
        ck_assert_ptr_nonnull(
            strstr(
                handlebars_error_msg(HBSCTX(callback_vm)),
                "partial missing could not be found"
            )
        );
        clear_intentional_error();
        handlebars_value_str(
            rv,
            handlebars_string_ctor(
                HBSCTX(callback_vm),
                HBS_STRL("caught")
            )
        );
        return rv;
    }

    unexpected_result = handlebars_builtin_each(
        argc,
        argv,
        options,
        callback_vm,
        rv
    );
    error->jmp = previous;
    ck_abort_msg(
        "Expected nested #each execution to throw, got %p",
        (void *) unexpected_result
    );
}

static struct handlebars_value * test_execute_if_program_as_callable(
    int argc,
    struct handlebars_value * argv,
    struct handlebars_options * options,
    struct handlebars_vm * callback_vm,
    struct handlebars_value * rv
)
{
    struct handlebars_string * unexpected_result;

    (void) rv;
    ck_assert_int_eq(argc, 1);
    unexpected_result = handlebars_vm_execute_program_ex(
        callback_vm,
        options->program,
        options->scope,
        &argv[0],
        NULL
    );
    if( unexpected_result != NULL ) {
        handlebars_string_delref(unexpected_result);
    }
    ck_abort_msg("Expected callable #if condition evaluation to throw");
}

static struct handlebars_value * test_true_callable(
    int argc,
    struct handlebars_value * argv,
    struct handlebars_options * options,
    struct handlebars_vm * callback_vm,
    struct handlebars_value * rv
)
{
    (void) argv;
    (void) options;
    (void) callback_vm;
    ck_assert_int_eq(argc, 1);
    handlebars_value_boolean(rv, true);
    return rv;
}

static struct handlebars_value * test_catch_if_helper(
    int argc,
    struct handlebars_value * argv,
    struct handlebars_options * options,
    struct handlebars_vm * callback_vm,
    struct handlebars_value * rv
)
{
    struct handlebars_error * error = HBSCTX(callback_vm)->e;
    struct handlebars_value * unexpected_result;
    jmp_buf * volatile previous = error->jmp;
    jmp_buf buf;

    if( handlebars_setjmp_ex(callback_vm, &buf) ) {
        error->jmp = previous;
        ck_assert_int_eq(handlebars_error_num(HBSCTX(callback_vm)), HANDLEBARS_ERROR);
        ck_assert_ptr_nonnull(
            strstr(
                handlebars_error_msg(HBSCTX(callback_vm)),
                "partial missing could not be found"
            )
        );
        clear_intentional_error();
        handlebars_value_str(
            rv,
            handlebars_string_ctor(
                HBSCTX(callback_vm),
                HBS_STRL("caught")
            )
        );
        return rv;
    }

    unexpected_result = handlebars_builtin_if(
        argc,
        argv,
        options,
        callback_vm,
        rv
    );
    error->jmp = previous;
    ck_abort_msg(
        "Expected direct #if execution to throw, got %p",
        (void *) unexpected_result
    );
}

#if !defined(HANDLEBARS_NO_REFCOUNT) || defined(HANDLEBARS_MEMORY)
static struct handlebars_value * test_with_context_helper(
    int argc,
    struct handlebars_value * argv,
    struct handlebars_options * options,
    struct handlebars_vm * callback_vm,
    struct handlebars_value * rv
)
{
    (void) argv;
    (void) options;
    ck_assert_int_eq(argc, 1);
    handlebars_value_str(
        rv,
        handlebars_string_ctor(
            HBSCTX(callback_vm),
            HBS_STRL("callable")
        )
    );
    return rv;
}

static struct handlebars_value * test_each_context_helper(
    int argc,
    struct handlebars_value * argv,
    struct handlebars_options * options,
    struct handlebars_vm * callback_vm,
    struct handlebars_value * rv
)
{
    HANDLEBARS_VALUE_DECL(item);
    struct handlebars_stack * items;

    (void) argv;
    (void) options;
    ck_assert_int_eq(argc, 1);
    handlebars_value_boolean(item, true);
    items = handlebars_stack_ctor(HBSCTX(callback_vm), 1);
    items = handlebars_stack_push(items, item);
    handlebars_value_array(rv, items);
    HANDLEBARS_VALUE_UNDECL(item);
    return rv;
}
#endif

#ifdef HANDLEBARS_MEMORY
static struct handlebars_value * test_allocating_helper(
    int argc,
    struct handlebars_value * argv,
    struct handlebars_options * options,
    struct handlebars_vm * callback_vm,
    struct handlebars_value * rv
)
{
    (void) argc;
    (void) argv;
    (void) options;
    handlebars_value_str(
        rv,
        handlebars_string_ctor(
            HBSCTX(callback_vm),
            HBS_STRL("success")
        )
    );
    return rv;
}
#endif

static void test_register_helper(const char * name, size_t length, handlebars_func helper_fn)
{
    HANDLEBARS_VALUE_DECL(helper);
    HANDLEBARS_VALUE_DECL(helpers);
    struct handlebars_map * helper_map = handlebars_map_ctor(context, 1);

    handlebars_value_helper(helper, helper_fn);
    helper_map = handlebars_map_str_update(helper_map, name, length, helper);
    handlebars_value_map(helpers, helper_map);
    handlebars_vm_set_helpers(vm, helpers);

    HANDLEBARS_VALUE_UNDECL(helpers);
    HANDLEBARS_VALUE_UNDECL(helper);
}

static struct handlebars_module * test_compile_template(const char * tmpl)
{
    struct handlebars_parser * local_parser = handlebars_parser_ctor(context);
    struct handlebars_compiler * local_compiler = handlebars_compiler_ctor(context);
    struct handlebars_ast_node * ast = handlebars_parse_ex(
        local_parser,
        handlebars_string_ctor(context, tmpl, strlen(tmpl)),
        0
    );
    struct handlebars_program * program;
    struct handlebars_module * module;

    ck_assert_msg(
        ast != NULL,
        "Template parse failed for '%s': %s",
        tmpl,
        handlebars_error_msg(context)
    );
    program = handlebars_compiler_compile_ex(local_compiler, ast);
    ck_assert_ptr_nonnull(program);
    module = handlebars_program_serialize(context, program);
    handlebars_compiler_dtor(local_compiler);
    handlebars_parser_dtor(local_parser);
    return module;
}

static struct handlebars_string * test_execute_with_bar(
    struct handlebars_module * module,
    struct handlebars_value * bar
)
{
    HANDLEBARS_VALUE_DECL(input);
    struct handlebars_map * input_map = handlebars_map_ctor(context, 1);
    struct handlebars_string * output;

    input_map = handlebars_map_str_update(input_map, HBS_STRL("bar"), bar);
    handlebars_value_map(input, input_map);
    output = handlebars_vm_execute(vm, module, input);
    HANDLEBARS_VALUE_UNDECL(input);
    return output;
}

struct test_lazy_array_user {
    struct handlebars_user user;
    struct handlebars_value item;
    long count;
    size_t find_calls;
    size_t last_index;
};

static enum handlebars_value_type test_lazy_array_type(
    struct handlebars_value * value
)
{
    (void) value;
    return HANDLEBARS_VALUE_TYPE_ARRAY;
}

static struct handlebars_value * test_lazy_array_find(
    struct handlebars_value * value,
    size_t index,
    struct handlebars_value * rv
)
{
    struct test_lazy_array_user * user = (struct test_lazy_array_user *)
        handlebars_value_get_user(value);

    user->find_calls++;
    user->last_index = index;
    if( index >= (size_t) user->count ) {
        return NULL;
    }
    (void) rv;
    handlebars_value_integer(&user->item, (long) index + 10);
    return &user->item;
}

static long test_lazy_array_count(struct handlebars_value * value)
{
    struct test_lazy_array_user * user = (struct test_lazy_array_user *)
        handlebars_value_get_user(value);

    return user->count;
}

static const struct handlebars_value_handlers test_lazy_array_handlers = {
    .name = "test-lazy-array",
    .type = &test_lazy_array_type,
    .array_find = &test_lazy_array_find,
    .count = &test_lazy_array_count
};

static const struct handlebars_value_handlers test_lazy_array_no_count_handlers = {
    .name = "test-lazy-array-no-count",
    .type = &test_lazy_array_type,
    .array_find = &test_lazy_array_find
};

struct test_lazy_map_user {
    struct handlebars_user user;
    struct handlebars_value item;
    bool has_length;
};

static enum handlebars_value_type test_lazy_map_type(
    struct handlebars_value * value
)
{
    (void) value;
    return HANDLEBARS_VALUE_TYPE_MAP;
}

static struct handlebars_value * test_lazy_map_find(
    struct handlebars_value * value,
    struct handlebars_string * key,
    struct handlebars_value * rv
)
{
    struct test_lazy_map_user * user = (struct test_lazy_map_user *)
        handlebars_value_get_user(value);

    (void) rv;
    if( !user->has_length || !hbs_str_eq_strl(key, HBS_STRL("length")) ) {
        return NULL;
    }
    handlebars_value_integer(&user->item, 37);
    return &user->item;
}

static const struct handlebars_value_handlers test_lazy_map_handlers = {
    .name = "test-lazy-map",
    .type = &test_lazy_map_type,
    .map_find = &test_lazy_map_find
};

static enum handlebars_value_type test_type_only_string_type(
    struct handlebars_value * value
)
{
    (void) value;
    return HANDLEBARS_VALUE_TYPE_STRING;
}

static const struct handlebars_value_handlers test_type_only_string_handlers = {
    .name = "test-type-only-string",
    .type = &test_type_only_string_type
};

static enum handlebars_value_type test_type_only_ptr_type(
    struct handlebars_value * value
)
{
    (void) value;
    return HANDLEBARS_VALUE_TYPE_PTR;
}

static const struct handlebars_value_handlers test_type_only_ptr_handlers = {
    .name = "test-type-only-ptr",
    .type = &test_type_only_ptr_type
};

#ifndef HANDLEBARS_NO_REFCOUNT
struct test_checked_pointer_payload {
    int * destructions;
};

static int test_checked_pointer_payload_dtor(
    struct test_checked_pointer_payload * payload
)
{
    (*payload->destructions)++;
    return 0;
}
#endif

static void test_value_lazy_array(
    struct handlebars_value * value,
    long count
)
{
    struct test_lazy_array_user * user = handlebars_talloc_zero(
        context,
        struct test_lazy_array_user
    );

    ck_assert_ptr_nonnull(user);
    handlebars_value_init(&user->item);
    user->count = count;
    handlebars_user_init(&user->user, context, &test_lazy_array_handlers);
    handlebars_value_user(value, &user->user);
}

static void test_value_lazy_array_without_count(
    struct handlebars_value * value,
    long count
)
{
    struct test_lazy_array_user * user = handlebars_talloc_zero(
        context,
        struct test_lazy_array_user
    );

    ck_assert_ptr_nonnull(user);
    handlebars_value_init(&user->item);
    user->count = count;
    handlebars_user_init(&user->user, context, &test_lazy_array_no_count_handlers);
    handlebars_value_user(value, &user->user);
}

static void test_value_lazy_map(
    struct handlebars_value * value,
    bool has_length
)
{
    struct test_lazy_map_user * user = handlebars_talloc_zero(
        context,
        struct test_lazy_map_user
    );

    ck_assert_ptr_nonnull(user);
    handlebars_value_init(&user->item);
    user->has_length = has_length;
    handlebars_user_init(&user->user, context, &test_lazy_map_handlers);
    handlebars_value_user(value, &user->user);
}

static void assert_value_expression_result(
    struct handlebars_value * value,
    bool escape,
    const char * expected
) {
    struct handlebars_string * expression;
    struct handlebars_string * appended;
    char expected_appended[128];
    int expected_length;

    expression = handlebars_value_expression(context, value, escape);
    ck_assert_hbs_str_eq_cstr(expression, expected);
    handlebars_talloc_free(expression);

    expected_length = snprintf(
        expected_appended,
        sizeof(expected_appended),
        "prefix:%s",
        expected
    );
    ck_assert_msg(
        expected_length >= 0 && (size_t) expected_length < sizeof(expected_appended),
        "Expected expression is too long"
    );
    appended = handlebars_value_expression_append(
        context,
        value,
        handlebars_string_ctor(context, HBS_STRL("prefix:")),
        escape
    );
    ck_assert_hbs_str_eq_cstr(appended, expected_appended);
    handlebars_talloc_free(appended);
}

enum value_traversal_operation {
    VALUE_TRAVERSAL_CONVERT,
    VALUE_TRAVERSAL_EXPRESSION,
    VALUE_TRAVERSAL_EXPRESSION_APPEND,
    VALUE_TRAVERSAL_DUMP
};

static void assert_value_traversal_rejected(
    struct handlebars_value * value,
    enum value_traversal_operation operation,
    const char * expected_error
) {
    jmp_buf * volatile previous = context->e->jmp;
    struct handlebars_string * volatile prefix = NULL;
    void * volatile unexpected_result = NULL;
    jmp_buf buf;

    if( operation == VALUE_TRAVERSAL_EXPRESSION_APPEND ) {
        prefix = handlebars_string_ctor(context, HBS_STRL("prefix"));
    }
    if( handlebars_setjmp_ex(context, &buf) ) {
        context->e->jmp = previous;
        ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_ERROR);
        ck_assert_ptr_nonnull(strstr(handlebars_error_msg(context), expected_error));
        ck_assert_ptr_null(context->e->iterator_cleanup);
        if( prefix != NULL ) {
            ck_assert_hbs_str_eq_cstr((struct handlebars_string *) prefix, "prefix");
            handlebars_talloc_free((struct handlebars_string *) prefix);
        }
        clear_intentional_error();
        return;
    }

    switch( operation ) {
        case VALUE_TRAVERSAL_CONVERT:
            handlebars_value_convert(value);
            break;
        case VALUE_TRAVERSAL_EXPRESSION:
            unexpected_result = handlebars_value_expression(context, value, false);
            break;
        case VALUE_TRAVERSAL_EXPRESSION_APPEND:
            unexpected_result = handlebars_value_expression_append(
                context,
                value,
                (struct handlebars_string *) prefix,
                false
            );
            break;
        case VALUE_TRAVERSAL_DUMP:
            unexpected_result = handlebars_value_dump(value, context, 0);
            break;
        default:
            ck_abort_msg("Unknown value traversal operation");
    }

    context->e->jmp = previous;
    if( unexpected_result != NULL ) {
        handlebars_talloc_free((void *) unexpected_result);
    }
    ck_abort_msg("Expected recursive value traversal to be rejected");
}

static void init_nested_array(
    struct handlebars_value * value,
    struct handlebars_context * owner,
    size_t depth
)
{
    handlebars_value_integer(value, 1);
    for( size_t i = 0; i < depth; i++ ) {
        struct handlebars_stack * stack = handlebars_stack_ctor(owner, 1);

        stack = handlebars_stack_push(stack, value);
        handlebars_value_array(value, stack);
    }
}


START_TEST(test_closure_rejects_negative_local_count)
{
    struct handlebars_closure * closure;
    jmp_buf * previous = context->e->jmp;
    jmp_buf buf;

    if( handlebars_setjmp_ex(context, &buf) ) {
        context->e->jmp = previous;
        ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_ERROR);
        return;
    }

    closure = handlebars_closure_ctor(vm, test_closure_callback, -1, NULL);
    context->e->jmp = previous;
    (void) closure;
    ck_abort_msg("Expected a negative closure local count to be rejected");
}
END_TEST

START_TEST(test_vm_string_length_handles_user_without_count)
{
    struct handlebars_module * module = test_compile_template("{{text.length}}");
    HANDLEBARS_VALUE_DECL(input);
    HANDLEBARS_VALUE_DECL(value);
    struct handlebars_map * input_map = handlebars_map_ctor(context, 1);
    struct handlebars_user * user = handlebars_talloc_zero(
        context,
        struct handlebars_user
    );
    struct handlebars_string * output;

    ck_assert_ptr_nonnull(user);
    handlebars_user_init(user, context, &test_type_only_string_handlers);
    handlebars_value_user(value, user);
    input_map = handlebars_map_str_update(input_map, HBS_STRL("text"), value);
    handlebars_value_map(input, input_map);

    output = handlebars_vm_execute(vm, module, input);

    ck_assert_ptr_nonnull(output);
    ck_assert_hbs_str_eq_cstr(output, "");

    handlebars_string_delref(output);
    HANDLEBARS_VALUE_UNDECL(value);
    HANDLEBARS_VALUE_UNDECL(input);
}
END_TEST

START_TEST(test_vm_user_collection_length_and_callback_contracts)
{
    struct handlebars_module * module = test_compile_template(
        "{{items.length}}|{{lookup items \"length\"}}|"
        "{{items.[0]}}|{{lookup items \"0\"}}|{{lookup items \"01\"}}|"
        "{{record.length}}|{{lookup record \"length\"}}|"
        "{{emptyRecord.length}}|{{lookup emptyRecord \"length\"}}"
    );
    HANDLEBARS_VALUE_DECL(input);
    HANDLEBARS_VALUE_DECL(value);
    struct handlebars_map * input_map = handlebars_map_ctor(context, 3);
    struct handlebars_string * output;

    test_value_lazy_array_without_count(value, 2);
    input_map = handlebars_map_str_update(input_map, HBS_STRL("items"), value);
    test_value_lazy_map(value, true);
    input_map = handlebars_map_str_update(input_map, HBS_STRL("record"), value);
    test_value_lazy_map(value, false);
    input_map = handlebars_map_str_update(input_map, HBS_STRL("emptyRecord"), value);
    handlebars_value_map(input, input_map);

    output = handlebars_vm_execute(vm, module, input);

    ck_assert_ptr_nonnull(output);
    ck_assert_hbs_str_eq_cstr(output, "||10|10||37|37||");

    handlebars_string_delref(output);
    HANDLEBARS_VALUE_UNDECL(value);
    HANDLEBARS_VALUE_UNDECL(input);
}
END_TEST

START_TEST(test_vm_array_index_canonical_boundaries)
{
    static const char * invalid_keys[] = {
        "", "00", "01", "+1", "-1", " 1", "1 ", "1x", "1.0", "\x80"
    };
    HANDLEBARS_VALUE_DECL(value);
    HANDLEBARS_VALUE_DECL(rv);
    struct test_lazy_array_user * user;
    struct handlebars_string * key;
    struct handlebars_value * result;
    char max_key[64];
    char overflow_key[65];
    int max_key_length;

    test_value_lazy_array(value, 2);
    user = (struct test_lazy_array_user *) handlebars_value_get_user(value);
    ck_assert_ptr_nonnull(user);

    key = handlebars_string_ctor(context, HBS_STRL("0"));
    result = handlebars_vm_lookup_property(vm, value, key, rv);
    ck_assert_ptr_eq(result, &user->item);
    ck_assert_int_eq(handlebars_value_get_intval(result), 10);
    ck_assert_uint_eq(user->find_calls, 1);
    ck_assert_uint_eq(user->last_index, 0);
    handlebars_string_delref(key);

    for( size_t i = 0; i < sizeof(invalid_keys) / sizeof(invalid_keys[0]); i++ ) {
        user->find_calls = 0;
        key = handlebars_string_ctor(context, invalid_keys[i], strlen(invalid_keys[i]));
        result = handlebars_vm_lookup_property(vm, value, key, rv);
        ck_assert_msg(result == NULL, "noncanonical array key %zu was accepted", i);
        ck_assert_msg(user->find_calls == 0, "noncanonical array key %zu reached callback", i);
        handlebars_string_delref(key);
    }

    max_key_length = snprintf(max_key, sizeof(max_key), "%zu", SIZE_MAX);
    ck_assert_msg(
        max_key_length > 0 && (size_t) max_key_length + 1 < sizeof(overflow_key),
        "SIZE_MAX did not fit the test buffer"
    );

    user->find_calls = 0;
    key = handlebars_string_ctor(context, max_key, (size_t) max_key_length);
    result = handlebars_vm_lookup_property(vm, value, key, rv);
    ck_assert_ptr_null(result);
    ck_assert_uint_eq(user->find_calls, 1);
    ck_assert_uint_eq(user->last_index, SIZE_MAX);
    handlebars_string_delref(key);

    memcpy(overflow_key, max_key, (size_t) max_key_length);
    overflow_key[max_key_length] = '0';
    overflow_key[max_key_length + 1] = '\0';
    user->find_calls = 0;
    key = handlebars_string_ctor(
        context,
        overflow_key,
        (size_t) max_key_length + 1
    );
    result = handlebars_vm_lookup_property(vm, value, key, rv);
    ck_assert_ptr_null(result);
    ck_assert_uint_eq(user->find_calls, 0);
    handlebars_string_delref(key);

    HANDLEBARS_VALUE_UNDECL(rv);
    HANDLEBARS_VALUE_UNDECL(value);
}
END_TEST

START_TEST(test_vm_owns_default_maps)
{
    ck_assert_ptr_eq(talloc_parent(handlebars_value_get_map(&vm->helpers)), vm);
    ck_assert_ptr_eq(talloc_parent(handlebars_value_get_map(&vm->partials)), vm);
}
END_TEST

static void assert_idle_builtin_rejects_bad_arity(
    handlebars_helper_func helper,
    const char * expected_error
)
{
    HANDLEBARS_VALUE_DECL(argv);
    HANDLEBARS_VALUE_DECL(rv);
    struct handlebars_options options = {
        .program = -1,
        .inverse = -1
    };
    struct handlebars_value * unexpected_result;
    jmp_buf * volatile previous = context->e->jmp;
    jmp_buf buf;

    if( handlebars_setjmp_ex(context, &buf) ) {
        context->e->jmp = previous;
        ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_ERROR);
        ck_assert_ptr_nonnull(
            strstr(handlebars_error_msg(context), expected_error)
        );
        clear_intentional_error();
        goto done;
    }

    unexpected_result = helper(0, argv, &options, vm, rv);
    context->e->jmp = previous;
    ck_abort_msg(
        "Expected idle VM builtin to reject bad arity, got %p",
        (void *) unexpected_result
    );

done:
    HANDLEBARS_VALUE_UNDECL(rv);
    HANDLEBARS_VALUE_UNDECL(argv);
}

START_TEST(test_idle_vm_builtins_reject_bad_arity)
{
    ck_assert_ptr_null(vm->stack);
    ck_assert_ptr_null(vm->contextStack);
    ck_assert_ptr_null(vm->hashStack);
    ck_assert_ptr_null(vm->blockParamStack);
    ck_assert_ptr_null(vm->partialBlockStack);
    ck_assert_ptr_null(vm->partialScopeStack);

    assert_idle_builtin_rejects_bad_arity(
        handlebars_builtin_each,
        "Must pass iterator to #each"
    );
    assert_idle_builtin_rejects_bad_arity(
        handlebars_builtin_with,
        "#with requires exactly one argument"
    );
    assert_idle_builtin_rejects_bad_arity(
        handlebars_builtin_if,
        "#if requires exactly one argument"
    );
}
END_TEST

START_TEST(test_idle_vm_builtins_handle_missing_programs)
{
    HANDLEBARS_VALUE_DECL(argv);
    HANDLEBARS_VALUE_DECL(rv);
    struct handlebars_options options = {
        .program = -1,
        .inverse = -1
    };
    struct handlebars_value * result;

    options.scope = argv;
    handlebars_value_array(argv, handlebars_stack_ctor(context, 0));
    result = handlebars_builtin_each(1, argv, &options, vm, rv);
    ck_assert_ptr_eq(result, rv);
    ck_assert_int_eq(handlebars_value_get_type(rv), HANDLEBARS_VALUE_TYPE_STRING);
    ck_assert_hbs_str_eq_cstr(handlebars_value_get_string(rv), "");
    handlebars_value_null(rv);

    handlebars_value_boolean(argv, true);
    result = handlebars_builtin_with(1, argv, &options, vm, rv);
    ck_assert_ptr_eq(result, rv);
    ck_assert_int_eq(handlebars_value_get_type(rv), HANDLEBARS_VALUE_TYPE_STRING);
    ck_assert_hbs_str_eq_cstr(handlebars_value_get_string(rv), "");
    handlebars_value_null(rv);

    handlebars_value_boolean(argv, true);
    result = handlebars_builtin_if(1, argv, &options, vm, rv);
    ck_assert_ptr_eq(result, rv);
    ck_assert_int_eq(handlebars_value_get_type(rv), HANDLEBARS_VALUE_TYPE_STRING);
    ck_assert_hbs_str_eq_cstr(handlebars_value_get_string(rv), "");
    handlebars_value_null(rv);

    handlebars_value_helper(argv, test_true_callable);
    result = handlebars_builtin_if(1, argv, &options, vm, rv);
    ck_assert_ptr_eq(result, rv);
    ck_assert_int_eq(handlebars_value_get_type(rv), HANDLEBARS_VALUE_TYPE_STRING);
    ck_assert_hbs_str_eq_cstr(handlebars_value_get_string(rv), "");

    HANDLEBARS_VALUE_UNDECL(rv);
    HANDLEBARS_VALUE_UNDECL(argv);
}
END_TEST

START_TEST(test_idle_if_callable_error_restores_data)
{
    HANDLEBARS_VALUE_DECL(argv);
    HANDLEBARS_VALUE_DECL(data);
    HANDLEBARS_VALUE_DECL(rv);
    struct handlebars_options options = {
        .program = -1,
        .inverse = -1,
        .scope = data
    };
    struct handlebars_value * unexpected_result;
    jmp_buf * volatile previous = context->e->jmp;
    jmp_buf buf;

    ck_assert_ptr_null(vm->stack);
    handlebars_value_integer(data, 7);
    handlebars_vm_set_data(vm, data);
    handlebars_value_helper(argv, test_set_data_and_throw_helper);

    if( handlebars_setjmp_ex(context, &buf) ) {
        context->e->jmp = previous;
        ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_ERROR);
        ck_assert_ptr_nonnull(
            strstr(
                handlebars_error_msg(context),
                "Intentional callable condition failure"
            )
        );
        clear_intentional_error();
        goto done;
    }

    unexpected_result = handlebars_builtin_if(1, argv, &options, vm, rv);
    context->e->jmp = previous;
    ck_abort_msg(
        "Expected idle callable #if condition to throw, got %p",
        (void *) unexpected_result
    );

done:
    ck_assert_int_eq(handlebars_value_get_type(&vm->data), HANDLEBARS_VALUE_TYPE_INTEGER);
    ck_assert_int_eq(handlebars_value_get_intval(&vm->data), 7);
    HANDLEBARS_VALUE_UNDECL(rv);
    HANDLEBARS_VALUE_UNDECL(data);
    HANDLEBARS_VALUE_UNDECL(argv);
}
END_TEST

#ifndef HANDLEBARS_NO_REFCOUNT
static int throwing_iterator_user_dtors;

static void throwing_iterator_user_dtor(struct handlebars_user * user)
{
    (void) user;
    throwing_iterator_user_dtors++;
}

static bool throwing_iterator_user_next(struct handlebars_value_iterator * it)
{
    handlebars_throw(it->user->ctx, HANDLEBARS_ERROR, "Intentional iterator failure");
    return false;
}

static bool throwing_iterator_user_init(
    struct handlebars_value_iterator * it,
    struct handlebars_value * value
) {
    (void) value;
    handlebars_value_integer(it->cur, 1);
    it->next = &throwing_iterator_user_next;
    return true;
}

static const struct handlebars_value_handlers throwing_iterator_user_handlers = {
    .name = "throwing-iterator-user",
    .dtor = &throwing_iterator_user_dtor,
    .iterator = &throwing_iterator_user_init
};

static const struct handlebars_value_handlers test_user_without_dtor_handlers = {
    .name = "test-user-without-dtor"
};

START_TEST(test_user_value_allows_optional_destructor)
{
    HANDLEBARS_VALUE_DECL(value);
    struct handlebars_user * user = handlebars_talloc_zero(
        context,
        struct handlebars_user
    );

    ck_assert_ptr_nonnull(user);
    handlebars_user_init(user, context, &test_user_without_dtor_handlers);
    handlebars_value_user(value, user);
    HANDLEBARS_VALUE_UNDECL(value);
    ASSERT_INIT_BLOCKS();
}
END_TEST
#endif

#ifndef HANDLEBARS_NO_REFCOUNT
START_TEST(test_delimiter_replacement_releases_old_values)
{
    struct handlebars_value * result;
    struct handlebars_options options = {0};
    size_t first_blocks;
    HANDLEBARS_VALUE_DECL(rv);
    HANDLEBARS_VALUE_ARRAY_DECL(argv, 2);

    handlebars_value_str(&argv[0], handlebars_string_ctor(context, HBS_STRL("<%")));
    handlebars_value_str(&argv[1], handlebars_string_ctor(context, HBS_STRL("%>")));
    result = handlebars_builtin_hbsc_set_delimiters(2, argv, &options, vm, rv);
    ck_assert_ptr_eq(result, rv);
    HANDLEBARS_VALUE_ARRAY_UNDECL(argv, 2);
    first_blocks = talloc_total_blocks(context);

    {
        HANDLEBARS_VALUE_ARRAY_DECL(replacement, 2);

        handlebars_value_str(
            &replacement[0],
            handlebars_string_ctor(context, HBS_STRL("[["))
        );
        handlebars_value_str(
            &replacement[1],
            handlebars_string_ctor(context, HBS_STRL("]]"))
        );
        result = handlebars_builtin_hbsc_set_delimiters(
            2,
            replacement,
            &options,
            vm,
            rv
        );
        ck_assert_ptr_eq(result, rv);
        HANDLEBARS_VALUE_ARRAY_UNDECL(replacement, 2);
    }

    ck_assert_uint_eq(talloc_total_blocks(context), first_blocks);
    HANDLEBARS_VALUE_UNDECL(rv);
}
END_TEST
#endif

START_TEST(test_vm_reusable_after_helper_error)
{
    HANDLEBARS_VALUE_DECL(helper);
    HANDLEBARS_VALUE_DECL(helpers);
    HANDLEBARS_VALUE_DECL(input);
    struct handlebars_map * helper_map = handlebars_map_ctor(context, 1);
    struct handlebars_module * failing = test_compile_template("{{boom}}");
    struct handlebars_module * succeeding = test_compile_template("ok");
    struct handlebars_string * output;
    jmp_buf * previous = context->e->jmp;
    jmp_buf buf;

    handlebars_value_helper(helper, test_throwing_helper);
    helper_map = handlebars_map_str_update(
        helper_map,
        HBS_STRL("boom"),
        helper
    );
    handlebars_value_map(helpers, helper_map);
    handlebars_vm_set_helpers(vm, helpers);

    if( handlebars_setjmp_ex(context, &buf) ) {
        context->e->jmp = previous;
        ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_ERROR);
    } else {
        (void) handlebars_vm_execute(vm, failing, input);
        context->e->jmp = previous;
        ck_abort_msg("Expected the helper to throw");
    }

    ck_assert_ptr_null(vm->stack);
    ck_assert_ptr_null(vm->contextStack);
    ck_assert_ptr_null(vm->hashStack);
    ck_assert_ptr_null(vm->blockParamStack);
    ck_assert_ptr_null(vm->partialBlockStack);
    ck_assert_ptr_null(vm->partialScopeStack);
    ck_assert_ptr_null(vm->last_context);
    ck_assert_ptr_null(vm->module);
    ck_assert_ptr_null(vm->buffer);

    output = handlebars_vm_execute(vm, succeeding, input);
    ck_assert_ptr_nonnull(output);
    ck_assert_hbs_str_eq_cstr(output, "ok");
    handlebars_string_delref(output);

    HANDLEBARS_VALUE_UNDECL(input);
    HANDLEBARS_VALUE_UNDECL(helpers);
    HANDLEBARS_VALUE_UNDECL(helper);
}
END_TEST

START_TEST(test_caught_each_error_restores_outer_data)
{
    HANDLEBARS_VALUE_DECL(item);
    HANDLEBARS_VALUE_DECL(items);
    HANDLEBARS_VALUE_DECL(input);
    struct handlebars_stack * item_stack;
    struct handlebars_map * input_map;
    struct handlebars_module * module;
    struct handlebars_string * output;

    handlebars_value_boolean(item, true);
    item_stack = handlebars_stack_ctor(context, 1);
    item_stack = handlebars_stack_push(item_stack, item);
    handlebars_value_array(items, item_stack);
    input_map = handlebars_map_ctor(context, 1);
    input_map = handlebars_map_str_add(
        input_map,
        HBS_STRL("items"),
        items
    );
    handlebars_value_map(input, input_map);
    test_register_helper(HBS_STRL("catchEach"), test_catch_each_helper);
    module = test_compile_template(
        "{{#catchEach items}}{{> missing}}{{/catchEach}}after={{@index}}"
    );

    output = handlebars_vm_execute(vm, module, input);
    ck_assert_ptr_nonnull(output);
    ck_assert_hbs_str_eq_cstr(output, "caughtafter=");
    handlebars_string_delref(output);

    HANDLEBARS_VALUE_UNDECL(input);
    HANDLEBARS_VALUE_UNDECL(items);
    HANDLEBARS_VALUE_UNDECL(item);
}
END_TEST

START_TEST(test_caught_if_errors_restore_outer_data_and_vm_reuse)
{
    HANDLEBARS_VALUE_DECL(holder);
    HANDLEBARS_VALUE_DECL(item);
    HANDLEBARS_VALUE_DECL(items);
    HANDLEBARS_VALUE_DECL(input);
    struct handlebars_stack * item_stack;
    struct handlebars_map * input_map;
    struct handlebars_module * module;

    handlebars_value_helper(holder, test_execute_if_program_as_callable);
    handlebars_value_boolean(item, true);
    item_stack = handlebars_stack_ctor(context, 2);
    item_stack = handlebars_stack_push(item_stack, item);
    item_stack = handlebars_stack_push(item_stack, item);
    handlebars_value_array(items, item_stack);
    input_map = handlebars_map_ctor(context, 2);
    input_map = handlebars_map_str_add(
        input_map,
        HBS_STRL("holder"),
        holder
    );
    input_map = handlebars_map_str_add(
        input_map,
        HBS_STRL("items"),
        items
    );
    handlebars_value_map(input, input_map);
    test_register_helper(HBS_STRL("catchIf"), test_catch_if_helper);
    module = test_compile_template(
        "{{#each items}}"
        "{{#catchIf true}}{{> missing}}{{/catchIf}}selected={{@index}};"
        "{{#catchIf ../holder}}{{> missing}}{{/catchIf}}callable={{@index}};"
        "{{/each}}"
    );

    for( int i = 0; i < 2; i++ ) {
        struct handlebars_string * output = handlebars_vm_execute(
            vm,
            module,
            input
        );

        ck_assert_ptr_nonnull(output);
        ck_assert_hbs_str_eq_cstr(
            output,
            "caughtselected=0;caughtcallable=0;"
            "caughtselected=1;caughtcallable=1;"
        );
        handlebars_string_delref(output);
        ck_assert_ptr_null(vm->stack);
        ck_assert_ptr_null(vm->contextStack);
        ck_assert_ptr_null(vm->hashStack);
        ck_assert_ptr_null(vm->blockParamStack);
        ck_assert_ptr_null(vm->partialBlockStack);
        ck_assert_ptr_null(vm->partialScopeStack);
        ck_assert_ptr_null(vm->buffer);
    }

    HANDLEBARS_VALUE_UNDECL(input);
    HANDLEBARS_VALUE_UNDECL(items);
    HANDLEBARS_VALUE_UNDECL(item);
    HANDLEBARS_VALUE_UNDECL(holder);
}
END_TEST

#ifndef HANDLEBARS_NO_REFCOUNT
static void assert_block_helper_error_releases_vm_temporaries(
    const char * tmpl,
    struct handlebars_value * input
)
{
    struct handlebars_module * failing = test_compile_template(tmpl);
    size_t baseline_blocks;

    baseline_blocks = talloc_total_blocks(vm);

    for( int i = 0; i < 3; i++ ) {
        jmp_buf * previous = context->e->jmp;
        jmp_buf buf;

        if( handlebars_setjmp_ex(context, &buf) ) {
            context->e->jmp = previous;
            ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_ERROR);
            ck_assert_ptr_nonnull(
                strstr(
                    handlebars_error_msg(context),
                    "partial missing could not be found"
                )
            );
            clear_intentional_error();
        } else {
            (void) handlebars_vm_execute(vm, failing, input);
            context->e->jmp = previous;
            ck_abort_msg("Expected the nested missing partial to throw");
        }

        ck_assert_msg(
            talloc_total_blocks(vm) == baseline_blocks,
            "failed render %d retained %zu VM blocks (baseline %zu)",
            i + 1,
            talloc_total_blocks(vm),
            baseline_blocks
        );
    }
}

START_TEST(test_with_helper_error_releases_vm_temporaries)
{
    HANDLEBARS_VALUE_DECL(holder);
    HANDLEBARS_VALUE_DECL(input);
    struct handlebars_map * input_map;

    handlebars_value_boolean(holder, true);
    input_map = handlebars_map_ctor(context, 1);
    input_map = handlebars_map_str_add(
        input_map,
        HBS_STRL("holder"),
        holder
    );
    handlebars_value_map(input, input_map);

    assert_block_helper_error_releases_vm_temporaries(
        "{{#with holder}}{{> missing}}{{/with}}",
        input
    );

    HANDLEBARS_VALUE_UNDECL(input);
    HANDLEBARS_VALUE_UNDECL(holder);
}
END_TEST

START_TEST(test_with_callable_error_releases_vm_temporaries)
{
    HANDLEBARS_VALUE_DECL(holder);
    HANDLEBARS_VALUE_DECL(input);
    struct handlebars_map * input_map;

    handlebars_value_helper(holder, test_with_context_helper);
    input_map = handlebars_map_ctor(context, 1);
    input_map = handlebars_map_str_add(
        input_map,
        HBS_STRL("holder"),
        holder
    );
    handlebars_value_map(input, input_map);

    assert_block_helper_error_releases_vm_temporaries(
        "{{#with holder}}{{> missing}}{{/with}}",
        input
    );

    HANDLEBARS_VALUE_UNDECL(input);
    HANDLEBARS_VALUE_UNDECL(holder);
}
END_TEST

START_TEST(test_if_helper_error_releases_vm_temporaries)
{
    HANDLEBARS_VALUE_DECL(input);

    assert_block_helper_error_releases_vm_temporaries(
        "{{#if true}}{{> missing}}{{/if}}",
        input
    );

    HANDLEBARS_VALUE_UNDECL(input);
}
END_TEST

START_TEST(test_if_callable_selected_program_error_releases_vm_temporaries)
{
    HANDLEBARS_VALUE_DECL(holder);
    HANDLEBARS_VALUE_DECL(input);
    struct handlebars_map * input_map;

    handlebars_value_helper(holder, test_with_context_helper);
    input_map = handlebars_map_ctor(context, 1);
    input_map = handlebars_map_str_add(
        input_map,
        HBS_STRL("holder"),
        holder
    );
    handlebars_value_map(input, input_map);

    assert_block_helper_error_releases_vm_temporaries(
        "{{#if holder}}{{> missing}}{{/if}}",
        input
    );

    HANDLEBARS_VALUE_UNDECL(input);
    HANDLEBARS_VALUE_UNDECL(holder);
}
END_TEST

START_TEST(test_unless_helper_error_releases_vm_temporaries)
{
    HANDLEBARS_VALUE_DECL(input);

    assert_block_helper_error_releases_vm_temporaries(
        "{{#unless false}}{{> missing}}{{/unless}}",
        input
    );

    HANDLEBARS_VALUE_UNDECL(input);
}
END_TEST

START_TEST(test_each_helper_error_releases_vm_temporaries)
{
    HANDLEBARS_VALUE_DECL(item);
    HANDLEBARS_VALUE_DECL(items);
    HANDLEBARS_VALUE_DECL(input);
    struct handlebars_stack * item_stack;
    struct handlebars_map * input_map;

    handlebars_value_map(item, handlebars_map_ctor(context, 0));
    item_stack = handlebars_stack_ctor(context, 1);
    item_stack = handlebars_stack_push(item_stack, item);
    handlebars_value_array(items, item_stack);
    input_map = handlebars_map_ctor(context, 1);
    input_map = handlebars_map_str_add(
        input_map,
        HBS_STRL("items"),
        items
    );
    handlebars_value_map(input, input_map);

    assert_block_helper_error_releases_vm_temporaries(
        "{{#each items}}{{> missing}}{{/each}}",
        input
    );

    HANDLEBARS_VALUE_UNDECL(input);
    HANDLEBARS_VALUE_UNDECL(items);
    HANDLEBARS_VALUE_UNDECL(item);
}
END_TEST

START_TEST(test_each_callable_error_releases_vm_temporaries)
{
    HANDLEBARS_VALUE_DECL(items);
    HANDLEBARS_VALUE_DECL(input);
    struct handlebars_map * input_map;

    handlebars_value_helper(items, test_each_context_helper);
    input_map = handlebars_map_ctor(context, 1);
    input_map = handlebars_map_str_add(
        input_map,
        HBS_STRL("items"),
        items
    );
    handlebars_value_map(input, input_map);

    assert_block_helper_error_releases_vm_temporaries(
        "{{#each items}}{{> missing}}{{/each}}",
        input
    );

    HANDLEBARS_VALUE_UNDECL(input);
    HANDLEBARS_VALUE_UNDECL(items);
}
END_TEST

START_TEST(test_each_helper_success_releases_nested_buffers)
{
    HANDLEBARS_VALUE_DECL(item);
    HANDLEBARS_VALUE_DECL(items);
    HANDLEBARS_VALUE_DECL(input);
    struct handlebars_stack * item_stack;
    struct handlebars_map * input_map;
    struct handlebars_module * module;
    struct handlebars_string * output;
    size_t baseline_blocks;

    handlebars_value_str(
        item,
        handlebars_string_ctor(context, HBS_STRL("x"))
    );
    item_stack = handlebars_stack_ctor(context, 3);
    item_stack = handlebars_stack_push(item_stack, item);
    item_stack = handlebars_stack_push(item_stack, item);
    item_stack = handlebars_stack_push(item_stack, item);
    handlebars_value_array(items, item_stack);
    input_map = handlebars_map_ctor(context, 1);
    input_map = handlebars_map_str_add(
        input_map,
        HBS_STRL("items"),
        items
    );
    handlebars_value_map(input, input_map);
    module = test_compile_template("{{#each items}}{{this}}{{/each}}");
    baseline_blocks = talloc_total_blocks(vm);

    output = handlebars_vm_execute(vm, module, input);
    ck_assert_ptr_nonnull(output);
    ck_assert_hbs_str_eq_cstr(output, "xxx");
    handlebars_string_delref(output);

    ck_assert_msg(
        talloc_total_blocks(vm) == baseline_blocks,
        "successful each render retained %zu VM blocks (baseline %zu)",
        talloc_total_blocks(vm),
        baseline_blocks
    );

    HANDLEBARS_VALUE_UNDECL(input);
    HANDLEBARS_VALUE_UNDECL(items);
    HANDLEBARS_VALUE_UNDECL(item);
}
END_TEST

START_TEST(test_with_helper_success_releases_nested_buffer)
{
    HANDLEBARS_VALUE_DECL(holder);
    HANDLEBARS_VALUE_DECL(input);
    struct handlebars_map * input_map;
    struct handlebars_module * module;
    struct handlebars_string * output;
    size_t baseline_blocks;

    handlebars_value_str(
        holder,
        handlebars_string_ctor(context, HBS_STRL("value"))
    );
    input_map = handlebars_map_ctor(context, 1);
    input_map = handlebars_map_str_add(
        input_map,
        HBS_STRL("holder"),
        holder
    );
    handlebars_value_map(input, input_map);
    module = test_compile_template("{{#with holder}}{{this}}{{/with}}");
    baseline_blocks = talloc_total_blocks(vm);

    output = handlebars_vm_execute(vm, module, input);
    ck_assert_ptr_nonnull(output);
    ck_assert_hbs_str_eq_cstr(output, "value");
    handlebars_string_delref(output);

    ck_assert_msg(
        talloc_total_blocks(vm) == baseline_blocks,
        "successful with render retained %zu VM blocks (baseline %zu)",
        talloc_total_blocks(vm),
        baseline_blocks
    );

    HANDLEBARS_VALUE_UNDECL(input);
    HANDLEBARS_VALUE_UNDECL(holder);
}
END_TEST

START_TEST(test_block_helper_missing_error_releases_vm_temporaries)
{
    HANDLEBARS_VALUE_DECL(holder);
    HANDLEBARS_VALUE_DECL(input);
    struct handlebars_map * input_map;

    handlebars_value_boolean(holder, true);
    input_map = handlebars_map_ctor(context, 1);
    input_map = handlebars_map_str_add(
        input_map,
        HBS_STRL("holder"),
        holder
    );
    handlebars_value_map(input, input_map);

    assert_block_helper_error_releases_vm_temporaries(
        "{{#holder}}{{> missing}}{{/holder}}",
        input
    );

    HANDLEBARS_VALUE_UNDECL(input);
    HANDLEBARS_VALUE_UNDECL(holder);
}
END_TEST
#endif

#ifdef HANDLEBARS_MEMORY
static void assert_block_helper_allocation_failures_unwind_vm(
    const char * tmpl,
    struct handlebars_value * input,
    const char * expected
)
{
    struct handlebars_module * module = test_compile_template(tmpl);
#ifndef HANDLEBARS_NO_REFCOUNT
    size_t baseline_blocks = talloc_total_blocks(vm);
#endif
    bool reached_success = false;

    for( int fail_at = 1; fail_at <= 128; fail_at++ ) {
        struct handlebars_string * output;

        handlebars_memory_fail_set_flags(handlebars_memory_fail_flag_alloc);
        handlebars_memory_fail_counter(fail_at);
        output = handlebars_vm_execute(vm, module, input);
        handlebars_memory_fail_disable();

        if( output != NULL ) {
            ck_assert_hbs_str_eq_cstr(output, expected);
            handlebars_string_delref(output);
            reached_success = true;
            break;
        }

        ck_assert_msg(
            handlebars_error_num(context) == HANDLEBARS_NOMEM,
            "allocation %d failed with %d (%s), expected HANDLEBARS_NOMEM",
            fail_at,
            handlebars_error_num(context),
            handlebars_error_msg(context)
        );
        ck_assert_ptr_null(vm->stack);
        ck_assert_ptr_null(vm->contextStack);
        ck_assert_ptr_null(vm->hashStack);
        ck_assert_ptr_null(vm->blockParamStack);
        ck_assert_ptr_null(vm->partialBlockStack);
        ck_assert_ptr_null(vm->partialScopeStack);
#ifndef HANDLEBARS_NO_REFCOUNT
        ck_assert_msg(
            talloc_total_blocks(vm) == baseline_blocks,
            "allocation %d retained %zu VM blocks (baseline %zu)",
            fail_at,
            talloc_total_blocks(vm),
            baseline_blocks
        );
#endif

        output = handlebars_vm_execute(vm, module, input);
        ck_assert_msg(output != NULL, "%s", handlebars_error_msg(context));
        ck_assert_hbs_str_eq_cstr(output, expected);
        handlebars_string_delref(output);
#ifndef HANDLEBARS_NO_REFCOUNT
        ck_assert_uint_eq(talloc_total_blocks(vm), baseline_blocks);
#endif
    }

    ck_assert(reached_success);
}

START_TEST(test_with_helper_allocation_failures_unwind_vm)
{
    HANDLEBARS_VALUE_DECL(input);

    assert_block_helper_allocation_failures_unwind_vm(
        "{{#with true}}success{{/with}}",
        input,
        "success"
    );

    HANDLEBARS_VALUE_UNDECL(input);
}
END_TEST

START_TEST(test_each_helper_allocation_failures_unwind_vm)
{
    HANDLEBARS_VALUE_DECL(item);
    HANDLEBARS_VALUE_DECL(items);
    HANDLEBARS_VALUE_DECL(input);
    struct handlebars_stack * item_stack;
    struct handlebars_map * input_map;

    handlebars_value_boolean(item, true);
    item_stack = handlebars_stack_ctor(context, 1);
    item_stack = handlebars_stack_push(item_stack, item);
    handlebars_value_array(items, item_stack);
    input_map = handlebars_map_ctor(context, 1);
    input_map = handlebars_map_str_add(
        input_map,
        HBS_STRL("items"),
        items
    );
    handlebars_value_map(input, input_map);

    assert_block_helper_allocation_failures_unwind_vm(
        "{{#each items}}success{{/each}}",
        input,
        "success"
    );

    HANDLEBARS_VALUE_UNDECL(input);
    HANDLEBARS_VALUE_UNDECL(items);
    HANDLEBARS_VALUE_UNDECL(item);
}
END_TEST

START_TEST(test_each_map_allocation_failures_unwind_vm)
{
    HANDLEBARS_VALUE_DECL(item);
    HANDLEBARS_VALUE_DECL(items);
    HANDLEBARS_VALUE_DECL(input);
    struct handlebars_map * items_map;
    struct handlebars_map * input_map;

    handlebars_value_str(
        item,
        handlebars_string_ctor(context, HBS_STRL("value"))
    );
    items_map = handlebars_map_ctor(context, 1);
    items_map = handlebars_map_str_add(
        items_map,
        HBS_STRL("only"),
        item
    );
    handlebars_value_map(items, items_map);
    input_map = handlebars_map_ctor(context, 1);
    input_map = handlebars_map_str_add(
        input_map,
        HBS_STRL("items"),
        items
    );
    handlebars_value_map(input, input_map);

    assert_block_helper_allocation_failures_unwind_vm(
        "{{#each items}}{{@key}}={{this}};{{/each}}",
        input,
        "only=value;"
    );

    HANDLEBARS_VALUE_UNDECL(input);
    HANDLEBARS_VALUE_UNDECL(items);
    HANDLEBARS_VALUE_UNDECL(item);
}
END_TEST

START_TEST(test_with_callable_allocation_failures_unwind_vm)
{
    HANDLEBARS_VALUE_DECL(holder);
    HANDLEBARS_VALUE_DECL(input);
    struct handlebars_map * input_map;

    handlebars_value_helper(holder, test_with_context_helper);
    input_map = handlebars_map_ctor(context, 1);
    input_map = handlebars_map_str_add(
        input_map,
        HBS_STRL("holder"),
        holder
    );
    handlebars_value_map(input, input_map);

    assert_block_helper_allocation_failures_unwind_vm(
        "{{#with holder}}{{this}}{{/with}}",
        input,
        "callable"
    );

    HANDLEBARS_VALUE_UNDECL(input);
    HANDLEBARS_VALUE_UNDECL(holder);
}
END_TEST

START_TEST(test_if_callable_allocation_failures_unwind_vm)
{
    HANDLEBARS_VALUE_DECL(holder);
    HANDLEBARS_VALUE_DECL(input);
    struct handlebars_map * input_map;

    handlebars_value_helper(holder, test_with_context_helper);
    input_map = handlebars_map_ctor(context, 1);
    input_map = handlebars_map_str_add(
        input_map,
        HBS_STRL("holder"),
        holder
    );
    handlebars_value_map(input, input_map);

    assert_block_helper_allocation_failures_unwind_vm(
        "{{#if holder}}success{{/if}}",
        input,
        "success"
    );

    HANDLEBARS_VALUE_UNDECL(input);
    HANDLEBARS_VALUE_UNDECL(holder);
}
END_TEST

START_TEST(test_each_callable_allocation_failures_unwind_vm)
{
    HANDLEBARS_VALUE_DECL(items);
    HANDLEBARS_VALUE_DECL(input);
    struct handlebars_map * input_map;

    handlebars_value_helper(items, test_each_context_helper);
    input_map = handlebars_map_ctor(context, 1);
    input_map = handlebars_map_str_add(
        input_map,
        HBS_STRL("items"),
        items
    );
    handlebars_value_map(input, input_map);

    assert_block_helper_allocation_failures_unwind_vm(
        "{{#each items}}{{this}}{{/each}}",
        input,
        "true"
    );

    HANDLEBARS_VALUE_UNDECL(input);
    HANDLEBARS_VALUE_UNDECL(items);
}
END_TEST

START_TEST(test_ambiguous_block_value_allocation_failures_unwind_vm)
{
    HANDLEBARS_VALUE_DECL(holder);
    HANDLEBARS_VALUE_DECL(input);
    struct handlebars_map * input_map;

    test_register_helper(
        HBS_STRL("blockHelperMissing"),
        test_allocating_helper
    );
    handlebars_value_boolean(holder, true);
    input_map = handlebars_map_ctor(context, 1);
    input_map = handlebars_map_str_add(
        input_map,
        HBS_STRL("holder"),
        holder
    );
    handlebars_value_map(input, input_map);

    assert_block_helper_allocation_failures_unwind_vm(
        "{{#holder}}ignored{{/holder}}",
        input,
        "success"
    );

    HANDLEBARS_VALUE_UNDECL(input);
    HANDLEBARS_VALUE_UNDECL(holder);
}
END_TEST

START_TEST(test_block_value_allocation_failures_unwind_vm)
{
    HANDLEBARS_VALUE_DECL(awesome);
    HANDLEBARS_VALUE_DECL(foo);
    HANDLEBARS_VALUE_DECL(input);
    struct handlebars_map * foo_map;
    struct handlebars_map * input_map;

    test_register_helper(
        HBS_STRL("blockHelperMissing"),
        test_allocating_helper
    );
    handlebars_value_boolean(awesome, true);
    foo_map = handlebars_map_ctor(context, 1);
    foo_map = handlebars_map_str_add(
        foo_map,
        HBS_STRL("awesome"),
        awesome
    );
    handlebars_value_map(foo, foo_map);
    input_map = handlebars_map_ctor(context, 1);
    input_map = handlebars_map_str_add(
        input_map,
        HBS_STRL("foo"),
        foo
    );
    handlebars_value_map(input, input_map);

    assert_block_helper_allocation_failures_unwind_vm(
        "{{#foo.awesome}}ignored{{/foo.awesome}}",
        input,
        "success"
    );

    HANDLEBARS_VALUE_UNDECL(input);
    HANDLEBARS_VALUE_UNDECL(foo);
    HANDLEBARS_VALUE_UNDECL(awesome);
}
END_TEST

START_TEST(test_invoke_helper_allocation_failures_unwind_vm)
{
    HANDLEBARS_VALUE_DECL(bar);
    HANDLEBARS_VALUE_DECL(input);
    struct handlebars_map * input_map;

    test_register_helper(HBS_STRL("foo"), test_allocating_helper);
    handlebars_value_str(
        bar,
        handlebars_string_ctor(context, HBS_STRL("success"))
    );
    input_map = handlebars_map_ctor(context, 1);
    input_map = handlebars_map_str_add(
        input_map,
        HBS_STRL("bar"),
        bar
    );
    handlebars_value_map(input, input_map);

    assert_block_helper_allocation_failures_unwind_vm(
        "{{foo bar}}",
        input,
        "success"
    );

    HANDLEBARS_VALUE_UNDECL(input);
    HANDLEBARS_VALUE_UNDECL(bar);
}
END_TEST

START_TEST(test_invoke_known_helper_allocation_failures_unwind_vm)
{
    HANDLEBARS_VALUE_DECL(input);

    test_register_helper(HBS_STRL("with"), test_allocating_helper);
    assert_block_helper_allocation_failures_unwind_vm(
        "{{#with true}}ignored{{/with}}",
        input,
        "success"
    );

    HANDLEBARS_VALUE_UNDECL(input);
}
END_TEST
#endif

START_TEST(test_inline_partial_error_unwinds_vm)
{
    HANDLEBARS_VALUE_DECL(input);
    struct handlebars_module * failing = test_compile_template(
        "{{#*inline \"myPartial\"}}prefix{{> missing}}{{/inline}}"
        "{{> myPartial}}"
    );
    struct handlebars_module * succeeding = test_compile_template(
        "{{#*inline \"myPartial\"}}ok{{/inline}}{{> myPartial}}"
    );
    struct handlebars_string * output;
    jmp_buf * previous = context->e->jmp;
    jmp_buf buf;
#ifndef HANDLEBARS_NO_REFCOUNT
    size_t baseline_blocks;
#endif

#ifndef HANDLEBARS_NO_REFCOUNT
    baseline_blocks = talloc_total_blocks(vm);
#endif

    if( handlebars_setjmp_ex(context, &buf) ) {
        context->e->jmp = previous;
        ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_ERROR);
        ck_assert_ptr_nonnull(strstr(handlebars_error_msg(context), "partial missing could not be found"));
    } else {
        (void) handlebars_vm_execute(vm, failing, input);
        context->e->jmp = previous;
        ck_abort_msg("Expected the nested missing partial to throw");
    }

    ck_assert_ptr_null(vm->stack);
    ck_assert_ptr_null(vm->contextStack);
    ck_assert_ptr_null(vm->hashStack);
    ck_assert_ptr_null(vm->blockParamStack);
    ck_assert_ptr_null(vm->partialBlockStack);
    ck_assert_ptr_null(vm->partialScopeStack);
    ck_assert_ptr_null(vm->last_context);
    ck_assert_ptr_null(vm->module);
    ck_assert_ptr_null(vm->buffer);
#ifndef HANDLEBARS_NO_REFCOUNT
    ck_assert_uint_eq(talloc_total_blocks(vm), baseline_blocks);
#endif

    clear_intentional_error();
    output = handlebars_vm_execute(vm, succeeding, input);
    ck_assert_ptr_nonnull(output);
    ck_assert_hbs_str_eq_cstr(output, "ok");
    handlebars_string_delref(output);
#ifndef HANDLEBARS_NO_REFCOUNT
    ck_assert_uint_eq(talloc_total_blocks(vm), baseline_blocks);
#endif

    HANDLEBARS_VALUE_UNDECL(input);
}
END_TEST

START_TEST(test_inline_partial_explicit_context_error_unwinds_vm)
{
    HANDLEBARS_VALUE_DECL(input);
    HANDLEBARS_VALUE_DECL(child);
    HANDLEBARS_VALUE_DECL(holder);
    HANDLEBARS_VALUE_DECL(value);
    HANDLEBARS_VALUE_DECL(captured_value);
    HANDLEBARS_VALUE_DECL(failing_partial);
    HANDLEBARS_VALUE_DECL(succeeding_partial);
    HANDLEBARS_VALUE_DECL(partials);
    struct handlebars_module * failing = test_compile_template("{{> failing holder}}");
    struct handlebars_module * succeeding = test_compile_template("{{> succeeding holder}}");
    struct handlebars_string * output;
#ifndef HANDLEBARS_NO_REFCOUNT
    size_t baseline_blocks;
#endif

    handlebars_value_str(
        value,
        handlebars_string_ctor(context, HBS_STRL("explicit"))
    );
    handlebars_value_str(
        captured_value,
        handlebars_string_ctor(context, HBS_STRL("captured"))
    );
    struct handlebars_map * child_map = handlebars_map_ctor(context, 1);
    child_map = handlebars_map_str_add(child_map, HBS_STRL("value"), value);
    handlebars_value_map(child, child_map);
    handlebars_value_map(holder, handlebars_map_ctor(context, 0));
    struct handlebars_map * input_map = handlebars_map_ctor(context, 3);
    input_map = handlebars_map_str_add(input_map, HBS_STRL("child"), child);
    input_map = handlebars_map_str_add(input_map, HBS_STRL("holder"), holder);
    input_map = handlebars_map_str_add(input_map, HBS_STRL("value"), captured_value);
    handlebars_value_map(input, input_map);
    handlebars_value_str(
        failing_partial,
        handlebars_string_ctor(
            context,
            HBS_STRL(
                "{{#*inline \"myPartial\"}}{{value}}{{> missing}}{{/inline}}"
                "{{> myPartial ../child}}"
            )
        )
    );
    handlebars_value_str(
        succeeding_partial,
        handlebars_string_ctor(
            context,
            HBS_STRL(
                "{{#*inline \"myPartial\"}}{{value}}{{/inline}}"
                "{{> myPartial ../child}}"
            )
        )
    );
    struct handlebars_map * partial_map = handlebars_map_ctor(context, 2);
    partial_map = handlebars_map_str_add(
        partial_map,
        HBS_STRL("failing"),
        failing_partial
    );
    partial_map = handlebars_map_str_add(
        partial_map,
        HBS_STRL("succeeding"),
        succeeding_partial
    );
    handlebars_value_map(partials, partial_map);
    handlebars_vm_set_partials(vm, partials);
#ifndef HANDLEBARS_NO_REFCOUNT
    baseline_blocks = talloc_total_blocks(vm);
#endif

    output = handlebars_vm_execute(vm, failing, input);
    ck_assert_ptr_nonnull(output);
    ck_assert_hbs_str_eq_cstr(output, "");
    handlebars_string_delref(output);
    ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_ERROR);
    ck_assert_ptr_nonnull(
        strstr(handlebars_error_msg(context), "partial missing could not be found")
    );

    ck_assert_ptr_null(vm->stack);
    ck_assert_ptr_null(vm->contextStack);
    ck_assert_ptr_null(vm->hashStack);
    ck_assert_ptr_null(vm->blockParamStack);
    ck_assert_ptr_null(vm->partialBlockStack);
    ck_assert_ptr_null(vm->partialScopeStack);
    ck_assert_ptr_null(vm->last_context);
    ck_assert_ptr_null(vm->module);
    ck_assert_ptr_null(vm->buffer);
#ifndef HANDLEBARS_NO_REFCOUNT
    ck_assert_uint_eq(talloc_total_blocks(vm), baseline_blocks);
#endif

    clear_intentional_error();
    output = handlebars_vm_execute(vm, succeeding, input);
    ck_assert_ptr_nonnull(output);
    ck_assert_hbs_str_eq_cstr(output, "explicit");
    handlebars_string_delref(output);
#ifndef HANDLEBARS_NO_REFCOUNT
    ck_assert_uint_eq(talloc_total_blocks(vm), baseline_blocks);
#endif

    HANDLEBARS_VALUE_UNDECL(partials);
    HANDLEBARS_VALUE_UNDECL(succeeding_partial);
    HANDLEBARS_VALUE_UNDECL(failing_partial);
    HANDLEBARS_VALUE_UNDECL(captured_value);
    HANDLEBARS_VALUE_UNDECL(value);
    HANDLEBARS_VALUE_UNDECL(holder);
    HANDLEBARS_VALUE_UNDECL(child);
    HANDLEBARS_VALUE_UNDECL(input);
}
END_TEST

START_TEST(test_inline_partial_recursive_partial_block_is_bounded)
{
    HANDLEBARS_VALUE_DECL(input);
    struct handlebars_module * failing = test_compile_template(
        "{{#*inline \"layout\"}}{{@partial-block}}{{/inline}}"
        "{{#> layout}}{{@partial-block}}{{/layout}}"
    );
    struct handlebars_module * succeeding = test_compile_template("ok");
    struct handlebars_string * output;
    jmp_buf * previous = context->e->jmp;
    jmp_buf buf;
#ifndef HANDLEBARS_NO_REFCOUNT
    size_t baseline_blocks = talloc_total_blocks(vm);
#endif

    if( handlebars_setjmp_ex(context, &buf) ) {
        context->e->jmp = previous;
        ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_STACK_OVERFLOW);
        ck_assert_ptr_nonnull(
            strstr(handlebars_error_msg(context), "VM program stack overflow")
        );
    } else {
        (void) handlebars_vm_execute(vm, failing, input);
        context->e->jmp = previous;
        ck_abort_msg("Expected recursive partial-block execution to be rejected");
    }

    ck_assert_ptr_null(vm->stack);
    ck_assert_ptr_null(vm->contextStack);
    ck_assert_ptr_null(vm->hashStack);
    ck_assert_ptr_null(vm->blockParamStack);
    ck_assert_ptr_null(vm->partialBlockStack);
    ck_assert_ptr_null(vm->partialScopeStack);
    ck_assert_ptr_null(vm->last_context);
    ck_assert_ptr_null(vm->module);
    ck_assert_ptr_null(vm->buffer);
#ifndef HANDLEBARS_NO_REFCOUNT
    ck_assert_uint_eq(talloc_total_blocks(vm), baseline_blocks);
#endif

    clear_intentional_error();
    output = handlebars_vm_execute(vm, succeeding, input);
    ck_assert_ptr_nonnull(output);
    ck_assert_hbs_str_eq_cstr(output, "ok");
    handlebars_string_delref(output);
#ifndef HANDLEBARS_NO_REFCOUNT
    ck_assert_uint_eq(talloc_total_blocks(vm), baseline_blocks);
#endif

    HANDLEBARS_VALUE_UNDECL(input);
}
END_TEST

START_TEST(test_inline_partial_error_after_stack_growth_unwinds_vm)
{
    HANDLEBARS_VALUE_DECL(input);
    HANDLEBARS_VALUE_DECL(node);
    HANDLEBARS_VALUE_DECL(recurse_partial);
    HANDLEBARS_VALUE_DECL(partials);
    struct handlebars_module * failing = test_compile_template("{{> recurse start}}");
    struct handlebars_module * succeeding = test_compile_template("ok");
    struct handlebars_string * output;

    handlebars_value_map(node, handlebars_map_ctor(context, 0));
    for( unsigned int i = 0; i < 47; i++ ) {
        struct handlebars_map * parent = handlebars_map_ctor(context, 1);
        parent = handlebars_map_str_add(parent, HBS_STRL("next"), node);
        handlebars_value_map(node, parent);
    }
    struct handlebars_map * input_map = handlebars_map_ctor(context, 1);
    input_map = handlebars_map_str_add(input_map, HBS_STRL("start"), node);
    handlebars_value_map(input, input_map);

    handlebars_value_str(
        recurse_partial,
        handlebars_string_ctor(
            context,
            HBS_STRL(
                "{{#if next}}"
                "{{#with next as |x|}}{{> recurse}}{{/with}}"
                "{{else}}"
                "{{#if true}}{{#if true}}{{#if true}}"
                "{{#*inline \"layout\"}}{{> @partial-block}}{{/inline}}"
                "{{#> layout}}{{> missing}}{{/layout}}"
                "{{/if}}{{/if}}{{/if}}"
                "{{/if}}"
            )
        )
    );
    struct handlebars_map * partial_map = handlebars_map_ctor(context, 1);
    partial_map = handlebars_map_str_add(
        partial_map,
        HBS_STRL("recurse"),
        recurse_partial
    );
    handlebars_value_map(partials, partial_map);
    handlebars_vm_set_partials(vm, partials);

    output = handlebars_vm_execute(vm, failing, input);
    ck_assert_ptr_nonnull(output);
    ck_assert_hbs_str_eq_cstr(output, "");
    handlebars_string_delref(output);
    ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_ERROR);
    ck_assert_ptr_nonnull(
        strstr(handlebars_error_msg(context), "partial missing could not be found")
    );

    ck_assert_ptr_null(vm->stack);
    ck_assert_ptr_null(vm->contextStack);
    ck_assert_ptr_null(vm->hashStack);
    ck_assert_ptr_null(vm->blockParamStack);
    ck_assert_ptr_null(vm->partialBlockStack);
    ck_assert_ptr_null(vm->partialScopeStack);
    ck_assert_ptr_null(vm->last_context);
    ck_assert_ptr_null(vm->module);
    ck_assert_ptr_null(vm->buffer);

    clear_intentional_error();
    output = handlebars_vm_execute(vm, succeeding, input);
    ck_assert_ptr_nonnull(output);
    ck_assert_hbs_str_eq_cstr(output, "ok");
    handlebars_string_delref(output);

    HANDLEBARS_VALUE_UNDECL(partials);
    HANDLEBARS_VALUE_UNDECL(recurse_partial);
    HANDLEBARS_VALUE_UNDECL(node);
    HANDLEBARS_VALUE_UNDECL(input);
}
END_TEST

#ifdef HANDLEBARS_MEMORY
static void assert_inline_partial_allocation_failures_unwind_vm(
    const char * tmpl
)
{
    struct handlebars_module * module = test_compile_template(tmpl);
    HANDLEBARS_VALUE_DECL(input);
#ifndef HANDLEBARS_NO_REFCOUNT
    size_t baseline_blocks = talloc_total_blocks(vm);
#endif
    bool reached_success = false;

    for( int fail_at = 1; fail_at <= 128; fail_at++ ) {
        struct handlebars_string * output;

        handlebars_memory_fail_set_flags(handlebars_memory_fail_flag_alloc);
        handlebars_memory_fail_counter(fail_at);
        output = handlebars_vm_execute(vm, module, input);
        handlebars_memory_fail_disable();

        if( output != NULL ) {
            ck_assert_hbs_str_eq_cstr(output, "success");
            handlebars_string_delref(output);
            reached_success = true;
            break;
        }

        ck_assert_msg(
            handlebars_error_num(context) == HANDLEBARS_NOMEM,
            "allocation %d failed with %d (%s), expected HANDLEBARS_NOMEM",
            fail_at,
            handlebars_error_num(context),
            handlebars_error_msg(context)
        );
        ck_assert_ptr_null(vm->stack);
        ck_assert_ptr_null(vm->contextStack);
        ck_assert_ptr_null(vm->hashStack);
        ck_assert_ptr_null(vm->blockParamStack);
        ck_assert_ptr_null(vm->partialBlockStack);
        ck_assert_ptr_null(vm->partialScopeStack);
#ifndef HANDLEBARS_NO_REFCOUNT
        ck_assert_msg(
            talloc_total_blocks(vm) == baseline_blocks,
            "allocation %d retained %zu VM blocks (baseline %zu)",
            fail_at,
            talloc_total_blocks(vm),
            baseline_blocks
        );
#endif

        output = handlebars_vm_execute(vm, module, input);
        ck_assert_msg(output != NULL, "%s", handlebars_error_msg(context));
        ck_assert_hbs_str_eq_cstr(output, "success");
        handlebars_string_delref(output);
#ifndef HANDLEBARS_NO_REFCOUNT
        ck_assert_uint_eq(talloc_total_blocks(vm), baseline_blocks);
#endif
    }

    ck_assert(reached_success);
    HANDLEBARS_VALUE_UNDECL(input);
}

START_TEST(test_inline_partial_allocation_failures_unwind_vm)
{
    assert_inline_partial_allocation_failures_unwind_vm(
        "{{#*inline \"myPartial\"}}success{{/inline}}{{> myPartial}}"
    );
}
END_TEST

START_TEST(test_inline_partial_scalar_name_allocation_failures_unwind_vm)
{
    assert_inline_partial_allocation_failures_unwind_vm(
        "{{#*inline 123}}success{{/inline}}{{> 123}}"
    );
}
END_TEST

START_TEST(test_inline_partial_block_allocation_failures_unwind_vm)
{
    assert_inline_partial_allocation_failures_unwind_vm(
        "{{#*inline \"layout\"}}{{> @partial-block}}{{/inline}}"
        "{{#> layout}}success{{/layout}}"
    );
}
END_TEST

START_TEST(test_inline_partial_scope_allocation_failures_unwind_vm)
{
    assert_inline_partial_allocation_failures_unwind_vm(
        "{{#*inline \"first\"}}unused{{/inline}}"
        "{{#*inline \"second\" unused=1}}success{{/inline}}"
        "{{> second}}"
    );
}
END_TEST
#endif

START_TEST(test_subexpression_rejects_non_callable_context_value)
{
    struct handlebars_module * module = test_compile_template("{{foo (bar)}}");
    HANDLEBARS_VALUE_DECL(bar);
#ifndef HANDLEBARS_NO_REFCOUNT
    size_t blocks_after_first_error;
#endif

    test_register_helper(HBS_STRL("foo"), test_passthrough_helper);
    handlebars_value_str(bar, handlebars_string_ctor(context, HBS_STRL("not callable")));

    struct handlebars_string * output = test_execute_with_bar(module, bar);
    ck_assert_ptr_null(output);
    ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_ERROR);
    ck_assert_ptr_nonnull(strstr(handlebars_error_msg(context), "bar"));
#ifndef HANDLEBARS_NO_REFCOUNT
    blocks_after_first_error = talloc_total_blocks(context);

    output = test_execute_with_bar(module, bar);
    ck_assert_ptr_null(output);
    ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_ERROR);
    ck_assert_uint_eq(talloc_total_blocks(context), blocks_after_first_error);
#endif

    HANDLEBARS_VALUE_UNDECL(bar);
}
END_TEST

START_TEST(test_subexpression_allows_falsey_context_values)
{
    struct handlebars_module * module = test_compile_template("{{foo (bar)}}");
    HANDLEBARS_VALUE_DECL(bar);
    struct handlebars_string * output;

    test_register_helper(HBS_STRL("foo"), test_passthrough_helper);

    handlebars_value_boolean(bar, false);
    output = test_execute_with_bar(module, bar);
    ck_assert_ptr_nonnull(output);
    ck_assert_hbs_str_eq_cstr(output, "");
    handlebars_string_delref(output);

    handlebars_value_integer(bar, 0);
    output = test_execute_with_bar(module, bar);
    ck_assert_ptr_nonnull(output);
    ck_assert_hbs_str_eq_cstr(output, "");
    handlebars_string_delref(output);

    handlebars_value_float(bar, 0.0);
    output = test_execute_with_bar(module, bar);
    ck_assert_ptr_nonnull(output);
    ck_assert_hbs_str_eq_cstr(output, "");
    handlebars_string_delref(output);

    handlebars_value_str(bar, handlebars_string_ctor(context, HBS_STRL("")));
    output = test_execute_with_bar(module, bar);
    ck_assert_ptr_nonnull(output);
    ck_assert_hbs_str_eq_cstr(output, "");
    handlebars_string_delref(output);

    HANDLEBARS_VALUE_UNDECL(bar);
}
END_TEST

START_TEST(test_subexpression_rejects_empty_containers)
{
    struct handlebars_module * module = test_compile_template("{{foo (bar)}}");
    HANDLEBARS_VALUE_DECL(bar);
    struct handlebars_string * output;

    test_register_helper(HBS_STRL("foo"), test_passthrough_helper);

    handlebars_value_map(bar, handlebars_map_ctor(context, 0));
    output = test_execute_with_bar(module, bar);
    ck_assert_ptr_null(output);
    ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_ERROR);

    handlebars_value_dtor(bar);
    handlebars_value_array(bar, handlebars_stack_ctor(context, 0));
    output = test_execute_with_bar(module, bar);
    ck_assert_ptr_null(output);
    ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_ERROR);

    HANDLEBARS_VALUE_UNDECL(bar);
}
END_TEST

START_TEST(test_subexpression_calls_callable_context_value)
{
    struct handlebars_module * module = test_compile_template("{{foo (bar)}}");
    HANDLEBARS_VALUE_DECL(bar);

    test_register_helper(HBS_STRL("foo"), test_passthrough_helper);
    handlebars_value_helper(bar, test_context_helper);

    struct handlebars_string * output = test_execute_with_bar(module, bar);
    ck_assert_ptr_nonnull(output);
    ck_assert_hbs_str_eq_cstr(output, "ok");
    handlebars_string_delref(output);

    HANDLEBARS_VALUE_UNDECL(bar);
}
END_TEST

START_TEST(test_subexpression_allows_missing_context_value)
{
    struct handlebars_module * module = test_compile_template("{{foo (bar)}}");
    HANDLEBARS_VALUE_DECL(input);

    test_register_helper(HBS_STRL("foo"), test_passthrough_helper);

    struct handlebars_string * output = handlebars_vm_execute(vm, module, input);
    ck_assert_ptr_nonnull(output);
    ck_assert_hbs_str_eq_cstr(output, "");
    handlebars_string_delref(output);

    HANDLEBARS_VALUE_UNDECL(input);
}
END_TEST

START_TEST(test_vm_emulates_string_and_array_length_properties)
{
    static const char * template =
        "{{text.length}}|{{items.length}}|"
        "{{emptyText.length}}|{{emptyItems.length}}|"
        "{{#with text as |value|}}{{value.length}}{{/with}}|"
        "{{@root.items.length}}|{{record.length}}";
    struct handlebars_module * module = test_compile_template(template);
    HANDLEBARS_VALUE_DECL(input);
    HANDLEBARS_VALUE_DECL(value);
    struct handlebars_map * input_map = handlebars_map_ctor(context, 5);
    struct handlebars_map * record_map = handlebars_map_ctor(context, 1);
    struct handlebars_stack * items = handlebars_stack_ctor(context, 3);
    struct handlebars_string * output;

    handlebars_value_str(value, handlebars_string_ctor(context, HBS_STRL("four")));
    input_map = handlebars_map_str_update(input_map, HBS_STRL("text"), value);

    for( long i = 1; i <= 3; i++ ) {
        handlebars_value_integer(value, i);
        items = handlebars_stack_push(items, value);
    }
    handlebars_value_array(value, items);
    input_map = handlebars_map_str_update(input_map, HBS_STRL("items"), value);

    handlebars_value_str(value, handlebars_string_ctor(context, HBS_STRL("")));
    input_map = handlebars_map_str_update(input_map, HBS_STRL("emptyText"), value);

    handlebars_value_array(value, handlebars_stack_ctor(context, 0));
    input_map = handlebars_map_str_update(input_map, HBS_STRL("emptyItems"), value);

    handlebars_value_integer(value, 9);
    record_map = handlebars_map_str_update(record_map, HBS_STRL("length"), value);
    handlebars_value_map(value, record_map);
    input_map = handlebars_map_str_update(input_map, HBS_STRL("record"), value);

    handlebars_value_map(input, input_map);
    output = handlebars_vm_execute(vm, module, input);

    ck_assert_ptr_nonnull(output);
    ck_assert_hbs_str_eq_cstr(output, "4|3|0|0|4|3|9");

    handlebars_string_delref(output);
    HANDLEBARS_VALUE_UNDECL(value);
    HANDLEBARS_VALUE_UNDECL(input);
}
END_TEST

START_TEST(test_vm_length_data_path_stays_missing_after_missing_segment)
{
    struct handlebars_module * module = test_compile_template(
        "{{@root.items.missing.length}}"
    );
    HANDLEBARS_VALUE_DECL(input);
    HANDLEBARS_VALUE_DECL(value);
    struct handlebars_map * input_map = handlebars_map_ctor(context, 1);
    struct handlebars_stack * items = handlebars_stack_ctor(context, 1);
    struct handlebars_string * output;

    handlebars_value_integer(value, 1);
    items = handlebars_stack_push(items, value);
    handlebars_value_array(value, items);
    input_map = handlebars_map_str_update(input_map, HBS_STRL("items"), value);
    handlebars_value_map(input, input_map);

    output = handlebars_vm_execute(vm, module, input);

    ck_assert_ptr_nonnull(output);
    ck_assert_hbs_str_eq_cstr(output, "");

    handlebars_string_delref(output);
    HANDLEBARS_VALUE_UNDECL(value);
    HANDLEBARS_VALUE_UNDECL(input);
}
END_TEST

START_TEST(test_vm_emulates_length_for_custom_lazy_arrays)
{
    struct handlebars_module * module = test_compile_template(
        "{{items.length}}|{{items.[0]}}|{{lookup items 0}}|{{empty.length}}"
    );
    HANDLEBARS_VALUE_DECL(input);
    HANDLEBARS_VALUE_DECL(value);
    struct handlebars_map * input_map = handlebars_map_ctor(context, 2);
    struct handlebars_string * output;

    test_value_lazy_array(value, 2);
    input_map = handlebars_map_str_update(input_map, HBS_STRL("items"), value);
    test_value_lazy_array(value, 0);
    input_map = handlebars_map_str_update(input_map, HBS_STRL("empty"), value);
    handlebars_value_map(input, input_map);

    output = handlebars_vm_execute(vm, module, input);

    ck_assert_ptr_nonnull(output);
    ck_assert_hbs_str_eq_cstr(output, "2|10|10|0");

    handlebars_string_delref(output);
    HANDLEBARS_VALUE_UNDECL(value);
    HANDLEBARS_VALUE_UNDECL(input);
}
END_TEST

START_TEST(test_vm_length_block_param_stays_missing_after_missing_segment)
{
    struct handlebars_module * module = test_compile_template(
        "{{#with items as |value|}}{{value.missing.length}}{{/with}}"
    );
    HANDLEBARS_VALUE_DECL(input);
    HANDLEBARS_VALUE_DECL(value);
    struct handlebars_map * input_map = handlebars_map_ctor(context, 1);
    struct handlebars_stack * items = handlebars_stack_ctor(context, 1);
    struct handlebars_string * output;

    handlebars_value_integer(value, 1);
    items = handlebars_stack_push(items, value);
    handlebars_value_array(value, items);
    input_map = handlebars_map_str_update(input_map, HBS_STRL("items"), value);
    handlebars_value_map(input, input_map);

    output = handlebars_vm_execute(vm, module, input);

    ck_assert_ptr_nonnull(output);
    ck_assert_hbs_str_eq_cstr(output, "");

    handlebars_string_delref(output);
    HANDLEBARS_VALUE_UNDECL(value);
    HANDLEBARS_VALUE_UNDECL(input);
}
END_TEST


START_TEST(test_boolean_true)
{
    HANDLEBARS_VALUE_DECL(value);
    handlebars_value_boolean(value, true);
    ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_TRUE);
    ck_assert_int_eq(handlebars_value_get_boolval(value), 1);
    ck_assert(handlebars_value_is_scalar(value));
    HANDLEBARS_VALUE_UNDECL(value);
    ASSERT_INIT_BLOCKS();
}
END_TEST

START_TEST(test_boolean_false)
{
    HANDLEBARS_VALUE_DECL(value);
    handlebars_value_boolean(value, false);
    ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_FALSE);
    ck_assert_int_eq(handlebars_value_get_boolval(value), 0);
    ck_assert(handlebars_value_is_scalar(value));
    HANDLEBARS_VALUE_UNDECL(value);
    ASSERT_INIT_BLOCKS();
}
END_TEST

START_TEST(test_int)
{
    HANDLEBARS_VALUE_DECL(value);
    handlebars_value_integer(value, 2358);
    ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_INTEGER);
    ck_assert_int_eq(handlebars_value_get_intval(value), 2358);
    ck_assert(handlebars_value_is_scalar(value));
    HANDLEBARS_VALUE_UNDECL(value);
    ASSERT_INIT_BLOCKS();
}
END_TEST

START_TEST(test_float)
{
    HANDLEBARS_VALUE_DECL(value);
    handlebars_value_float(value, 1234.4321);
    ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_FLOAT);
    // Note: converting to int - precision issue
    ck_assert_int_eq(handlebars_value_get_floatval(value), 1234.4321);
    ck_assert(handlebars_value_is_scalar(value));
    HANDLEBARS_VALUE_UNDECL(value);
    ASSERT_INIT_BLOCKS();
}
END_TEST

START_TEST(test_string)
{
    HANDLEBARS_VALUE_DECL(value);
    handlebars_value_str(value, handlebars_string_ctor(context, HBS_STRL("test")));
	ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_STRING);
    const char * tmp = handlebars_value_get_strval(value);
	ck_assert_str_eq(tmp, "test");
	ck_assert_int_eq(handlebars_value_get_strlen(value), 4);
    ck_assert(handlebars_value_is_scalar(value));
    HANDLEBARS_VALUE_UNDECL(value);
    ASSERT_INIT_BLOCKS();
}
END_TEST

START_TEST(test_value_getter_defaults)
{
    HANDLEBARS_VALUE_DECL(value);

    ck_assert_ptr_null(handlebars_value_get_map(value));
    ck_assert_ptr_null(handlebars_value_get_stack(value));
    ck_assert_ptr_null(handlebars_value_get_string(value));
    ck_assert_ptr_null(handlebars_value_get_user(value));
    ck_assert_ptr_null(handlebars_value_get_closure(value));
    ck_assert_ptr_null(handlebars_value_get_strval(value));
    ck_assert_uint_eq(handlebars_value_get_strlen(value), 0);
    ck_assert_int_eq(handlebars_value_get_intval(value), 0);
    ck_assert_double_eq(handlebars_value_get_floatval(value), 0);
    ck_assert_int_eq(handlebars_value_get_flags(value), 0);

    HANDLEBARS_VALUE_UNDECL(value);
    ASSERT_INIT_BLOCKS();
}
END_TEST

START_TEST(test_checked_pointer_retrieval)
{
    HANDLEBARS_VALUE_DECL(value);
    struct handlebars_ptr * ptr;
    struct handlebars_user * user;
    char matching_type[] = "int";
    int payload = 42;
    int sentinel = 0;
    void * result = &sentinel;

    ptr = handlebars_ptr_ctor(context, int, &payload, true);
    ck_assert(handlebars_ptr_try_get(ptr, matching_type, &result));
    ck_assert_ptr_eq(result, &payload);
    ck_assert_ptr_eq(handlebars_ptr_get_ptr(ptr, int), &payload);

    result = &sentinel;
    ck_assert(!handlebars_ptr_try_get(ptr, "long", &result));
    ck_assert_ptr_null(result);

    handlebars_value_ptr(value, ptr);
    ck_assert(handlebars_value_ptr_try_get(value, matching_type, &result));
    ck_assert_ptr_eq(result, &payload);
    ck_assert_ptr_eq(handlebars_value_get_ptr(value, int), &payload);

    result = &sentinel;
    ck_assert(!handlebars_value_ptr_try_get(value, "long", &result));
    ck_assert_ptr_null(result);

    handlebars_value_integer(value, 7);
    result = &sentinel;
    ck_assert(!handlebars_value_ptr_try_get(value, matching_type, &result));
    ck_assert_ptr_null(result);

    user = handlebars_talloc_zero(context, struct handlebars_user);
    ck_assert_ptr_nonnull(user);
    handlebars_user_init(user, context, &test_type_only_ptr_handlers);
    handlebars_value_user(value, user);
    ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_PTR);
    ck_assert_int_eq(handlebars_value_get_real_type(value), HANDLEBARS_VALUE_TYPE_USER);
    result = &sentinel;
    ck_assert(!handlebars_value_ptr_try_get(value, matching_type, &result));
    ck_assert_ptr_null(result);

    HANDLEBARS_VALUE_UNDECL(value);
    ASSERT_INIT_BLOCKS();
}
END_TEST

#ifndef HANDLEBARS_NO_REFCOUNT
START_TEST(test_checked_pointer_retrieval_is_borrowed)
{
    HANDLEBARS_VALUE_DECL(value);
    struct test_checked_pointer_payload * payload;
    struct handlebars_ptr * ptr;
    int destructions = 0;
    void * result = NULL;

    payload = handlebars_talloc_zero(context, struct test_checked_pointer_payload);
    payload->destructions = &destructions;
    talloc_set_destructor(payload, test_checked_pointer_payload_dtor);
    ptr = handlebars_ptr_ctor_ex(context, "test-payload", payload, false);

    ck_assert(handlebars_ptr_try_get(ptr, "test-payload", &result));
    ck_assert_ptr_eq(result, payload);
    ck_assert_ptr_eq(
        handlebars_ptr_get_ptr_ex(ptr, "test-payload"),
        payload
    );
    ck_assert_int_eq(destructions, 0);
    handlebars_ptr_delref(ptr);
    ck_assert_int_eq(destructions, 1);

    payload = handlebars_talloc_zero(context, struct test_checked_pointer_payload);
    payload->destructions = &destructions;
    talloc_set_destructor(payload, test_checked_pointer_payload_dtor);
    ptr = handlebars_ptr_ctor_ex(context, "test-payload", payload, false);
    handlebars_value_ptr(value, ptr);

    ck_assert(handlebars_value_ptr_try_get(value, "test-payload", &result));
    ck_assert_ptr_eq(result, payload);
    ck_assert_ptr_eq(
        handlebars_value_get_ptr_ex(value, "test-payload"),
        payload
    );
    ck_assert_int_eq(destructions, 1);
    HANDLEBARS_VALUE_UNDECL(value);
    ck_assert_int_eq(destructions, 2);
    ASSERT_INIT_BLOCKS();
}
END_TEST
#endif

#if !IS_WIN
START_TEST(test_pointer_getter_type_mismatch_aborts)
{
    int payload = 42;
    struct handlebars_ptr * ptr;
    void * unexpected_result;
    int status;
    pid_t pid;

    ptr = handlebars_ptr_ctor(context, int, &payload, true);
    pid = fork();
    ck_assert_int_ne(pid, -1);
    if( pid == 0 ) {
        unexpected_result = handlebars_ptr_get_ptr_ex(ptr, "long");
        (void) unexpected_result;
        _exit(EXIT_FAILURE);
    }

    ck_assert_int_eq(waitpid(pid, &status, 0), pid);
    ck_assert(WIFSIGNALED(status));
    ck_assert_int_eq(WTERMSIG(status), SIGABRT);
}
END_TEST

START_TEST(test_value_pointer_getter_non_pointer_aborts)
{
    HANDLEBARS_VALUE_DECL(value);
    void * unexpected_result;
    int status;
    pid_t pid;

    handlebars_value_integer(value, 42);
    pid = fork();
    ck_assert_int_ne(pid, -1);
    if( pid == 0 ) {
        unexpected_result = handlebars_value_get_ptr_ex(value, "int");
        (void) unexpected_result;
        _exit(EXIT_FAILURE);
    }

    ck_assert_int_eq(waitpid(pid, &status, 0), pid);
    ck_assert(WIFSIGNALED(status));
    ck_assert_int_eq(WTERMSIG(status), SIGABRT);
    HANDLEBARS_VALUE_UNDECL(value);
}
END_TEST
#endif

START_TEST(test_value_to_string_and_expression)
{
    HANDLEBARS_VALUE_DECL(value);
    struct handlebars_string * converted;

    assert_value_expression_result(value, false, "");
    /* Non-string conversions return a fresh, unreferenced string. */
    converted = handlebars_value_to_string(value, context);
    ck_assert_hbs_str_eq_cstr(converted, "");
    handlebars_talloc_free(converted);

    handlebars_value_boolean(value, true);
    assert_value_expression_result(value, false, "true");
    converted = handlebars_value_to_string(value, context);
    ck_assert_hbs_str_eq_cstr(converted, "true");
    handlebars_talloc_free(converted);

    handlebars_value_boolean(value, false);
    assert_value_expression_result(value, false, "false");
    converted = handlebars_value_to_string(value, context);
    ck_assert_hbs_str_eq_cstr(converted, "false");
    handlebars_talloc_free(converted);

    handlebars_value_integer(value, -42);
    assert_value_expression_result(value, false, "-42");
    converted = handlebars_value_to_string(value, context);
    ck_assert_hbs_str_eq_cstr(converted, "-42");
    handlebars_talloc_free(converted);

    handlebars_value_float(value, 12.5);
    assert_value_expression_result(value, false, "12.5");
    converted = handlebars_value_to_string(value, context);
    ck_assert_hbs_str_eq_cstr(converted, "12.5");
    handlebars_talloc_free(converted);

    handlebars_value_str(value, handlebars_string_ctor(context, HBS_STRL("<&\"")));
    assert_value_expression_result(value, false, "<&\"");
    assert_value_expression_result(value, true, "&lt;&amp;&quot;");
    /* String conversions retain and return the value's existing string. */
    converted = handlebars_value_to_string(value, context);
    ck_assert_hbs_str_eq_cstr(converted, "<&\"");
    handlebars_string_delref(converted);

    handlebars_value_set_flag(value, HANDLEBARS_VALUE_FLAG_SAFE_STRING);
    assert_value_expression_result(value, true, "<&\"");

    HANDLEBARS_VALUE_UNDECL(value);
    ASSERT_INIT_BLOCKS();
}
END_TEST

START_TEST(test_value_equality)
{
    HANDLEBARS_VALUE_DECL(left);
    HANDLEBARS_VALUE_DECL(right);

    ck_assert(handlebars_value_eq(left, right));
    handlebars_value_boolean(left, true);
    ck_assert(!handlebars_value_eq(left, right));
    handlebars_value_boolean(right, true);
    ck_assert(handlebars_value_eq(left, right));

    handlebars_value_float(left, 1.5);
    handlebars_value_float(right, 1.5);
    ck_assert(handlebars_value_eq(left, right));
    handlebars_value_float(right, 2.5);
    ck_assert(!handlebars_value_eq(left, right));

    handlebars_value_integer(left, 42);
    handlebars_value_integer(right, 42);
    ck_assert(handlebars_value_eq(left, right));
    handlebars_value_integer(right, 43);
    ck_assert(!handlebars_value_eq(left, right));

    handlebars_value_str(left, handlebars_string_ctor(context, HBS_STRL("same")));
    handlebars_value_str(right, handlebars_string_ctor(context, HBS_STRL("same")));
    ck_assert(handlebars_value_eq(left, right));
    handlebars_value_value(right, left);
    ck_assert(handlebars_value_eq(left, right));
    handlebars_value_str(right, handlebars_string_ctor(context, HBS_STRL("different")));
    ck_assert(!handlebars_value_eq(left, right));

    handlebars_value_array(left, handlebars_stack_ctor(context, 0));
    handlebars_value_value(right, left);
    ck_assert(handlebars_value_eq(left, right));
    handlebars_value_array(right, handlebars_stack_ctor(context, 0));
    ck_assert(!handlebars_value_eq(left, right));

    handlebars_value_map(left, handlebars_map_ctor(context, 0));
    handlebars_value_value(right, left);
    ck_assert(handlebars_value_eq(left, right));
    handlebars_value_map(right, handlebars_map_ctor(context, 0));
    ck_assert(!handlebars_value_eq(left, right));

    handlebars_value_helper(left, handlebars_builtin_if);
    handlebars_value_helper(right, handlebars_builtin_if);
    ck_assert(handlebars_value_eq(left, right));
    handlebars_value_helper(right, handlebars_builtin_unless);
    ck_assert(!handlebars_value_eq(left, right));

    HANDLEBARS_VALUE_UNDECL(right);
    HANDLEBARS_VALUE_UNDECL(left);
    ASSERT_INIT_BLOCKS();
}
END_TEST

START_TEST(test_value_self_assignment)
{
    HANDLEBARS_VALUE_DECL(value);

    handlebars_value_str(value, handlebars_string_ctor(context, HBS_STRL("self")));
    handlebars_value_value(value, value);

    ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_STRING);
    ck_assert_hbs_str_eq_cstr(handlebars_value_get_string(value), "self");
    HANDLEBARS_VALUE_UNDECL(value);
    ASSERT_INIT_BLOCKS();
}
END_TEST

START_TEST(test_array_iterator)
{
    HANDLEBARS_VALUE_DECL(value);
    HANDLEBARS_VALUE_DECL(tmp);
    size_t i = 0;

    handlebars_value_array(value, handlebars_stack_ctor(context, 3));

    handlebars_value_integer(tmp, 1);
    handlebars_value_array_push(value, tmp);

    handlebars_value_integer(tmp, 2);
    handlebars_value_array_push(value, tmp);

    handlebars_value_integer(tmp, 3);
    handlebars_value_array_push(value, tmp);

    ck_assert(!handlebars_value_is_scalar(value));

    HANDLEBARS_VALUE_FOREACH_IDX(value, index, child) {
        ck_assert_ptr_ne(child, NULL);
        ck_assert_int_eq(handlebars_value_get_type(child), HANDLEBARS_VALUE_TYPE_INTEGER);
        ck_assert_uint_eq(index, i);
        ck_assert_int_eq((size_t) handlebars_value_get_intval(child), ++i);
    } HANDLEBARS_VALUE_FOREACH_END();

    HANDLEBARS_VALUE_UNDECL(tmp);
    HANDLEBARS_VALUE_UNDECL(value);
    ASSERT_INIT_BLOCKS();
}
END_TEST

START_TEST(test_value_container_mutators_try_succeed)
{
    HANDLEBARS_VALUE_DECL(array);
    HANDLEBARS_VALUE_DECL(map);
    HANDLEBARS_VALUE_DECL(child);
    HANDLEBARS_VALUE_DECL(found);
    struct handlebars_string * key = handlebars_string_ctor(context, HBS_STRL("answer"));

    handlebars_string_addref(key);
    handlebars_value_array(array, handlebars_stack_ctor(context, 1));
    handlebars_value_integer(child, 41);
    ck_assert_int_eq(
        handlebars_value_array_push_try(array, child),
        HANDLEBARS_SUCCESS
    );
    handlebars_value_integer(child, 42);
    ck_assert_int_eq(
        handlebars_value_array_set_try(array, 0, child),
        HANDLEBARS_SUCCESS
    );
    ck_assert_int_eq(
        handlebars_value_get_intval(handlebars_value_array_find(array, 0, found)),
        42
    );

    handlebars_value_map(map, handlebars_map_ctor(context, 0));
    ck_assert_int_eq(
        handlebars_value_map_update_try(map, key, child),
        HANDLEBARS_SUCCESS
    );
    ck_assert_int_eq(
        handlebars_value_get_intval(handlebars_value_map_find(map, key, found)),
        42
    );

    handlebars_string_delref(key);
    HANDLEBARS_VALUE_UNDECL(found);
    HANDLEBARS_VALUE_UNDECL(child);
    HANDLEBARS_VALUE_UNDECL(map);
    HANDLEBARS_VALUE_UNDECL(array);
    ASSERT_INIT_BLOCKS();
}
END_TEST

START_TEST(test_value_container_mutators_try_reject_non_native_values)
{
    HANDLEBARS_VALUE_DECL(value);
    HANDLEBARS_VALUE_DECL(child);
    struct handlebars_string * key = handlebars_string_ctor(context, HBS_STRL("key"));

    handlebars_string_addref(key);
    handlebars_value_integer(child, 42);
    handlebars_value_integer(value, 7);
    ck_assert_int_eq(
        handlebars_value_array_set_try(value, 0, child),
        HANDLEBARS_TYPE_ERROR
    );
    ck_assert_int_eq(
        handlebars_value_array_push_try(value, child),
        HANDLEBARS_TYPE_ERROR
    );
    ck_assert_int_eq(
        handlebars_value_map_update_try(value, key, child),
        HANDLEBARS_TYPE_ERROR
    );
    ck_assert_int_eq(handlebars_value_get_intval(value), 7);

    test_value_lazy_array(value, 2);
    ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_ARRAY);
    ck_assert_int_eq(handlebars_value_get_real_type(value), HANDLEBARS_VALUE_TYPE_USER);
    ck_assert_int_eq(
        handlebars_value_array_push_try(value, child),
        HANDLEBARS_TYPE_ERROR
    );
    ck_assert_int_eq(handlebars_value_count(value), 2);

    test_value_lazy_map(value, true);
    ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_MAP);
    ck_assert_int_eq(handlebars_value_get_real_type(value), HANDLEBARS_VALUE_TYPE_USER);
    ck_assert_int_eq(
        handlebars_value_map_update_try(value, key, child),
        HANDLEBARS_TYPE_ERROR
    );
    ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_SUCCESS);

    handlebars_string_delref(key);
    HANDLEBARS_VALUE_UNDECL(child);
    HANDLEBARS_VALUE_UNDECL(value);
    ASSERT_INIT_BLOCKS();
}
END_TEST

static void test_seed_context_error(
    enum handlebars_error_type type,
    const char * message
) {
    jmp_buf * volatile previous = context->e->jmp;
    jmp_buf buf;

    if( !handlebars_setjmp_ex(context, &buf) ) {
        handlebars_throw(context, type, "%s", message);
    }
    context->e->jmp = previous;
}

START_TEST(test_value_container_mutators_type_error_preserves_diagnostics)
{
    HANDLEBARS_VALUE_DECL(value);
    HANDLEBARS_VALUE_DECL(child);
    struct handlebars_string * key = handlebars_string_ctor(context, HBS_STRL("key"));
    jmp_buf * previous = context->e->jmp;

    handlebars_string_addref(key);
    handlebars_value_integer(child, 42);
    handlebars_value_integer(value, 7);
    test_seed_context_error(HANDLEBARS_PARSEERR, "sentinel diagnostic");

    ck_assert_int_eq(
        handlebars_value_array_set_try(value, 0, child),
        HANDLEBARS_TYPE_ERROR
    );
    ck_assert_int_eq(
        handlebars_value_array_push_try(value, child),
        HANDLEBARS_TYPE_ERROR
    );
    ck_assert_int_eq(
        handlebars_value_map_update_try(value, key, child),
        HANDLEBARS_TYPE_ERROR
    );
    ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_PARSEERR);
    ck_assert_str_eq(handlebars_error_msg(context), "sentinel diagnostic");
    ck_assert_ptr_eq(context->e->jmp, previous);
    ck_assert_int_eq(handlebars_value_get_intval(value), 7);
    handlebars_error_clear(context);

    test_value_lazy_array(value, 2);
    test_seed_context_error(HANDLEBARS_UNKNOWN_HELPER, "lazy array sentinel");
    ck_assert_int_eq(
        handlebars_value_array_push_try(value, child),
        HANDLEBARS_TYPE_ERROR
    );
    ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_UNKNOWN_HELPER);
    ck_assert_str_eq(handlebars_error_msg(context), "lazy array sentinel");
    ck_assert_ptr_eq(context->e->jmp, previous);
    ck_assert_int_eq(handlebars_value_count(value), 2);
    handlebars_error_clear(context);

    test_value_lazy_map(value, true);
    test_seed_context_error(HANDELBARS_EXTERNAL_ERROR, "lazy map sentinel");
    ck_assert_int_eq(
        handlebars_value_map_update_try(value, key, child),
        HANDLEBARS_TYPE_ERROR
    );
    ck_assert_int_eq(handlebars_error_num(context), HANDELBARS_EXTERNAL_ERROR);
    ck_assert_str_eq(handlebars_error_msg(context), "lazy map sentinel");
    ck_assert_ptr_eq(context->e->jmp, previous);
    handlebars_error_clear(context);

    handlebars_string_delref(key);
    HANDLEBARS_VALUE_UNDECL(child);
    HANDLEBARS_VALUE_UNDECL(value);
    ASSERT_INIT_BLOCKS();
}
END_TEST

START_TEST(test_value_array_push_try_preserves_self_alias_during_growth)
{
    HANDLEBARS_VALUE_DECL(value);
    struct handlebars_value * stored;

    handlebars_value_array(value, handlebars_stack_ctor(context, 0));
    ck_assert_int_eq(
        handlebars_value_array_push_try(value, value),
        HANDLEBARS_SUCCESS
    );
    ck_assert_int_eq(handlebars_value_count(value), 1);

    stored = handlebars_stack_get(value->v.stack, 0);
    ck_assert_ptr_nonnull(stored);
    ck_assert_int_eq(
        handlebars_value_get_real_type(stored),
        HANDLEBARS_VALUE_TYPE_ARRAY
    );
    ck_assert_int_eq(handlebars_value_count(stored), 0);

    /* Release the retained pre-growth snapshot without leaving a cycle. */
    handlebars_value_null(stored);
    HANDLEBARS_VALUE_UNDECL(value);
    ASSERT_INIT_BLOCKS();
}
END_TEST

START_TEST(test_value_map_update_try_preserves_self_alias_during_growth)
{
    HANDLEBARS_VALUE_DECL(value);
    struct handlebars_string * key = handlebars_string_ctor(context, HBS_STRL("self"));
    struct handlebars_value * stored;

    handlebars_string_addref(key);
    handlebars_value_map(value, handlebars_map_ctor(context, 0));
    ck_assert_int_eq(
        handlebars_value_map_update_try(value, key, value),
        HANDLEBARS_SUCCESS
    );
    ck_assert_int_eq(handlebars_value_count(value), 1);

    stored = handlebars_map_find(value->v.map, key);
    ck_assert_ptr_nonnull(stored);
    ck_assert_int_eq(
        handlebars_value_get_real_type(stored),
        HANDLEBARS_VALUE_TYPE_MAP
    );
    ck_assert_int_eq(handlebars_value_count(stored), 0);

    handlebars_value_null(stored);
    handlebars_string_delref(key);
    HANDLEBARS_VALUE_UNDECL(value);
    ASSERT_INIT_BLOCKS();
}
END_TEST

#ifndef HANDLEBARS_NO_REFCOUNT
START_TEST(test_value_container_mutators_try_preserve_copy_on_write_aliases)
{
    HANDLEBARS_VALUE_DECL(array);
    HANDLEBARS_VALUE_DECL(array_alias);
    HANDLEBARS_VALUE_DECL(map);
    HANDLEBARS_VALUE_DECL(map_alias);
    HANDLEBARS_VALUE_DECL(found);
    HANDLEBARS_VALUE_DECL(tmp);
    struct handlebars_map * backing_map = handlebars_map_ctor(context, 2);
    struct handlebars_string * key;
    struct handlebars_value * source;

    handlebars_value_array(array, handlebars_stack_ctor(context, 2));
    handlebars_value_integer(tmp, 10);
    handlebars_value_array_push(array, tmp);
    handlebars_value_value(array_alias, array);
    source = handlebars_stack_get(array->v.stack, 0);
    ck_assert_ptr_nonnull(source);

    ck_assert_int_eq(
        handlebars_value_array_push_try(array, source),
        HANDLEBARS_SUCCESS
    );
    ck_assert_ptr_ne(array->v.stack, array_alias->v.stack);
    ck_assert_int_eq(handlebars_value_count(array), 2);
    ck_assert_int_eq(handlebars_value_count(array_alias), 1);
    ck_assert_int_eq(
        handlebars_value_get_intval(handlebars_value_array_find(array, 1, found)),
        10
    );
    ck_assert_ptr_null(handlebars_value_array_find(array_alias, 1, found));

    handlebars_value_integer(tmp, 1);
    backing_map = handlebars_map_str_update(backing_map, HBS_STRL("a"), tmp);
    handlebars_value_integer(tmp, 2);
    backing_map = handlebars_map_str_update(backing_map, HBS_STRL("b"), tmp);
    handlebars_value_map(map, backing_map);
    handlebars_value_value(map_alias, map);
    key = handlebars_map_get_key_at_index(map->v.map, 0);
    source = handlebars_map_str_find(map->v.map, HBS_STRL("b"));
    ck_assert_ptr_nonnull(key);
    ck_assert_ptr_nonnull(source);

    ck_assert_int_eq(
        handlebars_value_map_update_try(map, key, source),
        HANDLEBARS_SUCCESS
    );
    ck_assert_ptr_ne(map->v.map, map_alias->v.map);
    ck_assert_int_eq(
        handlebars_value_get_intval(
            handlebars_value_map_str_find(map, HBS_STRL("a"), found)
        ),
        2
    );
    ck_assert_int_eq(
        handlebars_value_get_intval(
            handlebars_value_map_str_find(map_alias, HBS_STRL("a"), found)
        ),
        1
    );

    HANDLEBARS_VALUE_UNDECL(tmp);
    HANDLEBARS_VALUE_UNDECL(found);
    HANDLEBARS_VALUE_UNDECL(map_alias);
    HANDLEBARS_VALUE_UNDECL(map);
    HANDLEBARS_VALUE_UNDECL(array_alias);
    HANDLEBARS_VALUE_UNDECL(array);
    ASSERT_INIT_BLOCKS();
}
END_TEST
#endif

START_TEST(test_value_array_push_try_stack_overflow_preserves_outer_boundary)
{
    HANDLEBARS_VALUE_DECL(iterable);
    HANDLEBARS_VALUE_DECL(target);
    HANDLEBARS_VALUE_DECL(child);
    HANDLEBARS_VALUE_ITERATOR_DECL(iter);
    struct handlebars_stack * stack = handlebars_stack_alloca(context, 1);
    jmp_buf * volatile previous = context->e->jmp;
    jmp_buf outer;

    handlebars_value_integer(child, 1);
    handlebars_value_array(iterable, handlebars_stack_ctor(context, 2));
    handlebars_value_array_push(iterable, child);
    handlebars_value_integer(child, 2);
    handlebars_value_array_push(iterable, child);

    handlebars_value_array(target, stack);
    handlebars_value_integer(child, 7);
    ck_assert_int_eq(
        handlebars_value_array_push_try(target, child),
        HANDLEBARS_SUCCESS
    );

    if( handlebars_setjmp_ex(context, &outer) ) {
        context->e->jmp = previous;
        ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_ERROR);
        ck_assert_str_eq(handlebars_error_msg(context), "outer boundary remains active");
        handlebars_value_iterator_close(iter);
        handlebars_error_clear(context);
        handlebars_stack_dtor(stack);
        handlebars_value_init(target);
        HANDLEBARS_VALUE_UNDECL(child);
        HANDLEBARS_VALUE_UNDECL(target);
        HANDLEBARS_VALUE_UNDECL(iterable);
        ASSERT_INIT_BLOCKS();
        return;
    }

    ck_assert(HANDLEBARS_VALUE_ITERATOR_INIT(iter, iterable));
    handlebars_value_integer(child, 8);
    ck_assert_int_eq(
        handlebars_value_array_push_try(target, child),
        HANDLEBARS_STACK_OVERFLOW
    );
    ck_assert_ptr_eq(context->e->jmp, &outer);
    ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_STACK_OVERFLOW);
    ck_assert_ptr_nonnull(strstr(handlebars_error_msg(context), "Stack overflow"));
    ck_assert_int_eq(handlebars_value_count(target), 1);
    ck_assert_int_eq(handlebars_value_get_intval(handlebars_stack_get(stack, 0)), 7);
    ck_assert_int_eq(handlebars_value_get_intval(iter->cur), 1);
    ck_assert(handlebars_value_iterator_next(iter));
    ck_assert_int_eq(handlebars_value_get_intval(iter->cur), 2);

    handlebars_throw(context, HANDLEBARS_ERROR, "outer boundary remains active");
    ck_abort_msg("Expected the restored outer boundary to catch the error");
}
END_TEST

START_TEST(test_value_array_set_try_catches_bounds_errors)
{
    HANDLEBARS_VALUE_DECL(value);
    HANDLEBARS_VALUE_DECL(child);
    HANDLEBARS_VALUE_DECL(found);
    jmp_buf * previous = context->e->jmp;

    handlebars_value_array(value, handlebars_stack_ctor(context, 1));
    handlebars_value_integer(child, 1);
    ck_assert_int_eq(
        handlebars_value_array_push_try(value, child),
        HANDLEBARS_SUCCESS
    );

    handlebars_value_integer(child, 2);
    ck_assert_int_eq(
        handlebars_value_array_set_try(value, 2, child),
        HANDLEBARS_STACK_OVERFLOW
    );
    ck_assert_ptr_eq(context->e->jmp, previous);
    ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_STACK_OVERFLOW);
    ck_assert_int_eq(handlebars_value_count(value), 1);
    ck_assert_int_eq(
        handlebars_value_get_intval(handlebars_value_array_find(value, 0, found)),
        1
    );
    handlebars_error_clear(context);

    HANDLEBARS_VALUE_UNDECL(found);
    HANDLEBARS_VALUE_UNDECL(child);
    HANDLEBARS_VALUE_UNDECL(value);
    ASSERT_INIT_BLOCKS();
}
END_TEST

#ifdef HANDLEBARS_MEMORY
START_TEST(test_value_array_push_try_catches_allocation_failure)
{
    HANDLEBARS_VALUE_DECL(value);
    HANDLEBARS_VALUE_DECL(child);
    HANDLEBARS_VALUE_DECL(found);

    handlebars_value_array(value, handlebars_stack_ctor(context, 1));
    handlebars_value_integer(child, 1);
    ck_assert_int_eq(
        handlebars_value_array_push_try(value, child),
        HANDLEBARS_SUCCESS
    );

    handlebars_value_integer(child, 2);
    handlebars_memory_fail_set_flags(handlebars_memory_fail_flag_alloc);
    handlebars_memory_fail_enable();
    ck_assert_int_eq(
        handlebars_value_array_push_try(value, child),
        HANDLEBARS_NOMEM
    );
    handlebars_memory_fail_disable();
    ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_NOMEM);
    ck_assert_int_eq(handlebars_value_count(value), 1);
    ck_assert_int_eq(
        handlebars_value_get_intval(handlebars_value_array_find(value, 0, found)),
        1
    );
    handlebars_error_clear(context);

    ck_assert_int_eq(
        handlebars_value_array_push_try(value, child),
        HANDLEBARS_SUCCESS
    );
    ck_assert_int_eq(handlebars_value_count(value), 2);

    HANDLEBARS_VALUE_UNDECL(found);
    HANDLEBARS_VALUE_UNDECL(child);
    HANDLEBARS_VALUE_UNDECL(value);
    ASSERT_INIT_BLOCKS();
}
END_TEST

START_TEST(test_value_map_update_try_catches_allocation_failure)
{
    HANDLEBARS_VALUE_DECL(value);
    HANDLEBARS_VALUE_DECL(child);
    HANDLEBARS_VALUE_DECL(found);
    struct handlebars_map * map = handlebars_map_ctor(context, 4);
    struct handlebars_string * key = handlebars_string_ctor(context, HBS_STRL("e"));
    jmp_buf * previous = context->e->jmp;
    size_t blocks_before;
    int fail_at;

    handlebars_string_addref(key);
    for( long i = 0; i < 4; i++ ) {
        char name[2] = {(char) ('a' + i), '\0'};
        handlebars_value_integer(child, i + 1);
        map = handlebars_map_str_update(map, name, 1, child);
    }
    handlebars_value_map(value, map);
    handlebars_value_integer(child, 5);
    blocks_before = talloc_total_blocks(context);

    for( fail_at = 1; fail_at < 32; fail_at++ ) {
        enum handlebars_error_type error;

        handlebars_memory_fail_set_flags(handlebars_memory_fail_flag_alloc);
        handlebars_memory_fail_counter(fail_at);
        error = handlebars_value_map_update_try(value, key, child);
        handlebars_memory_fail_disable();

        ck_assert_ptr_eq(context->e->jmp, previous);
        if( error == HANDLEBARS_SUCCESS ) {
            break;
        }

        ck_assert_int_eq(error, HANDLEBARS_NOMEM);
        ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_NOMEM);
        ck_assert_ptr_nonnull(handlebars_error_msg(context));
        ck_assert_int_eq(handlebars_value_count(value), 4);
        ck_assert_int_eq(
            handlebars_value_get_intval(
                handlebars_value_map_str_find(value, HBS_STRL("a"), found)
            ),
            1
        );
        ck_assert_ptr_null(handlebars_value_map_find(value, key, found));
        handlebars_error_clear(context);
        ck_assert_msg(
            talloc_total_blocks(context) == blocks_before,
            "checked map update leaked at allocation %d: before=%zu after=%zu",
            fail_at,
            blocks_before,
            talloc_total_blocks(context)
        );
    }

    ck_assert_int_lt(fail_at, 32);
    ck_assert_int_eq(handlebars_value_count(value), 5);
    ck_assert_int_eq(
        handlebars_value_get_intval(handlebars_value_map_find(value, key, found)),
        5
    );

    handlebars_string_delref(key);
    HANDLEBARS_VALUE_UNDECL(found);
    HANDLEBARS_VALUE_UNDECL(child);
    HANDLEBARS_VALUE_UNDECL(value);
    ASSERT_INIT_BLOCKS();
}
END_TEST
#endif

START_TEST(test_iterator_initializer_respects_explicit_current_storage)
{
    struct guarded_iterator {
        struct handlebars_value_iterator iterator;
        unsigned char guard[sizeof(struct handlebars_value)];
    } guarded = {0};
    HANDLEBARS_VALUE_DECL(value);
    HANDLEBARS_VALUE_DECL(current);
    HANDLEBARS_VALUE_DECL(tmp);

    memset(guarded.guard, 0xa5, sizeof(guarded.guard));
    handlebars_value_array(value, handlebars_stack_ctor(context, 1));
    handlebars_value_integer(tmp, 42);
    handlebars_value_array_push(value, tmp);

    ck_assert(handlebars_value_iterator_init_internal(
        &guarded.iterator,
        current,
        value
    ));
    ck_assert_ptr_eq(guarded.iterator.cur, current);
    ck_assert_int_eq(handlebars_value_get_intval(guarded.iterator.cur), 42);
    for( size_t i = 0; i < sizeof(guarded.guard); i++ ) {
        ck_assert_uint_eq(guarded.guard[i], 0xa5);
    }

    handlebars_value_iterator_close(&guarded.iterator);
    HANDLEBARS_VALUE_UNDECL(tmp);
    HANDLEBARS_VALUE_UNDECL(current);
    HANDLEBARS_VALUE_UNDECL(value);
    ASSERT_INIT_BLOCKS();
}
END_TEST

START_TEST(test_array_iterator_retains_stack)
{
    HANDLEBARS_VALUE_DECL(value);
    HANDLEBARS_VALUE_DECL(tmp);
    HANDLEBARS_VALUE_ITERATOR_DECL(iter);

    handlebars_value_array(value, handlebars_stack_ctor(context, 2));
    handlebars_value_integer(tmp, 1);
    handlebars_value_array_push(value, tmp);
    handlebars_value_integer(tmp, 2);
    handlebars_value_array_push(value, tmp);

    ck_assert(HANDLEBARS_VALUE_ITERATOR_INIT(iter, value));
    handlebars_value_dtor(value);

    ck_assert_int_eq(handlebars_value_get_intval(iter->cur), 1);
    ck_assert(handlebars_value_iterator_next(iter));
    ck_assert_int_eq(handlebars_value_get_intval(iter->cur), 2);
    ck_assert(!handlebars_value_iterator_next(iter));

    HANDLEBARS_VALUE_UNDECL(tmp);
    HANDLEBARS_VALUE_UNDECL(value);
    ASSERT_INIT_BLOCKS();
}
END_TEST

START_TEST(test_map_iterator)
{
    HANDLEBARS_VALUE_DECL(value);
    HANDLEBARS_VALUE_DECL(tmp);
    struct handlebars_map * tmp_map;
    int i = 0;

    tmp_map = handlebars_map_ctor(context, 0); // zero may trigger extra rehashes possibly - good for testing

    handlebars_value_integer(tmp, 1);
    tmp_map = handlebars_map_str_update(tmp_map, HBS_STRL("a"), tmp);

    handlebars_value_integer(tmp, 2);
    tmp_map = handlebars_map_str_update(tmp_map, HBS_STRL("c"), tmp);

    handlebars_value_integer(tmp, 3);
    tmp_map = handlebars_map_str_update(tmp_map, HBS_STRL("b"), tmp);

    handlebars_value_map(value, tmp_map);

    ck_assert(!handlebars_value_is_scalar(value));

    HANDLEBARS_VALUE_FOREACH_KV(value, key, child) {
        ++i;
        ck_assert_ptr_ne(child, NULL);
        ck_assert_int_eq(handlebars_value_get_type(child), HANDLEBARS_VALUE_TYPE_INTEGER);
        ck_assert_ptr_ne(key, NULL);
        switch( i ) {
            case 1: ck_assert_hbs_str_eq_cstr(key, "a"); break;
            case 2: ck_assert_hbs_str_eq_cstr(key, "c"); break;
            case 3: ck_assert_hbs_str_eq_cstr(key, "b"); break;
            default: ck_abort_msg("should never get here"); break; // LCOV_EXCL_LINE
        }
        ck_assert_int_eq(handlebars_value_get_intval(child), i);
    } HANDLEBARS_VALUE_FOREACH_END();

    HANDLEBARS_VALUE_UNDECL(tmp);
    HANDLEBARS_VALUE_UNDECL(value);
    ASSERT_INIT_BLOCKS();
}
END_TEST

START_TEST(test_map_iterator_sparse)
{
    HANDLEBARS_VALUE_DECL(value);
    HANDLEBARS_VALUE_DECL(tmp);
    struct handlebars_map * tmp_map;
    int i = 0;

    tmp_map = handlebars_map_ctor(context, 9);

    handlebars_value_integer(tmp, 1);
    tmp_map = handlebars_map_str_update(tmp_map, HBS_STRL("a"), tmp);

    handlebars_value_integer(tmp, 2);
    tmp_map = handlebars_map_str_update(tmp_map, HBS_STRL("c"), tmp);

    handlebars_value_integer(tmp, 3);
    tmp_map = handlebars_map_str_update(tmp_map, HBS_STRL("b"), tmp);

    tmp_map = handlebars_map_str_remove(tmp_map, HBS_STRL("c"));

    handlebars_value_map(value, tmp_map);

    ck_assert(!handlebars_value_is_scalar(value));

    HANDLEBARS_VALUE_FOREACH_KV(value, key, child) {
        ++i;
        ck_assert_ptr_ne(child, NULL);
        ck_assert_int_eq(handlebars_value_get_type(child), HANDLEBARS_VALUE_TYPE_INTEGER);
        ck_assert_ptr_ne(key, NULL);
        switch( i ) {
            case 1:
                ck_assert_hbs_str_eq_cstr(key, "a");
                ck_assert_int_eq(handlebars_value_get_intval(child), 1);
                break;
            case 2:
                ck_assert_hbs_str_eq_cstr(key, "b");
                ck_assert_int_eq(handlebars_value_get_intval(child), 3);
                break;
            default: ck_abort_msg("should never get here"); break; // LCOV_EXCL_LINE
        }
    } HANDLEBARS_VALUE_FOREACH_END();

    HANDLEBARS_VALUE_UNDECL(tmp);
    HANDLEBARS_VALUE_UNDECL(value);
    ASSERT_INIT_BLOCKS();
}
END_TEST

START_TEST(test_map_iterator_nested)
{
    HANDLEBARS_VALUE_DECL(value);
    HANDLEBARS_VALUE_DECL(tmp);
    struct handlebars_map * map = handlebars_map_ctor(context, 3);
    size_t outer_count = 0;
    size_t inner_count = 0;

    for( long i = 0; i < 3; i++ ) {
        char key[2] = {(char) ('a' + i), '\0'};
        handlebars_value_integer(tmp, i + 1);
        map = handlebars_map_str_update(map, key, 1, tmp);
    }
    handlebars_value_map(value, map);

    HANDLEBARS_VALUE_FOREACH(value, outer) {
        HANDLEBARS_VALUE_ITERATOR_DECL(inner_iter);
        size_t current_inner_count = 0;
        (void) outer;
        outer_count++;

        if( HANDLEBARS_VALUE_ITERATOR_INIT(inner_iter, value) ) {
            do {
                ck_assert_ptr_nonnull(inner_iter->cur);
                current_inner_count++;
                inner_count++;
            } while( handlebars_value_iterator_next(inner_iter) );
        }

        ck_assert_uint_eq(current_inner_count, 3);
    } HANDLEBARS_VALUE_FOREACH_END();

    ck_assert_uint_eq(outer_count, 3);
    ck_assert_uint_eq(inner_count, 9);
    HANDLEBARS_VALUE_UNDECL(tmp);
    HANDLEBARS_VALUE_UNDECL(value);
    ASSERT_INIT_BLOCKS();
}
END_TEST

START_TEST(test_map_iterator_retains_map)
{
    HANDLEBARS_VALUE_DECL(value);
    HANDLEBARS_VALUE_DECL(tmp);
    HANDLEBARS_VALUE_ITERATOR_DECL(iter);
    struct handlebars_map * map = handlebars_map_ctor(context, 2);

    handlebars_value_integer(tmp, 1);
    map = handlebars_map_str_update(map, HBS_STRL("a"), tmp);
    handlebars_value_integer(tmp, 2);
    map = handlebars_map_str_update(map, HBS_STRL("b"), tmp);
    handlebars_value_map(value, map);

    ck_assert(HANDLEBARS_VALUE_ITERATOR_INIT(iter, value));
    handlebars_value_dtor(value);

    ck_assert_hbs_str_eq_cstr(iter->key, "a");
    ck_assert_int_eq(handlebars_value_get_intval(iter->cur), 1);
    ck_assert(handlebars_value_iterator_next(iter));
    ck_assert_hbs_str_eq_cstr(iter->key, "b");
    ck_assert_int_eq(handlebars_value_get_intval(iter->cur), 2);
    ck_assert(!handlebars_value_iterator_next(iter));

    HANDLEBARS_VALUE_UNDECL(tmp);
    HANDLEBARS_VALUE_UNDECL(value);
    ASSERT_INIT_BLOCKS();
}
END_TEST

START_TEST(test_map_iterator_break_releases_snapshot)
{
    HANDLEBARS_VALUE_DECL(value);
    HANDLEBARS_VALUE_DECL(tmp);
    struct handlebars_map * map = handlebars_map_ctor(context, 3);
    struct handlebars_map * original;

    handlebars_value_integer(tmp, 1);
    map = handlebars_map_str_update(map, HBS_STRL("a"), tmp);
    handlebars_value_map(value, map);
    original = handlebars_value_get_map(value);

    HANDLEBARS_VALUE_FOREACH(value, child) {
        (void) child;
        break;
    } HANDLEBARS_VALUE_FOREACH_END();

    value->v.map = handlebars_map_rehash(value->v.map, true);
    ck_assert_ptr_ne(value->v.map, original);
    ck_assert_int_eq(handlebars_value_get_intval(handlebars_map_str_find(value->v.map, HBS_STRL("a"))), 1);

    HANDLEBARS_VALUE_UNDECL(tmp);
    HANDLEBARS_VALUE_UNDECL(value);
    ASSERT_INIT_BLOCKS();
}
END_TEST

START_TEST(test_map_iterator_mutation_uses_snapshot)
{
    HANDLEBARS_VALUE_DECL(value);
    HANDLEBARS_VALUE_DECL(tmp);
    struct handlebars_map * map = handlebars_map_ctor(context, 3);
    long index = 0;

    for( long i = 0; i < 3; i++ ) {
        char key[2] = {(char) ('a' + i), '\0'};
        handlebars_value_integer(tmp, i + 1);
        map = handlebars_map_str_update(map, key, 1, tmp);
    }
    handlebars_value_map(value, map);

    HANDLEBARS_VALUE_FOREACH_KV(value, key, child) {
        index++;
        ck_assert_int_eq(handlebars_value_get_intval(child), index);
        handlebars_value_integer(tmp, index + 10);
        handlebars_value_map_update(value, key, tmp);
    } HANDLEBARS_VALUE_FOREACH_END();

    ck_assert_int_eq(index, 3);
    ck_assert_int_eq(handlebars_value_get_intval(handlebars_map_str_find(value->v.map, HBS_STRL("a"))), 11);
    ck_assert_int_eq(handlebars_value_get_intval(handlebars_map_str_find(value->v.map, HBS_STRL("b"))), 12);
    ck_assert_int_eq(handlebars_value_get_intval(handlebars_map_str_find(value->v.map, HBS_STRL("c"))), 13);

    HANDLEBARS_VALUE_UNDECL(tmp);
    HANDLEBARS_VALUE_UNDECL(value);
    ASSERT_INIT_BLOCKS();
}
END_TEST

#ifndef HANDLEBARS_NO_REFCOUNT
START_TEST(test_map_foreach_removal_preserves_value_owned_map)
{
    struct handlebars_map * map = handlebars_map_ctor(context, 3);
    bool owner_still_points_to_map;
    HANDLEBARS_VALUE_DECL(owner);
    HANDLEBARS_VALUE_DECL(tmp);

    for( long i = 0; i < 3; i++ ) {
        char key[2] = {(char) ('a' + i), '\0'};
        handlebars_value_integer(tmp, i + 1);
        map = handlebars_map_str_update(map, key, 1, tmp);
    }
    handlebars_value_map(owner, map);
    map = handlebars_value_get_map(owner);

    handlebars_map_foreach(map, index, key, child) {
        (void) index;
        (void) child;
        map = handlebars_map_remove(map, key);
        break;
    } handlebars_map_foreach_end(map);

    owner_still_points_to_map = handlebars_value_get_map(owner) == map;
    if( !owner_still_points_to_map ) {
        /* Repair the pre-fix ownership mismatch before fixture cleanup. */
        owner->v.map = map;
    }

    ck_assert_msg(
        owner_still_points_to_map,
        "foreach removal detached the map from its owning value"
    );
    ck_assert_uint_eq(handlebars_map_count(map), 2);

    HANDLEBARS_VALUE_UNDECL(tmp);
    HANDLEBARS_VALUE_UNDECL(owner);
    ASSERT_INIT_BLOCKS();
}
END_TEST

START_TEST(test_map_foreach_removal_preserves_shared_value_owned_map)
{
    struct handlebars_map * map = handlebars_map_ctor(context, 3);
    bool owners_still_point_to_map;
    HANDLEBARS_VALUE_DECL(first_owner);
    HANDLEBARS_VALUE_DECL(second_owner);
    HANDLEBARS_VALUE_DECL(tmp);

    for( long i = 0; i < 3; i++ ) {
        char key[2] = {(char) ('a' + i), '\0'};
        handlebars_value_integer(tmp, i + 1);
        map = handlebars_map_str_update(map, key, 1, tmp);
    }
    handlebars_value_map(first_owner, map);
    handlebars_value_map(second_owner, map);
    map = handlebars_value_get_map(first_owner);

    handlebars_map_foreach(map, entry_index, key, child) {
        (void) entry_index;
        (void) child;
        map = handlebars_map_remove(map, key);
        break;
    } handlebars_map_foreach_end(map);

    owners_still_point_to_map = handlebars_value_get_map(first_owner) == map
        && handlebars_value_get_map(second_owner) == map;
    if( !owners_still_point_to_map ) {
        /* Repair the pre-fix ownership mismatch before fixture cleanup. */
        first_owner->v.map = map;
    }

    ck_assert_msg(
        owners_still_point_to_map,
        "foreach removal detached the map from its shared owning values"
    );
    ck_assert_uint_eq(handlebars_map_count(map), 2);

    HANDLEBARS_VALUE_UNDECL(tmp);
    HANDLEBARS_VALUE_UNDECL(second_owner);
    HANDLEBARS_VALUE_UNDECL(first_owner);
    ASSERT_INIT_BLOCKS();
}
END_TEST
#endif

START_TEST(test_mixed_map_iterators_reject_mutation)
{
    struct handlebars_map_iterator map_iterator = {0};
    struct handlebars_map * map = handlebars_map_ctor(context, 3);
    struct handlebars_string * mutation_key;
    struct handlebars_string * map_key;
    struct handlebars_value * map_child;
    struct handlebars_value * found;
    HANDLEBARS_VALUE_DECL(value);
    HANDLEBARS_VALUE_DECL(tmp);
    HANDLEBARS_VALUE_DECL(result);
    HANDLEBARS_VALUE_ITERATOR_DECL(value_iterator);

    for( long i = 0; i < 3; i++ ) {
        char key[2] = {(char) ('a' + i), '\0'};
        handlebars_value_integer(tmp, i + 1);
        map = handlebars_map_str_update(map, key, 1, tmp);
    }
    handlebars_value_map(value, map);

    ck_assert(HANDLEBARS_VALUE_ITERATOR_INIT(value_iterator, value));
    ck_assert_hbs_str_eq_cstr(value_iterator->key, "a");
    ck_assert(handlebars_map_iterator_init(&map_iterator, value->v.map));

    handlebars_value_integer(tmp, 99);
    mutation_key = handlebars_string_ctor(context, HBS_STRL("b"));
    handlebars_string_addref(mutation_key);
    ck_assert_int_eq(
        handlebars_value_map_update_try(value, mutation_key, tmp),
        HANDLEBARS_ERROR
    );
    handlebars_string_delref(mutation_key);
    ck_assert_ptr_nonnull(strstr(
        handlebars_error_msg(context),
        "direct and value iterators are both active"
    ));

    found = handlebars_value_map_str_find(value, HBS_STRL("b"), result);
    ck_assert_ptr_nonnull(found);
    ck_assert_int_eq(handlebars_value_get_intval(found), 2);

    ck_assert(handlebars_value_iterator_next(value_iterator));
    ck_assert_hbs_str_eq_cstr(value_iterator->key, "b");
    ck_assert_int_eq(handlebars_value_get_intval(value_iterator->cur), 2);

    handlebars_value_iterator_close(value_iterator);
    ck_assert(handlebars_map_iterator_next(&map_iterator, &map_key, &map_child));
    ck_assert_hbs_str_eq_cstr(map_key, "a");
    ck_assert_int_eq(handlebars_value_get_intval(map_child), 1);
    handlebars_map_iterator_close(&map_iterator);
    clear_intentional_error();
    HANDLEBARS_VALUE_UNDECL(result);
    HANDLEBARS_VALUE_UNDECL(tmp);
    HANDLEBARS_VALUE_UNDECL(value);
    ASSERT_INIT_BLOCKS();
}
END_TEST

START_TEST(test_map_iterator_longjmp_releases_snapshot)
{
    HANDLEBARS_VALUE_DECL(value);
    HANDLEBARS_VALUE_DECL(tmp);
    struct handlebars_map * map = handlebars_map_ctor(context, 9);
    struct handlebars_map * original;
    jmp_buf * volatile previous = context->e->jmp;
    jmp_buf buf;

    handlebars_value_integer(tmp, 1);
    map = handlebars_map_str_update(map, HBS_STRL("a"), tmp);
    handlebars_value_map(value, map);
    original = value->v.map;

    if( handlebars_setjmp_ex(context, &buf) ) {
        context->e->jmp = previous;
#ifdef HANDLEBARS_NO_REFCOUNT
        value->v.map = handlebars_map_rehash(value->v.map, true);
        ck_assert_ptr_ne(value->v.map, original);
#else
        value->v.map = handlebars_map_rehash(value->v.map, false);
        ck_assert_ptr_eq(value->v.map, original);
#endif
        clear_intentional_error();
        HANDLEBARS_VALUE_UNDECL(tmp);
        HANDLEBARS_VALUE_UNDECL(value);
        ASSERT_INIT_BLOCKS();
        return;
    }

    HANDLEBARS_VALUE_FOREACH(value, child) {
        (void) child;
        handlebars_throw(context, HANDLEBARS_ERROR, "Intentional iterator failure");
    } HANDLEBARS_VALUE_FOREACH_END();
    ck_abort_msg("Expected iteration to throw");
}
END_TEST

START_TEST(test_array_iterator_longjmp_releases_snapshot)
{
    HANDLEBARS_VALUE_DECL(value);
    HANDLEBARS_VALUE_DECL(tmp);
    struct handlebars_stack * original;
    jmp_buf * volatile previous = context->e->jmp;
    jmp_buf buf;

    handlebars_value_array(value, handlebars_stack_ctor(context, 4));
    handlebars_value_integer(tmp, 1);
    handlebars_value_array_push(value, tmp);
    original = value->v.stack;

    if( handlebars_setjmp_ex(context, &buf) ) {
        context->e->jmp = previous;
        handlebars_value_integer(tmp, 2);
        handlebars_value_array_push(value, tmp);
        ck_assert_ptr_eq(value->v.stack, original);
        clear_intentional_error();
        HANDLEBARS_VALUE_UNDECL(tmp);
        HANDLEBARS_VALUE_UNDECL(value);
        ASSERT_INIT_BLOCKS();
        return;
    }

    HANDLEBARS_VALUE_FOREACH(value, child) {
        (void) child;
        handlebars_throw(context, HANDLEBARS_ERROR, "Intentional iterator failure");
    } HANDLEBARS_VALUE_FOREACH_END();
    ck_abort_msg("Expected iteration to throw");
}
END_TEST

START_TEST(test_nested_error_boundary_preserves_outer_iterator)
{
    HANDLEBARS_VALUE_DECL(value);
    HANDLEBARS_VALUE_DECL(tmp);
    HANDLEBARS_VALUE_ITERATOR_DECL(iter);
    jmp_buf * volatile previous = context->e->jmp;
    jmp_buf outer;
    jmp_buf inner;

    handlebars_value_array(value, handlebars_stack_ctor(context, 2));
    handlebars_value_integer(tmp, 1);
    handlebars_value_array_push(value, tmp);
    handlebars_value_integer(tmp, 2);
    handlebars_value_array_push(value, tmp);

    if( handlebars_setjmp_ex(context, &outer) ) {
        context->e->jmp = previous;
        ck_abort_msg("The inner error escaped its boundary");
    }
    ck_assert(HANDLEBARS_VALUE_ITERATOR_INIT(iter, value));

    if( handlebars_setjmp_ex(context, &inner) ) {
        context->e->jmp = &outer;
        ck_assert_int_eq(handlebars_value_get_intval(iter->cur), 1);
        ck_assert(handlebars_value_iterator_next(iter));
        ck_assert_int_eq(handlebars_value_get_intval(iter->cur), 2);
        handlebars_value_iterator_close(iter);
        context->e->jmp = previous;
        clear_intentional_error();
        HANDLEBARS_VALUE_UNDECL(tmp);
        HANDLEBARS_VALUE_UNDECL(value);
        ASSERT_INIT_BLOCKS();
        return;
    }

    handlebars_throw(context, HANDLEBARS_ERROR, "Intentional nested failure");
}
END_TEST

#ifndef HANDLEBARS_NO_REFCOUNT
START_TEST(test_user_iterator_longjmp_releases_owner)
{
    HANDLEBARS_VALUE_DECL(value);
    HANDLEBARS_VALUE_ITERATOR_DECL(iter);
    struct handlebars_user * user = handlebars_talloc_zero(
        context,
        struct handlebars_user
    );
    jmp_buf * volatile previous = context->e->jmp;
    jmp_buf buf;

    throwing_iterator_user_dtors = 0;
    ck_assert_ptr_nonnull(user);
    handlebars_user_init(user, context, &throwing_iterator_user_handlers);
    handlebars_value_user(value, user);

    if( handlebars_setjmp_ex(context, &buf) ) {
        context->e->jmp = previous;
        handlebars_value_dtor(value);
        ck_assert_int_eq(throwing_iterator_user_dtors, 1);
        clear_intentional_error();
        HANDLEBARS_VALUE_UNDECL(value);
        ASSERT_INIT_BLOCKS();
        return;
    }

    ck_assert(HANDLEBARS_VALUE_ITERATOR_INIT(iter, value));
    (void) handlebars_value_iterator_next(iter);
    ck_abort_msg("Expected user iterator to throw");
}
END_TEST
#endif

#ifdef HANDLEBARS_NO_REFCOUNT
START_TEST(test_map_iterator_no_refcount_guard_is_nested)
{
    HANDLEBARS_VALUE_DECL(value);
    HANDLEBARS_VALUE_DECL(tmp);
    HANDLEBARS_VALUE_ITERATOR_DECL(first);
    HANDLEBARS_VALUE_ITERATOR_DECL(second);
    struct handlebars_map * map = handlebars_map_ctor(context, 1);
    struct handlebars_map * original;

    handlebars_value_integer(tmp, 1);
    map = handlebars_map_str_update(map, HBS_STRL("a"), tmp);
    handlebars_value_map(value, map);
    original = value->v.map;

    ck_assert(HANDLEBARS_VALUE_ITERATOR_INIT(first, value));
    ck_assert(HANDLEBARS_VALUE_ITERATOR_INIT(second, value));

    value->v.map = handlebars_map_rehash(value->v.map, true);
    ck_assert_ptr_eq(value->v.map, original);

    handlebars_value_iterator_close(first);
    value->v.map = handlebars_map_rehash(value->v.map, true);
    ck_assert_ptr_eq(value->v.map, original);

    handlebars_value_iterator_close(second);
    value->v.map = handlebars_map_rehash(value->v.map, true);
    ck_assert_ptr_ne(value->v.map, original);

    HANDLEBARS_VALUE_UNDECL(tmp);
    HANDLEBARS_VALUE_UNDECL(value);
    ASSERT_INIT_BLOCKS();
}
END_TEST
#endif

START_TEST(test_array_find)
{
    HANDLEBARS_VALUE_DECL(value);
	HANDLEBARS_VALUE_DECL(rv);
    struct handlebars_value * value2;

    handlebars_value_array(value, handlebars_stack_ctor(context, 2));
    do {
        struct handlebars_string * tmp_str;
        HANDLEBARS_VALUE_DECL(tmp);

        handlebars_value_integer(tmp, 2358);
        handlebars_value_array_push(value, tmp);

        tmp_str = handlebars_string_ctor(context, HBS_STRL("test"));
        handlebars_value_str(tmp, tmp_str);
        handlebars_value_array_push(value, tmp);
        HANDLEBARS_VALUE_UNDECL(tmp);
    } while(0);

	ck_assert_ptr_ne(value, NULL);
	ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_ARRAY);

	value2 = handlebars_value_array_find(value, 0, rv);
	ck_assert_ptr_ne(value2, NULL);
	ck_assert_int_eq(handlebars_value_get_type(value2), HANDLEBARS_VALUE_TYPE_INTEGER);
	ck_assert_int_eq(handlebars_value_get_intval(value2), 2358);

	value2 = handlebars_value_array_find(value, 1, rv);
	ck_assert_ptr_ne(value2, NULL);
	ck_assert_int_eq(handlebars_value_get_type(value2), HANDLEBARS_VALUE_TYPE_STRING);
    const char * tmp = handlebars_value_get_strval(value2);
	ck_assert_str_eq(tmp, "test");
	ck_assert_int_eq(handlebars_value_get_strlen(value2), 4);

	value2 = handlebars_value_array_find(value, 2, rv);
	ck_assert_ptr_eq(value2, NULL);

	HANDLEBARS_VALUE_UNDECL(rv);
    HANDLEBARS_VALUE_UNDECL(value);
    ASSERT_INIT_BLOCKS();
}
END_TEST

START_TEST(test_map_find)
{
    HANDLEBARS_VALUE_DECL(rv);
    HANDLEBARS_VALUE_DECL(value);
	struct handlebars_value * value2;

    do {
        HANDLEBARS_VALUE_DECL(tmp);
        struct handlebars_map * map = handlebars_map_ctor(context, 2);
        struct handlebars_string * tmp_str;

        handlebars_value_integer(tmp, 2358);
        tmp_str = handlebars_string_ctor(context, HBS_STRL("a"));
        map = handlebars_map_update(map, tmp_str, tmp);

        handlebars_value_str(tmp, handlebars_string_ctor(context, HBS_STRL("test")));
        tmp_str = handlebars_string_ctor(context, HBS_STRL("b"));
        map = handlebars_map_update(map, tmp_str, tmp);

        handlebars_value_map(value, map);
        HANDLEBARS_VALUE_UNDECL(tmp);
    } while(0);

	ck_assert_ptr_ne(value, NULL);
	ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_MAP);

	value2 = handlebars_value_map_str_find(value, HBS_STRL("a"), rv);
	ck_assert_ptr_ne(value2, NULL);
	ck_assert_int_eq(handlebars_value_get_type(value2), HANDLEBARS_VALUE_TYPE_INTEGER);
	ck_assert_int_eq(handlebars_value_get_intval(value2), 2358);

	value2 = handlebars_value_map_str_find(value, HBS_STRL("b"), rv);
	ck_assert_ptr_ne(value2, NULL);
	ck_assert_int_eq(handlebars_value_get_type(value2), HANDLEBARS_VALUE_TYPE_STRING);
    const char * tmp = handlebars_value_get_strval(value2);
	ck_assert_str_eq(tmp, "test");
	ck_assert_int_eq(handlebars_value_get_strlen(value2), 4);

	value2 = handlebars_value_map_str_find(value, HBS_STRL("c"), rv);
	ck_assert_ptr_eq(value2, NULL);

    HANDLEBARS_VALUE_UNDECL(value);
    HANDLEBARS_VALUE_UNDECL(rv);
    ASSERT_INIT_BLOCKS();
}
END_TEST

START_TEST(test_readable_type)
{
    ck_assert_str_eq("null", handlebars_value_type_readable(HANDLEBARS_VALUE_TYPE_NULL));
    ck_assert_str_eq("true", handlebars_value_type_readable(HANDLEBARS_VALUE_TYPE_TRUE));
    ck_assert_str_eq("false", handlebars_value_type_readable(HANDLEBARS_VALUE_TYPE_FALSE));
    ck_assert_str_eq("integer", handlebars_value_type_readable(HANDLEBARS_VALUE_TYPE_INTEGER));
    ck_assert_str_eq("float", handlebars_value_type_readable(HANDLEBARS_VALUE_TYPE_FLOAT));
    ck_assert_str_eq("string", handlebars_value_type_readable(HANDLEBARS_VALUE_TYPE_STRING));
    ck_assert_str_eq("array", handlebars_value_type_readable(HANDLEBARS_VALUE_TYPE_ARRAY));
    ck_assert_str_eq("map", handlebars_value_type_readable(HANDLEBARS_VALUE_TYPE_MAP));
    ck_assert_str_eq("user", handlebars_value_type_readable(HANDLEBARS_VALUE_TYPE_USER));
    ck_assert_str_eq("ptr", handlebars_value_type_readable(HANDLEBARS_VALUE_TYPE_PTR));
    ck_assert_str_eq("helper", handlebars_value_type_readable(HANDLEBARS_VALUE_TYPE_HELPER));
    ck_assert_str_eq("closure", handlebars_value_type_readable(HANDLEBARS_VALUE_TYPE_CLOSURE));
#ifndef HANDLEBARS_ENABLE_DEBUG
    // @TODO maybe we should add another test with tcase_add_test_raise_signal?
    ck_assert_str_eq("unknown", handlebars_value_type_readable((enum handlebars_value_type) 1488));
#endif
}
END_TEST

START_TEST(test_iterator_void)
{
    HANDLEBARS_VALUE_DECL(value);
    HANDLEBARS_VALUE_ITERATOR_DECL(iter);
    ck_assert(!HANDLEBARS_VALUE_ITERATOR_INIT(iter, value));
    ck_assert(!handlebars_value_iterator_next(iter));
    HANDLEBARS_VALUE_UNDECL(value);
}
END_TEST

START_TEST(test_recursive_value_traversal_allows_shared_subgraph)
{
    HANDLEBARS_VALUE_DECL(child);
    HANDLEBARS_VALUE_DECL(parent);
    HANDLEBARS_VALUE_DECL(tmp);
    struct handlebars_string * expression;
    char * dump;

    handlebars_value_array(child, handlebars_stack_ctor(context, 1));
    handlebars_value_integer(tmp, 1);
    handlebars_value_array_push(child, tmp);

    handlebars_value_array(parent, handlebars_stack_ctor(context, 2));
    handlebars_value_array_push(parent, child);
    handlebars_value_array_push(parent, child);

    handlebars_value_convert(parent);
    expression = handlebars_value_expression(context, parent, false);
    ck_assert_hbs_str_eq_cstr(expression, "1,1");
    handlebars_talloc_free(expression);
    dump = handlebars_value_dump(parent, context, 0);
    ck_assert_ptr_nonnull(dump);
    handlebars_talloc_free(dump);

    HANDLEBARS_VALUE_UNDECL(tmp);
    HANDLEBARS_VALUE_UNDECL(parent);
    HANDLEBARS_VALUE_UNDECL(child);
#ifndef HANDLEBARS_NO_REFCOUNT
    ASSERT_INIT_BLOCKS();
#endif
}
END_TEST

START_TEST(test_recursive_value_traversal_rejects_cycle)
{
    HANDLEBARS_VALUE_DECL(value);
    struct handlebars_value * child;

    handlebars_value_array(value, handlebars_stack_ctor(context, 1));
    handlebars_value_array_push(value, value);

    assert_value_traversal_rejected(value, VALUE_TRAVERSAL_CONVERT, "Cyclic value reference");
    assert_value_traversal_rejected(value, VALUE_TRAVERSAL_EXPRESSION, "Cyclic value reference");
    assert_value_traversal_rejected(value, VALUE_TRAVERSAL_EXPRESSION_APPEND, "Cyclic value reference");
    assert_value_traversal_rejected(value, VALUE_TRAVERSAL_DUMP, "Cyclic value reference");

    child = handlebars_stack_get(value->v.stack, 0);
    ck_assert_ptr_nonnull(child);
    handlebars_value_null(child);
    HANDLEBARS_VALUE_UNDECL(value);
#ifndef HANDLEBARS_NO_REFCOUNT
    ASSERT_INIT_BLOCKS();
#endif
}
END_TEST

START_TEST(test_recursive_value_traversal_rejects_excessive_depth)
{
    HANDLEBARS_VALUE_DECL(value);

    init_nested_array(value, context, HANDLEBARS_VALUE_MAX_DEPTH + 1);
    assert_value_traversal_rejected(value, VALUE_TRAVERSAL_CONVERT, "maximum depth");
    assert_value_traversal_rejected(value, VALUE_TRAVERSAL_EXPRESSION, "maximum depth");
    assert_value_traversal_rejected(value, VALUE_TRAVERSAL_EXPRESSION_APPEND, "maximum depth");
    assert_value_traversal_rejected(value, VALUE_TRAVERSAL_DUMP, "maximum depth");

    HANDLEBARS_VALUE_UNDECL(value);
#ifndef HANDLEBARS_NO_REFCOUNT
    ASSERT_INIT_BLOCKS();
#endif
}
END_TEST

START_TEST(test_recursive_value_traversal_unwinds_cross_context_iterators)
{
    HANDLEBARS_VALUE_DECL(value);
    HANDLEBARS_VALUE_DECL(tmp);
    struct handlebars_context * owner = handlebars_context_ctor_ex(context);
    struct handlebars_map * map;
    struct handlebars_value * child;
    jmp_buf * volatile previous;
    jmp_buf buf;

    ck_assert_ptr_nonnull(owner);
    previous = owner->e->jmp;
    if( handlebars_setjmp_ex(owner, &buf) ) {
        owner->e->jmp = previous;
        ck_abort_msg("Traversal unexpectedly threw through the value owner context");
    }

    init_nested_array(value, owner, HANDLEBARS_VALUE_MAX_DEPTH + 1);
    assert_value_traversal_rejected(value, VALUE_TRAVERSAL_EXPRESSION, "maximum depth");
    ck_assert_ptr_null(owner->e->iterator_cleanup);
    assert_value_traversal_rejected(value, VALUE_TRAVERSAL_EXPRESSION_APPEND, "maximum depth");
    ck_assert_ptr_null(owner->e->iterator_cleanup);
    assert_value_traversal_rejected(value, VALUE_TRAVERSAL_DUMP, "maximum depth");
    ck_assert_ptr_null(owner->e->iterator_cleanup);

    handlebars_value_null(value);
    map = handlebars_map_ctor(owner, 1);
    map = handlebars_map_str_update(map, HBS_STRL("self"), tmp);
    handlebars_value_map(value, map);
    child = handlebars_map_str_find(map, HBS_STRL("self"));
    ck_assert_ptr_nonnull(child);
    handlebars_value_value(child, value);
    assert_value_traversal_rejected(value, VALUE_TRAVERSAL_DUMP, "Cyclic value reference");
    ck_assert_ptr_null(owner->e->iterator_cleanup);
    handlebars_value_null(child);

    owner->e->jmp = previous;
    HANDLEBARS_VALUE_UNDECL(tmp);
    HANDLEBARS_VALUE_UNDECL(value);
    handlebars_context_dtor(owner);
#ifndef HANDLEBARS_NO_REFCOUNT
    ASSERT_INIT_BLOCKS();
#endif
}
END_TEST

START_TEST(test_dump_null)
{
    HANDLEBARS_VALUE_DECL(value);
    char * dumped = handlebars_value_dump(value, context, 0);
    ck_assert_str_eq("NULL", dumped);
    handlebars_talloc_free(dumped);
    HANDLEBARS_VALUE_UNDECL(value);
}
END_TEST

START_TEST(test_dump_true)
{
    HANDLEBARS_VALUE_DECL(value);
    handlebars_value_boolean(value, true);
    char * dumped = handlebars_value_dump(value, context, 0);
    ck_assert_str_eq("boolean(true)", dumped);
    handlebars_talloc_free(dumped);
    HANDLEBARS_VALUE_UNDECL(value);
}
END_TEST

START_TEST(test_dump_false)
{
    HANDLEBARS_VALUE_DECL(value);
    handlebars_value_boolean(value, false);
    char * dumped = handlebars_value_dump(value, context, 0);
    ck_assert_str_eq("boolean(false)", dumped);
    handlebars_talloc_free(dumped);
    HANDLEBARS_VALUE_UNDECL(value);
}
END_TEST

START_TEST(test_dump_integer)
{
    HANDLEBARS_VALUE_DECL(value);
    handlebars_value_integer(value, 1488);
    char * dumped = handlebars_value_dump(value, context, 0);
    ck_assert_str_eq("integer(1488)", dumped);
    handlebars_talloc_free(dumped);
    HANDLEBARS_VALUE_UNDECL(value);
}
END_TEST

START_TEST(test_dump_float)
{
    HANDLEBARS_VALUE_DECL(value);
    handlebars_value_float(value, 1488.0);
    char * dumped = handlebars_value_dump(value, context, 0);
    ck_assert_str_eq("float(1488)", dumped);
    handlebars_talloc_free(dumped);
    HANDLEBARS_VALUE_UNDECL(value);
}
END_TEST

START_TEST(test_dump_string_and_non_scalar_values)
{
    HANDLEBARS_VALUE_DECL(value);
    int payload = 42;
    char expected[64];
    char * dumped;

    handlebars_value_str(value, handlebars_string_ctor(context, HBS_STRL("test")));
    dumped = handlebars_value_dump(value, context, 0);
    ck_assert_str_eq("string(test)", dumped);
    handlebars_talloc_free(dumped);

    handlebars_value_helper(value, handlebars_builtin_if);
    dumped = handlebars_value_dump(value, context, 0);
    snprintf(expected, sizeof(expected), "(function, real type %d)", value->type);
    ck_assert_str_eq(expected, dumped);
    handlebars_talloc_free(dumped);

    handlebars_value_ptr(value, handlebars_ptr_ctor(context, int, &payload, true));
    dumped = handlebars_value_dump(value, context, 0);
    snprintf(expected, sizeof(expected), "unknown type %d", value->type);
    ck_assert_str_eq(expected, dumped);
    handlebars_talloc_free(dumped);

    HANDLEBARS_VALUE_UNDECL(value);
    ASSERT_INIT_BLOCKS();
}
END_TEST

START_TEST(test_dump_array)
{
    HANDLEBARS_VALUE_DECL(tmp);
    HANDLEBARS_VALUE_DECL(value);
    handlebars_value_array(value, handlebars_stack_ctor(context, 3));

    handlebars_value_integer(tmp, 1);
    handlebars_value_array_push(value, tmp);

    handlebars_value_integer(tmp, 2);
    handlebars_value_array_push(value, tmp);

    handlebars_value_integer(tmp, 3);
    handlebars_value_array_push(value, tmp);

    char * dumped = handlebars_value_dump(value, context, 0);
    ck_assert_str_eq("[\n\
    0 => integer(1)\n\
    1 => integer(2)\n\
    2 => integer(3)\n\
]", dumped);
    handlebars_talloc_free(dumped);
    HANDLEBARS_VALUE_UNDECL(value);
    HANDLEBARS_VALUE_UNDECL(tmp);
    ASSERT_INIT_BLOCKS();
}
END_TEST

START_TEST(test_dump_map)
{
    HANDLEBARS_VALUE_DECL(value);
    HANDLEBARS_VALUE_DECL(tmp);
    struct handlebars_map * tmp_map;

    tmp_map = handlebars_map_ctor(context, 0); // zero may trigger extra rehashes possibly - good for testing

    handlebars_value_integer(tmp, 1);
    tmp_map = handlebars_map_str_update(tmp_map, HBS_STRL("a"), tmp);

    handlebars_value_integer(tmp, 2);
    tmp_map = handlebars_map_str_update(tmp_map, HBS_STRL("c"), tmp);

    handlebars_value_integer(tmp, 3);
    tmp_map = handlebars_map_str_update(tmp_map, HBS_STRL("b"), tmp);

    handlebars_value_map(value, tmp_map);

    char * dumped = handlebars_value_dump(value, context, 0);
    ck_assert_str_eq("{\n\
    a => integer(1)\n\
    c => integer(2)\n\
    b => integer(3)\n\
}", dumped);
    handlebars_talloc_free(dumped);

    HANDLEBARS_VALUE_UNDECL(tmp);
    HANDLEBARS_VALUE_UNDECL(value);
    ASSERT_INIT_BLOCKS();
}
END_TEST

static Suite * suite(void);
static Suite * suite(void)
{
    Suite * s = suite_create("Value");

    REGISTER_TEST_FIXTURE(s, test_boolean_true, "Boolean - true");
    REGISTER_TEST_FIXTURE(s, test_boolean_false, "Boolean - false");
    REGISTER_TEST_FIXTURE(s, test_int, "Integer");
    REGISTER_TEST_FIXTURE(s, test_float, "Float");
    REGISTER_TEST_FIXTURE(s, test_string, "String");
    REGISTER_TEST_FIXTURE(s, test_value_getter_defaults, "Getter defaults");
    REGISTER_TEST_FIXTURE(s, test_checked_pointer_retrieval, "Checked pointer retrieval");
#ifndef HANDLEBARS_NO_REFCOUNT
    REGISTER_TEST_FIXTURE(s, test_checked_pointer_retrieval_is_borrowed, "Checked pointer retrieval is borrowed");
#endif
#if !IS_WIN
    REGISTER_TEST_FIXTURE(s, test_pointer_getter_type_mismatch_aborts, "Pointer getter mismatch aborts");
    REGISTER_TEST_FIXTURE(s, test_value_pointer_getter_non_pointer_aborts, "Value pointer getter non-pointer aborts");
#endif
    REGISTER_TEST_FIXTURE(s, test_value_to_string_and_expression, "String conversion and expression rendering");
    REGISTER_TEST_FIXTURE(s, test_value_equality, "Value equality");
    REGISTER_TEST_FIXTURE(s, test_value_self_assignment, "Value self-assignment");
    REGISTER_TEST_FIXTURE(s, test_closure_rejects_negative_local_count, "Closure local count bounds");
    REGISTER_TEST_FIXTURE(s, test_vm_owns_default_maps, "VM owns its default maps");
    REGISTER_TEST_FIXTURE(s, test_idle_vm_builtins_reject_bad_arity, "Idle VM builtins reject bad arity");
    REGISTER_TEST_FIXTURE(s, test_idle_vm_builtins_handle_missing_programs, "Idle VM builtins handle missing programs");
    REGISTER_TEST_FIXTURE(s, test_idle_if_callable_error_restores_data, "Idle if callable errors restore VM data");
#ifndef HANDLEBARS_NO_REFCOUNT
    REGISTER_TEST_FIXTURE(s, test_user_value_allows_optional_destructor, "Optional user destructor");
#endif
#ifndef HANDLEBARS_NO_REFCOUNT
    REGISTER_TEST_FIXTURE(s, test_delimiter_replacement_releases_old_values, "Delimiter replacement ownership");
#endif
    REGISTER_TEST_FIXTURE(s, test_vm_reusable_after_helper_error, "VM reuse after helper error");
    REGISTER_TEST_FIXTURE(s, test_caught_each_error_restores_outer_data, "Caught each errors restore outer data");
    REGISTER_TEST_FIXTURE(s, test_caught_if_errors_restore_outer_data_and_vm_reuse, "Caught if errors restore outer data and VM reuse");
#ifndef HANDLEBARS_NO_REFCOUNT
    REGISTER_TEST_FIXTURE(s, test_with_helper_error_releases_vm_temporaries, "With helper errors release VM temporaries");
    REGISTER_TEST_FIXTURE(s, test_with_callable_error_releases_vm_temporaries, "With callable errors release VM temporaries");
    REGISTER_TEST_FIXTURE(s, test_if_helper_error_releases_vm_temporaries, "If helper errors release VM temporaries");
    REGISTER_TEST_FIXTURE(s, test_if_callable_selected_program_error_releases_vm_temporaries, "If callable selected-program errors release VM temporaries");
    REGISTER_TEST_FIXTURE(s, test_unless_helper_error_releases_vm_temporaries, "Unless helper errors release VM temporaries");
    REGISTER_TEST_FIXTURE(s, test_each_helper_error_releases_vm_temporaries, "Each helper errors release VM temporaries");
    REGISTER_TEST_FIXTURE(s, test_each_callable_error_releases_vm_temporaries, "Each callable errors release VM temporaries");
    REGISTER_TEST_FIXTURE(s, test_each_helper_success_releases_nested_buffers, "Each helper success releases nested buffers");
    REGISTER_TEST_FIXTURE(s, test_with_helper_success_releases_nested_buffer, "With helper success releases nested buffer");
    REGISTER_TEST_FIXTURE(s, test_block_helper_missing_error_releases_vm_temporaries, "Block helper missing errors release VM temporaries");
#endif
    REGISTER_TEST_FIXTURE(s, test_inline_partial_error_unwinds_vm, "Inline partial errors unwind VM state");
    REGISTER_TEST_FIXTURE(s, test_inline_partial_explicit_context_error_unwinds_vm, "Inline partial explicit-context errors unwind VM state");
    REGISTER_TEST_FIXTURE(s, test_inline_partial_recursive_partial_block_is_bounded, "Recursive inline partial blocks are bounded");
    REGISTER_TEST_FIXTURE(s, test_inline_partial_error_after_stack_growth_unwinds_vm, "Inline partial errors unwind after captured stack growth");
#ifdef HANDLEBARS_MEMORY
    REGISTER_TEST_FIXTURE(s, test_with_helper_allocation_failures_unwind_vm, "With helper allocation failures unwind VM state");
    REGISTER_TEST_FIXTURE(s, test_each_helper_allocation_failures_unwind_vm, "Each helper allocation failures unwind VM state");
    REGISTER_TEST_FIXTURE(s, test_each_map_allocation_failures_unwind_vm, "Each map allocation failures unwind VM state");
    REGISTER_TEST_FIXTURE(s, test_with_callable_allocation_failures_unwind_vm, "With callable allocation failures unwind VM state");
    REGISTER_TEST_FIXTURE(s, test_if_callable_allocation_failures_unwind_vm, "If callable allocation failures unwind VM state");
    REGISTER_TEST_FIXTURE(s, test_each_callable_allocation_failures_unwind_vm, "Each callable allocation failures unwind VM state");
    REGISTER_TEST_FIXTURE(s, test_ambiguous_block_value_allocation_failures_unwind_vm, "Ambiguous block value allocation failures unwind VM state");
    REGISTER_TEST_FIXTURE(s, test_block_value_allocation_failures_unwind_vm, "Block value allocation failures unwind VM state");
    REGISTER_TEST_FIXTURE(s, test_invoke_helper_allocation_failures_unwind_vm, "Invoke helper allocation failures unwind VM state");
    REGISTER_TEST_FIXTURE(s, test_invoke_known_helper_allocation_failures_unwind_vm, "Invoke known helper allocation failures unwind VM state");
    REGISTER_TEST_FIXTURE(s, test_inline_partial_allocation_failures_unwind_vm, "Inline partial allocation failures unwind VM state");
    REGISTER_TEST_FIXTURE(s, test_inline_partial_scalar_name_allocation_failures_unwind_vm, "Inline partial scalar-name allocation failures unwind VM state");
    REGISTER_TEST_FIXTURE(s, test_inline_partial_block_allocation_failures_unwind_vm, "Inline partial block allocation failures unwind VM state");
    REGISTER_TEST_FIXTURE(s, test_inline_partial_scope_allocation_failures_unwind_vm, "Inline partial scope allocation failures unwind VM state");
#endif
    REGISTER_TEST_FIXTURE(s, test_subexpression_rejects_non_callable_context_value, "Subexpression rejects non-callable context value");
    REGISTER_TEST_FIXTURE(s, test_subexpression_allows_falsey_context_values, "Subexpression allows falsey context values");
    REGISTER_TEST_FIXTURE(s, test_subexpression_rejects_empty_containers, "Subexpression rejects empty containers");
    REGISTER_TEST_FIXTURE(s, test_subexpression_calls_callable_context_value, "Subexpression calls callable context value");
    REGISTER_TEST_FIXTURE(s, test_subexpression_allows_missing_context_value, "Subexpression allows missing context value");
    REGISTER_TEST_FIXTURE(s, test_vm_emulates_string_and_array_length_properties, "VM emulates string and array length properties");
    REGISTER_TEST_FIXTURE(s, test_vm_string_length_handles_user_without_count, "VM string length handles USER values without count handlers");
    REGISTER_TEST_FIXTURE(s, test_vm_user_collection_length_and_callback_contracts, "VM honors USER collection length and callback contracts");
    REGISTER_TEST_FIXTURE(s, test_vm_array_index_canonical_boundaries, "VM accepts only canonical array index boundaries");
    REGISTER_TEST_FIXTURE(s, test_vm_length_data_path_stays_missing_after_missing_segment, "VM length data paths stay missing after a missing segment");
    REGISTER_TEST_FIXTURE(s, test_vm_emulates_length_for_custom_lazy_arrays, "VM emulates length for custom lazy arrays");
    REGISTER_TEST_FIXTURE(s, test_vm_length_block_param_stays_missing_after_missing_segment, "VM length block-param paths stay missing after a missing segment");
    REGISTER_TEST_FIXTURE(s, test_array_iterator, "Array iterator");
    REGISTER_TEST_FIXTURE(s, test_value_container_mutators_try_succeed, "Checked container mutators succeed");
    REGISTER_TEST_FIXTURE(s, test_value_container_mutators_try_reject_non_native_values, "Checked container mutators reject non-native values");
    REGISTER_TEST_FIXTURE(s, test_value_container_mutators_type_error_preserves_diagnostics, "Checked container type errors preserve diagnostics");
    REGISTER_TEST_FIXTURE(s, test_value_array_push_try_preserves_self_alias_during_growth, "Checked array push preserves a self alias during growth");
    REGISTER_TEST_FIXTURE(s, test_value_map_update_try_preserves_self_alias_during_growth, "Checked map update preserves a self alias during growth");
#ifndef HANDLEBARS_NO_REFCOUNT
    REGISTER_TEST_FIXTURE(s, test_value_container_mutators_try_preserve_copy_on_write_aliases, "Checked container mutators preserve copy-on-write aliases");
#endif
    REGISTER_TEST_FIXTURE(s, test_value_array_push_try_stack_overflow_preserves_outer_boundary, "Checked stack-backed array overflow preserves the outer boundary");
    REGISTER_TEST_FIXTURE(s, test_value_array_set_try_catches_bounds_errors, "Checked array set catches bounds errors");
#ifdef HANDLEBARS_MEMORY
    REGISTER_TEST_FIXTURE(s, test_value_array_push_try_catches_allocation_failure, "Checked array push catches allocation failures");
    REGISTER_TEST_FIXTURE(s, test_value_map_update_try_catches_allocation_failure, "Checked map update catches allocation failures");
#endif
    REGISTER_TEST_FIXTURE(s, test_iterator_initializer_respects_explicit_current_storage, "Iterator initialization respects explicit current storage");
    REGISTER_TEST_FIXTURE(s, test_array_iterator_retains_stack, "Array iterator retains its backing stack");
    REGISTER_TEST_FIXTURE(s, test_map_iterator, "Map iterator");
    REGISTER_TEST_FIXTURE(s, test_map_iterator_sparse, "Map iterator (sparse)");
    REGISTER_TEST_FIXTURE(s, test_map_iterator_nested, "Nested map iterators");
    REGISTER_TEST_FIXTURE(s, test_map_iterator_retains_map, "Map iterator retains its backing map");
    REGISTER_TEST_FIXTURE(s, test_map_iterator_break_releases_snapshot, "Breaking map iteration releases its snapshot");
    REGISTER_TEST_FIXTURE(s, test_map_iterator_mutation_uses_snapshot, "Map mutation preserves the active iterator snapshot");
#ifndef HANDLEBARS_NO_REFCOUNT
    REGISTER_TEST_FIXTURE(s, test_map_foreach_removal_preserves_value_owned_map, "Map foreach removal preserves its value-owned map");
    REGISTER_TEST_FIXTURE(s, test_map_foreach_removal_preserves_shared_value_owned_map, "Map foreach removal preserves its shared value-owned map");
#endif
    REGISTER_TEST_FIXTURE(s, test_mixed_map_iterators_reject_mutation, "Mixed map iterators reject mutation");
    REGISTER_TEST_FIXTURE(s, test_map_iterator_longjmp_releases_snapshot, "Map iterator releases its snapshot during error unwind");
    REGISTER_TEST_FIXTURE(s, test_array_iterator_longjmp_releases_snapshot, "Array iterator releases its snapshot during error unwind");
    REGISTER_TEST_FIXTURE(s, test_nested_error_boundary_preserves_outer_iterator, "Nested error boundaries preserve outer iterators");
#ifndef HANDLEBARS_NO_REFCOUNT
    REGISTER_TEST_FIXTURE(s, test_user_iterator_longjmp_releases_owner, "User iterator releases its owner during error unwind");
#endif
#ifdef HANDLEBARS_NO_REFCOUNT
    REGISTER_TEST_FIXTURE(s, test_map_iterator_no_refcount_guard_is_nested, "No-refcount map iterator guards are nested");
#endif
    REGISTER_TEST_FIXTURE(s, test_array_find, "Array Find");
    REGISTER_TEST_FIXTURE(s, test_map_find, "Map Find");
    REGISTER_TEST_FIXTURE(s, test_readable_type, "Readable Type");
    REGISTER_TEST_FIXTURE(s, test_iterator_void, "Void iterator");
    REGISTER_TEST_FIXTURE(s, test_recursive_value_traversal_allows_shared_subgraph, "Recursive value traversal allows shared subgraphs");
    REGISTER_TEST_FIXTURE(s, test_recursive_value_traversal_rejects_cycle, "Recursive value traversal rejects cycles");
    REGISTER_TEST_FIXTURE(s, test_recursive_value_traversal_rejects_excessive_depth, "Recursive value traversal rejects excessive depth");
    REGISTER_TEST_FIXTURE(s, test_recursive_value_traversal_unwinds_cross_context_iterators, "Recursive value traversal unwinds cross-context iterators");
    REGISTER_TEST_FIXTURE(s, test_dump_null, "dump - null");
    REGISTER_TEST_FIXTURE(s, test_dump_true, "dump - true");
    REGISTER_TEST_FIXTURE(s, test_dump_false, "dump - false");
    REGISTER_TEST_FIXTURE(s, test_dump_integer, "dump - integer");
    REGISTER_TEST_FIXTURE(s, test_dump_float, "dump - float");
    REGISTER_TEST_FIXTURE(s, test_dump_string_and_non_scalar_values, "dump - string and non-scalar values");
    REGISTER_TEST_FIXTURE(s, test_dump_array, "dump - array");
    REGISTER_TEST_FIXTURE(s, test_dump_map, "dump - map");

    return s;
}

int main(void)
{
    return default_main(&suite);
}
