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

#include "handlebars_memory.h"
#include "handlebars_value.h"
#include "handlebars_yaml.h"
#include "utils.h"


static void assert_yaml_error(
    const char * yaml,
    size_t length,
    const char * expected
)
{
    HANDLEBARS_VALUE_DECL(value);
    jmp_buf * previous = context->e->jmp;
    const char * volatile expected_message = expected;
    const size_t blocks_before = talloc_total_blocks(context);
    jmp_buf buf;

    if( handlebars_setjmp_ex(context, &buf) ) {
        context->e->jmp = previous;
        ck_assert_msg(
            strstr(handlebars_error_msg(context), expected_message) != NULL,
            "Expected YAML error containing '%s', got '%s'",
            expected_message,
            handlebars_error_msg(context)
        );
        HANDLEBARS_VALUE_UNDECL(value);
        ck_assert_uint_eq(talloc_total_blocks(context), blocks_before + 1);
        return;
    }

    handlebars_value_init_yaml_stringl(context, value, yaml, length);
    context->e->jmp = previous;
    HANDLEBARS_VALUE_UNDECL(value);
    ck_abort_msg("Expected YAML conversion to fail with '%s'", expected_message);
}


START_TEST(test_boolean_yaml_true)
{
    HANDLEBARS_VALUE_DECL(value);
    handlebars_value_init_yaml_string(context, value, "---\ntrue");
    ck_assert_ptr_ne(value, NULL);
    ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_TRUE);
    ck_assert_int_eq(handlebars_value_get_boolval(value), 1);
    HANDLEBARS_VALUE_UNDECL(value);
    ASSERT_INIT_BLOCKS();
}
END_TEST

START_TEST(test_boolean_yaml_false)
{
    HANDLEBARS_VALUE_DECL(value);
    handlebars_value_init_yaml_string(context, value, "---\nfalse");
    ck_assert_ptr_ne(value, NULL);
    ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_FALSE);
    ck_assert_int_eq(handlebars_value_get_boolval(value), 0);
    HANDLEBARS_VALUE_UNDECL(value);
    ASSERT_INIT_BLOCKS();
}
END_TEST

START_TEST(test_int_yaml)
{
    HANDLEBARS_VALUE_DECL(value);
    handlebars_value_init_yaml_string(context, value, "---\n2358");
    ck_assert_ptr_ne(value, NULL);
    ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_INTEGER);
    ck_assert_int_eq(handlebars_value_get_intval(value), 2358);
    HANDLEBARS_VALUE_UNDECL(value);
    ASSERT_INIT_BLOCKS();
}
END_TEST

START_TEST(test_float_yaml)
{
    HANDLEBARS_VALUE_DECL(value);
    handlebars_value_init_yaml_string(context, value, "---\n1234.4321");
    ck_assert_ptr_ne(value, NULL);
    ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_FLOAT);
    // Note: converting to int - precision issue
    ck_assert_int_eq(handlebars_value_get_floatval(value), 1234.4321);
    HANDLEBARS_VALUE_UNDECL(value);
    ASSERT_INIT_BLOCKS();
}
END_TEST

START_TEST(test_string_yaml)
{
    HANDLEBARS_VALUE_DECL(value);
	handlebars_value_init_yaml_string(context, value, "---\n\"test\"");
	ck_assert_ptr_ne(value, NULL);
	ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_STRING);
    const char * tmp = handlebars_value_get_strval(value);
	ck_assert_str_eq(tmp, "test");
	ck_assert_int_eq(handlebars_value_get_strlen(value), 4);
    HANDLEBARS_VALUE_UNDECL(value);
    ASSERT_INIT_BLOCKS();
}
END_TEST

START_TEST(test_parse_error_yaml)
{
    assert_yaml_error(HBS_STRL("---\n'"), "YAML Parse Error");
}
END_TEST

START_TEST(test_empty_document_yaml)
{
    assert_yaml_error(HBS_STRL(""), "YAML Parse Error: empty document");
}
END_TEST

START_TEST(test_complex_mapping_key_yaml)
{
    assert_yaml_error(
        HBS_STRL("---\n? [a, b]\n: value\n"),
        "Unsupported YAML mapping key type"
    );
}
END_TEST

START_TEST(test_cyclic_alias_yaml)
{
    assert_yaml_error(
        HBS_STRL("--- &root\n- *root\n"),
        "Cyclic YAML alias reference"
    );
}
END_TEST

