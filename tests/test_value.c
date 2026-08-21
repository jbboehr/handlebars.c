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
#include "handlebars_stack.h"
#include "handlebars_string.h"
#include "handlebars_value.h"
#include "handlebars_value_handlers.h"
#include "handlebars_vm.h"
#include "handlebars_vm_private.h"

#include "utils.h"


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

static void clear_intentional_error(void)
{
    handlebars_talloc_free((char *) context->e->msg);
    context->e->msg = NULL;
    context->e->num = HANDLEBARS_SUCCESS;
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

START_TEST(test_vm_owns_default_maps)
{
    ck_assert_ptr_eq(talloc_parent(handlebars_value_get_map(&vm->helpers)), vm);
    ck_assert_ptr_eq(talloc_parent(handlebars_value_get_map(&vm->partials)), vm);
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

    ck_assert(handlebars_value_iterator_init(iter, value));
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

        if( handlebars_value_iterator_init(inner_iter, value) ) {
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

    ck_assert(handlebars_value_iterator_init(iter, value));
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
    ck_assert(handlebars_value_iterator_init(iter, value));

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

    ck_assert(handlebars_value_iterator_init(iter, value));
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

    ck_assert(handlebars_value_iterator_init(first, value));
    ck_assert(handlebars_value_iterator_init(second, value));

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
    ck_assert(!handlebars_value_iterator_init(iter, value));
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
    REGISTER_TEST_FIXTURE(s, test_value_self_assignment, "Value self-assignment");
    REGISTER_TEST_FIXTURE(s, test_closure_rejects_negative_local_count, "Closure local count bounds");
    REGISTER_TEST_FIXTURE(s, test_vm_owns_default_maps, "VM owns its default maps");
#ifndef HANDLEBARS_NO_REFCOUNT
    REGISTER_TEST_FIXTURE(s, test_user_value_allows_optional_destructor, "Optional user destructor");
#endif
#ifndef HANDLEBARS_NO_REFCOUNT
    REGISTER_TEST_FIXTURE(s, test_delimiter_replacement_releases_old_values, "Delimiter replacement ownership");
#endif
    REGISTER_TEST_FIXTURE(s, test_vm_reusable_after_helper_error, "VM reuse after helper error");
    REGISTER_TEST_FIXTURE(s, test_array_iterator, "Array iterator");
    REGISTER_TEST_FIXTURE(s, test_array_iterator_retains_stack, "Array iterator retains its backing stack");
    REGISTER_TEST_FIXTURE(s, test_map_iterator, "Map iterator");
    REGISTER_TEST_FIXTURE(s, test_map_iterator_sparse, "Map iterator (sparse)");
    REGISTER_TEST_FIXTURE(s, test_map_iterator_nested, "Nested map iterators");
    REGISTER_TEST_FIXTURE(s, test_map_iterator_retains_map, "Map iterator retains its backing map");
    REGISTER_TEST_FIXTURE(s, test_map_iterator_break_releases_snapshot, "Breaking map iteration releases its snapshot");
    REGISTER_TEST_FIXTURE(s, test_map_iterator_mutation_uses_snapshot, "Map mutation preserves the active iterator snapshot");
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
    REGISTER_TEST_FIXTURE(s, test_dump_array, "dump - array");
    REGISTER_TEST_FIXTURE(s, test_dump_map, "dump - map");

    return s;
}

int main(void)
{
    return default_main(&suite);
}
