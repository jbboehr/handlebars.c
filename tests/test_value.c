
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <check.h>
#include <stdio.h>
#include <talloc.h>

#if defined(HAVE_JSON_C_JSON_H)
#include <json-c/json.h>
#include <json-c/json_object.h>
#include <json-c/json_tokener.h>
#elif defined(HAVE_JSON_JSON_H)
#include <json/json.h>
#include <json/json_object.h>
#include <json/json_tokener.h>
#endif

#include "handlebars_memory.h"
#include "handlebars_value.h"
#include "utils.h"



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
    struct handlebars_value * value = handlebars_value_from_json_string(context, "true");
    ck_assert_ptr_ne(value, NULL);
    ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_TRUE);
    ck_assert_int_eq(handlebars_value_get_boolval(value), 1);
#ifndef HANDLEBARS_NO_REFCOUNT
    ck_assert_int_eq(0, handlebars_value_delref(value));
    ck_assert_int_eq(init_blocks, talloc_total_blocks(context));
#endif
}
END_TEST

START_TEST(test_boolean_yaml_true)
{
    struct handlebars_value * value = handlebars_value_from_yaml_string(context, "---\ntrue");
    ck_assert_ptr_ne(value, NULL);
    ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_TRUE);
    ck_assert_int_eq(handlebars_value_get_boolval(value), 1);
#ifndef HANDLEBARS_NO_REFCOUNT
    ck_assert_int_eq(0, handlebars_value_delref(value));
    ck_assert_int_eq(init_blocks, talloc_total_blocks(context));
#endif
}
END_TEST

START_TEST(test_boolean_json_false)
{
    struct handlebars_value * value = handlebars_value_from_json_string(context, "false");
    ck_assert_ptr_ne(value, NULL);
    ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_FALSE);
    ck_assert_int_eq(handlebars_value_get_boolval(value), 0);
#ifndef HANDLEBARS_NO_REFCOUNT
    ck_assert_int_eq(0, handlebars_value_delref(value));
    ck_assert_int_eq(init_blocks, talloc_total_blocks(context));
#endif
}
END_TEST

START_TEST(test_int)
{
    struct handlebars_value * value = handlebars_value_from_json_string(context, "2358");
    ck_assert_ptr_ne(value, NULL);
    ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_INTEGER);
    ck_assert_int_eq(handlebars_value_get_intval(value), 2358);
#ifndef HANDLEBARS_NO_REFCOUNT
    ck_assert_int_eq(0, handlebars_value_delref(value));
    ck_assert_int_eq(init_blocks, talloc_total_blocks(context));
#endif
}
END_TEST

START_TEST(test_int_yaml)
{
    struct handlebars_value * value = handlebars_value_from_yaml_string(context, "---\n2358");
    ck_assert_ptr_ne(value, NULL);
    ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_INTEGER);
    ck_assert_int_eq(handlebars_value_get_intval(value), 2358);
#ifndef HANDLEBARS_NO_REFCOUNT
    ck_assert_int_eq(0, handlebars_value_delref(value));
    ck_assert_int_eq(init_blocks, talloc_total_blocks(context));
#endif
}
END_TEST

START_TEST(test_float)
{
    struct handlebars_value * value = handlebars_value_from_json_string(context, "1234.4321");
    ck_assert_ptr_ne(value, NULL);
    ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_FLOAT);
    // Note: converting to int - precision issue
    ck_assert_int_eq(handlebars_value_get_floatval(value), 1234.4321);
#ifndef HANDLEBARS_NO_REFCOUNT
    ck_assert_int_eq(0, handlebars_value_delref(value));
    ck_assert_int_eq(init_blocks, talloc_total_blocks(context));
#endif
}
END_TEST

START_TEST(test_float_yaml)
{
    struct handlebars_value * value = handlebars_value_from_yaml_string(context, "---\n1234.4321");
    ck_assert_ptr_ne(value, NULL);
    ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_FLOAT);
    // Note: converting to int - precision issue
    ck_assert_int_eq(handlebars_value_get_floatval(value), 1234.4321);
#ifndef HANDLEBARS_NO_REFCOUNT
    ck_assert_int_eq(0, handlebars_value_delref(value));
    ck_assert_int_eq(init_blocks, talloc_total_blocks(context));
#endif
}
END_TEST

