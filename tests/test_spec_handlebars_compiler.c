
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

#include "handlebars_compiler.h"
#include "handlebars_context.h"
#include "handlebars_memory.h"
#include "handlebars_opcode_printer.h"
#include "utils.h"

struct compiler_test {
    char * suite_name;
    char * description;
    char * it;
    char * tmpl;
    char * expected;
    int exception;
    char * message;
};

static const char * suite_names[] = {
  "basic", "blocks", "builtins", "data", "helpers", "partials",
  "regressions", "string-params", "subexpressions", "track-ids",
  "whitespace-control", NULL
};

static TALLOC_CTX * rootctx = NULL;
static struct compiler_test * tests = NULL;
static size_t tests_len = 0;
static size_t tests_size = 0;
static char * export_dir = NULL;


static int loadSpecTest(const char * name, json_object * object)
{
    json_object * cur = NULL;
    int nreq = 0;
    
    // Get test
    struct compiler_test * test = &(tests[tests_len++]);
    
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
    
    // @todo
    /*
    // Get expected
    cur = json_object_object_get(object, "expected");
    if( cur && json_object_get_type(cur) == json_type_string ) {
        test->expected = handlebars_talloc_strdup(rootctx, json_object_get_string(cur));
        handlebars_rtrim(test->expected, " \t\r\n");
        nreq++;
    }
    */
    
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
    
    return 0;
}

static int loadSpec(const char * name)
{
    char * filename = talloc_asprintf(rootctx, "%s/%s.json", export_dir, name);
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
    
    // (Re)allocate tests array
    tests_size += array_len;
    tests = talloc_realloc(rootctx, tests, struct compiler_test, tests_size);
    
    // Iterate over array
    for( int i = 0; i < array_len; i++ ) {
        array_item = json_object_array_get_idx(result, i);
        if( json_object_get_type(array_item) != json_type_object ) {
            fprintf(stderr, "Warning: test case was not an object\n");
            continue;
        }
        loadSpecTest(name, array_item);
    }
    
error:
    if( filename ) {
        handlebars_talloc_free(filename);
    }
    if( data ) {
        free(data);
    }
    if( result ) {
        json_object_put(result);
    }
    return error;
}

static int loadAllSpecs()
{
    char ** suite_name_ptr;
    int error = 0;
    
    for( suite_name_ptr = suite_names; *suite_name_ptr != NULL; suite_name_ptr++ ) {
        error = loadSpec(*suite_name_ptr);
        if( error != 0 ) {
            break;
        }
    }
    
    return error;
}

START_TEST(handlebars_spec_compiler)
{
    struct compiler_test * test = &tests[_i];
    struct handlebars_context * ctx;
    struct handlebars_compiler * compiler;
    struct handlebars_opcode_printer * printer;
    int retval;
    
    // Initialize
    ctx = handlebars_context_ctor();
    compiler = handlebars_compiler_ctor(ctx);
    printer = handlebars_opcode_printer_ctor(ctx);
    
    // Parse
    ctx->tmpl = test->tmpl;
    retval = handlebars_yy_parse(ctx);
    
    //ck_assert_int_eq(retval <= 0, test->exception > 0);
    
    // Compile
    handlebars_compiler_compile(compiler, ctx->program);
    ck_assert_int_eq(0, compiler->errnum);
    
    // Printer
    handlebars_opcode_printer_print(printer, compiler);
    fprintf(stdout, "%s\n", printer->output);
    
    handlebars_context_dtor(ctx);
}
END_TEST

Suite * parser_suite(void)
{
    const char * title = "Handlebars Compiler Spec";
    Suite * s = suite_create(title);
    
    TCase * tc_handlebars_spec_compiler = tcase_create(title);
    // tcase_add_checked_fixture(tc_ ## name, setup, teardown);
    tcase_add_loop_test(tc_handlebars_spec_compiler, handlebars_spec_compiler, 0, tests_len - 1);
    suite_add_tcase(s, tc_handlebars_spec_compiler);
    
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
    
#if defined(_WIN64) || defined(_WIN32) || defined(__WIN32__) || defined(__CYGWIN32__)
    iswin = 1;
#endif
    memdebug = getenv("MEMDEBUG") ? atoi(getenv("MEMDEBUG")) : 0;
    
    if( memdebug ) {
        talloc_enable_leak_report_full();
    }
    rootctx = talloc_new(NULL);
    
    // Get the export dir
    export_dir = getenv("handlebars_export_dir");
    if( export_dir == NULL ) {
        export_dir = "./spec/handlebars/export";
    }
    
    // Load the spec
    error = loadAllSpecs();
    if( error != 0 ) {
        goto error;
    }
    fprintf(stderr, "Loaded %lu test cases\n", tests_len);
    
    // Run tests
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
