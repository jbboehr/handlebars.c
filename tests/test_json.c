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
#include <json.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
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

START_TEST(test_null_json_object)
{
    HANDLEBARS_VALUE_DECL(value);
    handlebars_value_init_json_object(context, value, NULL);
    ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_NULL);
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

START_TEST(test_empty_array_iterator_json)
{
    HANDLEBARS_VALUE_DECL(value);
    int count = 0;

    handlebars_value_init_json_string(context, value, "[]");
    HANDLEBARS_VALUE_FOREACH(value, child) {
        (void) child;
        count++;
    } HANDLEBARS_VALUE_FOREACH_END();

    ck_assert_int_eq(count, 0);
    ck_assert_int_eq(handlebars_value_count(value), 0);
    HANDLEBARS_VALUE_UNDECL(value);
    ASSERT_INIT_BLOCKS();
}
END_TEST

START_TEST(test_array_iterator_json_replaces_value_with_null)
{
    HANDLEBARS_VALUE_DECL(value);
    size_t count = 0;

    handlebars_value_init_json_string(context, value, "[1, null]");
    HANDLEBARS_VALUE_FOREACH(value, child) {
        if( count == 0 ) {
            ck_assert_int_eq(handlebars_value_get_type(child), HANDLEBARS_VALUE_TYPE_INTEGER);
            ck_assert_int_eq(handlebars_value_get_intval(child), 1);
        } else {
            ck_assert_int_eq(handlebars_value_get_type(child), HANDLEBARS_VALUE_TYPE_NULL);
        }
        count++;
    } HANDLEBARS_VALUE_FOREACH_END();

    ck_assert_uint_eq(count, 2);
    HANDLEBARS_VALUE_UNDECL(value);
    ASSERT_INIT_BLOCKS();
}
END_TEST

