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

#if defined(HAVE_LIBYAML)
#include <yaml.h>
#endif

#if defined(HAVE_JSON_C_JSON_H) || defined(JSONC_INCLUDE_WITH_C)
#include <json-c/json.h>
#include <json-c/json_object.h>
#include <json-c/json_tokener.h>
#elif defined(HAVE_JSON_JSON_H) || defined(HAVE_LIBJSONC)
#include <json/json.h>
#include <json/json_object.h>
#include <json/json_tokener.h>
#endif

#include <assert.h>
#include <check.h>
#include <stdio.h>
#include <talloc.h>
#include <yaml.h>

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



struct mustache_test {
    char * name;
    char * desc;
    struct handlebars_value * data;
    struct handlebars_value * partials;
    char * tmpl;
    char * expected;
    long flags;
//    char * message;
//    short exception;
    struct handlebars_context * ctx;

//    char ** known_helpers;
//    struct json_object * raw;
};

static TALLOC_CTX * rootctx;
static struct mustache_test * tests;
static size_t tests_len = 0;
static size_t tests_size = 0;
static char * spec_dir;


static bool loadSpecTestPartials(yaml_document_t * document, yaml_node_t * node, struct mustache_test * test)
{
    yaml_node_pair_t * pair;

    if( !node || node->type != YAML_MAPPING_NODE ) {
        fprintf(stderr, "Test partials was not a mapping node, was %d\n", node->type);
        return false;
    }

    test->partials = handlebars_value_from_yaml_node(test->ctx, document, node);
    /*for( pair = node->data.mapping.pairs.start; pair < node->data.mapping.pairs.top; pair++ ) {

    }*/
}

static bool loadSpecTest(yaml_document_t * document, yaml_node_t * node)
{
    yaml_node_pair_t * pair;
    struct mustache_test * test;

    if( !node || node->type != YAML_MAPPING_NODE ) {
        fprintf(stderr, "Test value was not a mapping node, was %d\n", node->type);
        return false;
    }

    test = &(tests[tests_len++]);
    memset(test, 0, sizeof(struct mustache_test));
    test->ctx = handlebars_context_ctor_ex(rootctx);
    test->flags = handlebars_compiler_flag_compat | handlebars_compiler_flag_mustache_style_lambdas;

    for( pair = node->data.mapping.pairs.start; pair < node->data.mapping.pairs.top; pair++ ) {
        yaml_node_t * key = yaml_document_get_node(document, pair->key);
        yaml_node_t * value = yaml_document_get_node(document, pair->value);
        assert(key->type == YAML_SCALAR_NODE);

        if( 0 == strcmp("name", key->data.scalar.value) ) {
            assert(value->type == YAML_SCALAR_NODE);
            test->name = handlebars_talloc_strdup(rootctx, value->data.scalar.value);
        } else if( 0 == strcmp("desc", key->data.scalar.value) ) {
            assert(value->type == YAML_SCALAR_NODE);
            test->desc = handlebars_talloc_strdup(rootctx, value->data.scalar.value);
        } else if( 0 == strcmp("data", key->data.scalar.value) ) {
            test->data = handlebars_value_from_yaml_node(test->ctx, document, value);
        } else if( 0 == strcmp("template", key->data.scalar.value) ) {
            assert(value->type == YAML_SCALAR_NODE);
            test->tmpl = handlebars_talloc_strdup(rootctx, value->data.scalar.value);
        } else if( 0 == strcmp("expected", key->data.scalar.value) ) {
            assert(value->type == YAML_SCALAR_NODE);
            test->expected = handlebars_talloc_strdup(rootctx, value->data.scalar.value);
        } else if( 0 == strcmp("partials", key->data.scalar.value) ) {
            //assert(value->type == YAML_MAPPING_NODE);
            loadSpecTestPartials(document, value, test);
        } else {
            fprintf(stderr, "Unknown test key: %s\n", key->data.scalar.value);
        }
    }

    return true;
}

static bool loadSpecTests(yaml_document_t * document, yaml_node_t * node)
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
    tests = talloc_realloc(rootctx, tests, struct mustache_test, tests_size);

    // Load tests
    for( item = node->data.sequence.items.start; item < node->data.sequence.items.top; item ++) {
        yaml_node_t * value = yaml_document_get_node(document, *item);
        loadSpecTest(document, value);
    }

    return true;
}

