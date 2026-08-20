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
#include <string.h>
#include <talloc.h>

#include "handlebars.h"
#include "handlebars_compiler.h"
#include "handlebars_helpers.h"
#include "handlebars_json.h"
#include "handlebars_map.h"
#include "handlebars_memory.h"
#include "handlebars_opcode_serializer.h"
#include "handlebars_parser.h"
#include "handlebars_partial_loader.h"
#include "handlebars_string.h"
#include "handlebars_value.h"
#include "handlebars_vm.h"
#include "handlebars.tab.h"
#include "handlebars.lex.h"
#include "utils.h"



static struct handlebars_string * execute_template(const char *template, const char * partial_name, const char * partial_value)
{
    struct handlebars_string *retval = NULL;
    struct handlebars_module * module;

    // Parse
    struct handlebars_ast_node * ast = handlebars_parse_ex(parser, handlebars_string_ctor(HBSCTX(parser), template, strlen(template)), 0);

    // Check error
    if( handlebars_error_num(context) != HANDLEBARS_SUCCESS ) {
        // @todo maybe check message
        ck_abort_msg("%s", handlebars_error_msg(context));
        return NULL;
    }

    // Compile
    struct handlebars_program * program = handlebars_compiler_compile_ex(compiler, ast);
    if( handlebars_error_num(context) != HANDLEBARS_SUCCESS ) {
        ck_abort_msg("%s", handlebars_error_msg(context));
        return NULL;
    }

    // Serialize
    module = handlebars_program_serialize(context, program);

    // Setup helpers
    HANDLEBARS_VALUE_DECL(helpers);
    handlebars_value_map(helpers, handlebars_map_ctor(HBSCTX(vm), 0));
    handlebars_vm_set_helpers(vm, helpers);

    // Setup partial loader
    char * top_srcdir = getenv("top_srcdir");
    struct handlebars_string * path;
    if (NULL != top_srcdir) {
        path = handlebars_string_asprintf(HBSCTX(vm), "%s/tests", top_srcdir);
    } else {
        path = handlebars_string_ctor(HBSCTX(vm), HBS_STRL("."));
    }
    HANDLEBARS_VALUE_DECL(partials);
    handlebars_vm_set_partials(
        vm,
        handlebars_value_partial_loader_init(HBSCTX(vm),
            path,
            handlebars_string_ctor(HBSCTX(vm), HBS_STRL(".hbs")),
            partials)
    );

    // setup context
    HANDLEBARS_VALUE_DECL(input);
    handlebars_value_init_json_string(context, input, "{\"foo\":\"bar\"}");
    handlebars_value_convert(input); // @TODO shouldn't have to do this

    // Execute
    struct handlebars_string * buffer = handlebars_vm_execute(vm, module, input);

    ck_assert_msg(handlebars_error_msg(HBSCTX(vm)) == NULL, "%s", handlebars_error_msg(HBSCTX(vm)));

    // Test iterator/count
    ck_assert_int_eq(1, handlebars_value_count(partials));
    HANDLEBARS_VALUE_FOREACH_KV(partials, key, child) {
        ck_assert_str_eq(partial_name, hbs_str_val(key));
        ck_assert_str_eq(partial_value, handlebars_value_get_strval(child));
    } HANDLEBARS_VALUE_FOREACH_END();

    retval = talloc_steal(NULL, buffer);

    HANDLEBARS_VALUE_UNDECL(partials);
    HANDLEBARS_VALUE_UNDECL(input);
    HANDLEBARS_VALUE_UNDECL(helpers);

    return retval;
}

START_TEST(test_partial_loader_1)
{
    struct handlebars_string *rv = execute_template("{{> fixture1 .}}", "fixture1", "|{{foo}}|");
    ck_assert_hbs_str_eq_cstr(rv, "|bar|");
    talloc_free(rv);
}
END_TEST

START_TEST(test_partial_loader_2)
{
    struct handlebars_string *rv = execute_template("{{> fixture1 .}}{{> fixture1 .}}", "fixture1", "|{{foo}}|");
    ck_assert_hbs_str_eq_cstr(rv, "|bar||bar|");
    talloc_free(rv);
}
END_TEST

START_TEST(test_partial_loader_error)
{
    jmp_buf buf;

    if( handlebars_setjmp_ex(context, &buf) ) {
        fprintf(stderr, "Got expected error: %s\n", handlebars_error_message(context));
        ck_assert(1);
        return;
    }

    (void) execute_template("{{> nonexist .}}", "nonexist", "");
    ck_assert(0);
}
END_TEST

