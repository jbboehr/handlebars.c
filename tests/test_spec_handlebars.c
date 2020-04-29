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

#include "handlebars_ast_printer.h"
#include "handlebars_compiler.h"
#include "handlebars_helpers.h"
#include "handlebars_opcode_serializer.h"
#include "handlebars_string.h"
#include "handlebars_value.h"
#include "handlebars_vm.h"
#include "handlebars.tab.h"
#include "handlebars.lex.h"

#include "utils.h"

struct generic_test {
    const char * suiteName;
    char * description;
    char * it;
    char * tmpl;
    struct json_object * context;
    struct json_object * data;
    struct json_object * helpers;
    struct json_object * globalHelpers;
    struct json_object * partials;
    struct json_object * globalPartials;
    char * expected;
    char * message;
    short exception;
    //struct handlebars_context * ctx;

    char ** known_helpers;
    long flags;
    struct json_object * raw;
};

static int memdebug;
static TALLOC_CTX * rootctx;
TALLOC_CTX * memctx;
static struct generic_test ** tests;
static size_t tests_len = 0;
static size_t tests_size = 0;
static char * spec_dir;
static int runs = 1;

long json_load_compile_flags(struct json_object * object)
{
    long flags = 0;
    json_object * cur = NULL;

    if( (cur = json_object_object_get(object, "compat")) && json_object_get_boolean(cur) ) {
        flags |= handlebars_compiler_flag_compat;
    }
    if( (cur = json_object_object_get(object, "data")) && json_object_get_boolean(cur) ) {
        flags |= handlebars_compiler_flag_compat; // @todo correct?
    }
    if( (cur = json_object_object_get(object, "knownHelpersOnly")) && json_object_get_boolean(cur) ) {
        flags |= handlebars_compiler_flag_known_helpers_only;
    }
    if( (cur = json_object_object_get(object, "stringParams")) && json_object_get_boolean(cur) ) {
        flags |= handlebars_compiler_flag_string_params;
    }
    if( (cur = json_object_object_get(object, "trackIds")) && json_object_get_boolean(cur) ) {
        flags |= handlebars_compiler_flag_track_ids;
    }
    if( (cur = json_object_object_get(object, "preventIndent")) && json_object_get_boolean(cur) ) {
        flags |= handlebars_compiler_flag_prevent_indent;
    }
    if( (cur = json_object_object_get(object, "explicitPartialContext")) && json_object_get_boolean(cur) ) {
        flags |= handlebars_compiler_flag_explicit_partial_context;
    }
    if( (cur = json_object_object_get(object, "ignoreStandalone")) && json_object_get_boolean(cur) ) {
        flags |= handlebars_compiler_flag_ignore_standalone;
    }
    if( (cur = json_object_object_get(object, "strict")) && json_object_get_boolean(cur) ) {
        flags |= handlebars_compiler_flag_strict;
    }
    if( (cur = json_object_object_get(object, "assumeObjects")) && json_object_get_boolean(cur) ) {
        flags |= handlebars_compiler_flag_assume_objects;
    }

    return flags;
}

char ** json_load_known_helpers(void * ctx, struct json_object * object)
{
    struct json_object * array_item = NULL;
    int array_len = 0;
    // Let's just allocate a nice fat array >.>
    char ** known_helpers = talloc_zero_array(ctx, char *, 32);
    char ** ptr = known_helpers;
    const char ** ptr2 = handlebars_builtins_names();

    for( ; *ptr2 ; ++ptr2 ) {
        *ptr = handlebars_talloc_strdup(ctx, *ptr2);
        ptr++;
    }

    json_object_object_foreach(object, key, value) {
        *ptr = handlebars_talloc_strdup(ctx, key);
        ptr++;
    }

    *ptr++ = NULL;

    return known_helpers;
}

static void loadRuntimeOptions(struct generic_test * test, json_object * object)
{
    json_object * cur = NULL;

    // Get data
    cur = json_object_object_get(object, "data");
    if( cur ) {
        test->data = cur;
    }
}

