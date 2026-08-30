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
    size_t nested_count = 0;
    HANDLEBARS_VALUE_FOREACH_KV(partials, key, child) {
        HANDLEBARS_VALUE_ITERATOR_DECL(nested_iter);
        ck_assert_str_eq(partial_name, hbs_str_val(key));
        ck_assert_str_eq(partial_value, handlebars_value_get_strval(child));

        if( HANDLEBARS_VALUE_ITERATOR_INIT(nested_iter, partials) ) {
            do {
                ck_assert_str_eq(partial_name, hbs_str_val(nested_iter->key));
                ck_assert_str_eq(partial_value, handlebars_value_get_strval(nested_iter->cur));
                nested_count++;
            } while( handlebars_value_iterator_next(nested_iter) );
        }
    } HANDLEBARS_VALUE_FOREACH_END();
    ck_assert_uint_eq(nested_count, 1);

    retval = talloc_steal(NULL, buffer);

    HANDLEBARS_VALUE_UNDECL(partials);
    HANDLEBARS_VALUE_UNDECL(input);
    HANDLEBARS_VALUE_UNDECL(helpers);

    return retval;
}

static struct handlebars_module * compile_template(const char * source)
{
    struct handlebars_parser * local_parser = handlebars_parser_ctor(context);
    struct handlebars_compiler * local_compiler = handlebars_compiler_ctor(context);
    struct handlebars_string * tmpl = handlebars_string_ctor(
        context,
        source,
        strlen(source)
    );
    struct handlebars_ast_node * ast = handlebars_parse_ex(local_parser, tmpl, 0);
    struct handlebars_program * program = handlebars_compiler_compile_ex(
        local_compiler,
        ast
    );
    struct handlebars_module * module = handlebars_program_serialize(
        context,
        program
    );

    handlebars_compiler_dtor(local_compiler);
    handlebars_parser_dtor(local_parser);
    return module;
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

START_TEST(test_partial_loader_cross_context_error_returns_from_vm_try)
{
    struct handlebars_context * loader_context = handlebars_context_ctor_ex(root);
    struct handlebars_module * missing_module = compile_template("{{> nonexist-cross-context .}}");
    struct handlebars_module * present_module = compile_template("{{> fixture1 .}}");
    struct handlebars_string * path;
    struct handlebars_string * extension;
    struct handlebars_string * output = NULL;
    enum handlebars_error_type error;
    char * top_srcdir = getenv("top_srcdir");
    jmp_buf loader_outer;
    jmp_buf outer;
    HANDLEBARS_VALUE_DECL(partials);
    HANDLEBARS_VALUE_DECL(input);
    HANDLEBARS_VALUE_DECL(empty);

    ck_assert_ptr_nonnull(loader_context);
    if( top_srcdir != NULL ) {
        path = handlebars_string_asprintf(loader_context, "%s/tests", top_srcdir);
    } else {
        path = handlebars_string_ctor(loader_context, HBS_STRL("."));
    }
    extension = handlebars_string_ctor(loader_context, HBS_STRL(".hbs"));
    handlebars_value_partial_loader_init(
        loader_context,
        path,
        extension,
        partials
    );
    handlebars_vm_set_partials(vm, partials);
    loader_context->e->jmp = &loader_outer;
    context->e->jmp = &outer;

    error = handlebars_vm_execute_try(vm, missing_module, input, &output);

    ck_assert_int_eq(error, HANDLEBARS_ERROR);
    ck_assert_ptr_null(output);
    ck_assert_ptr_eq(context->e->jmp, &outer);
    ck_assert_ptr_eq(loader_context->e->jmp, &loader_outer);
    ck_assert_ptr_nonnull(handlebars_error_msg(context));
    ck_assert_ptr_nonnull(strstr(handlebars_error_msg(context), "File to open partial"));

    error = handlebars_vm_execute_try(vm, present_module, input, &output);
    ck_assert_msg(
        error == HANDLEBARS_SUCCESS,
        "second render failed: %s",
        handlebars_error_msg(context)
    );
    ck_assert_ptr_nonnull(output);
    ck_assert_hbs_str_eq_cstr(output, "||");
    ck_assert_int_eq(handlebars_error_num(loader_context), HANDLEBARS_SUCCESS);
    ck_assert_ptr_null(handlebars_error_msg(loader_context));
    ck_assert_ptr_eq(loader_context->e->jmp, &loader_outer);
    handlebars_string_delref(output);

    handlebars_vm_set_partials(vm, empty);
    HANDLEBARS_VALUE_UNDECL(empty);
    HANDLEBARS_VALUE_UNDECL(input);
    HANDLEBARS_VALUE_UNDECL(partials);
    handlebars_context_dtor(loader_context);
}
END_TEST

START_TEST(test_partial_loader_try_api_preserves_results_and_jump_target)
{
    struct handlebars_string * path;
    struct handlebars_string * extension;
    struct handlebars_string * missing;
    struct handlebars_string * present;
    enum handlebars_error_type error;
    char * top_srcdir = getenv("top_srcdir");
    jmp_buf outer;
    HANDLEBARS_VALUE_DECL(partials);
    HANDLEBARS_VALUE_DECL(result);

    if( top_srcdir != NULL ) {
        path = handlebars_string_asprintf(context, "%s/tests", top_srcdir);
    } else {
        path = handlebars_string_ctor(context, HBS_STRL("."));
    }
    extension = handlebars_string_ctor(context, HBS_STRL(".hbs"));
    missing = handlebars_string_ctor(context, HBS_STRL("nonexist-try"));
    present = handlebars_string_ctor(context, HBS_STRL("fixture1"));
    context->e->jmp = &outer;

    error = handlebars_value_partial_loader_init_try(
        context,
        path,
        extension,
        partials
    );
    ck_assert_int_eq(error, HANDLEBARS_SUCCESS);
    ck_assert_int_eq(handlebars_value_get_type(partials), HANDLEBARS_VALUE_TYPE_MAP);
    ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_SUCCESS);
    ck_assert_ptr_null(handlebars_error_msg(context));
    ck_assert_ptr_eq(context->e->jmp, &outer);

