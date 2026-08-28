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
#include <yaml.h>

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

START_TEST(test_yaml_try_returns_errors_without_longjmp)
{
    HANDLEBARS_VALUE_DECL(value);
    yaml_document_t document;
    yaml_node_t * node;
    jmp_buf * previous = context->e->jmp;
    jmp_buf outer;
    enum handlebars_error_type error;
    int node_id;

    if( setjmp(outer) != 0 ) {
        context->e->jmp = previous;
        ck_abort_msg("A YAML try conversion escaped through longjmp");
    }
    context->e->jmp = &outer;
    handlebars_value_integer(value, 42);

    error = handlebars_value_init_yaml_string_try(context, value, "---\n'");

    ck_assert_int_eq(error, HANDLEBARS_ERROR);
    ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_INTEGER);
    ck_assert_int_eq(handlebars_value_get_intval(value), 42);
    ck_assert_ptr_eq(context->e->jmp, &outer);
    ck_assert_ptr_nonnull(strstr(handlebars_error_msg(context), "YAML Parse Error"));

    error = handlebars_value_init_yaml_stringl_try(
        context,
        value,
        HBS_STRL("---\n[1, 2]")
    );

    ck_assert_int_eq(error, HANDLEBARS_SUCCESS);
    ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_ARRAY);
    ck_assert_int_eq(handlebars_value_count(value), 2);
    ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_SUCCESS);
    ck_assert_ptr_null(handlebars_error_msg(context));
    ck_assert_ptr_eq(context->e->jmp, &outer);

    ck_assert(yaml_document_initialize(&document, NULL, NULL, NULL, 0, 0));
    node_id = yaml_document_add_scalar(
        &document,
        NULL,
        (yaml_char_t *) "7",
        1,
        YAML_PLAIN_SCALAR_STYLE
    );
    ck_assert_int_gt(node_id, 0);
    node = yaml_document_get_node(&document, node_id);
    ck_assert_ptr_nonnull(node);

    error = handlebars_value_init_yaml_node_try(
        context,
        value,
        &document,
        node
    );

    ck_assert_int_eq(error, HANDLEBARS_SUCCESS);
    ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_INTEGER);
    ck_assert_int_eq(handlebars_value_get_intval(value), 7);
    ck_assert_ptr_eq(context->e->jmp, &outer);

    yaml_document_delete(&document);
    context->e->jmp = previous;
    HANDLEBARS_VALUE_UNDECL(value);
    ASSERT_INIT_BLOCKS();
}
END_TEST

