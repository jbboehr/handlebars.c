 /**
 * Copyright (C) 2020 John Boehr
 *
 * This file is part of handlebars.c.
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

#include "handlebars_memory.h"
#include "handlebars_value.h"
#include "utils.h"


START_TEST(test_boolean_true)
{
    struct handlebars_value * value = handlebars_value_ctor(context);
    handlebars_value_boolean(value, true);
    ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_TRUE);
    ck_assert_int_eq(handlebars_value_get_boolval(value), 1);
    handlebars_value_delref(value);
    ASSERT_INIT_BLOCKS();
}
END_TEST

START_TEST(test_boolean_false)
{
    struct handlebars_value * value = handlebars_value_ctor(context);
    handlebars_value_boolean(value, false);
    ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_FALSE);
    ck_assert_int_eq(handlebars_value_get_boolval(value), 0);
    handlebars_value_delref(value);
    ASSERT_INIT_BLOCKS();
}
END_TEST

START_TEST(test_int)
{
    struct handlebars_value * value = handlebars_value_ctor(context);
    handlebars_value_integer(value, 2358);
    ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_INTEGER);
    ck_assert_int_eq(handlebars_value_get_intval(value), 2358);
    handlebars_value_delref(value);
    ASSERT_INIT_BLOCKS();
}
END_TEST

START_TEST(test_float)
{
    struct handlebars_value * value = handlebars_value_ctor(context);
    handlebars_value_float(value, 1234.4321);
    ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_FLOAT);
    // Note: converting to int - precision issue
    ck_assert_int_eq(handlebars_value_get_floatval(value), 1234.4321);
    handlebars_value_delref(value);
    ASSERT_INIT_BLOCKS();
}
END_TEST

START_TEST(test_string)
{
    struct handlebars_value * value = handlebars_value_ctor(context);
    handlebars_value_str(value, handlebars_string_ctor(context, HBS_STRL("test")));
	ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_STRING);
    const char * tmp = handlebars_value_get_strval(value);
	ck_assert_str_eq(tmp, "test");
	ck_assert_int_eq(handlebars_value_get_strlen(value), 4);
    handlebars_value_delref(value);
    ASSERT_INIT_BLOCKS();
}
END_TEST

START_TEST(test_array_iterator)
{
    struct handlebars_value * value;
    struct handlebars_value * tmp;
    size_t i = 0;

    value = handlebars_value_ctor(context);
    handlebars_value_array_init(value, 3);

    tmp = handlebars_value_ctor(context);
    handlebars_value_integer(tmp, 1);
    handlebars_value_array_push(value, tmp);

    tmp = handlebars_value_ctor(context);
    handlebars_value_integer(tmp, 2);
    handlebars_value_array_push(value, tmp);

    tmp = handlebars_value_ctor(context);
    handlebars_value_integer(tmp, 3);
    handlebars_value_array_push(value, tmp);

    HANDLEBARS_VALUE_FOREACH_IDX(value, index, child) {
        ck_assert_ptr_ne(child, NULL);
        ck_assert_int_eq(handlebars_value_get_type(child), HANDLEBARS_VALUE_TYPE_INTEGER);
        ck_assert_uint_eq(index, i);
        ck_assert_int_eq((size_t) handlebars_value_get_intval(child), ++i);
    } HANDLEBARS_VALUE_FOREACH_END();

    handlebars_value_delref(value);
    ASSERT_INIT_BLOCKS();
}
END_TEST

START_TEST(test_map_iterator)
{
    struct handlebars_value * value;
    struct handlebars_value * tmp;
    struct handlebars_map * tmp_map;
    int i = 0;

    tmp_map = handlebars_map_ctor(context, 0); // zero may trigger extra rehashes possibly - good for testing

    tmp = handlebars_value_ctor(context);
    handlebars_value_integer(tmp, 1);
    tmp_map = handlebars_map_str_update(tmp_map, HBS_STRL("a"), tmp);

    tmp = handlebars_value_ctor(context);
    handlebars_value_integer(tmp, 2);
    tmp_map = handlebars_map_str_update(tmp_map, HBS_STRL("c"), tmp);

    tmp = handlebars_value_ctor(context);
    handlebars_value_integer(tmp, 3);
    tmp_map = handlebars_map_str_update(tmp_map, HBS_STRL("b"), tmp);

    value = handlebars_value_ctor(context);
    handlebars_value_map(value, tmp_map);

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

    handlebars_value_delref(value);
    ASSERT_INIT_BLOCKS();
}
END_TEST

START_TEST(test_array_find)
{
	struct handlebars_value * value2;
    struct handlebars_value * value = handlebars_value_ctor(context);
    handlebars_value_array_init(value, 2);
    do {
        struct handlebars_value * tmp;
        struct handlebars_string * tmp_str;

        tmp = handlebars_value_ctor(context);
        handlebars_value_integer(tmp, 2358);
        handlebars_value_array_push(value, tmp);

        tmp = handlebars_value_ctor(context);
        tmp_str = handlebars_string_ctor(context, HBS_STRL("test"));
        handlebars_value_str(tmp, tmp_str);
        handlebars_value_array_push(value, tmp);
    } while(0);

	ck_assert_ptr_ne(value, NULL);
	ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_ARRAY);

	value2 = handlebars_value_array_find(value, 0);
	ck_assert_ptr_ne(value2, NULL);
	ck_assert_int_eq(handlebars_value_get_type(value2), HANDLEBARS_VALUE_TYPE_INTEGER);
	ck_assert_int_eq(handlebars_value_get_intval(value2), 2358);
    handlebars_value_delref(value2);

	value2 = handlebars_value_array_find(value, 1);
	ck_assert_ptr_ne(value2, NULL);
	ck_assert_int_eq(handlebars_value_get_type(value2), HANDLEBARS_VALUE_TYPE_STRING);
    const char * tmp = handlebars_value_get_strval(value2);
	ck_assert_str_eq(tmp, "test");
	ck_assert_int_eq(handlebars_value_get_strlen(value2), 4);
    handlebars_value_delref(value2);

	value2 = handlebars_value_array_find(value, 2);
	ck_assert_ptr_eq(value2, NULL);

    handlebars_value_delref(value);
    ASSERT_INIT_BLOCKS();
}
END_TEST

START_TEST(test_map_find)
{
	struct handlebars_value * value2;
    struct handlebars_value * value = handlebars_value_ctor(context);
    handlebars_value_addref(value);
    do {
        struct handlebars_value * tmp;
        struct handlebars_map * map = handlebars_map_ctor(context, 2);
        struct handlebars_string * tmp_str;

        tmp = handlebars_value_ctor(context);
        handlebars_value_integer(tmp, 2358);
        tmp_str = handlebars_string_ctor(context, HBS_STRL("a"));
        map = handlebars_map_update(map, tmp_str, tmp);

        tmp = handlebars_value_ctor(context);
        handlebars_value_str(tmp, handlebars_string_ctor(context, HBS_STRL("test")));
        tmp_str = handlebars_string_ctor(context, HBS_STRL("b"));
        map = handlebars_map_update(map, tmp_str, tmp);

        handlebars_value_map(value, map);
    } while(0);

	ck_assert_ptr_ne(value, NULL);
	ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_MAP);

	value2 = handlebars_value_map_str_find(value, HBS_STRL("a"));
	ck_assert_ptr_ne(value2, NULL);
	ck_assert_int_eq(handlebars_value_get_type(value2), HANDLEBARS_VALUE_TYPE_INTEGER);
	ck_assert_int_eq(handlebars_value_get_intval(value2), 2358);
    handlebars_value_delref(value2);

	value2 = handlebars_value_map_str_find(value, HBS_STRL("b"));
	ck_assert_ptr_ne(value2, NULL);
	ck_assert_int_eq(handlebars_value_get_type(value2), HANDLEBARS_VALUE_TYPE_STRING);
    const char * tmp = handlebars_value_get_strval(value2);
	ck_assert_str_eq(tmp, "test");
	ck_assert_int_eq(handlebars_value_get_strlen(value2), 4);
    handlebars_value_delref(value2);

	value2 = handlebars_value_map_str_find(value, HBS_STRL("c"));
	ck_assert_ptr_eq(value2, NULL);

    handlebars_value_delref(value);
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
    REGISTER_TEST_FIXTURE(s, test_array_find, "Array Find");
    REGISTER_TEST_FIXTURE(s, test_map_find, "Map Find");

    return s;
}

int main(void)
{
    int number_failed;
    int memdebug;
    int error;

    talloc_set_log_stderr();

    // Check if memdebug enabled
    memdebug = getenv("MEMDEBUG") ? atoi(getenv("MEMDEBUG")) : 0;
    if( memdebug ) {
        talloc_enable_leak_report_full();
    }

    // Set up test suite
    Suite * s = suite();
    SRunner * sr = srunner_create(s);
    if( IS_WIN || memdebug ) {
        srunner_set_fork_status(sr, CK_NOFORK);
    }
    srunner_run_all(sr, CK_ENV);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    error = (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;

    // Generate report for memdebug
    if( memdebug ) {
        talloc_report_full(NULL, stderr);
    }

    // Return
    return error;
}