static void expect_partial_loader_error(
    struct handlebars_value * partials,
    struct handlebars_string * key
) {
    jmp_buf * volatile previous = context->e->jmp;
    HANDLEBARS_VALUE_DECL(rv);
    jmp_buf buf;

    if( handlebars_setjmp_ex(context, &buf) ) {
        context->e->jmp = previous;
        ck_assert_ptr_nonnull(handlebars_error_msg(context));
        ck_assert_ptr_nonnull(strstr(handlebars_error_msg(context), "File to open partial"));
        HANDLEBARS_VALUE_UNDECL(rv);
        return;
    }

    (void) handlebars_value_map_find(partials, key, rv);
    context->e->jmp = previous;
    HANDLEBARS_VALUE_UNDECL(rv);
    ck_abort_msg("Expected partial loading to fail");
}

START_TEST(test_partial_loader_recovers_after_error)
{
    HANDLEBARS_VALUE_DECL(partials);
    HANDLEBARS_VALUE_DECL(rv);
    char * top_srcdir = getenv("top_srcdir");
    struct handlebars_string * path;
    struct handlebars_string * extension = handlebars_string_ctor(context, HBS_STRL(".hbs"));
    struct handlebars_string * missing = handlebars_string_ctor(context, HBS_STRL("nonexist-recovery"));
    struct handlebars_string * present = handlebars_string_ctor(context, HBS_STRL("fixture1"));
    size_t blocks_before;
    size_t blocks_after_first_error;

    if( top_srcdir != NULL ) {
        path = handlebars_string_asprintf(context, "%s/tests", top_srcdir);
    } else {
        path = handlebars_string_ctor(context, HBS_STRL("."));
    }
    handlebars_value_partial_loader_init(context, path, extension, partials);

    blocks_before = talloc_total_blocks(context);
    expect_partial_loader_error(partials, missing);
    blocks_after_first_error = talloc_total_blocks(context);
    ck_assert_uint_eq(blocks_after_first_error, blocks_before + 1);

    expect_partial_loader_error(partials, missing);
    ck_assert_uint_eq(talloc_total_blocks(context), blocks_after_first_error);

    ck_assert_ptr_nonnull(handlebars_value_map_find(partials, present, rv));
    ck_assert_str_eq(handlebars_value_get_strval(rv), "|{{foo}}|");
    ck_assert_int_eq(handlebars_value_count(partials), 1);

    HANDLEBARS_VALUE_UNDECL(rv);
    HANDLEBARS_VALUE_UNDECL(partials);
}
END_TEST

START_TEST(test_partial_loader_result_outlives_loader)
{
    HANDLEBARS_VALUE_DECL(partials);
    HANDLEBARS_VALUE_DECL(rv);
    char * top_srcdir = getenv("top_srcdir");
    struct handlebars_string * path;
    struct handlebars_string * extension = handlebars_string_ctor(context, HBS_STRL(".hbs"));
    struct handlebars_string * present = handlebars_string_ctor(context, HBS_STRL("fixture1"));

    if( top_srcdir != NULL ) {
        path = handlebars_string_asprintf(context, "%s/tests", top_srcdir);
    } else {
        path = handlebars_string_ctor(context, HBS_STRL("."));
    }
    handlebars_value_partial_loader_init(context, path, extension, partials);

    ck_assert_ptr_nonnull(handlebars_value_map_find(partials, present, rv));
    HANDLEBARS_VALUE_UNDECL(partials);

    ck_assert_str_eq(handlebars_value_get_strval(rv), "|{{foo}}|");
    HANDLEBARS_VALUE_UNDECL(rv);
}
END_TEST

#ifdef HANDLEBARS_MEMORY
static bool partial_loader_find_with_alloc_failure(
    struct handlebars_value * partials,
    struct handlebars_string * key,
    int fail_at
) {
    jmp_buf * volatile previous = context->e->jmp;
    HANDLEBARS_VALUE_DECL(rv);
    jmp_buf buf;

    if( handlebars_setjmp_ex(context, &buf) ) {
        handlebars_memory_fail_disable();
        context->e->jmp = previous;
        ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_NOMEM);
        ck_assert_ptr_nonnull(handlebars_error_msg(context));
        ck_assert_ptr_nonnull(strstr(handlebars_error_msg(context), "Out of memory"));
        HANDLEBARS_VALUE_UNDECL(rv);
        return true;
    }

    handlebars_memory_fail_set_flags(handlebars_memory_fail_flag_alloc);
    handlebars_memory_fail_counter(fail_at);
    (void) handlebars_value_map_find(partials, key, rv);
    handlebars_memory_fail_disable();
    context->e->jmp = previous;
    HANDLEBARS_VALUE_UNDECL(rv);
    return false;
}
#endif

