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

#include "handlebars.h"
#include "handlebars_compiler.h"
#include "handlebars_json.h"
#include "handlebars_memory.h"
#include "handlebars_opcode_serializer.h"
#include "handlebars_parser.h"
#include "handlebars_string.h"
#include "handlebars_value.h"
#include "handlebars_vm.h"
#include "handlebars.tab.h"
#include "handlebars.lex.h"
#include "utils.h"



// @todo try to get this to include every language feature
static const char * tmpl = "{{#if foo}} {{bar}} {{/if}}  {{{blah}}} {{{{raw}}}} {{{{/raw}}}}"
        "{{#a}}{{^}}{{/a}} {{bar baz=foo}} {{> partial}} {{../depth}} {{this}} "
        "{{#unless}}{{else}}{{/unless}} {{&unescaped}} {{~strip~}}  "
        "{{!-- comment --}} {{! coment }} {{blah (a (b)}}";

START_TEST(test_random_alloc_fail_tokenizer)
{
    size_t i = _i;
    // int retval;
    struct handlebars_token ** tokens;

    // for( i = 1; i < 300; i++ ) {
    //     struct handlebars_context * ctx = handlebars_context_ctor_ex(root);
    //     struct handlebars_parser * parser = handlebars_parser_ctor(ctx);
        struct handlebars_string * tmpl_str = handlebars_string_ctor(HBSCTX(parser), HBS_STRL(tmpl));

        // For now, don't do yy alloc
        handlebars_memory_fail_set_flags(handlebars_memory_fail_flag_alloc);
        handlebars_memory_fail_counter(i);
        tokens = handlebars_lex_ex(parser, tmpl_str);
        (void) tokens;
        handlebars_memory_fail_disable();

    //     handlebars_context_dtor(ctx);
    //     ctx = NULL;
    // }
}
END_TEST

START_TEST(test_random_alloc_fail_parser)
{
    size_t i = _i;
    // int retval;

    // for( i = 1; i < 300; i++ ) {
    //     struct handlebars_context * ctx = handlebars_context_ctor_ex(root);
    //     struct handlebars_parser * parser = handlebars_parser_ctor(ctx);
        struct handlebars_string * tmpl_str = handlebars_string_ctor(HBSCTX(parser), HBS_STRL(tmpl));
        struct handlebars_ast_node * ast;

        // For now, don't do yy alloc
        handlebars_memory_fail_set_flags(handlebars_memory_fail_flag_alloc);
        handlebars_memory_fail_counter(i);
        ast = handlebars_parse_ex(parser, tmpl_str, 0);
        (void) ast;
        handlebars_memory_fail_disable();

    //     handlebars_context_dtor(ctx);
    //     ctx = NULL;
    // }
}
END_TEST

START_TEST(test_random_alloc_fail_compiler)
    {
        size_t i = _i;
        // int retval;

        // for( i = 1; i < 300; i++ ) {
        //     struct handlebars_context * ctx = handlebars_context_ctor_ex(root);
        //     struct handlebars_parser * parser = handlebars_parser_ctor(ctx);
        //     struct handlebars_compiler * compiler = handlebars_compiler_ctor(ctx);
            struct handlebars_string * tmpl_str = handlebars_string_ctor(HBSCTX(parser), HBS_STRL(tmpl));
            struct handlebars_ast_node * ast = handlebars_parse_ex(parser, tmpl_str, 0);

            // For now, don't do yy alloc
            handlebars_memory_fail_set_flags(handlebars_memory_fail_flag_alloc);
            handlebars_memory_fail_counter(i);
            handlebars_compiler_compile(compiler, ast);
            handlebars_memory_fail_disable();

        //     handlebars_context_dtor(ctx);
        // }
    }
END_TEST

START_TEST(test_random_alloc_fail_vm)
    {
        size_t i = _i;
        // int retval;

        // for( i = 1; i < 300; i++ ) {
        //     struct handlebars_context * ctx = handlebars_context_ctor_ex(root);
        //     struct handlebars_parser * parser = handlebars_parser_ctor(ctx);
        //     struct handlebars_compiler * compiler = handlebars_compiler_ctor(ctx);
        //     struct handlebars_vm * vm = handlebars_vm_ctor(ctx);
            struct handlebars_string * result;
            HANDLEBARS_VALUE_DECL(value);
            handlebars_value_init_json_string(context, value, "{\"foo\": {\"bar\": 2}}");
            handlebars_value_convert(value);

            struct handlebars_string * tmpl_str = handlebars_string_ctor(HBSCTX(parser), HBS_STRL(tmpl));
            struct handlebars_ast_node * ast = handlebars_parse_ex(parser, tmpl_str, 0);
            struct handlebars_program * program = handlebars_compiler_compile_ex(compiler, ast);
            struct handlebars_module * module = handlebars_program_serialize(context, program);

            // For now, don't do yy alloc
            handlebars_memory_fail_set_flags(handlebars_memory_fail_flag_alloc);
            handlebars_memory_fail_counter(i);
            result = handlebars_vm_execute(vm, module, value);
            (void) result;
            handlebars_memory_fail_disable();

            HANDLEBARS_VALUE_UNDECL(value);

        //     handlebars_context_dtor(ctx);
        // }
    }
END_TEST

static Suite * suite(void);
static Suite * suite(void)
{
    Suite * s = suite_create("Random Memory Allocation Failures");

    TCase * tc_random_alloc_fail_tokenizer = tcase_create("Random Memory Allocation Failures (Tokenizer)");
    tcase_add_checked_fixture(tc_random_alloc_fail_tokenizer, default_setup, default_teardown);
    tcase_add_loop_test(tc_random_alloc_fail_tokenizer, test_random_alloc_fail_tokenizer, 1, 300);
    suite_add_tcase(s, tc_random_alloc_fail_tokenizer);

    TCase * tc_random_alloc_fail_parser = tcase_create("Random Memory Allocation Failures (Parser)");
    tcase_add_checked_fixture(tc_random_alloc_fail_parser, default_setup, default_teardown);
    tcase_add_loop_test(tc_random_alloc_fail_parser, test_random_alloc_fail_parser, 1, 300);
    suite_add_tcase(s, tc_random_alloc_fail_parser);

    TCase * tc_random_alloc_fail_compiler = tcase_create("Random Memory Allocation Failures (Compiler)");
    tcase_add_checked_fixture(tc_random_alloc_fail_compiler, default_setup, default_teardown);
    tcase_add_loop_test(tc_random_alloc_fail_compiler, test_random_alloc_fail_compiler, 1, 300);
    suite_add_tcase(s, tc_random_alloc_fail_compiler);

    TCase * tc_random_alloc_fail_vm = tcase_create("Random Memory Allocation Failures (VM)");
    tcase_add_checked_fixture(tc_random_alloc_fail_vm, default_setup, default_teardown);
    tcase_add_loop_test(tc_random_alloc_fail_vm, test_random_alloc_fail_vm, 1, 300);
    suite_add_tcase(s, tc_random_alloc_fail_vm);

    return s;
}

int main(void)
{
    return default_main(&suite);
}
