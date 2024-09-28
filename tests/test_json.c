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
#include "handlebars_json.h"
#include "handlebars_string.h"
#include "handlebars_value.h"
#include "utils.h"


START_TEST(test_boolean_json_true)
{
    HANDLEBARS_VALUE_DECL(value);
    handlebars_value_init_json_string(context, value, "true");
    ck_assert_ptr_ne(value, NULL);
    ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_TRUE);
    ck_assert_int_eq(handlebars_value_get_boolval(value), 1);
    HANDLEBARS_VALUE_UNDECL(value);
    ASSERT_INIT_BLOCKS();
}
END_TEST

START_TEST(test_boolean_json_false)
{
    HANDLEBARS_VALUE_DECL(value);
    handlebars_value_init_json_string(context, value, "false");
    ck_assert_ptr_ne(value, NULL);
    ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_FALSE);
    ck_assert_int_eq(handlebars_value_get_boolval(value), 0);
    HANDLEBARS_VALUE_UNDECL(value);
    ASSERT_INIT_BLOCKS();
}
END_TEST

START_TEST(test_int_json)
{
    HANDLEBARS_VALUE_DECL(value);
    handlebars_value_init_json_string(context, value, "2358");
    ck_assert_ptr_ne(value, NULL);
    ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_INTEGER);
    ck_assert_int_eq(handlebars_value_get_intval(value), 2358);
    HANDLEBARS_VALUE_UNDECL(value);
    ASSERT_INIT_BLOCKS();
}
END_TEST

START_TEST(test_float_json)
{
    HANDLEBARS_VALUE_DECL(value);
    handlebars_value_init_json_string(context, value, "1234.4321");
    ck_assert_ptr_ne(value, NULL);
    ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_FLOAT);
    // Note: converting to int - precision issue
    ck_assert_int_eq(handlebars_value_get_floatval(value), 1234.4321);
    HANDLEBARS_VALUE_UNDECL(value);
    ASSERT_INIT_BLOCKS();
}
END_TEST

START_TEST(test_string_json)
{
    HANDLEBARS_VALUE_DECL(value);
	handlebars_value_init_json_string(context, value, "\"test\"");
	ck_assert_ptr_ne(value, NULL);
	ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_STRING);
    const char * tmp = handlebars_value_get_strval(value);
	ck_assert_str_eq(tmp, "test");
	ck_assert_int_eq(handlebars_value_get_strlen(value), 4);
    HANDLEBARS_VALUE_UNDECL(value);
    ASSERT_INIT_BLOCKS();
}
END_TEST

START_TEST(test_array_iterator_json)
{
    HANDLEBARS_VALUE_DECL(value);
    handlebars_value_init_json_string(context, value, "[1, 2, 3]");
    int i = 0;

    HANDLEBARS_VALUE_FOREACH_IDX(value, index, child) {
        ck_assert_ptr_ne(child, NULL);
        ck_assert_int_eq(handlebars_value_get_type(child), HANDLEBARS_VALUE_TYPE_INTEGER);
        ck_assert_int_eq(index, i);
        ck_assert_int_eq(handlebars_value_get_intval(child), ++i);
    } HANDLEBARS_VALUE_FOREACH_END();

    HANDLEBARS_VALUE_UNDECL(value);
    ASSERT_INIT_BLOCKS();
}
END_TEST

START_TEST(test_map_iterator_json)
{
    HANDLEBARS_VALUE_DECL(value);
    handlebars_value_init_json_string(context, value, "{\"a\": 1, \"c\": 2, \"b\": 3}");
    int i = 0;

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

    HANDLEBARS_VALUE_UNDECL(value);
    ASSERT_INIT_BLOCKS();
}
END_TEST