static void loadSpecTest(json_object * object, const char *suiteName)
{
    json_object * cur = NULL;

    // Get test
    struct generic_test * test = tests[tests_len++] = handlebars_talloc_zero(tests, struct generic_test);
    test->suiteName = suiteName;
    test->raw = object;

    // Get description
    cur = json_object_object_get(object, "description");
    if( cur && json_object_get_type(cur) == json_type_string ) {
        test->description = handlebars_talloc_strdup(test, json_object_get_string(cur));
    }

    // Get it
    cur = json_object_object_get(object, "it");
    if( cur && json_object_get_type(cur) == json_type_string ) {
        test->it = handlebars_talloc_strdup(test, json_object_get_string(cur));
    }

    // Get template
    cur = json_object_object_get(object, "template");
    if( cur && json_object_get_type(cur) == json_type_string ) {
        test->tmpl = handlebars_talloc_strdup(test, json_object_get_string(cur));
    }

    // Get expected
    cur = json_object_object_get(object, "expected");
    if( cur && json_object_get_type(cur) == json_type_string ) {
        test->expected = handlebars_talloc_strdup(test, json_object_get_string(cur));
    } else {
        // fprintf(stderr, "Warning: Expected was not a string\n");
    }

    // Get message
    // cur = json_object_object_get(object, "message");
    // if( cur && json_object_get_type(cur) == json_type_string ) {
    //     test->message = handlebars_talloc_strdup(test, json_object_get_string(cur));
    // }

    // Get exception
    cur = json_object_object_get(object, "exception");
    if( cur && json_object_get_type(cur) == json_type_boolean ) {
        test->exception = json_object_get_boolean(cur);
    } else if (cur && json_object_get_type(cur) == json_type_string) {
        test->exception = true;
        test->message = handlebars_talloc_strdup(test, json_object_get_string(cur));
    }

    // Get data
    cur = json_object_object_get(object, "data");
    if( cur ) {
        test->context = cur;
    } else {
        // fprintf(stderr, "Warning: Data was not set\n");
    }

    // Get options
    cur = json_object_object_get(object, "runtimeOptions");
    if( cur && json_object_get_type(cur) == json_type_object ) {
        loadRuntimeOptions(test, cur);
    }

    // Get helpers
    if( NULL != (cur = json_object_object_get(object, "helpers")) ) {
        test->helpers = cur;
    }
    if( NULL != (cur = json_object_object_get(object, "globalHelpers")) ) {
        test->globalHelpers = cur;
    }

    // Get partials
    if( NULL != (cur = json_object_object_get(object, "partials")) ) {
        test->partials = cur;
    }
    if( NULL != (cur = json_object_object_get(object, "globalPartials")) ) {
        test->globalPartials = cur;
    }

    // Get compile options
    cur = json_object_object_get(object, "compileOptions");
    if( cur && json_object_get_type(cur) == json_type_object ) {
        test->flags = json_load_compile_flags(cur);
        struct json_object * cur2 = json_object_object_get(cur, "knownHelpers");
        if( cur2 ) {
            test->known_helpers = json_load_known_helpers(test, cur2);
        }
    }
}