    handlebars_value_integer(result, 42);
    error = handlebars_value_partial_loader_find_try(partials, missing, result);
    ck_assert_int_eq(error, HANDLEBARS_ERROR);
    ck_assert_int_eq(handlebars_value_get_type(result), HANDLEBARS_VALUE_TYPE_INTEGER);
    ck_assert_int_eq(handlebars_value_get_intval(result), 42);
    ck_assert_ptr_eq(context->e->jmp, &outer);
    ck_assert_ptr_nonnull(handlebars_error_msg(context));
    ck_assert_ptr_nonnull(strstr(handlebars_error_msg(context), "File to open partial"));

    error = handlebars_value_partial_loader_find_try(partials, present, result);
    ck_assert_int_eq(error, HANDLEBARS_SUCCESS);
    ck_assert_int_eq(handlebars_value_get_type(result), HANDLEBARS_VALUE_TYPE_STRING);
    ck_assert_str_eq(handlebars_value_get_strval(result), "|{{foo}}|");
    ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_SUCCESS);
    ck_assert_ptr_null(handlebars_error_msg(context));
    ck_assert_ptr_eq(context->e->jmp, &outer);

    HANDLEBARS_VALUE_UNDECL(result);
    HANDLEBARS_VALUE_UNDECL(partials);
}
END_TEST

START_TEST(test_partial_loader_try_supports_aliased_result)
{
    struct handlebars_string * path;
    struct handlebars_string * extension;
    struct handlebars_string * missing;
    struct handlebars_string * present;
    enum handlebars_error_type error;
    char * top_srcdir = getenv("top_srcdir");
    jmp_buf outer;
    HANDLEBARS_VALUE_DECL(partials);

    if( top_srcdir != NULL ) {
        path = handlebars_string_asprintf(context, "%s/tests", top_srcdir);
    } else {
        path = handlebars_string_ctor(context, HBS_STRL("."));
    }
    extension = handlebars_string_ctor(context, HBS_STRL(".hbs"));
    missing = handlebars_string_ctor(context, HBS_STRL("nonexist-alias"));
    present = handlebars_string_ctor(context, HBS_STRL("fixture1"));
    context->e->jmp = &outer;
    error = handlebars_value_partial_loader_init_try(
        context,
        path,
        extension,
        partials
    );
    ck_assert_int_eq(error, HANDLEBARS_SUCCESS);

    error = handlebars_value_partial_loader_find_try(partials, missing, partials);
    ck_assert_int_eq(error, HANDLEBARS_ERROR);
    ck_assert_int_eq(handlebars_value_get_type(partials), HANDLEBARS_VALUE_TYPE_MAP);
    ck_assert_int_eq(handlebars_value_count(partials), 0);
    ck_assert_ptr_eq(context->e->jmp, &outer);

    error = handlebars_value_partial_loader_find_try(partials, present, partials);
    ck_assert_int_eq(error, HANDLEBARS_SUCCESS);
    ck_assert_int_eq(handlebars_value_get_type(partials), HANDLEBARS_VALUE_TYPE_STRING);
    ck_assert_str_eq(handlebars_value_get_strval(partials), "|{{foo}}|");
    ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_SUCCESS);
    ck_assert_ptr_null(handlebars_error_msg(context));
    ck_assert_ptr_eq(context->e->jmp, &outer);

    HANDLEBARS_VALUE_UNDECL(partials);
}
END_TEST