START_TEST(test_array_iterator_json_retains_owner)
{
    HANDLEBARS_VALUE_DECL(value);
    HANDLEBARS_VALUE_ITERATOR_DECL(iter);

    handlebars_value_init_json_string(context, value, "[1, 2]");
    ck_assert(HANDLEBARS_VALUE_ITERATOR_INIT(iter, value));
    handlebars_value_dtor(value);

    ck_assert_int_eq(handlebars_value_get_intval(iter->cur), 1);
    ck_assert(handlebars_value_iterator_next(iter));
    ck_assert_int_eq(handlebars_value_get_intval(iter->cur), 2);
    ck_assert(!handlebars_value_iterator_next(iter));

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

START_TEST(test_empty_map_iterator_json)
{
    HANDLEBARS_VALUE_DECL(value);
    int count = 0;

    handlebars_value_init_json_string(context, value, "{}");
    HANDLEBARS_VALUE_FOREACH(value, child) {
        (void) child;
        count++;
    } HANDLEBARS_VALUE_FOREACH_END();

    ck_assert_int_eq(count, 0);
    ck_assert_int_eq(handlebars_value_count(value), 0);
    HANDLEBARS_VALUE_UNDECL(value);
    ASSERT_INIT_BLOCKS();
}
END_TEST

START_TEST(test_map_iterator_json_break_cleanup)
{
    HANDLEBARS_VALUE_DECL(value);
    int count = 0;

    handlebars_value_init_json_string(context, value, "{\"a\": 1, \"b\": 2}");
    HANDLEBARS_VALUE_FOREACH_KV(value, key, child) {
        ck_assert_ptr_nonnull(key);
        ck_assert_ptr_nonnull(child);
        count++;
        break;
    } HANDLEBARS_VALUE_FOREACH_END();

    ck_assert_int_eq(count, 1);
    HANDLEBARS_VALUE_UNDECL(value);
    ASSERT_INIT_BLOCKS();
}
END_TEST

START_TEST(test_map_iterator_json_retains_owner)
{
    HANDLEBARS_VALUE_DECL(value);
    HANDLEBARS_VALUE_ITERATOR_DECL(iter);

    handlebars_value_init_json_string(context, value, "{\"a\": 1, \"b\": 2}");
    ck_assert(HANDLEBARS_VALUE_ITERATOR_INIT(iter, value));
    handlebars_value_dtor(value);

    ck_assert_hbs_str_eq_cstr(iter->key, "a");
    ck_assert_int_eq(handlebars_value_get_intval(iter->cur), 1);
    ck_assert(handlebars_value_iterator_next(iter));
    ck_assert_hbs_str_eq_cstr(iter->key, "b");
    ck_assert_int_eq(handlebars_value_get_intval(iter->cur), 2);
    ck_assert(!handlebars_value_iterator_next(iter));

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

    size_t blocks = talloc_total_blocks(context);
    for( size_t i = 0; i < 100; i++ ) {
        ck_assert_ptr_null(handlebars_value_map_str_find(value, HBS_STRL("missing"), rv));
    }
    ck_assert_uint_eq(talloc_total_blocks(context), blocks);

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

START_TEST(test_json_expression_dispatch)
{
    HANDLEBARS_VALUE_DECL(alias);
    HANDLEBARS_VALUE_DECL(other);
    HANDLEBARS_VALUE_DECL(value);
    struct handlebars_string * expression;
    struct handlebars_string * prefix;
    struct handlebars_string * result;
    size_t blocks_before;
#ifdef HANDLEBARS_MEMORY
    int allocation_calls;
#endif

    handlebars_value_init_json_string(context, value, "{\"a\": 1}");
    handlebars_value_value(alias, value);
    ck_assert(handlebars_value_eq(value, alias));
    handlebars_value_init_json_string(context, other, "{\"a\": 1}");
    ck_assert(!handlebars_value_eq(value, other));

    prefix = handlebars_string_ctor(context, HBS_STRL("prefix:"));
    blocks_before = talloc_total_blocks(context);
    result = handlebars_value_expression_append(context, value, prefix, false);
    ck_assert_ptr_eq(result, prefix);
    ck_assert_hbs_str_eq_cstr(result, "prefix:");
    ck_assert_uint_eq(talloc_total_blocks(context), blocks_before);
    handlebars_talloc_free(result);

#ifdef HANDLEBARS_MEMORY
    prefix = handlebars_string_ctor(context, HBS_STRL("prefix:"));
    handlebars_memory_fail_set_flags(handlebars_memory_fail_flag_alloc);
    handlebars_memory_fail_counter(1);
    result = handlebars_value_expression_append(context, value, prefix, false);
    allocation_calls = handlebars_memory_get_call_counter();
    handlebars_memory_fail_disable();
    ck_assert_ptr_eq(result, prefix);
    ck_assert_int_eq(allocation_calls, 0);
    handlebars_talloc_free(result);
#endif

    expression = handlebars_value_expression(context, value, false);
    ck_assert_hbs_str_eq_cstr(expression, "");
    handlebars_talloc_free(expression);

    handlebars_value_init_json_string(context, value, "[1, \"x\", true]");
    result = handlebars_value_expression_append(
        context,
        value,
        handlebars_string_ctor(context, HBS_STRL("prefix:")),
        false
    );
    ck_assert_hbs_str_eq_cstr(result, "prefix:1,x,true");
    handlebars_talloc_free(result);

    expression = handlebars_value_expression(context, value, false);
    ck_assert_hbs_str_eq_cstr(expression, "1,x,true");
    handlebars_talloc_free(expression);

    HANDLEBARS_VALUE_UNDECL(value);
    HANDLEBARS_VALUE_UNDECL(other);
    HANDLEBARS_VALUE_UNDECL(alias);
    ASSERT_INIT_BLOCKS();
}
END_TEST

static void assert_json_convert_rejected(
    struct handlebars_value * value,
    const char * expected_error
) {
    jmp_buf * volatile previous = context->e->jmp;
    jmp_buf buf;

    if( handlebars_setjmp_ex(context, &buf) ) {
        context->e->jmp = previous;
        ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_ERROR);
        ck_assert_ptr_nonnull(strstr(handlebars_error_msg(context), expected_error));
        clear_intentional_error();
        return;
    }

    handlebars_value_convert(value);
    context->e->jmp = previous;
    ck_abort_msg("Expected recursive JSON conversion to be rejected");
}

START_TEST(test_convert_json_rejects_cycle)
{
    HANDLEBARS_VALUE_DECL(value);
    struct json_object * json = json_object_new_object();
    struct json_object * child = json_object_new_object();

    ck_assert_ptr_nonnull(json);
    ck_assert_ptr_nonnull(child);
    ck_assert_int_eq(json_object_object_add(json, "child", child), 0);
    json_object_get(json);
    ck_assert_int_eq(json_object_object_add(child, "parent", json), 0);
    handlebars_value_init_json_object(context, value, json);
    json_object_put(json);

    assert_json_convert_rejected(value, "Cyclic JSON value reference");

    json_object_object_del(child, "parent");
    HANDLEBARS_VALUE_UNDECL(value);
    ASSERT_INIT_BLOCKS();
}
END_TEST

START_TEST(test_convert_json_rejects_excessive_depth)
{
    HANDLEBARS_VALUE_DECL(value);
    struct json_object * json = json_object_new_int(1);

    ck_assert_ptr_nonnull(json);
    for( size_t i = 0; i < HANDLEBARS_VALUE_MAX_DEPTH + 1; i++ ) {
        struct json_object * parent = json_object_new_array();

        ck_assert_ptr_nonnull(parent);
        ck_assert_int_eq(json_object_array_add(parent, json), 0);
        json = parent;
    }
    handlebars_value_init_json_object(context, value, json);
    json_object_put(json);

    assert_json_convert_rejected(value, "maximum depth");

    HANDLEBARS_VALUE_UNDECL(value);
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

START_TEST(test_json_try_returns_errors_without_longjmp)
{
    HANDLEBARS_VALUE_DECL(value);
    struct json_object * object;
    jmp_buf * previous = context->e->jmp;
    jmp_buf outer;
    enum handlebars_error_type error;

    if( setjmp(outer) != 0 ) {
        context->e->jmp = previous;
        ck_abort_msg("A JSON try conversion escaped through longjmp");
    }
    context->e->jmp = &outer;
    handlebars_value_integer(value, 42);

    error = handlebars_value_init_json_string_try(context, value, "{\"key\":1");

    ck_assert_int_eq(error, HANDLEBARS_ERROR);
    ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_INTEGER);
    ck_assert_int_eq(handlebars_value_get_intval(value), 42);
    ck_assert_ptr_eq(context->e->jmp, &outer);
    ck_assert_ptr_nonnull(strstr(handlebars_error_msg(context), "JSON Parse error"));

    error = handlebars_value_init_json_stringl_try(
        context,
        value,
        "0",
        (size_t) INT_MAX + 1
    );

    ck_assert_int_eq(error, HANDLEBARS_ERROR);
    ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_INTEGER);
    ck_assert_int_eq(handlebars_value_get_intval(value), 42);
    ck_assert_ptr_eq(context->e->jmp, &outer);
    ck_assert_ptr_nonnull(strstr(handlebars_error_msg(context), "length exceeds parser limit"));

    error = handlebars_value_init_json_stringl_try(
        context,
        value,
        HBS_STRL("[1,2]")
    );

    ck_assert_int_eq(error, HANDLEBARS_SUCCESS);
    ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_ARRAY);
    ck_assert_int_eq(handlebars_value_count(value), 2);
    ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_SUCCESS);
    ck_assert_ptr_null(handlebars_error_msg(context));
    ck_assert_ptr_eq(context->e->jmp, &outer);

    object = json_object_new_int64(7);
    ck_assert_ptr_nonnull(object);
    error = handlebars_value_init_json_object_try(context, value, object);
    json_object_put(object);

    ck_assert_int_eq(error, HANDLEBARS_SUCCESS);
    ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_INTEGER);
    ck_assert_int_eq(handlebars_value_get_intval(value), 7);
    ck_assert_ptr_eq(context->e->jmp, &outer);

    context->e->jmp = previous;
    HANDLEBARS_VALUE_UNDECL(value);
    ASSERT_INIT_BLOCKS();
}
END_TEST

