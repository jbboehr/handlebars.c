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

// json-c undeprecated json_object_object_get, but the version in xenial
// is too old, so let's silence deprecated warnings for json-c
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <json.h>
#include <json_object.h>
#include <json_tokener.h>
#pragma GCC diagnostic pop

#include "handlebars.h"
#include "handlebars_ast.h"
#include "handlebars_memory.h"
#include "handlebars_parser.h"
#include "handlebars_string.h"
#include "handlebars_token.h"
#include "handlebars.tab.h"
#include "handlebars.lex.h"
#include "utils.h"



struct tokenizer_test {
    char * description;
    char * it;
    char * tmpl;
    struct handlebars_string * expected;
    struct handlebars_context * ctx;
};

static struct tokenizer_test * tests;
static size_t tests_len = 0;
static size_t tests_size = 0;
static const char * spec_filename = NULL;

static void loadSpecTestExpected(struct tokenizer_test * test, json_object * object)
{
    size_t array_len = 0;
    json_object * cur = NULL;
    int token_int = -1;
    const char * name = NULL;
    const char * text = NULL;
    struct handlebars_token * token = NULL;
    struct handlebars_string * string;

    // Get number of tokens cases
    array_len = json_object_array_length(object);

    // Allocate token list
    test->expected = handlebars_string_init(test->ctx, 256);

    // Iterate over array
    for( size_t i = 0; i < array_len; i++ ) {
        struct json_object * array_item = json_object_array_get_idx(object, i);
        if( json_object_get_type(array_item) != json_type_object ) {
            fprintf(stderr, "Warning: expected token was not an object\n");
            continue;
        }

        // Get name
        cur = json_object_object_get(array_item, "name");
        if( cur && json_object_get_type(cur) == json_type_string ) {
            name = json_object_get_string(cur);
        } else {
            fprintf(stderr, "Warning: expected token name was not a string\n");
            continue;
        }

        // Get text
        cur = json_object_object_get(array_item, "text");
        if( cur && json_object_get_type(cur) == json_type_string ) {
            text = json_object_get_string(cur);
        } else {
            fprintf(stderr, "Warning: expected token text was not a string\n");
            continue;
        }

        // Convert name to integer T_T
        token_int = handlebars_token_reverse_readable_type(name);
        if( token_int == -1 ) {
            fprintf(stderr, "Warning: failed reverse lookup to int on token name\n");
            continue;
        }

        // Make token object
        string = handlebars_string_ctor(test->ctx, text, strlen(text));
        token = handlebars_token_ctor(test->ctx, token_int, string);
        if( token == NULL ) {
            fprintf(stderr, "Warning: failed to allocate token struct\n");
            continue;
        }

        // Append
        struct handlebars_string * tmp = handlebars_token_print(test->ctx, token, 1);
        test->expected = handlebars_string_append_str(context, test->expected, tmp);
        handlebars_talloc_free(tmp);
    }

    test->expected = handlebars_string_rtrim(test->expected, HBS_STRL(" \t\r\n"));
}

static void loadSpecTest(json_object * object)
{
    json_object * cur = NULL;

    // Get test
    struct tokenizer_test * test = &(tests[tests_len++]);
    test->ctx = handlebars_context_ctor_ex(root);

    // Get description
    cur = json_object_object_get(object, "description");
    if( cur && json_object_get_type(cur) == json_type_string ) {
        test->description = handlebars_talloc_strdup(root, json_object_get_string(cur));
    }

    // Get it
    cur = json_object_object_get(object, "it");
    if( cur && json_object_get_type(cur) == json_type_string ) {
        test->it = handlebars_talloc_strdup(root, json_object_get_string(cur));
    }

    // Get template
    cur = json_object_object_get(object, "template");
    if( cur && json_object_get_type(cur) == json_type_string ) {
        test->tmpl = handlebars_talloc_strdup(root, json_object_get_string(cur));
    }

    // Get expected
    cur = json_object_object_get(object, "expected");
    if( cur && json_object_get_type(cur) == json_type_array ) {
        loadSpecTestExpected(test, cur);
    } else {
        fprintf(stderr, "Warning: Expected was not an array\n");
    }
}

static int loadSpec(const char * filename) {
    int error = 0;
    char * data = NULL;
    size_t data_len = 0;
    struct json_object * result = NULL;
    struct json_object * array_item = NULL;
    size_t array_len = 0;

    // Read JSON file
    error = file_get_contents(filename, &data, &data_len);
    if( error != 0 ) {
        fprintf(stderr, "Failed to read spec file: %s, code: %d\n", filename, error);
        goto error;
    }

    // Parse JSON
    result = json_tokener_parse(data);
    // @todo: parsing errors seem to cause segfaults....
    if( result == NULL ) {
        fprintf(stderr, "Failed so parse JSON\n");
        error = 1;
        goto error;
    }

    // Root object should be array
    if( json_object_get_type(result) != json_type_array ) {
        fprintf(stderr, "Root JSON value was not array\n");
        error = 1;
        goto error;
    }

    // Get number of test cases
    array_len = json_object_array_length(result);

    // Allocate tests array
    tests_size = array_len + 1;
    tests = talloc_zero_array(root, struct tokenizer_test, tests_size);

    // Iterate over array
    for( size_t i = 0; i < array_len; i++ ) {
        array_item = json_object_array_get_idx(result, i);
        if( json_object_get_type(array_item) != json_type_object ) {
            fprintf(stderr, "Warning: test case was not an object\n");
            continue;
        }
        loadSpecTest(array_item);
    }
error:
    if( data ) {
        free(data);
    }
    HBS_TEST_JSON_DTOR(tests, result);
    return error;
}

START_TEST(handlebars_spec_tokenizer)
{
    struct tokenizer_test * test = &tests[_i];

    struct handlebars_token ** tokens = handlebars_lex_ex(parser, handlebars_string_ctor(HBSCTX(parser), test->tmpl, strlen(test->tmpl)));

    struct handlebars_string * actual = handlebars_string_init(context, 256);
    for ( ; *tokens; tokens++ ) {
        struct handlebars_string * tmp = handlebars_token_print(context, *tokens, 1);
        actual = handlebars_string_append_str(context, actual, tmp);
        handlebars_talloc_free(tmp);
    }

    actual = handlebars_string_rtrim(actual, HBS_STRL(" \t\r\n"));

    ck_assert_str_eq_msg(hbs_str_val(test->expected), hbs_str_val(actual), test->tmpl);
}
END_TEST

static Suite * suite(void);
static Suite * suite(void)
{
    // Load the spec
    if( 0 != loadSpec(spec_filename) ) {
        abort();
    }
    fprintf(stderr, "Loaded %zu test cases\n", tests_len);

    // Setup the suite
    const char * title = "Handlebars Tokenizer Spec";
    Suite * s = suite_create(title);

    TCase * tc_handlebars_spec_tokenizer = tcase_create(title);
    tcase_add_checked_fixture(tc_handlebars_spec_tokenizer, default_setup, default_teardown);
    tcase_add_loop_test(tc_handlebars_spec_tokenizer, handlebars_spec_tokenizer, 0, tests_len - 1);
    suite_add_tcase(s, tc_handlebars_spec_tokenizer);

    return s;
}

int main(int argc, char *argv[])
{
    // Load the spec
    spec_filename = getenv("handlebars_tokenizer_spec");
    if( spec_filename == NULL && argc >= 2 ) {
        spec_filename = argv[1];
    }
    if( spec_filename == NULL ) {
        spec_filename = "./spec/handlebars/spec/tokenizer.json";
    }

    // Run the suite
    return default_main(&suite);
}