START_TEST(test_string)
{
	struct handlebars_value * value = handlebars_value_from_json_string(context, "\"test\"");
	ck_assert_ptr_ne(value, NULL);
	ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_STRING);
    char * tmp = handlebars_value_get_strval(value);
	ck_assert_str_eq(tmp, "test");
    handlebars_talloc_free(tmp);
	ck_assert_int_eq(handlebars_value_get_strlen(value), 4);
#ifndef HANDLEBARS_NO_REFCOUNT
    ck_assert_int_eq(0, handlebars_value_delref(value));
    ck_assert_int_eq(init_blocks, talloc_total_blocks(context));
#endif
}
END_TEST

START_TEST(test_array_iterator)
{
    struct handlebars_value * value;
    struct handlebars_value * tmp;
    struct handlebars_value_iterator it;
    int i = 0;

    value = handlebars_value_ctor(context);
    handlebars_value_array_init(value);

    tmp = handlebars_value_ctor(context);
    handlebars_value_integer(tmp, 1);
    handlebars_stack_push(value->v.stack, tmp);
    handlebars_value_delref(tmp);

    tmp = handlebars_value_ctor(context);
    handlebars_value_integer(tmp, 2);
    handlebars_stack_push(value->v.stack, tmp);
    handlebars_value_delref(tmp);

    tmp = handlebars_value_ctor(context);
    handlebars_value_integer(tmp, 3);
    handlebars_stack_push(value->v.stack, tmp);
    handlebars_value_delref(tmp);

    handlebars_value_iterator_init(&it, value);

    ck_assert_ptr_ne(it.current, NULL);

    for( ; it.current != NULL; it.next(&it) ) {
        ck_assert_ptr_ne(it.current, NULL);
        ck_assert_int_eq(it.current->type, HANDLEBARS_VALUE_TYPE_INTEGER);
        ck_assert_int_eq(it.current->v.lval, ++i);
    }

#ifndef HANDLEBARS_NO_REFCOUNT
    ck_assert_int_eq(0, handlebars_value_delref(value));
    ck_assert_int_eq(init_blocks, talloc_total_blocks(context));
#endif
}
END_TEST

START_TEST(test_map_iterator)
{
    struct handlebars_value * value;
    struct handlebars_value * tmp;
    struct handlebars_value_iterator it;
    int i = 0;

    value = handlebars_value_ctor(context);
    handlebars_value_map_init(value);

    tmp = handlebars_value_ctor(context);
    handlebars_value_integer(tmp, 1);
    handlebars_map_str_update(value->v.map, HBS_STRL("a"), tmp);
    handlebars_value_delref(tmp);

    tmp = handlebars_value_ctor(context);
    handlebars_value_integer(tmp, 2);
    handlebars_map_str_update(value->v.map, HBS_STRL("c"), tmp);
    handlebars_value_delref(tmp);

    tmp = handlebars_value_ctor(context);
    handlebars_value_integer(tmp, 3);
    handlebars_map_str_update(value->v.map, HBS_STRL("b"), tmp);
    handlebars_value_delref(tmp);

    handlebars_value_iterator_init(&it, value);

    ck_assert_ptr_ne(it.current, NULL);

    for( ; it.current != NULL; it.next(&it) ) {
        ++i;
        ck_assert_ptr_ne(it.current, NULL);
        ck_assert_int_eq(it.current->type, HANDLEBARS_VALUE_TYPE_INTEGER);
        ck_assert_ptr_ne(it.key, NULL);
        switch( i ) {
            case 1: ck_assert_str_eq(it.key->val, "a"); break;
            case 2: ck_assert_str_eq(it.key->val, "c"); break;
            case 3: ck_assert_str_eq(it.key->val, "b"); break;
        }
        ck_assert_int_eq(it.current->v.lval, i);
    }

#ifndef HANDLEBARS_NO_REFCOUNT
    ck_assert_int_eq(0, handlebars_value_delref(value));
    ck_assert_int_eq(init_blocks, talloc_total_blocks(context));
#endif
}
END_TEST