START_TEST(test_deeply_nested_yaml)
{
    const size_t depth = 300;
    const size_t length = depth * 2 + 1;
    char * yaml = talloc_array(context, char, length);

    ck_assert_ptr_nonnull(yaml);
    memset(yaml, '[', depth);
    yaml[depth] = '0';
    memset(yaml + depth + 1, ']', depth);
    assert_yaml_error(yaml, length, "YAML nesting depth exceeds limit");
    talloc_free(yaml);
}
END_TEST

START_TEST(test_shared_alias_yaml)
{
    HANDLEBARS_VALUE_DECL(rv);
    HANDLEBARS_VALUE_DECL(rv2);
    HANDLEBARS_VALUE_DECL(value);
    struct handlebars_value * copy;
    struct handlebars_value * name;

    handlebars_value_init_yaml_string(
        context,
        value,
        "---\nbase: &base\n  name: test\ncopy: *base\n"
    );
    ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_MAP);

    copy = handlebars_value_map_str_find(value, HBS_STRL("copy"), rv);
    ck_assert_ptr_nonnull(copy);
    ck_assert_int_eq(handlebars_value_get_type(copy), HANDLEBARS_VALUE_TYPE_MAP);

    name = handlebars_value_map_str_find(copy, HBS_STRL("name"), rv2);
    ck_assert_ptr_nonnull(name);
    ck_assert_int_eq(handlebars_value_get_type(name), HANDLEBARS_VALUE_TYPE_STRING);
    ck_assert_str_eq(handlebars_value_get_strval(name), "test");

    HANDLEBARS_VALUE_UNDECL(value);
    HANDLEBARS_VALUE_UNDECL(rv2);
    HANDLEBARS_VALUE_UNDECL(rv);
    ASSERT_INIT_BLOCKS();
}
END_TEST

START_TEST(test_duplicate_mapping_key_yaml)
{
    HANDLEBARS_VALUE_DECL(rv);
    HANDLEBARS_VALUE_DECL(rv2);
    HANDLEBARS_VALUE_DECL(value);
    struct handlebars_value * entry;
    struct handlebars_value * name;

    handlebars_value_init_yaml_string(
        context,
        value,
        "---\nentry:\n  name: old\nentry:\n  name: new\n"
    );
    entry = handlebars_value_map_str_find(value, HBS_STRL("entry"), rv);
    ck_assert_ptr_nonnull(entry);
    name = handlebars_value_map_str_find(entry, HBS_STRL("name"), rv2);
    ck_assert_ptr_nonnull(name);
    ck_assert_str_eq(handlebars_value_get_strval(name), "new");

    HANDLEBARS_VALUE_UNDECL(value);
    HANDLEBARS_VALUE_UNDECL(rv2);
    HANDLEBARS_VALUE_UNDECL(rv);
    ASSERT_INIT_BLOCKS();
}
END_TEST

START_TEST(test_length_delimited_yaml)
{
    const char yaml[] = {'-', '-', '-', '\n', 'a', ':', ' ', '1', '\0', 'x'};

    assert_yaml_error(yaml, sizeof(yaml), "YAML Parse Error");
}
END_TEST

static Suite * suite(void);
static Suite * suite(void)
{
    Suite * s = suite_create("YAML");

    REGISTER_TEST_FIXTURE(s, test_boolean_yaml_true, "Boolean - true");
    REGISTER_TEST_FIXTURE(s, test_boolean_yaml_false, "Boolean - false");
    REGISTER_TEST_FIXTURE(s, test_int_yaml, "Integer");
    REGISTER_TEST_FIXTURE(s, test_float_yaml, "Float");
    REGISTER_TEST_FIXTURE(s, test_string_yaml, "String");
    REGISTER_TEST_FIXTURE(s, test_parse_error_yaml, "YAML Parse Error");
    REGISTER_TEST_FIXTURE(s, test_empty_document_yaml, "Empty document");
    REGISTER_TEST_FIXTURE(s, test_complex_mapping_key_yaml, "Complex mapping key");
    REGISTER_TEST_FIXTURE(s, test_cyclic_alias_yaml, "Cyclic alias");
    REGISTER_TEST_FIXTURE(s, test_deeply_nested_yaml, "Deeply nested document");
    REGISTER_TEST_FIXTURE(s, test_shared_alias_yaml, "Shared alias");
    REGISTER_TEST_FIXTURE(s, test_duplicate_mapping_key_yaml, "Duplicate mapping key");
    REGISTER_TEST_FIXTURE(s, test_length_delimited_yaml, "Length-delimited input");

    return s;
}

int main(void)
{
    return default_main(&suite);
}
