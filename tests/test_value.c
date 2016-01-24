
#include <check.h>
#include <stdio.h>
#include <talloc.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if defined(HAVE_JSON_C_JSON_H)
#include <json-c/json.h>
#include <json-c/json_object.h>
#include <json-c/json_tokener.h>
#elif defined(HAVE_JSON_JSON_H)
#include <json/json.h>
#include <json/json_object.h>
#include <json/json_tokener.h>
#endif

#include "utils.h"

#include "handlebars_memory.h"

#include "handlebars_value.h"

static TALLOC_CTX * ctx;

static void setup(void)
{
    handlebars_memory_fail_disable();
    ctx = talloc_new(NULL);
}

static void teardown(void)
{
    handlebars_memory_fail_disable();
    talloc_free(ctx);
    ctx = NULL;
}

START_TEST(test_boolean_true)
{
    //struct handlebars_value * value = handlebars_value_ctor();
}
END_TEST

START_TEST(test_boolean_false)
{

}
END_TEST

START_TEST(test_boolean_json_true)
{

}
END_TEST

START_TEST(test_boolean_json_false)
{

}
END_TEST


START_TEST(test_boolean)
{
	struct handlebars_value * value = handlebars_value_from_json_string(ctx, "true");
	ck_assert_ptr_ne(value, NULL);
	ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_BOOLEAN);
	ck_assert_int_eq(handlebars_value_get_boolval(value), 1);
}
{
	struct handlebars_value * value = handlebars_value_from_json_string(ctx, "false");
	ck_assert_ptr_ne(value, NULL);
	ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_BOOLEAN);
	ck_assert_int_eq(handlebars_value_get_boolval(value), 0);
}
END_TEST

START_TEST(test_int)
{
	struct handlebars_value * value = handlebars_value_from_json_string(ctx, "2358");
	ck_assert_ptr_ne(value, NULL);
	ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_INTEGER);
	ck_assert_int_eq(handlebars_value_get_intval(value), 2358);
}
END_TEST

START_TEST(test_float)
{
	struct handlebars_value * value = handlebars_value_from_json_string(ctx, "1234.4321");
	ck_assert_ptr_ne(value, NULL);
	ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_FLOAT);
	ck_assert_int_eq(handlebars_value_get_floatval(value), 1234.4321);
}
END_TEST

START_TEST(test_string)
{
	struct handlebars_value * value = handlebars_value_from_json_string(ctx, "\"test\"");
	ck_assert_ptr_ne(value, NULL);
	ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_STRING);
	ck_assert_str_eq(handlebars_value_get_strval(value), "test");
	ck_assert_int_eq(handlebars_value_get_strlen(value), 4);
}
END_TEST

START_TEST(test_array_find)
{
	struct handlebars_value * value2;
	struct handlebars_value * value = handlebars_value_from_json_string(ctx, "[2358, \"test\"]");
	ck_assert_ptr_ne(value, NULL);
	ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_ARRAY);

	value2 = handlebars_value_array_find(value, 0);
	ck_assert_ptr_ne(value2, NULL);
	ck_assert_int_eq(handlebars_value_get_type(value2), HANDLEBARS_VALUE_TYPE_INTEGER);
	ck_assert_int_eq(handlebars_value_get_intval(value2), 2358);

	value2 = handlebars_value_array_find(value, 1);
	ck_assert_ptr_ne(value2, NULL);
	ck_assert_int_eq(handlebars_value_get_type(value2), HANDLEBARS_VALUE_TYPE_STRING);
	ck_assert_str_eq(handlebars_value_get_strval(value2), "test");
	ck_assert_int_eq(handlebars_value_get_strlen(value2), 4);

	value2 = handlebars_value_array_find(value, 2);
	ck_assert_ptr_eq(value2, NULL);
}
END_TEST