START_TEST(test_json_try_reuse_and_object_lifetime)
{
    static const char * malformed[] = {
        "{",
        "[1,",
        "{\"key\":}"
    };
    HANDLEBARS_VALUE_DECL(found);
    HANDLEBARS_VALUE_DECL(value);
    struct json_object * object;
    jmp_buf * previous = context->e->jmp;
    jmp_buf outer;
    enum handlebars_error_type error;

    if( setjmp(outer) != 0 ) {
        context->e->jmp = previous;
        ck_abort_msg("A reused JSON try conversion escaped through longjmp");
    }
    context->e->jmp = &outer;

    error = handlebars_value_init_json_string_try(
        context,
        value,
        "{\"keep\":\"alive\"}"
    );
    ck_assert_int_eq(error, HANDLEBARS_SUCCESS);

    for( size_t i = 0; i < sizeof(malformed) / sizeof(malformed[0]); i++ ) {
        error = handlebars_value_init_json_string_try(
            context,
            value,
            malformed[i]
        );

        ck_assert_int_eq(error, HANDLEBARS_ERROR);
        ck_assert_ptr_eq(context->e->jmp, &outer);
        ck_assert_ptr_nonnull(strstr(handlebars_error_msg(context), "JSON Parse error"));
        ck_assert_ptr_nonnull(
            handlebars_value_map_str_find(value, HBS_STRL("keep"), found)
        );
        ck_assert_str_eq(handlebars_value_get_strval(found), "alive");
    }

    object = json_object_new_object();
    ck_assert_ptr_nonnull(object);
    ck_assert_int_eq(
        json_object_object_add(
            object,
            "owned",
            json_object_new_string("after caller release")
        ),
        0
    );

    error = handlebars_value_init_json_object_try(context, value, object);
    json_object_put(object);

    ck_assert_int_eq(error, HANDLEBARS_SUCCESS);
    ck_assert_ptr_eq(context->e->jmp, &outer);
    ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_SUCCESS);
    ck_assert_ptr_null(handlebars_error_msg(context));
    ck_assert_ptr_nonnull(
        handlebars_value_map_str_find(value, HBS_STRL("owned"), found)
    );
    ck_assert_str_eq(
        handlebars_value_get_strval(found),
        "after caller release"
    );

    error = handlebars_value_init_json_object_try(context, value, NULL);
    ck_assert_int_eq(error, HANDLEBARS_SUCCESS);
    ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_NULL);
    ck_assert_ptr_eq(context->e->jmp, &outer);

    context->e->jmp = previous;
    HANDLEBARS_VALUE_UNDECL(value);
    HANDLEBARS_VALUE_UNDECL(found);
    ASSERT_INIT_BLOCKS();
}
END_TEST