START_TEST(test_partial_loader_vm_supports_shared_error_context)
{
    struct handlebars_context * loader_context = handlebars_talloc_zero(
        root,
        struct handlebars_context
    );
    struct handlebars_module * missing_module = compile_template("{{> nonexist-shared-error .}}");
    struct handlebars_module * present_module = compile_template("{{> fixture1 .}}");
    struct handlebars_string * path;
    struct handlebars_string * extension;
    struct handlebars_string * present;
    struct handlebars_string * output;
    enum handlebars_error_type error;
    char * top_srcdir = getenv("top_srcdir");
    jmp_buf outer;
    HANDLEBARS_VALUE_DECL(partials);
    HANDLEBARS_VALUE_DECL(input);
    HANDLEBARS_VALUE_DECL(result);
    HANDLEBARS_VALUE_DECL(empty);

    ck_assert_ptr_nonnull(loader_context);
    loader_context->e = context->e;
    if( top_srcdir != NULL ) {
        path = handlebars_string_asprintf(loader_context, "%s/tests", top_srcdir);
    } else {
        path = handlebars_string_ctor(loader_context, HBS_STRL("."));
    }
    extension = handlebars_string_ctor(loader_context, HBS_STRL(".hbs"));
    present = handlebars_string_ctor(loader_context, HBS_STRL("fixture1"));
    error = handlebars_value_partial_loader_init_try(
        loader_context,
        path,
        extension,
        partials
    );
    ck_assert_int_eq(error, HANDLEBARS_SUCCESS);
    handlebars_vm_set_partials(vm, partials);
    context->e->jmp = &outer;

    output = (struct handlebars_string *) (uintptr_t) 1;
    error = handlebars_vm_execute_try(vm, missing_module, input, &output);
    ck_assert_int_eq(error, HANDLEBARS_ERROR);
    ck_assert_ptr_null(output);
    ck_assert_ptr_eq(context->e->jmp, &outer);
    ck_assert_ptr_nonnull(strstr(handlebars_error_msg(context), "File to open partial"));

    error = handlebars_value_partial_loader_find_try(partials, present, result);
    ck_assert_int_eq(error, HANDLEBARS_SUCCESS);
    ck_assert_str_eq(handlebars_value_get_strval(result), "|{{foo}}|");
    ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_SUCCESS);
    ck_assert_ptr_null(handlebars_error_msg(context));
    ck_assert_ptr_eq(context->e->jmp, &outer);

    output = (struct handlebars_string *) (uintptr_t) 1;
    error = handlebars_vm_execute_try(vm, missing_module, input, &output);
    ck_assert_int_eq(error, HANDLEBARS_ERROR);
    ck_assert_ptr_null(output);
    ck_assert_ptr_eq(context->e->jmp, &outer);
    ck_assert_ptr_nonnull(strstr(handlebars_error_msg(context), "File to open partial"));

    error = handlebars_vm_execute_try(vm, present_module, input, &output);
    ck_assert_int_eq(error, HANDLEBARS_SUCCESS);
    ck_assert_ptr_nonnull(output);
    ck_assert_hbs_str_eq_cstr(output, "||");
    ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_SUCCESS);
    ck_assert_ptr_null(handlebars_error_msg(context));
    ck_assert_ptr_eq(context->e->jmp, &outer);
    handlebars_string_delref(output);

    handlebars_vm_set_partials(vm, empty);
    HANDLEBARS_VALUE_UNDECL(empty);
    HANDLEBARS_VALUE_UNDECL(result);
    HANDLEBARS_VALUE_UNDECL(input);
    HANDLEBARS_VALUE_UNDECL(partials);
    handlebars_context_dtor(loader_context);
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