START_TEST(test_map_find)
{
	struct handlebars_value * value2;
	struct handlebars_value * value = handlebars_value_from_json_string(ctx, "{\"a\": 2358, \"b\": \"test\"}");
	ck_assert_ptr_ne(value, NULL);
	ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_MAP);

	value2 = handlebars_value_map_find(value, "a", sizeof("a") - 1);
	ck_assert_ptr_ne(value2, NULL);
	ck_assert_int_eq(handlebars_value_get_type(value2), HANDLEBARS_VALUE_TYPE_INTEGER);
	ck_assert_int_eq(handlebars_value_get_intval(value2), 2358);

	value2 = handlebars_value_map_find(value, "b", sizeof("b") - 1);
	ck_assert_ptr_ne(value2, NULL);
	ck_assert_int_eq(handlebars_value_get_type(value2), HANDLEBARS_VALUE_TYPE_STRING);
	ck_assert_str_eq(handlebars_value_get_strval(value2), "test");
	ck_assert_int_eq(handlebars_value_get_strlen(value2), 4);

	value2 = handlebars_value_map_find(value, "c", sizeof("c") - 1);
	ck_assert_ptr_eq(value2, NULL);

}
END_TEST

START_TEST(test_complex)
{
	struct handlebars_value * value2;
	struct handlebars_value * value3;
	struct handlebars_value * value = handlebars_value_from_json_string(ctx, "{\"a\": 2358, \"b\": [1, 2.1], \"c\": {\"d\": \"test\"}}");
	ck_assert_ptr_ne(value, NULL);
	ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_MAP);

	value2 = handlebars_value_map_find(value, "a", sizeof("a") - 1);
	ck_assert_ptr_ne(value2, NULL);
	ck_assert_int_eq(handlebars_value_get_type(value2), HANDLEBARS_VALUE_TYPE_INTEGER);
	ck_assert_int_eq(handlebars_value_get_intval(value2), 2358);

	value2 = handlebars_value_map_find(value, "b", sizeof("b") - 1);
	ck_assert_ptr_ne(value2, NULL);
	ck_assert_int_eq(handlebars_value_get_type(value2), HANDLEBARS_VALUE_TYPE_ARRAY);

	value3 = handlebars_value_array_find(value2, 0);
	ck_assert_ptr_ne(value3, NULL);
	ck_assert_int_eq(handlebars_value_get_type(value3), HANDLEBARS_VALUE_TYPE_INTEGER);

	value3 = handlebars_value_array_find(value2, 1);
	ck_assert_ptr_ne(value3, NULL);
	ck_assert_int_eq(handlebars_value_get_type(value3), HANDLEBARS_VALUE_TYPE_FLOAT);

	value2 = handlebars_value_map_find(value, "c", sizeof("c") - 1);
	ck_assert_ptr_ne(value2, NULL);
	ck_assert_int_eq(handlebars_value_get_type(value2), HANDLEBARS_VALUE_TYPE_MAP);

	value3 = handlebars_value_map_find(value2, "d", sizeof("d") - 1);
	ck_assert_int_eq(handlebars_value_get_type(value3), HANDLEBARS_VALUE_TYPE_STRING);
	ck_assert_str_eq(handlebars_value_get_strval(value3), "test");
	ck_assert_int_eq(handlebars_value_get_strlen(value3), 4);
}
END_TEST

Suite * parser_suite(void)
{
    Suite * s = suite_create("Value");

    REGISTER_TEST_FIXTURE(s, test_boolean, "Boolean");
    REGISTER_TEST_FIXTURE(s, test_int, "Integer");
    REGISTER_TEST_FIXTURE(s, test_float, "Float");
    REGISTER_TEST_FIXTURE(s, test_string, "String");
    REGISTER_TEST_FIXTURE(s, test_array_find, "Array Find");
    REGISTER_TEST_FIXTURE(s, test_map_find, "Map Find");
    REGISTER_TEST_FIXTURE(s, test_complex, "Complex");

    return s;
}

int main(void)
{
    int number_failed;
    int memdebug;
    int error;

    // Check if memdebug enabled
    memdebug = getenv("MEMDEBUG") ? atoi(getenv("MEMDEBUG")) : 0;
    if( memdebug ) {
        talloc_enable_leak_report_full();
    }

    // Set up test suite
    Suite * s = parser_suite();
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