static int loadSpec(const char * spec)
{
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
    tests = talloc_realloc(rootctx, tests, struct generic_test *, tests_size);

    // Iterate over array
    for( int i = 0; i < array_len; i++ ) {
        array_item = json_object_array_get_idx(result, i);
        if( json_object_get_type(array_item) != json_type_object ) {
            fprintf(stderr, "Warning: test case was not an object\n");
            continue;
        }
        loadSpecTest(array_item, spec);
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

START_TEST(test_ast_to_string_on_handlebars_spec)
{
    struct generic_test * test = tests[_i];
    struct handlebars_context * ctx;
    struct handlebars_parser * parser;
    struct handlebars_string * tmpl;
    struct handlebars_string *ast_str;
    const char *actual;
    TALLOC_CTX * memctx = talloc_new(rootctx);

    ctx = handlebars_context_ctor_ex(memctx);
    parser = handlebars_parser_ctor(ctx);

    tmpl = handlebars_string_ctor(HBSCTX(parser), test->tmpl, strlen(test->tmpl));
    const char *expected = normalize_template_whitespace(memctx, tmpl->val, tmpl->len);

    // Won't work with a bunch of shit from handlebars - ast is lossy
    // We're mostly doing this to make sure it won't segfault on handlebars sytax since
    // it's mainly meant to be used with mustache templates
    if (test->exception || NULL != strstr(expected, "{{&") || NULL != strstr(expected, "{{else") ||
            NULL != strstr(expected, "{{!--") || NULL != strstr(expected, "[") || NULL != strstr(expected, "{{>(") ||
            NULL != strstr(expected, "\\{{") || NULL != strstr(tmpl->val, "{{'") ) {
        fprintf(stderr, "SKIPPED #%d\n", _i);
        goto done;
    }

    parser->tmpl = tmpl;
    handlebars_parse(parser);

    // Check error
    if( handlebars_error_num(ctx) != HANDLEBARS_SUCCESS ) {
        ck_assert_msg(0, handlebars_error_msg(ctx));
    }

    ast_str = handlebars_ast_to_string(ctx, parser->program);

    actual = normalize_template_whitespace(memctx, ast_str->val, ast_str->len);
    if (strcmp(actual, expected) != 0) {
            char *tmp = handlebars_talloc_asprintf(rootctx,
                                                   "Failed.\nSuite: %s\nTest: %s - %s\nFlags: %ld\nTemplate:\n%s\nExpected:\n%s\nActual:\n%s\n",
                                                   "" /*test->suite_name*/,
                                                   test->description, test->it, test->flags,
                                                   test->tmpl, expected, actual);
        ck_abort_msg(tmp);
    }

done:
    talloc_free(memctx);
}
END_TEST

int shouldnt_skip(struct generic_test * test)
{
#define MYCHECKALL(s, d) \
    if (0 == strcmp(s, test->suiteName) && 0 == strcmp(d, test->description)) return 0;
#define MYCHECK(s, d, i) \
    if( 0 == strcmp(s, test->suiteName) && 0 == strcmp(d, test->description) && 0 == strcmp(i, test->it) ) return 0;

    // Still having issues with whitespace
    MYCHECK("blocks", "blocks - standalone sections", "block standalone else sections can be disabled");

    // Decorators aren't implemented
    MYCHECKALL("blocks", "blocks - decorators");
    MYCHECK("helpers", "helpers - block params", "should take presednece over parent block params");

    // Regressions
    MYCHECK("regressions", "Regressions", "GH-1065: Sparse arrays")
    MYCHECK("regressions", "Regressions", "should support multiple levels of inline partials")
    MYCHECK("regressions", "Regressions", "GH-1089: should support failover content in multiple levels of inline partials")
    MYCHECK("regressions", "Regressions", "GH-1099: should support greater than 3 nested levels of inline partials");
    MYCHECK("regressions", "Regressions", "GH-1341: 4.0.7 release breaks {{#if @partial-block}} usage");
    MYCHECK("regressions", "Regressions", "GH-1186: Support block params for existing programs");

    // Subexpressions
    // This one might need to be handled in the parser
    MYCHECK("subexpressions", "subexpressions", "subexpressions can\'t just be property lookups");
    MYCHECK("subexpressions", "subexpressions", "in string params mode,");
    MYCHECK("subexpressions", "subexpressions", "as hashes in string params mode");
    MYCHECK("subexpressions", "subexpressions", "string params for inner helper processed correctly");

    // Partials
    MYCHECK("partials", "partials", "registering undefined partial throws an exception");
    MYCHECKALL("partials", "partials - partial blocks");
    MYCHECKALL("partials", "partials - inline partials");

    return 1;

#undef MYCCHECK
}

static inline void run_test(struct generic_test * test, int _i)
{
    struct handlebars_context * ctx;
    struct handlebars_compiler * compiler;
    struct handlebars_parser * parser;
    struct handlebars_vm * vm;
    struct handlebars_value * context;
    struct handlebars_value * helpers;
    struct handlebars_value_iterator it;
    struct handlebars_module * module;

#ifndef NDEBUG
    fprintf(stderr, "-----------\n");
    fprintf(stderr, "RAW: %s\n", json_object_to_json_string_ext(test->raw, JSON_C_TO_STRING_PRETTY));
    fprintf(stderr, "NUM: %d\n", _i);
    fprintf(stderr, "TMPL: %s\n", test->tmpl);
    fprintf(stderr, "FLAGS: %ld\n", test->flags);
#endif

    //ck_assert_msg(shouldnt_skip(test), "Skipped");
    if( !shouldnt_skip(test) ) {
        fprintf(stderr, "SKIPPED #%d\n", _i);
        return;
    }

    // Initialize
    ctx = handlebars_context_ctor_ex(memctx);
    parser = handlebars_parser_ctor(ctx);
    //ctx->ignore_standalone = test->opt_ignore_standalone;
    compiler = handlebars_compiler_ctor(ctx);

    // Parse
    parser->tmpl = handlebars_string_ctor(HBSCTX(parser), test->tmpl, strlen(test->tmpl));
    handlebars_parse(parser);

    // Check error
    if( handlebars_error_num(ctx) != HANDLEBARS_SUCCESS ) {
        // @todo maybe check message
        ck_assert_msg(test->exception, handlebars_error_msg(ctx));
        goto done;
    }

    // Compile
    handlebars_compiler_set_flags(compiler, test->flags);
    if( test->known_helpers ) {
        compiler->known_helpers = (const char **) test->known_helpers;
    }

    handlebars_compiler_compile(compiler, parser->program);
    if( handlebars_error_num(ctx) != HANDLEBARS_SUCCESS ) {
        // @todo check message
        ck_assert_int_eq(1, test->exception);
        goto done;
    }

    // Serialize
    module = handlebars_program_serialize(ctx, compiler->program);
    handlebars_compiler_dtor(compiler);

    // Setup VM
    vm = handlebars_vm_ctor(ctx);
    vm->flags = test->flags;

    // Setup helpers
    vm->helpers = handlebars_value_ctor(HBSCTX(vm));
    handlebars_value_map_init(vm->helpers);
    if( test->globalHelpers ) {
        helpers = handlebars_value_from_json_object(ctx, test->globalHelpers);
        load_fixtures(helpers);
        handlebars_value_iterator_init(&it, helpers);
        for (; it.current != NULL; it.next(&it)) {
            //if( it->current->type == HANDLEBARS_VALUE_TYPE_HELPER ) {
                handlebars_map_update(vm->helpers->v.map, it.key, it.current);
            //}
        }
        handlebars_value_delref(helpers);
    }
    if( test->helpers ) {
        helpers = handlebars_value_from_json_object(ctx, test->helpers);
        load_fixtures(helpers);
        handlebars_value_iterator_init(&it, helpers);
        for (; it.current != NULL; it.next(&it)) {
            //if( it->current->type == HANDLEBARS_VALUE_TYPE_HELPER ) {
            handlebars_map_update(vm->helpers->v.map, it.key, it.current);
            //}
        }
        handlebars_value_delref(helpers);
    }

    // Setup partials
    vm->partials = handlebars_value_ctor(ctx);
    handlebars_value_map_init(vm->partials);
    if( test->globalPartials ) {
        struct handlebars_value * partials = handlebars_value_from_json_object(ctx, test->globalPartials);
        load_fixtures(partials);
        handlebars_value_iterator_init(&it, partials);
        for (; it.current != NULL; it.next(&it)) {
            handlebars_map_update(vm->partials->v.map, it.key, it.current);
        }
        handlebars_value_delref(partials);
    }
    if( test->partials ) {
        struct handlebars_value * partials = handlebars_value_from_json_object(ctx, test->partials);
        load_fixtures(partials);
        handlebars_value_iterator_init(&it, partials);
        for (; it.current != NULL; it.next(&it)) {
            handlebars_map_update(vm->partials->v.map, it.key, it.current);
        }
        handlebars_value_delref(partials);
    }

    // Load context
    context = test->context ? handlebars_value_from_json_object(ctx, test->context) : handlebars_value_ctor(ctx);
    load_fixtures(context);

    // Load data
    if( test->data ) {
        vm->data = handlebars_value_from_json_object(ctx, test->data);
        load_fixtures(vm->data);
    }

    // Execute
    handlebars_vm_execute(vm, module, context);

#ifndef NDEBUG
    fprintf(stderr, "TMPL %s\n", test->tmpl);
    if( test->expected ) {
        fprintf(stderr, "EXPECTED: %s\n", test->expected);
        fprintf(stderr, "ACTUAL: %s\n", vm->buffer->val);
        fprintf(stderr, "%s\n", vm->buffer && 0 == strcmp(vm->buffer->val, test->expected) ? "PASS" : "FAIL");
    } else if( ctx->e->msg ) {
        fprintf(stderr, "ERROR: %s\n", ctx->e->msg);
    }
#endif

    if( test->exception ) {
        ck_assert_ptr_ne(handlebars_error_msg(HBSCTX(vm)), NULL);
//        if( test->message ) {
//            ck_assert_str_eq(vm->errmsg, test->message);
//        }
        if( test->message == NULL ) {
            // Just check if there was an error
            ck_assert_str_ne("", handlebars_error_msg(HBSCTX(vm)));
        } else if( test->message[0] == '/' && test->message[strlen(test->message) - 1] == '/' ) {
            // It's a regex
            char * tmp = strdup(test->message + 1);
            tmp[strlen(test->message) - 2] = '\0';
            char * regex_error = NULL;
            if( 0 == regex_compare(tmp, handlebars_error_msg(HBSCTX(vm)), &regex_error) ) {
                // ok
            } else {
                ck_assert_msg(0, regex_error);
            }
            free(tmp);
        } else {
            ck_assert_str_eq(test->message, handlebars_error_msg(HBSCTX(vm)));
        }
    } else {
        ck_assert_msg(handlebars_error_msg(HBSCTX(vm)) == NULL, handlebars_error_msg(HBSCTX(vm)));
        ck_assert_ptr_ne(test->expected, NULL);
        ck_assert_ptr_ne(vm->buffer, NULL);

        if (strcmp(vm->buffer->val, test->expected) != 0) {
            char *tmp = handlebars_talloc_asprintf(rootctx,
                                                   "Failed.\nSuite: %s\nTest: %s - %s\nFlags: %ld\nTemplate:\n%s\nExpected:\n%s\nActual:\n%s\n",
                                                   "" /*test->suite_name*/,
                                                   test->description, test->it, test->flags,
                                                   test->tmpl, test->expected, vm->buffer->val);
            ck_abort_msg(tmp);
        }
    }

    // Memdebug
    handlebars_value_delref(context);
    handlebars_value_delref(vm->helpers);
    handlebars_value_delref(vm->partials);
    handlebars_value_try_delref(vm->data);
    handlebars_vm_dtor(vm);
    if( memdebug ) {
        talloc_report_full(ctx, stderr);
    }

done:
    handlebars_context_dtor(ctx);
    ck_assert_int_eq(1, talloc_total_blocks(memctx));
}

START_TEST(test_handlebars_spec)
{
    struct generic_test * test = tests[_i];
    int i;

    for( i = 0; i < runs; i++ ) {
        run_test(test, _i);
    }
}
END_TEST

static void setup(void)
{
    memctx = talloc_new(rootctx);
}

static void teardown(void)
{
    talloc_free(memctx);
    memctx = NULL;
}

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

    TCase * tc_ast_to_string_on_handlebars_spec = tcase_create("AST to string on handlebars spec");
    tcase_add_loop_test(tc_ast_to_string_on_handlebars_spec, test_ast_to_string_on_handlebars_spec, start, end);
    suite_add_tcase(s, tc_ast_to_string_on_handlebars_spec);

    // tcase_add_checked_fixture(tc_ ## name, setup, teardown);
    tcase_add_loop_test(tc_handlebars_spec, test_handlebars_spec, start, end);
    tcase_add_checked_fixture(tc_handlebars_spec, setup, teardown);
    suite_add_tcase(s, tc_handlebars_spec);

    return s;
}

int main(int argc, char *argv[])
{
    int number_failed;
    int error;

    talloc_set_log_stderr();

    // Check if memdebug enabled
    memdebug = getenv("MEMDEBUG") ? atoi(getenv("MEMDEBUG")) : 0;
    if( memdebug ) {
        talloc_enable_leak_report_full();
    }
    rootctx = talloc_new(NULL);

    // Get runs
    if( getenv("TEST_RUNS") ) {
        runs = atoi(getenv("TEST_RUNS"));
    }

    // Load specs
    // Load the spec
    spec_dir = getenv("handlebars_spec_dir");
    if( spec_dir == NULL && argc >= 2 ) {
        spec_dir = argv[1];
    }
    if( spec_dir == NULL ) {
        spec_dir = "./spec/handlebars/spec";
    }
    loadSpec("basic");
    loadSpec("bench");
    loadSpec("blocks");
    loadSpec("builtins");
    loadSpec("data");
    loadSpec("helpers");
    loadSpec("partials");
    loadSpec("regressions");
    loadSpec("strict");
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


    // Generate report for memdebug
    if( memdebug ) {
        // What should we free here?
        handlebars_talloc_free(rootctx);
        //handlebars_talloc_free(tests);
        talloc_report_full(NULL, stderr);
    }

    // Return
    return error;
}