static int loadSpec(const char * spec)
{
    int error = 0;
    char * data = NULL;
    size_t data_len = 0;
    int array_len = 0;
    char filename[1024];
    FILE * fh = NULL;
    yaml_parser_t parser = {0};
    yaml_document_t document = {0};
    yaml_node_pair_t * pair;

    snprintf(filename, 1023, "%s/%s.yml", spec_dir, spec);

    // Initialize parser
    yaml_parser_initialize(&parser);

    // Open file
    fh = fopen(filename, "r");
    yaml_parser_set_input_file(&parser, fh);

    // Parse YAML
    yaml_parser_load(&parser, &document);

    // Root object should be map
    yaml_node_t * root = yaml_document_get_root_node(&document);

    if( root->type != YAML_MAPPING_NODE ) {
        fprintf(stderr, "Root YAML value was not a map\n");
        error = 1;
        goto error;
    }

    // Scan for tests key
    for( pair = root->data.mapping.pairs.start; pair < root->data.mapping.pairs.top; pair++ ) {
        yaml_node_t * key = yaml_document_get_node(&document, pair->key);
        yaml_node_t * value = yaml_document_get_node(&document, pair->value);
        if( key->type == YAML_SCALAR_NODE && 0 == strcmp(key->data.scalar.value, "tests") ) {
            loadSpecTests(&document, value);
        }
    }

error:
    yaml_document_delete(&document);
    yaml_parser_delete(&parser);
    if( fh ) {
        fclose(fh);
    }
    return error;
}

int shouldnt_skip(struct mustache_test * test)
{
#define MYCHECK(d, i) \
    if( 0 == strcmp(d, test->name) && 0 == strcmp(i, test->desc) ) return 0;

    MYCHECK("Standalone Indentation", "Each line of the partial should be indented before rendering.");
    MYCHECK("Section - Alternate Delimiters", "Lambdas used for sections should parse with the current delimiters.");

    return 1;

#undef MYCCHECK
}

START_TEST(test_ast_to_string_on_mustache_spec)
{
    struct mustache_test * test = &tests[_i];
    struct handlebars_context * ctx;
    struct handlebars_parser * parser;
    struct handlebars_string * origtmpl;
    struct handlebars_string * tmpl;
    struct handlebars_string *ast_str;
    const char *actual;
    const char *expected;
    TALLOC_CTX * memctx = talloc_new(rootctx);

    ctx = handlebars_context_ctor_ex(memctx);
    parser = handlebars_parser_ctor(ctx);

    origtmpl = handlebars_string_ctor(HBSCTX(parser), test->tmpl, strlen(test->tmpl));
    tmpl = handlebars_preprocess_delimiters(ctx, origtmpl, NULL, NULL);

    // Won't work with custom delimters or with '{{&'
    if (!handlebars_string_eq(origtmpl, tmpl) || NULL != strstr(origtmpl->val, "{{&")) {
        fprintf(stderr, "SKIPPED #%d\n", _i);
        goto done;
    }

    parser->tmpl = tmpl;
    handlebars_parse(parser);

    // Check error
    if( handlebars_error_num(ctx) != HANDLEBARS_SUCCESS ) {
        ck_assert_msg(0, "%s", handlebars_error_msg(ctx));
    }

    ast_str = handlebars_ast_to_string(ctx, parser->program);

    actual = normalize_template_whitespace(memctx, ast_str->val, ast_str->len);
    expected = normalize_template_whitespace(memctx, tmpl->val, tmpl->len);
    if (strcmp(actual, expected) != 0) {
        char *tmp = handlebars_talloc_asprintf(rootctx,
                                               "Failed.\nSuite: %s\nTest: %s - %s\nFlags: %ld\nTemplate:\n%s\nExpected:\n%s\nActual:\n%s\n",
                                               "" /*test->suite_name*/,
                                               test->name, test->desc, test->flags,
                                               test->tmpl, expected, actual);
        ck_abort_msg("%s", tmp);
    }

done:
    talloc_free(memctx);
}
END_TEST

