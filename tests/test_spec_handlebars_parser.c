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

#include <errno.h>
#include <check.h>
#include <stdio.h>
#include <pcre.h>
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
#include "handlebars_ast_printer.h"
#include "handlebars_memory.h"
#include "handlebars_parser.h"
#include "handlebars_string.h"
#include "handlebars.tab.h"
#include "handlebars.lex.h"
#include "utils.h"



struct parser_test {
    struct handlebars_context * ctx;
    char * description;
    char * it;
    char * tmpl;
    struct handlebars_string * expected;
    int exception;
    char * exceptionMatcher;
    char * message;
    const char * raw;
};

static struct parser_test * tests;
static size_t tests_len = 0;
static size_t tests_size = 0;
static const char * spec_filename = NULL;

static void loadSpecTest(json_object * object)
{
    json_object * cur = NULL;
    int nreq = 0;

    // Get test
    struct parser_test * test = &(tests[tests_len++]);
    test->ctx = handlebars_context_ctor_ex(root);
    test->raw = json_object_to_json_string_ext(object, JSON_C_TO_STRING_PRETTY);

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
    if( cur && json_object_get_type(cur) == json_type_string ) {
        test->expected = handlebars_string_ctor(test->ctx, json_object_get_string(cur), json_object_get_string_len(cur));
        test->expected = handlebars_string_rtrim(test->expected, HBS_STRL(" \t\r\n"));
        nreq++;
    }

    // Get exception
    cur = json_object_object_get(object, "exception");
    if( cur && json_object_get_type(cur) == json_type_boolean ) {
        test->exception = (int) json_object_get_boolean(cur);
        nreq++;
    } else if (cur && json_object_get_type(cur) == json_type_string) {
        test->exception = true;
        test->exceptionMatcher = handlebars_talloc_strdup(root, json_object_get_string(cur));
        nreq++;
    } else {
        test->exception = 0;
    }

    // Get message
    cur = json_object_object_get(object, "message");
    if( cur && json_object_get_type(cur) == json_type_string ) {
        test->message = handlebars_talloc_strdup(root, json_object_get_string(cur));
    }

    // Check
    if( nreq <= 0 ) {
        fprintf(stderr, "Warning: expected or exception/message must be specified\n");
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
    tests = talloc_zero_array(root, struct parser_test, tests_size);

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

START_TEST(handlebars_spec_parser)
{
    struct parser_test * test = &tests[_i];
    struct handlebars_context * ctx = handlebars_context_ctor();

#ifndef NDEBUG
    fprintf(stderr, "-----------\n");
    fprintf(stderr, "RAW: %s\n", test->raw);
    fprintf(stderr, "NUM: %d\n", _i);
    fprintf(stderr, "TMPL: %s\n", test->tmpl);
    fflush(stderr);
#endif

    struct handlebars_ast_node * ast = handlebars_parse_ex(parser, handlebars_string_ctor(HBSCTX(parser), test->tmpl, strlen(test->tmpl)), 0);

    if( handlebars_error_num(HBSCTX(parser)) != HANDLEBARS_SUCCESS ) {
        char * errmsg = handlebars_error_message((struct handlebars_context *) parser);
        char * errmsgjs = handlebars_error_message_js((struct handlebars_context *) parser);

#ifndef NDEBUG
        fprintf(stderr, "ERR: %s\n", errmsg);
        fprintf(stderr, "ERRJS: %s\n", errmsgjs);
        fflush(stderr);
#endif

        if( test->exception ) {
            if( test->exceptionMatcher == NULL ) {
                // Just check if there was an error
                ck_assert_str_ne("", errmsgjs ? errmsgjs : "");
            } else if( test->exceptionMatcher[0] == '/' && test->exceptionMatcher[strlen(test->exceptionMatcher) - 1] == '/' ) {
                // It's a regex
                char * tmp = strdup(test->exceptionMatcher + 1);
                tmp[strlen(test->exceptionMatcher) - 2] = '\0';
                char * regex_error = NULL;
                if( 0 == regex_compare(tmp, errmsgjs, &regex_error) ) {
                    // ok
                } else {
                    ck_assert_msg(0, "%s", regex_error);
                }
                free(tmp);
            } else {
                ck_assert_str_eq(test->exceptionMatcher, errmsg);
            }
        } else {
            char * lesigh = handlebars_talloc_strdup(ctx, "\nExpected: \n");
            lesigh = handlebars_talloc_strdup_append(lesigh, hbs_str_val(test->expected));
            lesigh = handlebars_talloc_strdup_append(lesigh, "\nActual (error): \n");
            lesigh = handlebars_talloc_strdup_append(lesigh, errmsg);
            lesigh = handlebars_talloc_strdup_append(lesigh, "\nTemplate: \n");
            lesigh = handlebars_talloc_strdup_append(lesigh, test->tmpl);
            ck_assert_msg(0, "%s", lesigh);
        }
    } else {
        struct handlebars_string * output = handlebars_ast_print(HBSCTX(parser), ast);

#ifndef NDEBUG
        fprintf(stderr, "AST: %s\n", hbs_str_val(output));
        fflush(stderr);
#endif

        if( !test->exception ) {
            ck_assert_ptr_ne(NULL, output);
            if( handlebars_string_eq(test->expected, output) ) {
                ck_assert_str_eq(hbs_str_val(test->expected), hbs_str_val(output));
            } else {
                char * lesigh = handlebars_talloc_strdup(ctx, "\nExpected: \n");
                lesigh = handlebars_talloc_strdup_append(lesigh, hbs_str_val(test->expected));
                lesigh = handlebars_talloc_strdup_append(lesigh, "\nActual: \n");
                lesigh = handlebars_talloc_strdup_append(lesigh, hbs_str_val(output));
                lesigh = handlebars_talloc_strdup_append(lesigh, "\nTemplate: \n");
                lesigh = handlebars_talloc_strdup_append(lesigh, test->tmpl);
                ck_assert_msg(0, "%s", lesigh);
            }
        } else {
            ck_assert_msg(0, "%s", test->message);
        }

        handlebars_talloc_free(output);
    }

    handlebars_context_dtor(ctx);
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
    const char * title = "Handlebars Parser Spec";
    Suite * s = suite_create(title);

    TCase * tc_handlebars_spec_parser = tcase_create(title);
    tcase_add_checked_fixture(tc_handlebars_spec_parser, default_setup, default_teardown);
    tcase_add_loop_test(tc_handlebars_spec_parser, handlebars_spec_parser, 0, tests_len - 1);
    suite_add_tcase(s, tc_handlebars_spec_parser);

    return s;
}

int main(int argc, char *argv[])
{
    // Load the spec
    spec_filename = getenv("handlebars_parser_spec");
    if( spec_filename == NULL && argc >= 2 ) {
        spec_filename = argv[1];
    }
    if( spec_filename == NULL ) {
        spec_filename = "./spec/handlebars/spec/parser.json";
    }

    // Run the suite
    return default_main(&suite);
}