START_TEST(test_array_iterator_json)
{
    struct handlebars_value * value = handlebars_value_from_json_string(context, "[1, 2, 3]");
    struct handlebars_value_iterator it;
    int i = 0;

    handlebars_value_iterator_init(&it, value);
    ck_assert_ptr_ne(it.current, NULL);

    for( ; it.current != NULL; it.next(&it) ) {
        ck_assert_ptr_ne(it.current, NULL);
        ck_assert_int_eq(it.current->type, HANDLEBARS_VALUE_TYPE_INTEGER);
        ck_assert_int_eq(it.current->v.lval, ++i);
    }

#ifndef HANDLEBARS_NO_REFCOUNT
    ck_assert_int_eq(0, handlebars_value_delref(value));
    ck_assert_int_eq(init_blocks, talloc_total_blocks(context));
#endif
}
END_TEST

START_TEST(test_map_iterator_json)
{
    struct handlebars_value * value = handlebars_value_from_json_string(context, "{\"a\": 1, \"c\": 2, \"b\": 3}");
    struct handlebars_value_iterator it;
    int i = 0;

    handlebars_value_iterator_init(&it, value);
    ck_assert_ptr_ne(it.current, NULL);

    for( ; it.current != NULL; it.next(&it) ) {
        ++i;
        ck_assert_ptr_ne(it.current, NULL);
        ck_assert_int_eq(it.current->type, HANDLEBARS_VALUE_TYPE_INTEGER);
        ck_assert_ptr_ne(it.key, NULL);
        switch( i ) {
            case 1: ck_assert_str_eq(it.key->val, "a"); break;
            case 2: ck_assert_str_eq(it.key->val, "c"); break;
            case 3: ck_assert_str_eq(it.key->val, "b"); break;
        }
        ck_assert_int_eq(it.current->v.lval, i);
    }

#ifndef HANDLEBARS_NO_REFCOUNT
    ck_assert_int_eq(0, handlebars_value_delref(value));
    ck_assert_int_eq(init_blocks, talloc_total_blocks(context));
#endif
}
END_TEST

START_TEST(test_array_find)
{
	struct handlebars_value * value2;
	struct handlebars_value * value = handlebars_value_from_json_string(context, "[2358, \"test\"]");
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
    char * tmp = handlebars_value_get_strval(value2);
	ck_assert_str_eq(tmp, "test");
    handlebars_talloc_free(tmp);
	ck_assert_int_eq(handlebars_value_get_strlen(value2), 4);
    handlebars_value_delref(value2);

	value2 = handlebars_value_array_find(value, 2);
	ck_assert_ptr_eq(value2, NULL);

#ifndef HANDLEBARS_NO_REFCOUNT
    ck_assert_int_eq(0, handlebars_value_delref(value));
    ck_assert_int_eq(init_blocks, talloc_total_blocks(context));
#endif
}
END_TEST

START_TEST(test_map_find)
{
	struct handlebars_value * value2;
	struct handlebars_value * value = handlebars_value_from_json_string(context, "{\"a\": 2358, \"b\": \"test\"}");
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
    char * tmp = handlebars_value_get_strval(value2);
	ck_assert_str_eq(tmp, "test");
    handlebars_talloc_free(tmp);
	ck_assert_int_eq(handlebars_value_get_strlen(value2), 4);
    handlebars_value_delref(value2);

	value2 = handlebars_value_map_str_find(value, HBS_STRL("c"));
	ck_assert_ptr_eq(value2, NULL);

#ifndef HANDLEBARS_NO_REFCOUNT
    ck_assert_int_eq(0, handlebars_value_delref(value));
    ck_assert_int_eq(init_blocks, talloc_total_blocks(context));
#endif
}
END_TEST