START_TEST(test_yaml_try_reuse_and_node_lifetime)
{
    static const char * malformed[] = {
        "---\n'",
        "---\n[1,",
        "---\nkey: ["
    };
    HANDLEBARS_VALUE_DECL(found);
    HANDLEBARS_VALUE_DECL(value);
    yaml_document_t document;
    yaml_document_t other_document;
    yaml_node_t * node;
    yaml_node_t * other_node;
    jmp_buf * previous = context->e->jmp;
    jmp_buf outer;
    enum handlebars_error_type error;
    int key_id;
    int mapping_id;
    int node_id;
    int other_node_id;

    if( setjmp(outer) != 0 ) {
        context->e->jmp = previous;
        ck_abort_msg("A reused YAML try conversion escaped through longjmp");
    }
    context->e->jmp = &outer;

    error = handlebars_value_init_yaml_string_try(
        context,
        value,
        "---\nkeep: alive\n"
    );
    ck_assert_int_eq(error, HANDLEBARS_SUCCESS);

    for( size_t i = 0; i < sizeof(malformed) / sizeof(malformed[0]); i++ ) {
        error = handlebars_value_init_yaml_string_try(
            context,
            value,
            malformed[i]
        );

        ck_assert_int_eq(error, HANDLEBARS_ERROR);
        ck_assert_ptr_eq(context->e->jmp, &outer);
        ck_assert_ptr_nonnull(strstr(handlebars_error_msg(context), "YAML Parse Error"));
        ck_assert_ptr_nonnull(
            handlebars_value_map_str_find(value, HBS_STRL("keep"), found)
        );
        ck_assert_str_eq(handlebars_value_get_strval(found), "alive");
    }

    ck_assert(yaml_document_initialize(&document, NULL, NULL, NULL, 0, 0));
    node_id = yaml_document_add_scalar(
        &document,
        NULL,
        (yaml_char_t *) "document node",
        -1,
        YAML_PLAIN_SCALAR_STYLE
    );
    ck_assert_int_gt(node_id, 0);

    ck_assert(yaml_document_initialize(
        &other_document,
        NULL,
        NULL,
        NULL,
        0,
        0
    ));
    other_node_id = yaml_document_add_scalar(
        &other_document,
        NULL,
        (yaml_char_t *) "other node",
        -1,
        YAML_PLAIN_SCALAR_STYLE
    );
    ck_assert_int_gt(other_node_id, 0);
    other_node = yaml_document_get_node(&other_document, other_node_id);
    ck_assert_ptr_nonnull(other_node);

    error = handlebars_value_init_yaml_node_try(
        context,
        value,
        &document,
        other_node
    );

    ck_assert_int_eq(error, HANDLEBARS_ERROR);
    ck_assert_ptr_nonnull(
        strstr(handlebars_error_msg(context), "node is not part of document")
    );
    ck_assert_ptr_nonnull(
        handlebars_value_map_str_find(value, HBS_STRL("keep"), found)
    );
    ck_assert_str_eq(handlebars_value_get_strval(found), "alive");
    ck_assert_ptr_eq(context->e->jmp, &outer);
    yaml_document_delete(&other_document);
    yaml_document_delete(&document);

    ck_assert(yaml_document_initialize(&document, NULL, NULL, NULL, 0, 0));
    mapping_id = yaml_document_add_mapping(
        &document,
        NULL,
        YAML_BLOCK_MAPPING_STYLE
    );
    key_id = yaml_document_add_scalar(
        &document,
        NULL,
        (yaml_char_t *) "owned",
        -1,
        YAML_PLAIN_SCALAR_STYLE
    );
    node_id = yaml_document_add_scalar(
        &document,
        NULL,
        (yaml_char_t *) "after document delete",
        -1,
        YAML_PLAIN_SCALAR_STYLE
    );
    ck_assert_int_gt(mapping_id, 0);
    ck_assert_int_gt(key_id, 0);
    ck_assert_int_gt(node_id, 0);
    ck_assert(yaml_document_append_mapping_pair(
        &document,
        mapping_id,
        key_id,
        node_id
    ));
    node = yaml_document_get_node(&document, mapping_id);
    ck_assert_ptr_nonnull(node);

    error = handlebars_value_init_yaml_node_try(
        context,
        value,
        &document,
        node
    );
    yaml_document_delete(&document);

    ck_assert_int_eq(error, HANDLEBARS_SUCCESS);
    ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_SUCCESS);
    ck_assert_ptr_null(handlebars_error_msg(context));
    ck_assert_ptr_eq(context->e->jmp, &outer);
    ck_assert_ptr_nonnull(
        handlebars_value_map_str_find(value, HBS_STRL("owned"), found)
    );
    ck_assert_str_eq(
        handlebars_value_get_strval(found),
        "after document delete"
    );

    context->e->jmp = previous;
    HANDLEBARS_VALUE_UNDECL(value);
    HANDLEBARS_VALUE_UNDECL(found);
    ASSERT_INIT_BLOCKS();
}
END_TEST

