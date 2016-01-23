
#include <check.h>
#include <stdio.h>
#include <talloc.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if defined(HAVE_JSON_C_JSON_H)
#include <json-c/json.h>
#include <json-c/json_object.h>
#include <json-c/json_tokener.h>
#elif defined(HAVE_JSON_JSON_H)
#include <json/json.h>
#include <json/json_object.h>
#include <json/json_tokener.h>
#endif

#include "handlebars_context.h"
#include "handlebars_compiler.h"
#include "handlebars_memory.h"
#include "handlebars_vm.h"
#include "handlebars.tab.h"
#include "handlebars.lex.h"

#include "utils.h"

struct generic_test {
    char * description;
    char * it;
    char * tmpl;
    struct json_object * context;
    char * expected;
    TALLOC_CTX * mem_ctx;

    char ** known_helpers;
    long flags;
};

static TALLOC_CTX * rootctx;
static struct generic_test * tests;
static size_t tests_len = 0;
static size_t tests_size = 0;
static char * spec_dir;

static void loadSpecTest(json_object * object)
{
    json_object * cur = NULL;

    // Get test
    struct generic_test * test = &(tests[tests_len++]);

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
    } else {
        fprintf(stderr, "Warning: Expected was not a string\n");
    }

    // Get data
    cur = json_object_object_get(object, "data");
    if( cur ) {
        test->context = cur;
    } else {
        fprintf(stderr, "Warning: Data was not set\n");
    }
}

static int loadSpec(const char * spec) {
    int error = 0;
    char * data = NULL;
    size_t data_len = 0;
    struct json_object * result = NULL;
    struct json_object * array_item = NULL;
    int array_len = 0;
    char filename[1024];

    snprintf(filename, 1023, "%s/%s.json", spec_dir, spec);

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

    // (Re)Allocate tests array
    tests_size += array_len;
    tests = talloc_realloc(rootctx, tests, struct generic_test, tests_size);

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
        // @todo free?
        //json_object_put(result);
    }
    return error;
}

START_TEST(test_handlebars_spec)
{
    struct generic_test * test = &tests[_i];
    struct handlebars_context * ctx;
    struct handlebars_compiler * compiler;
    struct handlebars_vm * vm;
    struct handlebars_value * context;
    int retval;

    // Initialize
    ctx = handlebars_context_ctor();
    //ctx->ignore_standalone = test->opt_ignore_standalone;
    compiler = handlebars_compiler_ctor(ctx);
    vm = handlebars_vm_ctor(ctx);

    // Parse
    ctx->tmpl = test->tmpl;
    retval = handlebars_yy_parse(ctx);

    // Compile
    handlebars_compiler_set_flags(compiler, test->flags);
    /* if( test->known_helpers ) {
        compiler->known_helpers = (const char **) test->known_helpers;
    } */

    handlebars_compiler_compile(compiler, ctx->program);
    ck_assert_int_eq(0, compiler->errnum);


    // Execute
    context = test->context ? handlebars_value_from_json_object(ctx, test->context) : handlebars_value_ctor(ctx);
    handlebars_vm_execute(vm, compiler, context);

    ck_assert_ptr_ne(test->expected, NULL);
    ck_assert_ptr_ne(vm->buffer, NULL);

#ifndef NDEBUG
    fprintf(stdout, "-----------\nTMPL: %s\n", test->tmpl);
    fprintf(stdout, "EXPECTED: %s\n", test->expected);
    fprintf(stdout, "ACTUAL: %s\n", vm->buffer);
    fprintf(stdout, "CMP: %d\n", strcmp(vm->buffer, test->expected));
#endif

    if( strcmp(vm->buffer, test->expected) != 0 ) {
        char * tmp = handlebars_talloc_asprintf(rootctx,
                                                "Failed.\nSuite: %s\nTest: %s - %s\nFlags: %ld\nTemplate:\n%s\nExpected:\n%s\nActual:\n%s\n",
                                                "" /*test->suite_name*/,
                                                test->description, test->it, test->flags,
                                                test->tmpl, test->expected, vm->buffer);
        ck_abort_msg(tmp);
    }

    //ck_assert_str_eq(vm->buffer, test->expected);
}
END_TEST

Suite * parser_suite(void)
{
    const char * title = "Handlebars Spec";
    Suite * s = suite_create(title);

    TCase * tc_handlebars_spec = tcase_create(title);
    // tcase_add_checked_fixture(tc_ ## name, setup, teardown);
    tcase_add_loop_test(tc_handlebars_spec, test_handlebars_spec, 0, tests_len - 1);
    suite_add_tcase(s, tc_handlebars_spec);

    return s;
}

int main(void)
{
    int number_failed;
    int memdebug;
    int error;

    // Check if memdebug enabled
    memdebug = getenv("MEMDEBUG") ? atoi(getenv("MEMDEBUG")) : 0;
    if( memdebug ) {
        talloc_enable_leak_report_full();
    }
    rootctx = talloc_new(NULL);

    // Load specs
    // Load the spec
    spec_dir = getenv("handlebars_spec_dir");
    if( spec_dir == NULL ) {
        spec_dir = "./spec/handlebars/spec";
    }
    loadSpec("basic");
    /* loadSpec("./spec/handlebars/spec/bench.json");
    loadSpec("./spec/handlebars/spec/blocks.json");
    loadSpec("./spec/handlebars/spec/builtins.json");
    loadSpec("./spec/handlebars/spec/data.json");
    loadSpec("./spec/handlebars/spec/helpers.json");
    loadSpec("./spec/handlebars/spec/partials.json");
    loadSpec("./spec/handlebars/spec/regressions.json");
    loadSpec("./spec/handlebars/spec/string-params.json");
    loadSpec("./spec/handlebars/spec/subexpressions.json");
    loadSpec("./spec/handlebars/spec/track-ids.json");
    loadSpec("./spec/handlebars/spec/whitespace-control.json"); */
    fprintf(stderr, "Loaded %lu test cases\n", tests_len);

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

    fprintf(stderr, "ARGGGG %d %d\n", number_failed, error);

    // Generate report for memdebug
    if( memdebug ) {
        talloc_report_full(NULL, stderr);
    }

    // Return
    return error;
}
