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
#include <string.h>
#include <talloc.h>

#include "handlebars.h"
#include "handlebars_compiler.h"
#include "handlebars_helpers.h"
#include "handlebars_opcode_serializer.h"
#include "handlebars_partial_loader.h"
#include "handlebars_string.h"
#include "handlebars_value.h"
#include "handlebars_vm.h"
#include "handlebars.tab.h"
#include "handlebars.lex.h"
#include "utils.h"

static TALLOC_CTX * rootctx;
TALLOC_CTX * memctx;
static int memdebug;

static struct handlebars_string * execute_template(const char *template)
{
    struct handlebars_context * ctx;
    struct handlebars_compiler * compiler;
    struct handlebars_parser * parser;
    struct handlebars_vm * vm;
    struct handlebars_string *retval = NULL;
    TALLOC_CTX * memctx = talloc_new(rootctx);

    // Initialize
    ctx = handlebars_context_ctor_ex(memctx);
    parser = handlebars_parser_ctor(ctx);
    compiler = handlebars_compiler_ctor(ctx);

    // Parse
    parser->tmpl = handlebars_string_ctor(HBSCTX(parser), template, strlen(template));
    handlebars_parse(parser);

    // Check error
    if( handlebars_error_num(ctx) != HANDLEBARS_SUCCESS ) {
        // @todo maybe check message
        ck_abort_msg(handlebars_error_msg(ctx));
        goto done;
    }

    // Compile
    handlebars_compiler_compile(compiler, parser->program);
    if( handlebars_error_num(ctx) != HANDLEBARS_SUCCESS ) {
        ck_abort_msg(handlebars_error_msg(ctx));
        goto done;
    }

    // Serialize
    struct handlebars_module * module = handlebars_program_serialize(ctx, compiler->program);
    handlebars_compiler_dtor(compiler);

    // Setup VM
    vm = handlebars_vm_ctor(ctx);

    // Setup helpers
    vm->helpers = handlebars_value_ctor(HBSCTX(vm));
    handlebars_value_map_init(vm->helpers);

    // Setup partial loader
    vm->partials = handlebars_value_partial_loader_ctor(HBSCTX(vm),
        handlebars_string_ctor(HBSCTX(vm), HBS_STRL(".")),
        handlebars_string_ctor(HBSCTX(vm), HBS_STRL(".hbs")));

    // setup context
    struct handlebars_value *context = handlebars_value_from_json_string(ctx, "{\"foo\":\"bar\"}");

    // Execute
    handlebars_vm_execute(vm, module, context);

    ck_assert_msg(handlebars_error_msg(HBSCTX(vm)) == NULL, handlebars_error_msg(HBSCTX(vm)));

    retval = talloc_steal(NULL, vm->buffer);

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

    return retval;
}

START_TEST(test_partial_loader_1)
{
    struct handlebars_string *rv = execute_template("{{> fixture1 .}}");
    ck_assert_str_eq(rv->val, "|bar|");
}
END_TEST

START_TEST(test_partial_loader_2)
{
    struct handlebars_string *rv = execute_template("{{> fixture1 .}}{{> fixture1 .}}");
    ck_assert_str_eq(rv->val, "|bar||bar|");
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

Suite * partial_loader_suite(void)
{
    Suite * s = suite_create("Partial loader");

	REGISTER_TEST_FIXTURE(s, test_partial_loader_1, "Partial loader 1");
	REGISTER_TEST_FIXTURE(s, test_partial_loader_2, "Partial loader 2");

    return s;
}

int main(void)
{
    int number_failed;
    int memdebug;
    int error;

    talloc_set_log_stderr();

    // Check if memdebug enabled
    memdebug = getenv("MEMDEBUG") ? atoi(getenv("MEMDEBUG")) : 0;
    if( memdebug ) {
        talloc_enable_leak_report_full();
    }

    // Set up test suite
    Suite * s = partial_loader_suite();
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
