
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <check.h>
#include <stdio.h>
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
#include "handlebars_context.h"
#include "handlebars_memory.h"
#include "handlebars_token.h"
#include "handlebars_token_list.h"
#include "handlebars_token_printer.h"
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
        nreq++;
    }
    
    // Get exception
    cur = json_object_object_get(object, "exception");
    if( cur && json_object_get_type(cur) == json_type_boolean ) {
        test->exception = (int) json_object_get_boolean(cur);
        nreq++;
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

static int loadSpec(char * filename) {
    int error = 0;
    char * data = NULL;
    size_t data_len = 0;
    struct json_object * result = NULL;
    struct json_object * array_item = NULL;
    int array_len = 0;
    
    // Read JSON file
    error = file_get_contents((const char *) filename, &data, &data_len);
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
    tests = talloc_array(rootctx, struct parser_test, tests_size);
    
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
    return error;
}

START_TEST(handlebars_spec_parser)
{
    struct parser_test * test = &tests[_i];
    struct handlebars_context * ctx = handlebars_context_ctor();
    int retval;
    char * errmsg;
    char errlinestr[32];
    
    ctx->tmpl = test->tmpl;
    
    retval = handlebars_yy_parse(ctx);
    
    if( ctx->error != NULL ) {
        char * errmsg = handlebars_context_get_errmsg(ctx);
        char * errmsgjs = handlebars_context_get_errmsg_js(ctx);
        
        if( test->exception ) {
            // It's a regexp, try to do substring search
            if( test->message[0] == '/' && test->message[strlen(test->message) - 1] == '/' ) {
                char * tmp = strdup(test->message + 1);
                tmp[strlen(test->message) - 2] = '\0';
                char * found = strstr(errmsgjs, tmp);
                if( found != NULL ) {
                    // ok
                } else {
                    //ck_assert_msg(found != NULL, errmsg);
                    ck_assert_str_eq(test->message, errmsgjs);
                }
                free(tmp);
            } else {
                ck_assert_str_eq(test->message, errmsg);
            }
        } else {
            char * lesigh = handlebars_talloc_strdup(ctx, errmsg);
            lesigh = handlebars_talloc_strdup_append(lesigh, test->tmpl);
            ck_assert_msg(0, lesigh);
        }
    } else {
        errno = 0;
        
        //char * output = handlebars_ast_print(ctx->program, 0);
        struct handlebars_ast_printer_context printctx = handlebars_ast_print2(ctx->program, 0);
        //_handlebars_ast_print(ctx->program, &printctx);
        char * output = printctx.output;
        
        if( !test->exception ) {
            ck_assert_int_eq(0, errno);
            ck_assert_int_eq(0, printctx.error);
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
    char * spec_filename;
    int memdebug = 0;
    int iswin = 0;
    int error = 0;
    
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
    srunner_run_all(sr, CK_VERBOSE);
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
