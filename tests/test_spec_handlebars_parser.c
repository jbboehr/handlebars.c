
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <check.h>
#include <stdio.h>
#include <pcre.h>
#include <talloc.h>

#if defined(HAVE_JSON_C_JSON_H)
#include "json-c/json.h"
#include "json-c/json_object.h"
#include "json-c/json_tokener.h"
#elif defined(HAVE_JSON_JSON_H)
#include "json/json.h"
#include "json/json_object.h"
#include "json/json_tokener.h"
#endif

#include "handlebars.h"
#include "handlebars_ast.h"
#include "handlebars_ast_printer.h"
#include "handlebars_memory.h"
#include "handlebars_string.h"
#include "handlebars_token_list.h"
#include "handlebars_utils.h"
#include "handlebars.tab.h"
#include "handlebars.lex.h"
#include "utils.h"

struct parser_test {
    char * description;
    char * it;
    char * tmpl;
    char * expected;
    int exception;
    char * message;
};

static TALLOC_CTX * rootctx;
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
    if( cur && json_object_get_type(cur) == json_type_string ) {
        test->expected = handlebars_talloc_strdup(rootctx, json_object_get_string(cur));
        handlebars_rtrim(test->expected, " \t\r\n");
        nreq++;
    }
    
    // Get exception
    cur = json_object_object_get(object, "exception");
    if( cur && json_object_get_type(cur) == json_type_boolean ) {
        test->exception = (int) json_object_get_boolean(cur);
        nreq++;
    } else {
        test->exception = 0;
    }
    
    // Get message
    cur = json_object_object_get(object, "message");
    if( cur && json_object_get_type(cur) == json_type_string ) {
        test->message = handlebars_talloc_strdup(rootctx, json_object_get_string(cur));
        nreq++;
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
    tests = handlebars_talloc_array(rootctx, struct parser_test, tests_size);
    
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

START_TEST(handlebars_spec_parser)
{
    struct parser_test * test = &tests[_i];
    struct handlebars_context * ctx = handlebars_context_ctor();
    struct handlebars_parser * parser;
    char * errmsg;
    char errlinestr[32];

    parser = handlebars_parser_ctor(ctx);
    parser->tmpl = handlebars_string_ctor(HBSCTX(parser), test->tmpl, strlen(test->tmpl));
    handlebars_parse(parser);
    
    if( parser->ctx.num ) {
        char * errmsg = handlebars_context_get_errmsg((struct handlebars_context *) parser);
        char * errmsgjs = handlebars_context_get_errmsg_js((struct handlebars_context *) parser);
        
        if( test->exception ) {
            if( test->message == NULL ) {
                // Just check if there was an error
                ck_assert_str_ne("", errmsgjs ? errmsgjs : "");
            } else if( test->message[0] == '/' && test->message[strlen(test->message) - 1] == '/' ) {
                // It's a regex
                char * tmp = strdup(test->message + 1);
                tmp[strlen(test->message) - 2] = '\0';
                char * regex_error = NULL;
                if( 0 == regex_compare(tmp, errmsgjs, &regex_error) ) {
                    // ok
                } else {
                    ck_assert_msg(0, regex_error);
                }
                free(tmp);
            } else {
                ck_assert_str_eq(test->message, errmsg);
            }
        } else {
            char * lesigh = handlebars_talloc_strdup(ctx, "\nExpected: \n");
            lesigh = handlebars_talloc_strdup_append(lesigh, test->expected);
            lesigh = handlebars_talloc_strdup_append(lesigh, "\nActual (error): \n");
            lesigh = handlebars_talloc_strdup_append(lesigh, errmsg);
            lesigh = handlebars_talloc_strdup_append(lesigh, "\nTemplate: \n");
            lesigh = handlebars_talloc_strdup_append(lesigh, test->tmpl);
            ck_assert_msg(0, lesigh);
        }
    } else {
        char * output = handlebars_ast_print(parser, parser->program, 0);
        
        if( !test->exception ) {
            ck_assert_ptr_ne(NULL, output);
            if( strcmp(test->expected, output) == 0 ) {
                ck_assert_str_eq(test->expected, output);
            } else {
                char * lesigh = handlebars_talloc_strdup(ctx, "\nExpected: \n");
                lesigh = handlebars_talloc_strdup_append(lesigh, test->expected);
                lesigh = handlebars_talloc_strdup_append(lesigh, "\nActual: \n");
                lesigh = handlebars_talloc_strdup_append(lesigh, output);
                lesigh = handlebars_talloc_strdup_append(lesigh, "\nTemplate: \n");
                lesigh = handlebars_talloc_strdup_append(lesigh, test->tmpl);
                ck_assert_msg(0, lesigh);
            }
        } else {
            ck_assert_msg(0, test->message);
        }
        
        handlebars_talloc_free(output);
    }
  
    handlebars_context_dtor(ctx);
}
END_TEST

Suite * parser_suite(void)
{
    const char * title = "Handlebars Parser Spec";
    Suite * s = suite_create(title);
    
    TCase * tc_handlebars_spec_parser = tcase_create(title);
    // tcase_add_checked_fixture(tc_ ## name, setup, teardown);
    tcase_add_loop_test(tc_handlebars_spec_parser, handlebars_spec_parser, 0, tests_len - 1);
    suite_add_tcase(s, tc_handlebars_spec_parser);
    
    return s;
}

int main(void)
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
    spec_filename = getenv("handlebars_parser_spec");
    if( spec_filename == NULL ) {
        spec_filename = "./spec/handlebars/spec/parser.json";
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
