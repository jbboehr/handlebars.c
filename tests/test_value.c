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

#ifndef HANDLEBARS_NO_REFCOUNT
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
#ifndef HANDLEBARS_NO_REFCOUNT
    REGISTER_TEST_FIXTURE(s, test_user_value_allows_optional_destructor, "Optional user destructor");
#endif
#ifndef HANDLEBARS_NO_REFCOUNT
    REGISTER_TEST_FIXTURE(s, test_delimiter_replacement_releases_old_values, "Delimiter replacement ownership");
#endif
    REGISTER_TEST_FIXTURE(s, test_vm_reusable_after_helper_error, "VM reuse after helper error");
    REGISTER_TEST_FIXTURE(s, test_array_iterator, "Array iterator");
    REGISTER_TEST_FIXTURE(s, test_map_iterator, "Map iterator");
    REGISTER_TEST_FIXTURE(s, test_map_iterator_sparse, "Map iterator (sparse)");
    REGISTER_TEST_FIXTURE(s, test_array_find, "Array Find");
    REGISTER_TEST_FIXTURE(s, test_map_find, "Map Find");
    REGISTER_TEST_FIXTURE(s, test_readable_type, "Readable Type");
    REGISTER_TEST_FIXTURE(s, test_iterator_void, "Void iterator");
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