START_TEST(test_json_try_handles_deep_and_cyclic_inputs)
{
    const size_t depth = 64;
    char deep[64 * 2 + 1];
    HANDLEBARS_VALUE_DECL(value);
    struct json_object * child;
    struct json_object * root_json;
    jmp_buf * previous = context->e->jmp;
    jmp_buf outer;
    enum handlebars_error_type error;

    if( setjmp(outer) != 0 ) {
        context->e->jmp = previous;
        ck_abort_msg("An adversarial JSON try conversion escaped through longjmp");
    }
    context->e->jmp = &outer;
    handlebars_value_integer(value, 42);

    memset(deep, '[', depth);
    memset(deep + depth, ']', depth);
    deep[sizeof(deep) - 1] = '\0';
    error = handlebars_value_init_json_stringl_try(
        context,
        value,
        deep,
        sizeof(deep) - 1
    );

    ck_assert_int_eq(error, HANDLEBARS_ERROR);
    ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_INTEGER);
    ck_assert_int_eq(handlebars_value_get_intval(value), 42);
    ck_assert_ptr_eq(context->e->jmp, &outer);

    root_json = json_object_new_object();
    child = json_object_new_object();
    ck_assert_ptr_nonnull(root_json);
    ck_assert_ptr_nonnull(child);
    ck_assert_int_eq(json_object_object_add(root_json, "child", child), 0);
    json_object_get(root_json);
    ck_assert_int_eq(json_object_object_add(child, "parent", root_json), 0);

    error = handlebars_value_init_json_object_try(context, value, root_json);
    json_object_put(root_json);

    ck_assert_int_eq(error, HANDLEBARS_SUCCESS);
    ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_MAP);
    ck_assert_int_eq(handlebars_value_count(value), 1);
    ck_assert_ptr_eq(context->e->jmp, &outer);

    json_object_object_del(child, "parent");
    context->e->jmp = previous;
    HANDLEBARS_VALUE_UNDECL(value);
    ASSERT_INIT_BLOCKS();
}
END_TEST