START_TEST(test_partial_loader_recovers_after_alloc_error)
{
#ifdef HANDLEBARS_MEMORY
    HANDLEBARS_VALUE_DECL(partials);
    HANDLEBARS_VALUE_DECL(rv);
    char * top_srcdir = getenv("top_srcdir");
    struct handlebars_string * path;
    struct handlebars_string * extension = handlebars_string_ctor(context, HBS_STRL(".hbs"));
    struct handlebars_string * present = handlebars_string_ctor(context, HBS_STRL("fixture1"));
    int fail_at;

    if( top_srcdir != NULL ) {
        path = handlebars_string_asprintf(context, "%s/tests", top_srcdir);
    } else {
        path = handlebars_string_ctor(context, HBS_STRL("."));
    }
    handlebars_value_partial_loader_init(context, path, extension, partials);

    for( fail_at = 1; fail_at < 32; fail_at++ ) {
        size_t blocks_before = talloc_total_blocks(context);
        if( !partial_loader_find_with_alloc_failure(partials, present, fail_at) ) {
            break;
        }
        ck_assert_uint_le(talloc_total_blocks(context), blocks_before + 1);
    }

    ck_assert_int_lt(fail_at, 32);
    ck_assert_ptr_nonnull(handlebars_value_map_find(partials, present, rv));
    ck_assert_str_eq(handlebars_value_get_strval(rv), "|{{foo}}|");
    ck_assert_int_eq(handlebars_value_count(partials), 1);

    HANDLEBARS_VALUE_UNDECL(rv);
    HANDLEBARS_VALUE_UNDECL(partials);
#else
    fprintf(stderr, "Skipped, memory testing functions are disabled\n");
#endif
}
END_TEST

START_TEST(test_partial_loader_empty)
{
    struct handlebars_string *rv = execute_template("{{> fixture4}}", "fixture4", "");
    ck_assert_hbs_str_eq_cstr(rv, "");
    talloc_free(rv);
}
END_TEST

START_TEST(test_partial_loader_rejects_traversal)
{
    HANDLEBARS_VALUE_DECL(partials);
    HANDLEBARS_VALUE_DECL(rv);
    struct handlebars_string * path = handlebars_string_ctor(context, HBS_STRL("tests"));
    struct handlebars_string * extension = handlebars_string_ctor(context, HBS_STRL(".hbs"));
    struct handlebars_string * key = handlebars_string_ctor(context, HBS_STRL("../fixture1"));
    handlebars_value_partial_loader_init(context, path, extension, partials);
    jmp_buf buf;

    if( handlebars_setjmp_ex(context, &buf) ) {
        ck_assert_str_eq(handlebars_error_msg(context), "Invalid partial name");
        HANDLEBARS_VALUE_UNDECL(rv);
        HANDLEBARS_VALUE_UNDECL(partials);
        return;
    }

    (void) handlebars_value_map_find(partials, key, rv);
    ck_abort_msg("Expected unsafe partial name to be rejected");
}
END_TEST

static Suite * suite(void);
static Suite * suite(void)
{
    Suite * s = suite_create("Partial loader");

	REGISTER_TEST_FIXTURE(s, test_partial_loader_1, "Partial loader 1");
	REGISTER_TEST_FIXTURE(s, test_partial_loader_2, "Partial loader 2");
	REGISTER_TEST_FIXTURE(s, test_partial_loader_error, "Partial loader error");
	REGISTER_TEST_FIXTURE(s, test_partial_loader_recovers_after_error, "Partial loader recovers after error");
	REGISTER_TEST_FIXTURE(s, test_partial_loader_result_outlives_loader, "Partial loader result outlives loader");
	REGISTER_TEST_FIXTURE(s, test_partial_loader_recovers_after_alloc_error, "Partial loader recovers after allocation error");
	REGISTER_TEST_FIXTURE(s, test_partial_loader_empty, "Partial loader empty file");
	REGISTER_TEST_FIXTURE(s, test_partial_loader_rejects_traversal, "Partial loader rejects traversal");

    return s;
}

int main(void)
{
    return default_main(&suite);
}
