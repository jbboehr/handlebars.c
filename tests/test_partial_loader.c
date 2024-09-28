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
#include "handlebars_opcode_serializer.h"
#include "handlebars_parser.h"
#include "handlebars_partial_loader.h"
#include "handlebars_string.h"
#include "handlebars_value.h"
#include "handlebars_vm.h"
#include "handlebars.tab.h"
#include "handlebars.lex.h"
#include "utils.h"



static struct handlebars_string * execute_template(const char *template)
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
        ck_assert_str_eq("fixture1", hbs_str_val(key));
        ck_assert_str_eq("|{{foo}}|", handlebars_value_get_strval(child));
    } HANDLEBARS_VALUE_FOREACH_END();

    retval = talloc_steal(NULL, buffer);

    HANDLEBARS_VALUE_UNDECL(partials);
    HANDLEBARS_VALUE_UNDECL(input);
    HANDLEBARS_VALUE_UNDECL(helpers);

    return retval;
}

START_TEST(test_partial_loader_1)
{
    struct handlebars_string *rv = execute_template("{{> fixture1 .}}");
    ck_assert_hbs_str_eq_cstr(rv, "|bar|");
    talloc_free(rv);
}
END_TEST

START_TEST(test_partial_loader_2)
{
    struct handlebars_string *rv = execute_template("{{> fixture1 .}}{{> fixture1 .}}");
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

    (void) execute_template("{{> nonexist .}}");
    ck_assert(0);
}
END_TEST

START_TEST(test_partial_loader_empty_error)
{
    jmp_buf buf;

    if( handlebars_setjmp_ex(context, &buf) ) {
        fprintf(stderr, "Got expected error: %s\n", handlebars_error_message(context));
        ck_assert(1);
        return;
    }

    (void) execute_template("{{> fixture4}}");
    ck_assert(0);
}
END_TEST

static Suite * suite(void);
static Suite * suite(void)
{
    Suite * s = suite_create("Partial loader");

	REGISTER_TEST_FIXTURE(s, test_partial_loader_1, "Partial loader 1");
	REGISTER_TEST_FIXTURE(s, test_partial_loader_2, "Partial loader 2");
	REGISTER_TEST_FIXTURE(s, test_partial_loader_error, "Partial loader error");
	REGISTER_TEST_FIXTURE(s, test_partial_loader_empty_error, "Partial loader empty error");

    return s;
}

int main(void)
{
    return default_main(&suite);
}