#if defined(HANDLEBARS_MEMORY) && !defined(HANDLEBARS_NO_REFCOUNT)
static void mark_json_object_deleted(
    struct json_object * object,
    void * userdata
)
{
    (void) object;
    *(bool *) userdata = true;
}
#endif

#ifdef HANDLEBARS_MEMORY
START_TEST(test_json_try_handles_allocation_failures)
{
    struct json_object * object = json_object_new_object();
    bool object_succeeded = false;
    bool string_succeeded = false;
#ifndef HANDLEBARS_NO_REFCOUNT
    bool object_deleted = false;
#endif

    ck_assert_ptr_nonnull(object);
#ifndef HANDLEBARS_NO_REFCOUNT
    json_object_set_userdata(
        object,
        &object_deleted,
        mark_json_object_deleted
    );
#endif
    ck_assert_int_eq(
        json_object_object_add(object, "key", json_object_new_string("value")),
        0
    );

    for( int fail_at = 1; fail_at <= 64; fail_at++ ) {
        HANDLEBARS_VALUE_DECL(value);
        size_t blocks_before = talloc_total_blocks(context);
        enum handlebars_error_type error;

        handlebars_value_integer(value, 42);
        handlebars_memory_fail_set_flags(handlebars_memory_fail_flag_alloc);
        handlebars_memory_fail_counter(fail_at);
        error = handlebars_value_init_json_object_try(context, value, object);
        handlebars_memory_fail_disable();

        if( error == HANDLEBARS_SUCCESS ) {
            ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_MAP);
            object_succeeded = true;
        } else {
            ck_assert_int_eq(error, HANDLEBARS_NOMEM);
            ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_INTEGER);
            ck_assert_int_eq(handlebars_value_get_intval(value), 42);
        }
        handlebars_error_clear(context);
        HANDLEBARS_VALUE_UNDECL(value);
#ifndef HANDLEBARS_NO_REFCOUNT
        ck_assert_uint_eq(talloc_total_blocks(context), blocks_before);
#endif
        if( object_succeeded ) {
            break;
        }
    }

    for( int fail_at = 1; fail_at <= 64; fail_at++ ) {
        HANDLEBARS_VALUE_DECL(value);
        size_t blocks_before = talloc_total_blocks(context);
        enum handlebars_error_type error;

        handlebars_value_integer(value, 42);
        handlebars_memory_fail_set_flags(handlebars_memory_fail_flag_alloc);
        handlebars_memory_fail_counter(fail_at);
        error = handlebars_value_init_json_stringl_try(
            context,
            value,
            HBS_STRL("{\"key\":\"value\"}")
        );
        handlebars_memory_fail_disable();

        if( error == HANDLEBARS_SUCCESS ) {
            ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_MAP);
            string_succeeded = true;
        } else {
            ck_assert_int_eq(error, HANDLEBARS_NOMEM);
            ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_INTEGER);
            ck_assert_int_eq(handlebars_value_get_intval(value), 42);
        }
        handlebars_error_clear(context);
        HANDLEBARS_VALUE_UNDECL(value);