#ifndef HANDLEBARS_NO_REFCOUNT
START_TEST(test_partial_loader_iteration_uses_snapshot)
{
    HANDLEBARS_VALUE_DECL(partials);
    HANDLEBARS_VALUE_DECL(rv);
    HANDLEBARS_VALUE_ITERATOR_DECL(iter);
    char * top_srcdir = getenv("top_srcdir");
    struct handlebars_string * path;
    struct handlebars_string * extension = handlebars_string_ctor(context, HBS_STRL(".hbs"));
    struct handlebars_string * first = handlebars_string_ctor(context, HBS_STRL("fixture1"));
    struct handlebars_string * second = handlebars_string_ctor(context, HBS_STRL("fixture4"));

    if( top_srcdir != NULL ) {
        path = handlebars_string_asprintf(context, "%s/tests", top_srcdir);
    } else {
        path = handlebars_string_ctor(context, HBS_STRL("."));
    }
    handlebars_value_partial_loader_init(context, path, extension, partials);

    ck_assert_ptr_nonnull(handlebars_value_map_find(partials, first, rv));
    ck_assert(HANDLEBARS_VALUE_ITERATOR_INIT(iter, partials));

    ck_assert_ptr_nonnull(handlebars_value_map_find(partials, second, rv));
    ck_assert_int_eq(handlebars_value_count(partials), 2);

    ck_assert_str_eq(hbs_str_val(iter->key), "fixture1");
    ck_assert_str_eq(handlebars_value_get_strval(iter->cur), "|{{foo}}|");
    ck_assert(!handlebars_value_iterator_next(iter));

    HANDLEBARS_VALUE_UNDECL(rv);
    HANDLEBARS_VALUE_UNDECL(partials);
}
END_TEST


START_TEST(test_partial_loader_iterator_retains_loader)
{
    HANDLEBARS_VALUE_DECL(partials);
    HANDLEBARS_VALUE_DECL(rv);
    HANDLEBARS_VALUE_ITERATOR_DECL(iter);
    char * top_srcdir = getenv("top_srcdir");
    struct handlebars_string * path;
    struct handlebars_string * extension = handlebars_string_ctor(context, HBS_STRL(".hbs"));
    struct handlebars_string * key = handlebars_string_ctor(context, HBS_STRL("fixture1"));

    if( top_srcdir != NULL ) {
        path = handlebars_string_asprintf(context, "%s/tests", top_srcdir);
    } else {
        path = handlebars_string_ctor(context, HBS_STRL("."));
    }
    handlebars_value_partial_loader_init(context, path, extension, partials);

    ck_assert_ptr_nonnull(handlebars_value_map_find(partials, key, rv));
    ck_assert(HANDLEBARS_VALUE_ITERATOR_INIT(iter, partials));
    handlebars_value_dtor(partials);

    ck_assert_hbs_str_eq_cstr(iter->key, "fixture1");
    ck_assert_str_eq(handlebars_value_get_strval(iter->cur), "|{{foo}}|");
    ck_assert(!handlebars_value_iterator_next(iter));

    HANDLEBARS_VALUE_UNDECL(rv);
    HANDLEBARS_VALUE_UNDECL(partials);
}
END_TEST
#endif

#ifdef HANDLEBARS_MEMORY
static bool partial_loader_init_with_alloc_failure(
    struct handlebars_string * path,
    struct handlebars_string * extension,
    struct handlebars_value * result,
    int fail_at
) {
    jmp_buf * volatile previous = context->e->jmp;
    jmp_buf buf;

    if( handlebars_setjmp_ex(context, &buf) ) {
        handlebars_memory_fail_disable();
        context->e->jmp = previous;
        return true;
    }

    handlebars_memory_fail_set_flags(handlebars_memory_fail_flag_alloc);
    handlebars_memory_fail_counter(fail_at);
    (void) handlebars_value_partial_loader_init(
        context,
        path,
        extension,
        result
    );
    handlebars_memory_fail_disable();
    context->e->jmp = previous;
    return false;
}

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

