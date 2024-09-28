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
#include "handlebars_memory.h"
#include "handlebars_ast_printer.h"
#include "handlebars_compiler.h"
#include "handlebars_helpers.h"
#include "handlebars_json.h"
#include "handlebars_opcode_serializer.h"
#include "handlebars_map.h"
#include "handlebars_parser.h"
#include "handlebars_string.h"
#include "handlebars_value.h"
#include "handlebars_vm.h"
#include "handlebars.tab.h"
#include "handlebars.lex.h"
#include "utils.h"



struct generic_test {
    const char * suite_name;
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
    char * exceptionMatcher;

    char ** known_helpers;
    long flags;
    const char * raw;
};

static struct generic_test ** tests;
static size_t tests_len = 0;
static size_t tests_size = 0;
static const char * spec_dir;
static int runs = 1;

long json_load_compile_flags(struct json_object * object);
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

char ** json_load_known_helpers(void * ctx, struct json_object * object, struct json_object * helpers);
char ** json_load_known_helpers(void * ctx, struct json_object * object, struct json_object * helpers)
{
    const char ** ptr2 = handlebars_builtins_names();

    // if (!object && !helpers) {
    //     return ptr2;
    // }

    // Let's just allocate a nice fat array >.>
    // @TODO FIXME
    char ** known_helpers = talloc_zero_array(ctx, char *, 32);
    char ** ptr = known_helpers;

    for( ; *ptr2 ; ++ptr2 ) {
        *ptr = handlebars_talloc_strdup(ctx, *ptr2);
        ptr++;
    }

    if (object) {
        json_object_object_foreach(object, key, value) {
            (void) value;
            *ptr = handlebars_talloc_strdup(ctx, key);
            ptr++;
            assert(ptr - known_helpers < 32);
        }
    }

    // Merge in from helpers
    if (helpers) {
        json_object_object_foreach(helpers, key, value) {
            (void) value;
            *ptr = handlebars_talloc_strdup(ctx, key);
            ptr++;
            assert(ptr - known_helpers < 32);
        }
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

static void loadSpecTest(json_object * object, const char *suite_name)
{
    json_object * cur = NULL;
    int nreq = 0;

    // Get test
    struct generic_test * test = tests[tests_len++] = handlebars_talloc_zero(tests, struct generic_test);
    test->suite_name = suite_name;
    test->raw = json_object_to_json_string_ext(object, JSON_C_TO_STRING_PRETTY);

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
        nreq++;
    } else {
        // fprintf(stderr, "Warning: Expected was not a string\n");
    }

    // Get message
    cur = json_object_object_get(object, "message");
    if( cur && json_object_get_type(cur) == json_type_string ) {
        test->message = handlebars_talloc_strdup(test, json_object_get_string(cur));
    }

    // Get exception
    cur = json_object_object_get(object, "exception");
    if( cur && json_object_get_type(cur) == json_type_boolean ) {
        test->exception = json_object_get_boolean(cur);
        nreq++;
    } else if (cur && json_object_get_type(cur) == json_type_string) {
        test->exception = true;
        test->exceptionMatcher = handlebars_talloc_strdup(test, json_object_get_string(cur));
        nreq++;
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

    // Get partials
    if( NULL != (cur = json_object_object_get(object, "partials")) ) {
        test->partials = cur;
    }

    // Get compile options
    cur = json_object_object_get(object, "compileOptions");
    struct json_object * kh = NULL;
    if( cur && json_object_get_type(cur) == json_type_object ) {
        test->flags = json_load_compile_flags(cur);
        // struct json_object * cur2 = json_object_object_get(cur, "knownHelpers");
    }
    test->known_helpers = json_load_known_helpers(test, kh, test->helpers);

    // Check
    if( nreq <= 0 ) {
        fprintf(stderr, "Warning: expected or exception/message must be specified\n");
    }
}

static int loadSpec(const char * spec)
{
    int error = 0;
    char * data = NULL;
    size_t data_len = 0;
    struct json_object * result = NULL;
    struct json_object * array_item = NULL;
    size_t array_len = 0;
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
    tests = talloc_realloc(root, tests, struct generic_test *, tests_size);

    // Iterate over array
    for( size_t i = 0; i < array_len; i++ ) {
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
    HBS_TEST_JSON_DTOR(tests, result);
    return error;
}

START_TEST(test_ast_to_string_on_handlebars_spec)
{
    struct generic_test * test = tests[_i];
    struct handlebars_string * tmpl;
    struct handlebars_string *ast_str;
    const char *actual;
    bool dont_match = false;

    if (test->exception) {
        fprintf(stderr, "SKIPPED #%d\n", _i);
        return;
    }

    tmpl = handlebars_string_ctor(HBSCTX(parser), test->tmpl, strlen(test->tmpl));
    const char *expected = normalize_template_whitespace(parser, tmpl);

    // Won't work with a bunch of shit from handlebars - ast is lossy
    // We're mostly doing this to make sure it won't segfault on handlebars sytax since
    // it's mainly meant to be used with mustache templates
    if (
        NULL != strstr(expected, "else") ||
        NULL != strstr(expected, "[") ||
        NULL != strstr(expected, "{{>(") ||
        NULL != strstr(expected, "\\{{")
    ) {
        dont_match = true;
    }

    struct handlebars_ast_node * ast = handlebars_parse_ex(parser, tmpl, 0);

    // Check error
    if( handlebars_error_num(context) != HANDLEBARS_SUCCESS ) {
        ck_assert_msg(0, "%s", handlebars_error_msg(context));
    }

    ast_str = handlebars_ast_to_string(context, ast);

    actual = normalize_template_whitespace(parser, ast_str);
    if (!dont_match && strcmp(actual, expected) != 0) {
        char *tmp = handlebars_talloc_asprintf(
            root,
            "Failed.\nNum: %d\nSuite: %s\nTest: %s - %s\nFlags: %ld\nTemplate:\n%s\nExpected:\n%s\nActual:\n%s\n",
            _i,
            test->suite_name,
            test->description,
            test->it,
            test->flags,
            test->tmpl,
            expected,
            actual
        );
        ck_abort_msg("%s", tmp);
    }
}
END_TEST

static bool should_skip(struct generic_test * test);
static bool should_skip(struct generic_test * test)
{
#define MYCHECKALL(s, d) \
    if (0 == strcmp(s, test->suite_name) && 0 == strcmp(d, test->description)) return true;
#define MYCHECK(s, d, i) \
    if( 0 == strcmp(s, test->suite_name) && 0 == strcmp(d, test->description) && 0 == strcmp(i, test->it) ) return true;

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
    MYCHECK("regressions", "Regressions", "GH-1186: Support block params for existing programs");

    // Subexpressions
    // This one might need to be handled in the parser
    MYCHECK("subexpressions", "subexpressions", "subexpressions can\'t just be property lookups");
    MYCHECK("subexpressions", "subexpressions", "in string params mode,");
    MYCHECK("subexpressions", "subexpressions", "as hashes in string params mode");
    MYCHECK("subexpressions", "subexpressions", "string params for inner helper processed correctly");

    // Partials
    MYCHECK("partials", "partials", "registering undefined partial throws an exception");
    MYCHECKALL("partials", "partials - inline partials");

    return false;

#undef MYCCHECK
}

static inline void run_test(struct generic_test * test, int _i)
{
    struct handlebars_module * module;

#ifndef NDEBUG
    fprintf(stderr, "-----------\n");
    fprintf(stderr, "RAW: %s\n", test->raw);
    fprintf(stderr, "NUM: %d\n", _i);
    fprintf(stderr, "TMPL: %s\n", test->tmpl);
    fprintf(stderr, "FLAGS: %ld\n", test->flags);
    fflush(stderr);
#endif

    //ck_assert_msg(shouldnt_skip(test), "Skipped");
    if( should_skip(test) ) {
        fprintf(stderr, "SKIPPED #%d\n", _i);
        fflush(stderr);
        return;
    }

    // Parse
    struct handlebars_ast_node * ast = handlebars_parse_ex(parser, handlebars_string_ctor(HBSCTX(parser), test->tmpl, strlen(test->tmpl)), test->flags);

    // Check error
    if( handlebars_error_num(context) != HANDLEBARS_SUCCESS ) {
        // @todo maybe check message
        ck_assert_msg(test->exception, "%s", handlebars_error_msg(context));
        return;
    }

    // Compile
    handlebars_compiler_set_flags(compiler, test->flags);
    if( test->known_helpers ) {
        handlebars_compiler_set_known_helpers(compiler, (const char **) test->known_helpers);
    }

    struct handlebars_program * program = handlebars_compiler_compile_ex(compiler, ast);
    if( handlebars_error_num(context) != HANDLEBARS_SUCCESS ) {
        // @todo check message
        ck_assert_int_eq(1, test->exception);
        return;
    }

    // Serialize
    module = handlebars_program_serialize(context, program);

    // Setup VM
    handlebars_vm_set_flags(vm, test->flags);

    // Setup helpers
    HANDLEBARS_VALUE_DECL(helpers);
    if( test->helpers ) {
        handlebars_value_init_json_object(context, helpers, test->helpers);
        load_fixtures(helpers);
    } else {
        handlebars_value_map(helpers, handlebars_map_ctor(HBSCTX(vm), 0));
    }
    handlebars_vm_set_helpers(vm, helpers);

    // Setup partials
    HANDLEBARS_VALUE_DECL(partials);
    if( test->partials ) {
        handlebars_value_init_json_object(context, partials, test->partials);
        load_fixtures(partials);
    } else {
        handlebars_value_map(partials, handlebars_map_ctor(HBSCTX(vm), 0));
    }
    handlebars_vm_set_partials(vm, partials);

    // Load context
    HANDLEBARS_VALUE_DECL(input);
    if (test->context) {
        handlebars_value_init_json_object(context, input, test->context);
        load_fixtures(input);
    }

    // Load data
    HANDLEBARS_VALUE_DECL(data);
    if( test->data ) {
        handlebars_value_init_json_object(context, data, test->data);
        load_fixtures(data);
        handlebars_vm_set_data(vm, data);
    }

    // Execute
    struct handlebars_string * buffer = handlebars_vm_execute(vm, module, input);

#ifndef NDEBUG
    fprintf(stderr, "TMPL %s\n", test->tmpl);
    if( test->expected ) {
        fprintf(stderr, "EXPECTED: %s\n", test->expected);
        fprintf(stderr, "ACTUAL: %s\n", buffer ? hbs_str_val(buffer) : "(nil)");
        fprintf(stderr, "%s\n", buffer && hbs_str_eq_strl(buffer, test->expected, strlen(test->expected)) ? "PASS" : "FAIL");
    } else if( context->e->msg ) {
        fprintf(stderr, "ERROR: %s\n", context->e->msg);
    }
    fflush(stderr);
#endif

    if( test->exception ) {
        ck_assert_ptr_ne(handlebars_error_msg(HBSCTX(vm)), NULL);
//        if( test->message ) {
//            ck_assert_str_eq(vm->errmsg, test->message);
//        }
        if( test->exceptionMatcher == NULL ) {
            // Just check if there was an error
            ck_assert_str_ne("", handlebars_error_msg(HBSCTX(vm)));
        } else if( test->exceptionMatcher[0] == '/' && test->exceptionMatcher[strlen(test->exceptionMatcher) - 1] == '/' ) {
            // It's a regex
            char * tmp = strdup(test->exceptionMatcher + 1);
            tmp[strlen(test->exceptionMatcher) - 2] = '\0';
            char * regex_error = NULL;
            if( 0 == regex_compare(tmp, handlebars_error_msg(HBSCTX(vm)), &regex_error) ) {
                // ok
            } else {
                ck_assert_msg(0, "%s", regex_error);
            }
            free(tmp);
        } else {
            ck_assert_str_eq(test->exceptionMatcher, handlebars_error_msg(HBSCTX(vm)));
        }
    } else {
        ck_assert_msg(handlebars_error_msg(HBSCTX(vm)) == NULL, "%s", handlebars_error_msg(HBSCTX(vm)));
        ck_assert_ptr_ne(test->expected, NULL);
        ck_assert_ptr_ne(buffer, NULL);

        if (!hbs_str_eq_strl(buffer, test->expected, strlen(test->expected))) {
            char *tmp = handlebars_talloc_asprintf(root,
                                                   "Failed.\nSuite: %s\nTest: %s - %s\nFlags: %ld\nTemplate:\n%s\nExpected:\n%s\nActual:\n%s\n",
                                                   "" /*test->suite_name*/,
                                                   test->description, test->it, test->flags,
                                                   test->tmpl, test->expected, hbs_str_val(buffer));
            ck_abort_msg("%s", tmp);
        }
    }

    HANDLEBARS_VALUE_UNDECL(data);
    HANDLEBARS_VALUE_UNDECL(input);
    HANDLEBARS_VALUE_UNDECL(partials);
    HANDLEBARS_VALUE_UNDECL(helpers);

    // Memdebug
    // handlebars_vm_dtor(vm);
    // if( memdebug ) {
    //     talloc_report_full(context, stderr);
    // }

    // ck_assert_int_eq(1, talloc_total_blocks(memctx));
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

static Suite * suite(void);
static Suite * suite(void)
{
    // Load the spec
    loadSpec("basic");
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
    fprintf(stderr, "Loaded %zu test cases\n", tests_len);

    // Setup the suite
    const char * title = "Handlebars Spec";
    TCase * tc_handlebars_spec = tcase_create(title);
    Suite * s = suite_create(title);
    int start = 0;
    int end = tests_len;

    if( getenv("TEST_NUM") != NULL ) {
        int num;
        sscanf(getenv("TEST_NUM"), "%d", &num);
        if (num < end && num >= start) {
            start = end = num;
            end++;
        }
    }

    TCase * tc_ast_to_string_on_handlebars_spec = tcase_create("AST to string on handlebars spec");
    tcase_add_checked_fixture(tc_ast_to_string_on_handlebars_spec, default_setup, default_teardown);
    tcase_add_loop_test(tc_ast_to_string_on_handlebars_spec, test_ast_to_string_on_handlebars_spec, start, end);
    suite_add_tcase(s, tc_ast_to_string_on_handlebars_spec);

    tcase_add_checked_fixture(tc_handlebars_spec, default_setup, default_teardown);
    tcase_add_loop_test(tc_handlebars_spec, test_handlebars_spec, start, end);
    suite_add_tcase(s, tc_handlebars_spec);

    return s;
}

int main(int argc, char *argv[])
{
    // Get runs
    if( getenv("TEST_RUNS") ) {
        runs = atoi(getenv("TEST_RUNS"));
    }

    // Load the spec
    spec_dir = getenv("handlebars_spec_dir");
    if( spec_dir == NULL && argc >= 2 ) {
        spec_dir = argv[1];
    }
    if( spec_dir == NULL ) {
        spec_dir = "./spec/handlebars/spec";
    }

    // Run the suite
    return default_main(&suite);
}