START_TEST(test_mustache_spec)
{
    struct mustache_test * test = &tests[_i];
    struct handlebars_context * ctx;
    struct handlebars_parser * parser;
    struct handlebars_compiler * compiler;
    struct handlebars_vm * vm;
    struct handlebars_value_iterator it;
    struct handlebars_string * tmpl;
    struct handlebars_module * module;
    TALLOC_CTX * memctx = talloc_new(rootctx);

#ifndef NDEBUG
    fprintf(stderr, "-----------\n");
    //fprintf(stderr, "RAW: %s\n", NULL);
    fprintf(stderr, "NUM: %d\n", _i);
    fprintf(stderr, "NAME: %s\n", test->name);
    fprintf(stderr, "DESC: %s\n", test->desc);
    fprintf(stderr, "TMPL: %s\n", test->tmpl);
    if( test->partials ) {
        fprintf(stderr, "PARTIALS:\n%s\n", handlebars_value_dump(test->partials, 0));
    }
#endif

    if( !shouldnt_skip(test) ) {
        fprintf(stderr, "SKIPPED #%d\n", _i);
        goto done;
    }

    // Initialize
    ctx = talloc_steal(memctx, test->ctx);
    parser = handlebars_parser_ctor(ctx);
    //ctx->ignore_standalone = test->opt_ignore_standalone;
    compiler = handlebars_compiler_ctor(ctx);
    tmpl = handlebars_string_ctor(HBSCTX(parser), test->tmpl, strlen(test->tmpl));
    tmpl = handlebars_preprocess_delimiters(ctx, tmpl, NULL, NULL);

    // Parse
    parser->tmpl = tmpl;
    handlebars_parse(parser);

    // Check error
    if( handlebars_error_num(ctx) != HANDLEBARS_SUCCESS ) {
        ck_assert_msg(0, "%s", handlebars_error_msg(ctx));
    }

    // Compile
    handlebars_compiler_set_flags(compiler, test->flags);
    handlebars_compiler_compile(compiler, parser->program);

    if( handlebars_error_num(ctx) != HANDLEBARS_SUCCESS ) {
        ck_assert_msg(0, "%s", handlebars_error_msg(ctx));
    }

    // Serialize
    module = handlebars_program_serialize(HBSCTX(compiler), compiler->program);

    // Setup VM
    vm = handlebars_vm_ctor(ctx);
    vm->helpers = handlebars_value_ctor(HBSCTX(vm));
    vm->flags = test->flags;

    // Setup partials
    vm->partials = handlebars_value_ctor(ctx);
    handlebars_value_map_init(vm->partials);
    if( test->partials ) {
        handlebars_value_iterator_init(&it, test->partials);
        for (; it.current != NULL; it.next(&it)) {
            handlebars_map_update(vm->partials->v.map, it.key, it.current);
        }
    }

    load_fixtures(test->data);

    // Execute
    handlebars_vm_execute(vm, module, test->data);

#ifndef NDEBUG
    if( test->expected ) {
        fprintf(stderr, "EXPECTED: %s\n", test->expected);
        fprintf(stderr, "ACTUAL: %s\n", vm->buffer->val);
        fprintf(stderr, "%s\n", vm->buffer && 0 == strcmp(vm->buffer->val, test->expected) ? "PASS" : "FAIL");
    } else if( handlebars_error_msg(ctx) ) {
        fprintf(stderr, "ERROR: %s\n", handlebars_error_msg(ctx));
    }
#endif

    // Check error
    if( handlebars_error_num(ctx) != HANDLEBARS_SUCCESS ) {
        ck_assert_msg(0, "%s", handlebars_error_msg(ctx));
    }

    ck_assert_ptr_ne(vm->buffer, NULL);

    if (strcmp(vm->buffer->val, test->expected) != 0) {
        char *tmp = handlebars_talloc_asprintf(rootctx,
                                               "Failed.\nSuite: %s\nTest: %s - %s\nFlags: %ld\nTemplate:\n%s\nExpected:\n%s\nActual:\n%s\n",
                                               "" /*test->suite_name*/,
                                               test->name, test->desc, test->flags,
                                               test->tmpl, test->expected, vm->buffer->val);
        ck_abort_msg("%s", tmp);
    }

    handlebars_context_dtor(ctx);
    ck_assert_int_eq(1, talloc_total_blocks(memctx));

done:
    talloc_free(memctx);
}
END_TEST

Suite * parser_suite(void)
{
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
    tcase_add_loop_test(tc_ast_to_string_on_mustache_spec, test_ast_to_string_on_mustache_spec, start, end);
    suite_add_tcase(s, tc_ast_to_string_on_mustache_spec);

    TCase * tc_mustache_spec = tcase_create("Mustache Spec");
    tcase_add_loop_test(tc_mustache_spec, test_mustache_spec, start, end);
    suite_add_tcase(s, tc_mustache_spec);

    return s;
}

int main(int argc, char *argv[])
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
    spec_dir = getenv("mustache_spec_dir");
    if( spec_dir == NULL && argc >= 2 ) {
        spec_dir = argv[1];
    }
    if( spec_dir == NULL ) {
        spec_dir = "./spec/mustache/specs";
    }
    loadSpec("comments");
    loadSpec("delimiters");
    loadSpec("interpolation");
    loadSpec("inverted");
    loadSpec("partials");
    loadSpec("sections");
    loadSpec("~lambdas");
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
        talloc_report_full(NULL, stderr);
    }

    // Return
    return error;
}