START_TEST(test_partial_loader_legacy_init_cleans_allocation_failures)
{
#ifdef HANDLEBARS_MEMORY
    char * top_srcdir = getenv("top_srcdir");
    struct handlebars_string * path;
    struct handlebars_string * extension = handlebars_string_ctor(context, HBS_STRL(".hbs"));
    size_t blocks_before;
    int fail_at;
    HANDLEBARS_VALUE_DECL(partials);

    if( top_srcdir != NULL ) {
        path = handlebars_string_asprintf(context, "%s/tests", top_srcdir);
    } else {
        path = handlebars_string_ctor(context, HBS_STRL("."));
    }
    blocks_before = talloc_total_blocks(context);

    for( fail_at = 1; fail_at < 32; fail_at++ ) {
        handlebars_value_integer(partials, 42);
        if( !partial_loader_init_with_alloc_failure(
                path,
                extension,
                partials,
                fail_at
            ) ) {
            break;
        }
        ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_NOMEM);
        ck_assert_int_eq(handlebars_value_get_type(partials), HANDLEBARS_VALUE_TYPE_INTEGER);
        ck_assert_int_eq(handlebars_value_get_intval(partials), 42);
        handlebars_error_clear(context);
        ck_assert_uint_eq(talloc_total_blocks(context), blocks_before);
    }
    ck_assert_int_lt(fail_at, 32);
    ck_assert_int_eq(handlebars_value_get_type(partials), HANDLEBARS_VALUE_TYPE_MAP);

    HANDLEBARS_VALUE_UNDECL(partials);
#else
    fprintf(stderr, "Skipped, memory testing functions are disabled\n");
#endif
}
END_TEST

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

START_TEST(test_partial_loader_try_handles_allocation_failures)
{
#ifdef HANDLEBARS_MEMORY
    char * top_srcdir = getenv("top_srcdir");
    struct handlebars_string * path;
    struct handlebars_string * extension = handlebars_string_ctor(context, HBS_STRL(".hbs"));
    struct handlebars_string * present = handlebars_string_ctor(context, HBS_STRL("fixture1"));
    enum handlebars_error_type error;
    size_t blocks_before;
    size_t find_blocks_before;
    int fail_at;
    jmp_buf outer;
    HANDLEBARS_VALUE_DECL(partials);
    HANDLEBARS_VALUE_DECL(result);

    if( top_srcdir != NULL ) {
        path = handlebars_string_asprintf(context, "%s/tests", top_srcdir);
    } else {
        path = handlebars_string_ctor(context, HBS_STRL("."));
    }
    context->e->jmp = &outer;
    blocks_before = talloc_total_blocks(context);
    for( fail_at = 1; fail_at < 32; fail_at++ ) {
        handlebars_value_integer(partials, 42);
        handlebars_memory_fail_set_flags(handlebars_memory_fail_flag_alloc);
        handlebars_memory_fail_counter(fail_at);
        error = handlebars_value_partial_loader_init_try(
            context,
            path,
            extension,
            partials
        );
        handlebars_memory_fail_disable();

        ck_assert_ptr_eq(context->e->jmp, &outer);
        if( error == HANDLEBARS_SUCCESS ) {
            break;
        }
        ck_assert_int_eq(error, HANDLEBARS_NOMEM);
        ck_assert_int_eq(handlebars_value_get_type(partials), HANDLEBARS_VALUE_TYPE_INTEGER);
        ck_assert_int_eq(handlebars_value_get_intval(partials), 42);
        handlebars_error_clear(context);
        ck_assert_uint_eq(talloc_total_blocks(context), blocks_before);
    }
    ck_assert_int_lt(fail_at, 32);
    find_blocks_before = talloc_total_blocks(context);

    for( fail_at = 1; fail_at < 32; fail_at++ ) {
        handlebars_value_integer(result, 42);
        handlebars_memory_fail_set_flags(handlebars_memory_fail_flag_alloc);
        handlebars_memory_fail_counter(fail_at);
        error = handlebars_value_partial_loader_find_try(
            partials,
            present,
            result
        );
        handlebars_memory_fail_disable();

        ck_assert_ptr_eq(context->e->jmp, &outer);
        if( error == HANDLEBARS_SUCCESS ) {
            break;
        }
        ck_assert_int_eq(error, HANDLEBARS_NOMEM);
        ck_assert_int_eq(handlebars_value_get_type(result), HANDLEBARS_VALUE_TYPE_INTEGER);
        ck_assert_int_eq(handlebars_value_get_intval(result), 42);
        handlebars_error_clear(context);
        ck_assert_uint_eq(talloc_total_blocks(context), find_blocks_before);
    }
    ck_assert_int_lt(fail_at, 32);
    ck_assert_str_eq(handlebars_value_get_strval(result), "|{{foo}}|");

    HANDLEBARS_VALUE_UNDECL(result);
    HANDLEBARS_VALUE_UNDECL(partials);
#else
    fprintf(stderr, "Skipped, memory testing functions are disabled\n");
#endif
}
END_TEST

