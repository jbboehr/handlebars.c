
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <check.h>
#include <stdio.h>
#include <talloc.h>

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
#include "handlebars_builtins.h"
#include "handlebars_memory.h"
#include "handlebars_value.h"
#include "handlebars_vm.h"
#include "handlebars.tab.h"
#include "handlebars.lex.h"

#include "utils.h"

struct generic_test {
    char * description;
    char * it;
    char * tmpl;
    struct json_object * context;
    struct json_object * data;
    struct json_object * helpers;
    struct json_object * globalHelpers;
    char * expected;
    char * message;
    short exception;
    TALLOC_CTX * mem_ctx;

    char ** known_helpers;
    long flags;
    struct json_object * raw;
};

static TALLOC_CTX * rootctx;
static struct generic_test * tests;
static size_t tests_len = 0;
static size_t tests_size = 0;
static char * spec_dir;

static void loadOptions(struct generic_test * test, json_object * object)
{
    json_object * cur = NULL;

    // Get data
    cur = json_object_object_get(object, "data");
    if( cur ) {
        test->data = cur;
    }
}

static void loadSpecTest(json_object * object)
{
    json_object * cur = NULL;

    // Get test
    struct generic_test * test = &(tests[tests_len++]);
    test->raw = object;

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

    // Get message
    cur = json_object_object_get(object, "message");
    if( cur && json_object_get_type(cur) == json_type_string ) {
        test->message = handlebars_talloc_strdup(rootctx, json_object_get_string(cur));
    }

    // Get exception
    cur = json_object_object_get(object, "exception");
    if( cur && json_object_get_type(cur) == json_type_boolean ) {
        test->exception = json_object_get_boolean(cur);
    }

    // Get data
    cur = json_object_object_get(object, "data");
    if( cur ) {
        test->context = cur;
    } else {
        fprintf(stderr, "Warning: Data was not set\n");
    }

    // Get options
    cur = json_object_object_get(object, "options");
    if( cur && json_object_get_type(cur) == json_type_object ) {
        loadOptions(test, cur);
    }

    // Get helpers
    if( NULL != (cur = json_object_object_get(object, "helpers")) ) {
        test->helpers = cur;
    }
    if( NULL != (cur = json_object_object_get(object, "globalHelpers")) ) {
        test->globalHelpers = cur;
    }

    // Get compile options
    cur = json_object_object_get(object, "compileOptions");
    if( cur && json_object_get_type(cur) == json_type_object ) {
        test->flags = json_load_compile_flags(cur);
        struct json_object * cur2 = json_object_object_get(cur, "knownHelpers");
        if( cur2 ) {
            test->known_helpers = json_load_known_helpers(rootctx, cur2);
        }
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

int shouldnt_skip(struct generic_test * test)
{
#define MYCHECK(d, i) \
    if( 0 == strcmp(d, test->description) && 0 == strcmp(i, test->it) ) return 0;

    // Still having issues with whitespace
    MYCHECK("standalone sections", "block standalone else sections can be disabled");

    // Decorators aren't implemented
    MYCHECK("decorators", "should apply mustache decorators");
    MYCHECK("decorators", "should apply block decorators");
    MYCHECK("decorators", "should apply allow undefined return");
    MYCHECK("decorators", "should support nested decorators");
    MYCHECK("decorators", "should apply multiple decorators");
    MYCHECK("decorators", "should access parent variables");
    MYCHECK("decorators", "should work with root program");
    MYCHECK("decorators", "should fail when accessing variables from root");

    MYCHECK("block params", "should take presednece over parent block params");
    MYCHECK("registration", "fails with multiple and args");

    // Regressions
    MYCHECK("Regressions", "GH-1065: Sparse arrays")
    MYCHECK("Regressions", "should support multiple levels of inline partials")
    MYCHECK("Regressions", "GH-1089: should support failover content in multiple levels of inline partials")
    MYCHECK("Regressions", "GH-1099: should support greater than 3 nested levels of inline partials");

    // Subexpressions
    // This one might need to be handled in the parser
    MYCHECK("subexpressions", "subexpressions can\'t just be property lookups");
    MYCHECK("subexpressions", "in string params mode,");
    MYCHECK("subexpressions", "as hashes in string params mode");
    MYCHECK("subexpressions", "string params for inner helper processed correctly");

    return 1;

#undef MYCCHECK
}

START_TEST(test_handlebars_spec)
{
    struct generic_test * test = &tests[_i];
    struct handlebars_context * ctx;
    struct handlebars_compiler * compiler;
    struct handlebars_vm * vm;
    struct handlebars_value * context;
    struct handlebars_value * helpers;
    struct handlebars_value_iterator * it;
    int retval;

#ifndef NDEBUG
    fprintf(stderr, "-----------\n");
    fprintf(stderr, "RAW: %s\n", json_object_to_json_string_ext(test->raw, JSON_C_TO_STRING_PRETTY));
    fprintf(stderr, "NUM: %d\n", _i);
    fprintf(stderr, "TMPL: %s\n", test->tmpl);
    fprintf(stderr, "FLAGS: %d\n", test->flags);
#endif

    //ck_assert_msg(shouldnt_skip(test), "Skipped");
    if( !shouldnt_skip(test) ) {
        fprintf(stderr, "SKIPPED #%d", _i);
        return;
    }

    // Initialize
    ctx = handlebars_context_ctor();
    //ctx->ignore_standalone = test->opt_ignore_standalone;
    compiler = handlebars_compiler_ctor(ctx);

    // Parse
    ctx->tmpl = test->tmpl;
    retval = handlebars_yy_parse(ctx);

    // Check error
    if( ctx->error ) {
        // @todo maybe check message
        ck_assert(test->exception);
        goto done;
    }

    // Compile
    handlebars_compiler_set_flags(compiler, test->flags);
    if( test->known_helpers ) {
        compiler->known_helpers = (const char **) test->known_helpers;
    }

    handlebars_compiler_compile(compiler, ctx->program);
    ck_assert_int_eq(0, compiler->errnum);

    // Setup VM
    vm = handlebars_vm_ctor(ctx);
    vm->flags = test->flags;

    // Setup helpers
    vm->helpers = handlebars_builtins(vm);
    if( test->globalHelpers ) {
        helpers = handlebars_value_from_json_object(ctx, test->globalHelpers);
        load_fixtures(helpers);
        it = handlebars_value_iterator_ctor(helpers);
        for (; it->current != NULL; handlebars_value_iterator_next(it)) {
            //if( it->current->type == HANDLEBARS_VALUE_TYPE_HELPER ) {
                handlebars_map_update(vm->helpers->v.map, it->key, it->current);
            //}
        }
        handlebars_value_delref(helpers);
    }
    if( test->helpers ) {
        helpers = handlebars_value_from_json_object(ctx, test->helpers);
        load_fixtures(helpers);
        it = handlebars_value_iterator_ctor(helpers);
        for (; it->current != NULL; handlebars_value_iterator_next(it)) {
            //if( it->current->type == HANDLEBARS_VALUE_TYPE_HELPER ) {
            handlebars_map_update(vm->helpers->v.map, it->key, it->current);
            //}
        }
        handlebars_value_delref(helpers);
    }

    // Load context
    context = test->context ? handlebars_value_from_json_object(ctx, test->context) : handlebars_value_ctor(ctx);
    handlebars_value_addref(context);
    load_fixtures(context);

    // Load data
    if( test->data ) {
        vm->data = handlebars_value_from_json_object(ctx, test->data);
        handlebars_value_convert(vm->data);
        load_fixtures(vm->data);
    }

    // Execute
    handlebars_vm_execute(vm, compiler, context);



#ifndef NDEBUG
    if( test->expected ) {
        fprintf(stderr, "EXPECTED: %s\n", test->expected);
        fprintf(stderr, "ACTUAL: %s\n", vm->buffer);
        fprintf(stderr, "%s\n", vm->buffer && 0 == strcmp(vm->buffer, test->expected) ? "PASS" : "FAIL");
    } else if( vm->errmsg ) {
        fprintf(stderr, "ERROR: %s\n", vm->errmsg);
    }
#endif

    if( test->exception ) {
        ck_assert_ptr_ne(vm->errmsg, NULL);
//        if( test->message ) {
//            ck_assert_str_eq(vm->errmsg, test->message);
//        }
        if( test->message == NULL ) {
            // Just check if there was an error
            ck_assert_str_ne("", vm->errmsg);
        } else if( test->message[0] == '/' && test->message[strlen(test->message) - 1] == '/' ) {
            // It's a regex
            char * tmp = strdup(test->message + 1);
            tmp[strlen(test->message) - 2] = '\0';
            char * regex_error = NULL;
            if( 0 == regex_compare(tmp, vm->errmsg, &regex_error) ) {
                // ok
            } else {
                ck_assert_msg(0, regex_error);
            }
            free(tmp);
        } else {
            ck_assert_str_eq(test->message, vm->errmsg);
        }
    } else {
        ck_assert_msg(vm->errmsg == NULL, vm->errmsg);
        ck_assert_ptr_ne(test->expected, NULL);
        ck_assert_ptr_ne(vm->buffer, NULL);

        if (strcmp(vm->buffer, test->expected) != 0) {
            char *tmp = handlebars_talloc_asprintf(rootctx,
                                                   "Failed.\nSuite: %s\nTest: %s - %s\nFlags: %ld\nTemplate:\n%s\nExpected:\n%s\nActual:\n%s\n",
                                                   "" /*test->suite_name*/,
                                                   test->description, test->it, test->flags,
                                                   test->tmpl, test->expected, vm->buffer);
            ck_abort_msg(tmp);
        }
    }

    //ck_assert_str_eq(vm->buffer, test->expected);
done:
    handlebars_context_dtor(ctx);
}
END_TEST

Suite * parser_suite(void)
{
    const char * title = "Handlebars Spec";
    TCase * tc_handlebars_spec = tcase_create(title);
    Suite * s = suite_create(title);
    int start = 0;
    int end = tests_len;

    if( getenv("TEST_NUM") != NULL ) {
        int num;
        sscanf(getenv("TEST_NUM"), "%d", &num);
        start = end = num;
        end++;
    }

    // tcase_add_checked_fixture(tc_ ## name, setup, teardown);
    tcase_add_loop_test(tc_handlebars_spec, test_handlebars_spec, start, end);
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
    //loadSpec("./spec/handlebars/spec/bench.json");
    loadSpec("blocks");
    loadSpec("builtins");
    loadSpec("data");
    loadSpec("helpers");
    //loadSpec("partials");
    //loadSpec("regressions");
    //loadSpec("string-params");
    loadSpec("subexpressions");
    //loadSpec("track-ids");
    loadSpec("whitespace-control");
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
