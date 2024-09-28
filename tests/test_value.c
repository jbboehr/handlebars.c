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
#include <talloc.h>

#include "handlebars.h"
#include "handlebars_memory.h"

#include "handlebars_map.h"
#include "handlebars_stack.h"
#include "handlebars_string.h"
#include "handlebars_value.h"

#include "utils.h"


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