START_TEST(test_partial_loader_vm_preserves_loader_status_under_alloc_failure)
{
#ifdef HANDLEBARS_MEMORY
    struct handlebars_context * loader_context = handlebars_context_ctor_ex(root);
    struct handlebars_module * module = compile_template("{{> nonexist-transfer-alloc .}}");
    struct handlebars_string * path;
    struct handlebars_string * extension;
    struct handlebars_string * output;
    enum handlebars_error_type loader_error;
    enum handlebars_error_type vm_error;
    char * top_srcdir = getenv("top_srcdir");
    bool saw_loader_filesystem_error = false;
    bool injected;
    int fail_at;
    jmp_buf loader_outer;
    jmp_buf vm_outer;
    HANDLEBARS_VALUE_DECL(partials);
    HANDLEBARS_VALUE_DECL(input);
    HANDLEBARS_VALUE_DECL(empty);

    ck_assert_ptr_nonnull(loader_context);
    if( top_srcdir != NULL ) {
        path = handlebars_string_asprintf(loader_context, "%s/tests", top_srcdir);
    } else {
        path = handlebars_string_ctor(loader_context, HBS_STRL("."));
    }
    extension = handlebars_string_ctor(loader_context, HBS_STRL(".hbs"));
    handlebars_value_partial_loader_init(
        loader_context,
        path,
        extension,
        partials
    );
    handlebars_vm_set_partials(vm, partials);
    context->e->jmp = &vm_outer;
    loader_context->e->jmp = &loader_outer;

    for( fail_at = 1; fail_at < 128; fail_at++ ) {
        output = NULL;
        handlebars_memory_fail_set_flags(handlebars_memory_fail_flag_alloc);
        handlebars_memory_fail_counter(fail_at);
        vm_error = handlebars_vm_execute_try(vm, module, input, &output);
        loader_error = handlebars_error_num(loader_context);
        injected = !handlebars_memory_fail_get_state();
        handlebars_memory_fail_disable();

        ck_assert_ptr_null(output);
        ck_assert_ptr_eq(context->e->jmp, &vm_outer);
        ck_assert_ptr_eq(loader_context->e->jmp, &loader_outer);
        if( loader_error == HANDLEBARS_ERROR ) {
            saw_loader_filesystem_error = true;
            ck_assert_msg(
                vm_error == loader_error,
                "VM changed loader status %d to %d at allocation %d",
                loader_error,
                vm_error,
                fail_at
            );
        }
        if( !injected ) {
            break;
        }
    }
    ck_assert(saw_loader_filesystem_error);

    handlebars_vm_set_partials(vm, empty);
    HANDLEBARS_VALUE_UNDECL(empty);
    HANDLEBARS_VALUE_UNDECL(input);
    HANDLEBARS_VALUE_UNDECL(partials);
    handlebars_context_dtor(loader_context);
#else
    fprintf(stderr, "Skipped, memory testing functions are disabled\n");
#endif
}
END_TEST