START_TEST(test_yaml_try_handles_cyclic_and_deep_inputs)
{
    const size_t depth = 300;
    const size_t length = depth * 2 + 1;
    HANDLEBARS_VALUE_DECL(value);
    char * yaml = talloc_array(context, char, length);
    jmp_buf * previous = context->e->jmp;
    jmp_buf outer;
    enum handlebars_error_type error;

    ck_assert_ptr_nonnull(yaml);
    if( setjmp(outer) != 0 ) {
        context->e->jmp = previous;
        ck_abort_msg("An adversarial YAML try conversion escaped through longjmp");
    }
    context->e->jmp = &outer;
    handlebars_value_integer(value, 42);

    error = handlebars_value_init_yaml_stringl_try(
        context,
        value,
        HBS_STRL("--- &root\n- *root\n")
    );
    ck_assert_int_eq(error, HANDLEBARS_ERROR);
    ck_assert_ptr_nonnull(strstr(handlebars_error_msg(context), "Cyclic YAML"));
    ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_INTEGER);
    ck_assert_int_eq(handlebars_value_get_intval(value), 42);
    ck_assert_ptr_eq(context->e->jmp, &outer);

    memset(yaml, '[', depth);
    yaml[depth] = '0';
    memset(yaml + depth + 1, ']', depth);
    error = handlebars_value_init_yaml_stringl_try(
        context,
        value,
        yaml,
        length
    );

    ck_assert_int_eq(error, HANDLEBARS_ERROR);
    ck_assert_ptr_nonnull(strstr(handlebars_error_msg(context), "nesting depth"));
    ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_INTEGER);
    ck_assert_int_eq(handlebars_value_get_intval(value), 42);
    ck_assert_ptr_eq(context->e->jmp, &outer);

    context->e->jmp = previous;
    clear_intentional_error();
    HANDLEBARS_VALUE_UNDECL(value);
    talloc_free(yaml);
    ASSERT_INIT_BLOCKS();
}
END_TEST

#ifdef HANDLEBARS_MEMORY
START_TEST(test_yaml_try_handles_allocation_failures)
{
    yaml_document_t document;
    yaml_node_t * node;
    bool node_succeeded = false;
    bool string_succeeded = false;
    int node_id;

    ck_assert(yaml_document_initialize(&document, NULL, NULL, NULL, 0, 0));
    node_id = yaml_document_add_scalar(
        &document,
        NULL,
        (yaml_char_t *) "value",
        5,
        YAML_PLAIN_SCALAR_STYLE
    );
    ck_assert_int_gt(node_id, 0);
    node = yaml_document_get_node(&document, node_id);
    ck_assert_ptr_nonnull(node);

    for( int fail_at = 1; fail_at <= 64; fail_at++ ) {
        HANDLEBARS_VALUE_DECL(value);
        size_t blocks_before = talloc_total_blocks(context);
        enum handlebars_error_type error;

        handlebars_value_integer(value, 42);
        handlebars_memory_fail_set_flags(handlebars_memory_fail_flag_alloc);
        handlebars_memory_fail_counter(fail_at);
        error = handlebars_value_init_yaml_node_try(
            context,
            value,
            &document,
            node
        );
        handlebars_memory_fail_disable();

        if( error == HANDLEBARS_SUCCESS ) {
            ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_STRING);
            ck_assert_str_eq(handlebars_value_get_strval(value), "value");
            node_succeeded = true;
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
        if( node_succeeded ) {
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
        error = handlebars_value_init_yaml_stringl_try(
            context,
            value,
            HBS_STRL("---\n[value]")
        );
        handlebars_memory_fail_disable();

        if( error == HANDLEBARS_SUCCESS ) {
            ck_assert_int_eq(handlebars_value_get_type(value), HANDLEBARS_VALUE_TYPE_ARRAY);
            ck_assert_int_eq(handlebars_value_count(value), 1);
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

    yaml_document_delete(&document);
    ck_assert(node_succeeded);
    ck_assert(string_succeeded);
    ASSERT_INIT_BLOCKS();
}
END_TEST
#endif

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
    REGISTER_TEST_FIXTURE(s, test_yaml_try_returns_errors_without_longjmp, "YAML try conversion returns errors without longjmp");
    REGISTER_TEST_FIXTURE(s, test_yaml_try_reuse_and_node_lifetime, "YAML try conversion preserves values across reuse and copies node inputs");
    REGISTER_TEST_FIXTURE(s, test_yaml_try_handles_cyclic_and_deep_inputs, "YAML try conversion handles cyclic and deep inputs");
#ifdef HANDLEBARS_MEMORY
    REGISTER_TEST_FIXTURE(s, test_yaml_try_handles_allocation_failures, "YAML try conversion handles allocation failures");
#endif

    return s;
}

int main(void)
{
    return default_main(&suite);
}