START_TEST(test_array_find_json)
{
    HANDLEBARS_VALUE_DECL(value);
    HANDLEBARS_VALUE_DECL(rv);
    struct handlebars_value * value2;

	handlebars_value_init_json_string(context, value, "[2358, \"test\"]");
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

START_TEST(test_map_find_json)
{
    HANDLEBARS_VALUE_DECL(rv);
    HANDLEBARS_VALUE_DECL(value);
	struct handlebars_value * value2;
	handlebars_value_init_json_string(context, value, "{\"a\": 2358, \"b\": \"test\"}");
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

START_TEST(test_complex_json)
{
    HANDLEBARS_VALUE_DECL(rv);
    HANDLEBARS_VALUE_DECL(rv2);
    HANDLEBARS_VALUE_DECL(value);
    struct handlebars_value * value2;
    struct handlebars_value * value3;

	handlebars_value_init_json_string(context, value, "{\"a\": 2358, \"b\": [1, 2.1], \"c\": {\"d\": \"test\"}}");
	ck_assert_ptr_ne(value, NULL);
	ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_MAP);

	value2 = handlebars_value_map_str_find(value, HBS_STRL("a"), rv);
	ck_assert_ptr_ne(value2, NULL);
	ck_assert_int_eq(handlebars_value_get_type(value2), HANDLEBARS_VALUE_TYPE_INTEGER);
	ck_assert_int_eq(handlebars_value_get_intval(value2), 2358);

	value2 = handlebars_value_map_str_find(value, HBS_STRL("b"), rv);
	ck_assert_ptr_ne(value2, NULL);
	ck_assert_int_eq(handlebars_value_get_type(value2), HANDLEBARS_VALUE_TYPE_ARRAY);

    do {
        value3 = handlebars_value_array_find(value2, 0, rv2);
        ck_assert_ptr_ne(value3, NULL);
        ck_assert_int_eq(handlebars_value_get_type(value3), HANDLEBARS_VALUE_TYPE_INTEGER);

        value3 = handlebars_value_array_find(value2, 1, rv2);
        ck_assert_ptr_ne(value3, NULL);
        ck_assert_int_eq(handlebars_value_get_type(value3), HANDLEBARS_VALUE_TYPE_FLOAT);
    } while(0);

	value2 = handlebars_value_map_str_find(value, HBS_STRL("c"), rv);
	ck_assert_ptr_ne(value2, NULL);
	ck_assert_int_eq(handlebars_value_get_type(value2), HANDLEBARS_VALUE_TYPE_MAP);

    do {
        value3 = handlebars_value_map_str_find(value2, HBS_STRL("d"), rv2);
        ck_assert_int_eq(handlebars_value_get_type(value3), HANDLEBARS_VALUE_TYPE_STRING);
        const char * tmp = handlebars_value_get_strval(value3);
        ck_assert_str_eq(tmp, "test");
        ck_assert_int_eq(handlebars_value_get_strlen(value3), 4);
    } while(0);

    HANDLEBARS_VALUE_UNDECL(value);
    HANDLEBARS_VALUE_UNDECL(rv2);
    HANDLEBARS_VALUE_UNDECL(rv);
    ASSERT_INIT_BLOCKS();
}
END_TEST

START_TEST(test_convert_json)
{
    HANDLEBARS_VALUE_DECL(rv);
    HANDLEBARS_VALUE_DECL(value);
    struct handlebars_value * value2;
    handlebars_value_init_json_string(context, value, "{\"a\": 2358, \"b\": [1, 2.1], \"c\": {\"d\": \"test\"}}");
    handlebars_value_convert(value);

    ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_MAP);

    value2 = handlebars_value_map_str_find(value, HBS_STRL("b"), rv);
    ck_assert_ptr_ne(value2, NULL);
    ck_assert_int_eq(handlebars_value_get_type(value2), HANDLEBARS_VALUE_TYPE_ARRAY);

    HANDLEBARS_VALUE_UNDECL(value);
    HANDLEBARS_VALUE_UNDECL(rv);
    ASSERT_INIT_BLOCKS();
}
END_TEST

START_TEST(test_parse_error_json)
{
    jmp_buf buf;
    HANDLEBARS_VALUE_DECL(value);

    if( handlebars_setjmp_ex(context, &buf) ) {
        char * error = NULL;
        if( 0 != regex_compare("^JSON Parse error", handlebars_error_msg(context), &error) ) {
            ck_abort_msg("%s", error);
        }
        return;
    }

    handlebars_value_init_json_string(context, value, "{\"key\":1");
    (void) value;
    ck_assert_msg(0, "Parse error should have longjmp'd");

    HANDLEBARS_VALUE_UNDECL(value);
}
END_TEST

static Suite * suite(void);
static Suite * suite(void)
{
    Suite * s = suite_create("JSON");

    REGISTER_TEST_FIXTURE(s, test_boolean_json_true, "Boolean - true");
    REGISTER_TEST_FIXTURE(s, test_boolean_json_false, "Boolean - false");
    REGISTER_TEST_FIXTURE(s, test_int_json, "Integer");
    REGISTER_TEST_FIXTURE(s, test_float_json, "Float");
    REGISTER_TEST_FIXTURE(s, test_string_json, "String");
    REGISTER_TEST_FIXTURE(s, test_array_iterator_json, "Array iterator");
    REGISTER_TEST_FIXTURE(s, test_map_iterator_json, "Map iterator");
    REGISTER_TEST_FIXTURE(s, test_array_find_json, "Array Find");
    REGISTER_TEST_FIXTURE(s, test_map_find_json, "Map Find");
    REGISTER_TEST_FIXTURE(s, test_complex_json, "Complex");
    REGISTER_TEST_FIXTURE(s, test_convert_json, "Convert");
    REGISTER_TEST_FIXTURE(s, test_parse_error_json, "JSON Parse Error");

    return s;
}

int main(void)
{
    return default_main(&suite);
}