START_TEST(test_partial_loader_try_cleans_failed_cache_growth)
{
#if defined(HANDLEBARS_MEMORY) && defined(HANDLEBARS_NO_REFCOUNT)
    char * top_srcdir = getenv("top_srcdir");
    struct handlebars_string * path;
    struct handlebars_string * extension = handlebars_string_ctor(context, HBS_STRL(".hbs"));
    struct handlebars_string * key;
    enum handlebars_error_type error;
    size_t blocks_before;
    int fail_at;
    char name[256];
    HANDLEBARS_VALUE_DECL(partials);
    HANDLEBARS_VALUE_DECL(result);

    if( top_srcdir != NULL ) {
        path = handlebars_string_asprintf(context, "%s/tests", top_srcdir);
    } else {
        path = handlebars_string_ctor(context, HBS_STRL("."));
    }
    ck_assert_int_eq(
        handlebars_value_partial_loader_init_try(
            context,
            path,
            extension,
            partials
        ),
        HANDLEBARS_SUCCESS
    );

    for( int index = 0; index < 32; index++ ) {
        size_t prefix_len = (size_t) index * 2;
        memset(name, 0, sizeof(name));
        for( int prefix = 0; prefix < index; prefix++ ) {
            memcpy(name + ((size_t) prefix * 2), "./", 2);
        }
        memcpy(name + prefix_len, "fixture1", sizeof("fixture1"));
        key = handlebars_string_ctor(
            context,
            name,
            prefix_len + sizeof("fixture1") - 1
        );
        error = handlebars_value_partial_loader_find_try(partials, key, result);
        ck_assert_int_eq(error, HANDLEBARS_SUCCESS);
    }
    ck_assert_int_eq(handlebars_value_count(partials), 32);

    memset(name, 0, sizeof(name));
    for( int prefix = 0; prefix < 32; prefix++ ) {
        memcpy(name + ((size_t) prefix * 2), "./", 2);
    }
    memcpy(name + 64, "fixture1", sizeof("fixture1"));
    key = handlebars_string_ctor(
        context,
        name,
        64 + sizeof("fixture1") - 1
    );
    blocks_before = talloc_total_blocks(context);

    for( fail_at = 1; fail_at < 128; fail_at++ ) {
        handlebars_value_integer(result, 42);
        handlebars_memory_fail_set_flags(handlebars_memory_fail_flag_alloc);
        handlebars_memory_fail_counter(fail_at);
        error = handlebars_value_partial_loader_find_try(partials, key, result);
        handlebars_memory_fail_disable();

        if( error == HANDLEBARS_SUCCESS ) {
            break;
        }
        ck_assert_int_eq(error, HANDLEBARS_NOMEM);
        ck_assert_int_eq(handlebars_value_get_type(result), HANDLEBARS_VALUE_TYPE_INTEGER);
        ck_assert_int_eq(handlebars_value_get_intval(result), 42);
        ck_assert_int_eq(handlebars_value_count(partials), 32);
        handlebars_error_clear(context);
        ck_assert_msg(
            talloc_total_blocks(context) == blocks_before,
            "failed cache growth leaked at allocation %d: before=%zu after=%zu",
            fail_at,
            blocks_before,
            talloc_total_blocks(context)
        );
    }
    ck_assert_int_lt(fail_at, 128);
    ck_assert_int_eq(handlebars_value_count(partials), 33);
    ck_assert_str_eq(handlebars_value_get_strval(result), "|{{foo}}|");

    HANDLEBARS_VALUE_UNDECL(result);
    HANDLEBARS_VALUE_UNDECL(partials);
#else
    fprintf(stderr, "Skipped, memory testing or no-refcount mode is disabled\n");
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

static void expect_partial_name_error(
    struct handlebars_value * partials,
    const char * name,
    size_t length,
    const char * expected_error
)
{
    HANDLEBARS_VALUE_DECL(rv);
    struct handlebars_string * key = handlebars_string_ctor(context, name, length);
    jmp_buf * volatile previous = context->e->jmp;
    jmp_buf buf;

    if( handlebars_setjmp_ex(context, &buf) ) {
        context->e->jmp = previous;
        ck_assert_ptr_nonnull(strstr(handlebars_error_msg(context), expected_error));
        clear_intentional_error();
        HANDLEBARS_VALUE_UNDECL(rv);
        return;
    }

    (void) handlebars_value_map_find(partials, key, rv);
    context->e->jmp = previous;
    HANDLEBARS_VALUE_UNDECL(rv);
    ck_abort_msg("Expected partial loading to fail");
}

START_TEST(test_partial_loader_rejects_unsafe_names)
{
    HANDLEBARS_VALUE_DECL(partials);
    struct handlebars_string * path = handlebars_string_ctor(context, HBS_STRL("tests"));
    struct handlebars_string * extension = handlebars_string_ctor(context, HBS_STRL(".hbs"));
    const char embedded_nul[] = {'f', 'o', 'o', '\0', 'b', 'a', 'r'};

    handlebars_value_partial_loader_init(context, path, extension, partials);
    expect_partial_name_error(partials, HBS_STRL(""), "Invalid partial name");
    expect_partial_name_error(partials, HBS_STRL("/fixture1"), "Invalid partial name");
    expect_partial_name_error(partials, HBS_STRL("\\fixture1"), "Invalid partial name");
    expect_partial_name_error(partials, HBS_STRL("dir\\fixture1"), "Invalid partial name");
    expect_partial_name_error(partials, embedded_nul, sizeof(embedded_nul), "Invalid partial name");
    expect_partial_name_error(partials, HBS_STRL("../fixture1"), "Invalid partial name");
    expect_partial_name_error(partials, HBS_STRL("dir/../fixture1"), "Invalid partial name");

    expect_partial_name_error(partials, HBS_STRL("..fixture1"), "File to open partial");
    expect_partial_name_error(partials, HBS_STRL("fixture1.."), "File to open partial");
    expect_partial_name_error(partials, HBS_STRL("a.b"), "File to open partial");
    expect_partial_name_error(partials, HBS_STRL("dir/sub"), "File to open partial");
    ck_assert_int_eq(handlebars_value_count(partials), 0);

    HANDLEBARS_VALUE_UNDECL(partials);
}
END_TEST

static Suite * suite(void);
static Suite * suite(void)
{
    Suite * s = suite_create("Partial loader");

	REGISTER_TEST_FIXTURE(s, test_partial_loader_1, "Partial loader 1");
	REGISTER_TEST_FIXTURE(s, test_partial_loader_2, "Partial loader 2");
	REGISTER_TEST_FIXTURE(s, test_partial_loader_error, "Partial loader error");
	REGISTER_TEST_FIXTURE(s, test_partial_loader_cross_context_error_returns_from_vm_try, "Partial loader cross-context errors return from VM try");
	REGISTER_TEST_FIXTURE(s, test_partial_loader_try_api_preserves_results_and_jump_target, "Partial loader try API preserves results and jump target");
	REGISTER_TEST_FIXTURE(s, test_partial_loader_try_supports_aliased_result, "Partial loader try supports aliased result");
	REGISTER_TEST_FIXTURE(s, test_partial_loader_vm_supports_shared_error_context, "Partial loader VM supports a shared error context");
	REGISTER_TEST_FIXTURE(s, test_partial_loader_recovers_after_error, "Partial loader recovers after error");
	REGISTER_TEST_FIXTURE(s, test_partial_loader_result_outlives_loader, "Partial loader result outlives loader");
#ifndef HANDLEBARS_NO_REFCOUNT
	REGISTER_TEST_FIXTURE(s, test_partial_loader_iteration_uses_snapshot, "Partial loader iteration uses an immutable snapshot");
	REGISTER_TEST_FIXTURE(s, test_partial_loader_iterator_retains_loader, "Partial loader iterator retains its loader");
#endif
	REGISTER_MEMORY_TEST_FIXTURE(s, test_partial_loader_recovers_after_alloc_error, "Partial loader recovers after allocation error");
	REGISTER_MEMORY_TEST_FIXTURE(s, test_partial_loader_legacy_init_cleans_allocation_failures, "Partial loader legacy initializer cleans allocation failures");
	REGISTER_MEMORY_TEST_FIXTURE(s, test_partial_loader_try_handles_allocation_failures, "Partial loader try handles allocation failures");
	REGISTER_MEMORY_TEST_FIXTURE(s, test_partial_loader_vm_preserves_loader_status_under_alloc_failure, "Partial loader VM preserves loader status under allocation failure");
	REGISTER_MEMORY_TEST_FIXTURE(s, test_partial_loader_try_cleans_failed_cache_growth, "Partial loader try cleans failed cache growth");
	REGISTER_TEST_FIXTURE(s, test_partial_loader_empty, "Partial loader empty file");
	REGISTER_TEST_FIXTURE(s, test_partial_loader_rejects_unsafe_names, "Partial loader rejects unsafe names");

    return s;
}

int main(void)
{
    return default_main(&suite);
}