#ifndef HANDLEBARS_NO_REFCOUNT
        ck_assert_uint_eq(talloc_total_blocks(context), blocks_before);
#endif
        if( string_succeeded ) {
            break;
        }
    }

    json_object_put(object);
#ifndef HANDLEBARS_NO_REFCOUNT
    ck_assert(object_deleted);
#endif
    ck_assert(object_succeeded);
    ck_assert(string_succeeded);
    ASSERT_INIT_BLOCKS();
}
END_TEST
#endif

static Suite * suite(void);
static Suite * suite(void)
{
    Suite * s = suite_create("JSON");

    REGISTER_TEST_FIXTURE(s, test_boolean_json_true, "Boolean - true");
    REGISTER_TEST_FIXTURE(s, test_boolean_json_false, "Boolean - false");
    REGISTER_TEST_FIXTURE(s, test_null_json_object, "Null object");
    REGISTER_TEST_FIXTURE(s, test_int_json, "Integer");
    REGISTER_TEST_FIXTURE(s, test_float_json, "Float");
    REGISTER_TEST_FIXTURE(s, test_string_json, "String");
    REGISTER_TEST_FIXTURE(s, test_array_iterator_json, "Array iterator");
    REGISTER_TEST_FIXTURE(s, test_empty_array_iterator_json, "Empty array iterator");
    REGISTER_TEST_FIXTURE(s, test_array_iterator_json_replaces_value_with_null, "Array iterator replaces its current value with null");
    REGISTER_TEST_FIXTURE(s, test_array_iterator_json_retains_owner, "Array iterator retains its JSON owner");
    REGISTER_TEST_FIXTURE(s, test_map_iterator_json, "Map iterator");
    REGISTER_TEST_FIXTURE(s, test_empty_map_iterator_json, "Empty map iterator");
    REGISTER_TEST_FIXTURE(s, test_map_iterator_json_break_cleanup, "Breaking map iteration cleans up iterator state");
    REGISTER_TEST_FIXTURE(s, test_map_iterator_json_retains_owner, "Map iterator retains its JSON owner");
    REGISTER_TEST_FIXTURE(s, test_array_find_json, "Array Find");
    REGISTER_TEST_FIXTURE(s, test_map_find_json, "Map Find");
    REGISTER_TEST_FIXTURE(s, test_complex_json, "Complex");
    REGISTER_TEST_FIXTURE(s, test_convert_json, "Convert");
    REGISTER_TEST_FIXTURE(s, test_json_expression_dispatch, "Expression dispatch");
    REGISTER_TEST_FIXTURE(s, test_convert_json_rejects_cycle, "Convert rejects cyclic JSON graphs");
    REGISTER_TEST_FIXTURE(s, test_convert_json_rejects_excessive_depth, "Convert rejects excessively deep JSON graphs");
    REGISTER_TEST_FIXTURE(s, test_parse_error_json, "JSON Parse Error");
    REGISTER_TEST_FIXTURE(s, test_json_try_returns_errors_without_longjmp, "JSON try conversion returns errors without longjmp");
    REGISTER_TEST_FIXTURE(s, test_json_try_reuse_and_object_lifetime, "JSON try conversion preserves values across reuse and owns object inputs");
    REGISTER_TEST_FIXTURE(s, test_json_try_handles_deep_and_cyclic_inputs, "JSON try conversion handles deep and cyclic inputs");
#ifdef HANDLEBARS_MEMORY
    REGISTER_TEST_FIXTURE(s, test_json_try_handles_allocation_failures, "JSON try conversion handles allocation failures");
#endif

    return s;
}

int main(void)
{
    return default_main(&suite);
}
