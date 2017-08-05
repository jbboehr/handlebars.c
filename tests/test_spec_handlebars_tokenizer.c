/**
 * Copyright (C) 2016 John Boehr
 *
 * This file is part of handlebars.c.
 *
 * handlebars.c is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * handlebars.c is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with handlebars.c.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <check.h>
#include <stdio.h>
#include <talloc.h>

#if defined(HAVE_JSON_C_JSON_H) || defined(JSONC_INCLUDE_WITH_C)
#include <json-c/json.h>
#include <json-c/json_object.h>
#include <json-c/json_tokener.h>
#elif defined(HAVE_JSON_JSON_H) || defined(HAVE_LIBJSONC)
#include <json/json.h>
#include <json/json_object.h>
#include <json/json_tokener.h>
#endif

#include "handlebars.h"
#include "handlebars_memory.h"

#include "handlebars_ast.h"
#include "handlebars_string.h"
#include "handlebars_token.h"
#include "handlebars_utils.h"
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

static TALLOC_CTX * rootctx;
static struct tokenizer_test * tests;
static size_t tests_len = 0;
static size_t tests_size = 0;
static const char * spec_filename = NULL;

static void loadSpecTestExpected(struct tokenizer_test * test, json_object * object)
{
    int array_len = 0;
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
    for( int i = 0; i < array_len; i++ ) {
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
        test->expected = handlebars_string_append(context, test->expected, tmp->val, tmp->len);
        handlebars_talloc_free(tmp);
    }

    test->expected = handlebars_string_rtrim(test->expected, HBS_STRL(" \t\r\n"));
}

static void loadSpecTest(json_object * object)
{
    json_object * cur = NULL;
    
    // Get test
    struct tokenizer_test * test = &(tests[tests_len++]);
    test->ctx = handlebars_context_ctor_ex(rootctx);
    
    // Get description
    cur = json_object_object_get(object, "description");
    if( cur && json_object_get_type(cur) == json_type_string ) {
        test->description = handlebars_talloc_strdup(rootctx, json_object_get_string(cur));
    }
    
    // Get it
    cur = json_object_object_get(object, "it");
    if( cur && json_object_get_type(cur) == json_type_string ) {
        test->it = handlebars_talloc_strdup(rootctx, json_object_get_string(cur));
    }
    
    // Get template
    cur = json_object_object_get(object, "template");
    if( cur && json_object_get_type(cur) == json_type_string ) {
        test->tmpl = handlebars_talloc_strdup(rootctx, json_object_get_string(cur));
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
    int array_len = 0;
    
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
    tests = talloc_zero_array(rootctx, struct tokenizer_test, tests_size);
    
    // Iterate over array
    for( int i = 0; i < array_len; i++ ) {
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
    if( result ) {
        json_object_put(result);
    }
    return error;
}

START_TEST(handlebars_spec_tokenizer)
{
    struct tokenizer_test * test = &tests[_i];
    struct handlebars_context * ctx = handlebars_context_ctor();
    struct handlebars_parser * parser;

    parser = handlebars_parser_ctor(ctx);
    parser->tmpl = handlebars_string_ctor(HBSCTX(parser), test->tmpl, strlen(test->tmpl));
    
    // Prepare token list
    struct handlebars_token * token = NULL;
    
    // Run
    YYSTYPE yylval_param;
    YYLTYPE yylloc_param;
    int token_int = 0;
    struct handlebars_string * actual = handlebars_string_init(ctx, 256);
    do {
        token_int = handlebars_yy_lex(&yylval_param, &yylloc_param, parser->scanner);
        if( token_int == END || token_int == INVALID ) break;
        YYSTYPE * lval = handlebars_yy_get_lval(parser->scanner);
        
        // Make token object
        token = handlebars_token_ctor(test->ctx, token_int, lval->string);
        
        // Append
        struct handlebars_string * tmp = handlebars_token_print(ctx, token, 1);
        actual = handlebars_string_append(ctx, actual, tmp->val, tmp->len);
        handlebars_talloc_free(tmp);
    } while( token );

    actual = handlebars_string_rtrim(actual, HBS_STRL(" \t\r\n"));

    ck_assert_str_eq_msg(test->expected->val, actual->val, test->tmpl);
    
    handlebars_context_dtor(ctx);
}
END_TEST

Suite * parser_suite(void)
{
    const char * title = "Handlebars Tokenizer Spec";
    Suite * s = suite_create(title);
    
    TCase * tc_handlebars_spec_tokenizer = tcase_create(title);
    // tcase_add_checked_fixture(tc_ ## name, setup, teardown);
    tcase_add_loop_test(tc_handlebars_spec_tokenizer, handlebars_spec_tokenizer, 0, tests_len - 1);
    suite_add_tcase(s, tc_handlebars_spec_tokenizer);
    
    return s;
}

int main(int argc, char *argv[])
{
    int number_failed;
    Suite * s;
    SRunner * sr;
    int memdebug = 0;
    int iswin = 0;
    int error = 0;

    talloc_set_log_stderr();
    
#if defined(_WIN64) || defined(_WIN32) || defined(__WIN32__) || defined(__CYGWIN32__)
    iswin = 1;
#endif
    memdebug = getenv("MEMDEBUG") ? atoi(getenv("MEMDEBUG")) : 0;
    
    if( memdebug ) {
        talloc_enable_leak_report_full();
    }
    rootctx = talloc_new(NULL);
    
    // Load the spec
    spec_filename = getenv("handlebars_tokenizer_spec");
    if( spec_filename == NULL && argc >= 2 ) {
        spec_filename = argv[1];
    }
    if( spec_filename == NULL ) {
        spec_filename = "./spec/handlebars/spec/tokenizer.json";
    }
    error = loadSpec(spec_filename);
    if( error != 0 ) {
        goto error;
    }
    fprintf(stderr, "Loaded %lu test cases\n", tests_len);
    
    s = parser_suite();
    sr = srunner_create(s);
    if( iswin || memdebug ) {
        srunner_set_fork_status(sr, CK_NOFORK);
    }
    srunner_run_all(sr, CK_ENV);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    error = (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
    
error:
    talloc_free(rootctx);
    if( memdebug ) {
        talloc_report_full(NULL, stderr);
    }
    return error;
}
