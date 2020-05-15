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

#include <check.h>
#include <string.h>
#include <talloc.h>

#include "handlebars.h"
#include "handlebars_compiler.h"
#include "handlebars_helpers.h"
#include "handlebars_json.h"
#include "handlebars_opcode_serializer.h"
#include "handlebars_parser.h"
#include "handlebars_partial_loader.h"
#include "handlebars_string.h"
#include "handlebars_value.h"
#include "handlebars_vm.h"
#include "handlebars.tab.h"
#include "handlebars.lex.h"
#include "utils.h"



TALLOC_CTX * memctx;
static int memdebug;

static struct handlebars_string * execute_template(const char *template)
{
    struct handlebars_string *retval = NULL;
    struct handlebars_module * module;
    struct handlebars_value *input;

    // Initialize
    // ctx = handlebars_context_ctor_ex(memctx);
    // parser = handlebars_parser_ctor(ctx);
    // compiler = handlebars_compiler_ctor(ctx);

    // Parse
    struct handlebars_ast_node * ast = handlebars_parse_ex(parser, handlebars_string_ctor(HBSCTX(parser), template, strlen(template)), 0);

    // Check error
    if( handlebars_error_num(context) != HANDLEBARS_SUCCESS ) {
        // @todo maybe check message
        ck_abort_msg(handlebars_error_msg(context));
        return NULL;
    }

    // Compile
    struct handlebars_program * program = handlebars_compiler_compile_ex(compiler, ast);
    if( handlebars_error_num(context) != HANDLEBARS_SUCCESS ) {
        ck_abort_msg(handlebars_error_msg(context));
        return NULL;
    }

    // Serialize
    module = handlebars_program_serialize(context, program);

    // Setup VM
    vm = handlebars_vm_ctor(context);

    // Setup helpers
    struct handlebars_value * helpers = handlebars_value_ctor(HBSCTX(vm));
    handlebars_value_map_init(helpers, 0);
    handlebars_vm_set_helpers(vm, helpers);

    // Setup partial loader
    handlebars_vm_set_partials(
        vm,
        handlebars_value_partial_loader_ctor(HBSCTX(vm),
            handlebars_string_ctor(HBSCTX(vm), HBS_STRL(".")),
            handlebars_string_ctor(HBSCTX(vm), HBS_STRL(".hbs")))
    );

    // setup context
    input = handlebars_value_from_json_string(context, "{\"foo\":\"bar\"}");

    // Execute
    struct handlebars_string * buffer = handlebars_vm_execute(vm, module, input);

    ck_assert_msg(handlebars_error_msg(HBSCTX(vm)) == NULL, handlebars_error_msg(HBSCTX(vm)));

    retval = talloc_steal(NULL, buffer);

    return retval;
}

START_TEST(test_partial_loader_1)
{
    struct handlebars_string *rv = execute_template("{{> fixture1 .}}");
    ck_assert_hbs_str_eq_cstr(rv, "|bar|\n");
}
END_TEST

START_TEST(test_partial_loader_2)
{
    struct handlebars_string *rv = execute_template("{{> fixture1 .}}{{> fixture1 .}}");
    ck_assert_hbs_str_eq_cstr(rv, "|bar|\n|bar|\n");
}
END_TEST

static Suite * suite(void);
static Suite * suite(void)
{
    Suite * s = suite_create("Partial loader");

	REGISTER_TEST_FIXTURE(s, test_partial_loader_1, "Partial loader 1");
	REGISTER_TEST_FIXTURE(s, test_partial_loader_2, "Partial loader 2");

    return s;
}

int main(void)
{
    int number_failed;
    // int memdebug;
    int error;

    talloc_set_log_stderr();

    // Check if memdebug enabled
    memdebug = getenv("MEMDEBUG") ? atoi(getenv("MEMDEBUG")) : 0;
    if( memdebug ) {
        talloc_enable_leak_report_full();
    }

    // Set up test suite
    Suite * s = suite();
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