START_TEST(test_complex)
{
	struct handlebars_value * value2;
	struct handlebars_value * value3;
	struct handlebars_value * value = handlebars_value_from_json_string(context, "{\"a\": 2358, \"b\": [1, 2.1], \"c\": {\"d\": \"test\"}}");
	ck_assert_ptr_ne(value, NULL);
	ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_MAP);

	value2 = handlebars_value_map_str_find(value, HBS_STRL("a"));
	ck_assert_ptr_ne(value2, NULL);
	ck_assert_int_eq(handlebars_value_get_type(value2), HANDLEBARS_VALUE_TYPE_INTEGER);
	ck_assert_int_eq(handlebars_value_get_intval(value2), 2358);
    handlebars_value_delref(value2);

	value2 = handlebars_value_map_str_find(value, HBS_STRL("b"));
	ck_assert_ptr_ne(value2, NULL);
	ck_assert_int_eq(handlebars_value_get_type(value2), HANDLEBARS_VALUE_TYPE_ARRAY);

    do {
        value3 = handlebars_value_array_find(value2, 0);
        ck_assert_ptr_ne(value3, NULL);
        ck_assert_int_eq(handlebars_value_get_type(value3), HANDLEBARS_VALUE_TYPE_INTEGER);
        handlebars_value_delref(value3);

        value3 = handlebars_value_array_find(value2, 1);
        ck_assert_ptr_ne(value3, NULL);
        ck_assert_int_eq(handlebars_value_get_type(value3), HANDLEBARS_VALUE_TYPE_FLOAT);
        handlebars_value_delref(value3);
    } while(0);
    handlebars_value_delref(value2);

	value2 = handlebars_value_map_str_find(value, HBS_STRL("c"));
	ck_assert_ptr_ne(value2, NULL);
	ck_assert_int_eq(handlebars_value_get_type(value2), HANDLEBARS_VALUE_TYPE_MAP);

    do {
        value3 = handlebars_value_map_str_find(value2, HBS_STRL("d"));
        ck_assert_int_eq(handlebars_value_get_type(value3), HANDLEBARS_VALUE_TYPE_STRING);
        char * tmp = handlebars_value_get_strval(value3);
        ck_assert_str_eq(tmp, "test");
        handlebars_talloc_free(tmp);
        ck_assert_int_eq(handlebars_value_get_strlen(value3), 4);
        handlebars_value_delref(value3);
    } while(0);
    handlebars_value_delref(value2);

#ifndef HANDLEBARS_NO_REFCOUNT
    ck_assert_int_eq(0, handlebars_value_delref(value));
    ck_assert_int_eq(init_blocks, talloc_total_blocks(context));
#endif
}
END_TEST

START_TEST(test_convert)
{
    struct handlebars_value * value2;
    struct handlebars_value * value = handlebars_value_from_json_string(context, "{\"a\": 2358, \"b\": [1, 2.1], \"c\": {\"d\": \"test\"}}");
    handlebars_value_convert(value);

    ck_assert_int_eq(value->type, HANDLEBARS_VALUE_TYPE_MAP);

    value2 = handlebars_value_map_str_find(value, HBS_STRL("b"));
    ck_assert_ptr_ne(value2, NULL);
    ck_assert_int_eq(value2->type, HANDLEBARS_VALUE_TYPE_ARRAY);
    handlebars_value_delref(value2);

#ifndef HANDLEBARS_NO_REFCOUNT
    ck_assert_int_eq(0, handlebars_value_delref(value));
    ck_assert_int_eq(init_blocks, talloc_total_blocks(context));
#endif
}
END_TEST

Suite * parser_suite(void)
{
    Suite * s = suite_create("Value");

    REGISTER_TEST_FIXTURE(s, test_boolean_json_true, "Boolean - true (JSON)");
    REGISTER_TEST_FIXTURE(s, test_boolean_json_false, "Boolean - false (JSON)");
    REGISTER_TEST_FIXTURE(s, test_boolean_yaml_true, "Boolean - true (YAML)");
    REGISTER_TEST_FIXTURE(s, test_int, "Integer");
    REGISTER_TEST_FIXTURE(s, test_int_yaml, "Integer (YAML)");
    REGISTER_TEST_FIXTURE(s, test_float, "Float");
    REGISTER_TEST_FIXTURE(s, test_float_yaml, "Float (YAML)");
    REGISTER_TEST_FIXTURE(s, test_string, "String");
    REGISTER_TEST_FIXTURE(s, test_array_iterator, "Array iterator");
    REGISTER_TEST_FIXTURE(s, test_map_iterator, "Map iterator");
    REGISTER_TEST_FIXTURE(s, test_array_iterator_json, "Array iterator (JSON)");
    REGISTER_TEST_FIXTURE(s, test_map_iterator_json, "Map iterator (JSON)");
    REGISTER_TEST_FIXTURE(s, test_array_find, "Array Find");
    REGISTER_TEST_FIXTURE(s, test_map_find, "Map Find");
    REGISTER_TEST_FIXTURE(s, test_complex, "Complex");
    REGISTER_TEST_FIXTURE(s, test_convert, "Convert");

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
