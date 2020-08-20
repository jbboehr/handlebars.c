 /**
 * Copyright (C) 2020 John Boehr
 *
 * This file is part of handlebars.c.
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

#include <yaml.h>

#include <assert.h>
#include <check.h>
#include <stdio.h>
#include <talloc.h>

#include "handlebars.h"
#include "handlebars_memory.h"
#include "handlebars_ast_printer.h"
#include "handlebars_compiler.h"
#include "handlebars_delimiters.h"
#include "handlebars_helpers.h"
#include "handlebars_map.h"
#include "handlebars_opcode_serializer.h"
#include "handlebars_parser.h"
#include "handlebars_string.h"
#include "handlebars_value.h"
#include "handlebars_vm.h"
#include "handlebars_yaml.h"
#include "handlebars.tab.h"
#include "handlebars.lex.h"
#include "utils.h"

// @TODO FIXME for yaml
#pragma GCC diagnostic warning "-Wpointer-sign"



struct mustache_test {
    const char * suite_name;
    char * name;
    char * desc;
    struct handlebars_value * data;
    struct handlebars_value * partials;
    char * tmpl;
    char * expected;
    long flags;
    struct handlebars_context * ctx;
};

static struct mustache_test * tests;
static size_t tests_len = 0;
static size_t tests_size = 0;
static const char * spec_dir;


static bool loadSpecTestPartials(yaml_document_t * document, yaml_node_t * node, struct mustache_test * test)
{
    if( !node || node->type != YAML_MAPPING_NODE ) {
        fprintf(stderr, "Test partials was not a mapping node, was %d\n", node->type);
        return false;
    }

    // ugh
    test->partials = handlebars_talloc_size(test->ctx, HANDLEBARS_VALUE_SIZE);
    handlebars_value_init(test->partials);
    handlebars_value_init_yaml_node(test->ctx, test->partials, document, node);
    return true;
}

static bool loadSpecTest(yaml_document_t * document, yaml_node_t * node, const char * spec)
{
    yaml_node_pair_t * pair;
    struct mustache_test * test;

    if( !node || node->type != YAML_MAPPING_NODE ) {
        fprintf(stderr, "Test value was not a mapping node, was %d\n", node->type);
        return false;
    }

    test = &(tests[tests_len++]);
    memset(test, 0, sizeof(struct mustache_test));
    test->suite_name = spec;
    test->ctx = handlebars_context_ctor_ex(tests);
    test->flags = handlebars_compiler_flag_compat | handlebars_compiler_flag_mustache_style_lambdas;

    for( pair = node->data.mapping.pairs.start; pair < node->data.mapping.pairs.top; pair++ ) {
        yaml_node_t * key = yaml_document_get_node(document, pair->key);
        yaml_node_t * value = yaml_document_get_node(document, pair->value);
        assert(key->type == YAML_SCALAR_NODE);

        if( 0 == strcmp("name", key->data.scalar.value) ) {
            assert(value->type == YAML_SCALAR_NODE);
            test->name = handlebars_talloc_strdup(tests, value->data.scalar.value);
        } else if( 0 == strcmp("desc", key->data.scalar.value) ) {
            assert(value->type == YAML_SCALAR_NODE);
            test->desc = handlebars_talloc_strdup(tests, value->data.scalar.value);
        } else if( 0 == strcmp("data", key->data.scalar.value) ) {
            test->data = handlebars_talloc_size(test->ctx, HANDLEBARS_VALUE_SIZE);
            handlebars_value_init(test->data);
            handlebars_value_init_yaml_node(test->ctx, test->data, document, value);
        } else if( 0 == strcmp("template", key->data.scalar.value) ) {
            assert(value->type == YAML_SCALAR_NODE);
            test->tmpl = handlebars_talloc_strdup(tests, value->data.scalar.value);
        } else if( 0 == strcmp("expected", key->data.scalar.value) ) {
            assert(value->type == YAML_SCALAR_NODE);
            test->expected = handlebars_talloc_strdup(tests, value->data.scalar.value);
        } else if( 0 == strcmp("partials", key->data.scalar.value) ) {
            //assert(value->type == YAML_MAPPING_NODE);
            loadSpecTestPartials(document, value, test);
        } else {
            fprintf(stderr, "Unknown test key: %s\n", key->data.scalar.value);
        }
    }

    return true;
}

static bool loadSpecTests(yaml_document_t * document, yaml_node_t * node, const char * spec)
{
    yaml_node_item_t * item;
    size_t length;

    if( !node || node->type != YAML_SEQUENCE_NODE ) {
        fprintf(stderr, "Tests value was not a sequence node, was %d\n", node->type);
        return false;
    }

    // (Re)Allocate tests array
    length = node->data.sequence.items.top - node->data.sequence.items.start;
    tests_size += length;
    tests = talloc_realloc(root, tests, struct mustache_test, tests_size);

    // Load tests
    for( item = node->data.sequence.items.start; item < node->data.sequence.items.top; item ++) {
        yaml_node_t * value = yaml_document_get_node(document, *item);
        loadSpecTest(document, value, spec);
    }

    return true;
}

static int loadSpec(const char * spec)
{
    int error = 0;
    char filename[1024];
    FILE * fh = NULL;
    yaml_parser_t yaml_parser = {0};
    yaml_document_t document = {{0}};
    yaml_node_pair_t * pair;

    snprintf(filename, 1023, "%s/%s.yml", spec_dir, spec);

    // Initialize parser
    yaml_parser_initialize(&yaml_parser);

    // Open file
    fh = fopen(filename, "r");
    yaml_parser_set_input_file(&yaml_parser, fh);

    // Parse YAML
    yaml_parser_load(&yaml_parser, &document);

    // Root object should be map
    yaml_node_t * root_node = yaml_document_get_root_node(&document);

    if( root_node->type != YAML_MAPPING_NODE ) {
        fprintf(stderr, "Root YAML value was not a map\n");
        error = 1;
        goto error;
    }

    // Scan for tests key
    for( pair = root_node->data.mapping.pairs.start; pair < root_node->data.mapping.pairs.top; pair++ ) {
        yaml_node_t * key = yaml_document_get_node(&document, pair->key);
        yaml_node_t * value = yaml_document_get_node(&document, pair->value);
        if( key->type == YAML_SCALAR_NODE && 0 == strcmp(key->data.scalar.value, "tests") ) {
            loadSpecTests(&document, value, spec);
        }
    }

error:
    yaml_document_delete(&document);
    yaml_parser_delete(&yaml_parser);
    if( fh ) {
        fclose(fh);
    }
    return error;
}

START_TEST(test_ast_to_string_on_mustache_spec)
{
    struct mustache_test * test = &tests[_i];
    struct handlebars_string * origtmpl;
    struct handlebars_string * tmpl;
    struct handlebars_string *ast_str;
    struct handlebars_ast_node * ast;
    const char *actual;
    const char *expected;

    origtmpl = handlebars_string_ctor(HBSCTX(parser), test->tmpl, strlen(test->tmpl));
    handlebars_string_addref(origtmpl); handlebars_string_addref(origtmpl); // ugh
    tmpl = handlebars_preprocess_delimiters(context, origtmpl, NULL, NULL);

    ast = handlebars_parse_ex(parser, tmpl, test->flags);

    // Check error
    if( handlebars_error_num(context) != HANDLEBARS_SUCCESS ) {
        ck_assert_msg(0, "%s", handlebars_error_msg(context));
    }

    ast_str = handlebars_ast_to_string(context, ast);

    actual = normalize_template_whitespace(context, ast_str);
    expected = normalize_template_whitespace(context, tmpl);
    if (strcmp(actual, expected) != 0) {
        char *tmp = handlebars_talloc_asprintf(
            tests,
            "Failed.\nNum: %d\nSuite: %s\nTest: %s - %s\nFlags: %ld\nTemplate:\n%s\nExpected:\n%s\nActual:\n%s\n",
            _i,
            test->suite_name,
            test->name,
            test->desc,
            test->flags,
            test->tmpl,
            expected,
            actual
        );
        ck_abort_msg("%s", tmp);
    }

    handlebars_string_delref(origtmpl);
}
END_TEST

START_TEST(test_mustache_spec)
{
    struct mustache_test * test = &tests[_i];
    struct handlebars_string * tmpl;
    struct handlebars_module * module;

#ifndef NDEBUG
    fprintf(stderr, "-----------\n");
    //fprintf(stderr, "RAW: %s\n", NULL);
    fprintf(stderr, "NUM: %d\n", _i);
    fprintf(stderr, "NAME: %s\n", test->name);
    fprintf(stderr, "DESC: %s\n", test->desc);
    fprintf(stderr, "TMPL: %s\n", test->tmpl);
    if( test->partials ) {
        char * tmp = handlebars_value_dump(test->partials, context, 0);
        fprintf(stderr, "PARTIALS:\n%s\n", tmp);
        talloc_free(tmp);
    }
#endif

    // Initialize
    tmpl = handlebars_string_ctor(HBSCTX(parser), test->tmpl, strlen(test->tmpl));
    tmpl = handlebars_preprocess_delimiters(context, tmpl, NULL, NULL);

    // Parse
    struct handlebars_ast_node * ast = handlebars_parse_ex(parser, tmpl, test->flags);

    // Check error
    if( handlebars_error_num(context) != HANDLEBARS_SUCCESS ) {
        ck_assert_msg(0, "%s", handlebars_error_msg(context));
    }

    // Compile
    handlebars_compiler_set_flags(compiler, test->flags);
    handlebars_compiler_compile(compiler, ast);

    if( handlebars_error_num(context) != HANDLEBARS_SUCCESS ) {
        ck_assert_msg(0, "%s", handlebars_error_msg(context));
    }

    // Serialize
    module = handlebars_program_serialize(HBSCTX(compiler), handlebars_compiler_get_program(compiler));

    // Setup VM
    vm = handlebars_vm_ctor(context);
    handlebars_vm_set_flags(vm, test->flags);

    // Setup partials
    if( test->partials ) {
        // @TODO this may have the wrong parent - might be bad
        handlebars_vm_set_partials(vm, test->partials);
    }

    // Setup input
    struct handlebars_value * input = test->data;
    load_fixtures(input);

    // Execute
    struct handlebars_string * buffer = handlebars_vm_execute(vm, module, input);

#ifndef NDEBUG
    if( test->expected ) {
        fprintf(stderr, "EXPECTED: %s\n", test->expected);
        fprintf(stderr, "ACTUAL: %s\n", hbs_str_val(buffer));
        fprintf(stderr, "%s\n", buffer &&  hbs_str_eq_strl(buffer, test->expected, strlen(test->expected)) ? "PASS" : "FAIL");
    } else if( handlebars_error_msg(context) ) {
        fprintf(stderr, "ERROR: %s\n", handlebars_error_msg(context));
    }
#endif

    // Check error
    if( handlebars_error_num(context) != HANDLEBARS_SUCCESS ) {
        ck_assert_msg(0, "%s", handlebars_error_msg(context));
    }

    ck_assert_ptr_ne(buffer, NULL);

    if (!hbs_str_eq_strl(buffer, test->expected, strlen(test->expected))) {
        char *tmp = handlebars_talloc_asprintf(
            tests,
            "Failed.\n"
            "Num: %d\n"
            "Suite: %s\n"
            "Test: %s - %s\n"
            "Flags: %ld\n"
            "Template:\n%s\n"
            "Data: %s\n"
            "Expected:\n%s\n"
            "Actual:\n%s\n",
            _i,
            test->suite_name,
            test->name, test->desc,
            test->flags,
            test->tmpl,
            handlebars_value_dump(test->data, context, 0),
            test->expected,
            hbs_str_val(buffer)
        );
        ck_abort_msg("%s", tmp);
    }

    // ugh
    // talloc_free(test->ctx);
}
END_TEST

static Suite * suite(void);
static Suite * suite(void)
{
    // Load specs
    loadSpec("comments");
    loadSpec("delimiters");
    loadSpec("interpolation");
    loadSpec("inverted");
    loadSpec("partials");
    loadSpec("sections");
    loadSpec("~lambdas");
    fprintf(stderr, "Loaded %zu test cases\n", tests_len);

    // Setup the suite
    Suite * s = suite_create("Mustache Spec");
    int start = 0;
    int end = tests_len;

    if( getenv("TEST_NUM") != NULL ) {
        int num;
        sscanf(getenv("TEST_NUM"), "%d", &num);
        start = end = num;
        end++;
    }

    TCase * tc_ast_to_string_on_mustache_spec = tcase_create("AST to string on mustache spec");
    tcase_add_checked_fixture(tc_ast_to_string_on_mustache_spec, default_setup, default_teardown);
    tcase_add_loop_test(tc_ast_to_string_on_mustache_spec, test_ast_to_string_on_mustache_spec, start, end);
    suite_add_tcase(s, tc_ast_to_string_on_mustache_spec);

    TCase * tc_mustache_spec = tcase_create("Mustache Spec");
    tcase_add_checked_fixture(tc_mustache_spec, default_setup, default_teardown);
    tcase_add_loop_test(tc_mustache_spec, test_mustache_spec, start, end);
    suite_add_tcase(s, tc_mustache_spec);

    return s;
}

int main(int argc, char *argv[])
{
    // Load the spec
    spec_dir = getenv("mustache_spec_dir");
    if( spec_dir == NULL && argc >= 2 ) {
        spec_dir = argv[1];
    }
    if( spec_dir == NULL ) {
        spec_dir = "./spec/mustache/specs";
    }

    // Run the suite
    return default_main(&suite);
}
