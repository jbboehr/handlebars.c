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
#include <talloc.h>

#define HANDLEBARS_AST_PRIVATE

#include "handlebars.h"
#include "handlebars_ast.h"
#include "handlebars_ast_helpers.h"
#include "handlebars_memory.h"
#include "handlebars_string.h"
#include "handlebars.tab.h"
#include "utils.h"



START_TEST(test_ast_helper_set_strip_flags)
{
    struct handlebars_ast_node node;
    struct handlebars_string * str1 = handlebars_string_ctor(context, HBS_STRL("{{"));
    struct handlebars_string * str2 = handlebars_string_ctor(context, HBS_STRL("{{~"));
    struct handlebars_string * str3 = handlebars_string_ctor(context, HBS_STRL("}}"));
    struct handlebars_string * str4 = handlebars_string_ctor(context, HBS_STRL("~}}"));

    memset(&node, 0, sizeof(struct handlebars_ast_node));

    handlebars_ast_helper_set_strip_flags(&node, str1, str3);
    ck_assert_int_eq(1, node.strip);

    handlebars_ast_helper_set_strip_flags(&node, str2, str4);
    ck_assert_int_eq(7, node.strip);

    handlebars_ast_helper_set_strip_flags(&node, NULL, NULL);
    ck_assert_int_eq(1, node.strip);
}
END_TEST

START_TEST(test_ast_helper_strip_comment)
{
    struct handlebars_string * tmp;

    tmp = handlebars_string_ctor(context, HBS_STRL(""));
    handlebars_ast_helper_strip_comment(tmp);
    ck_assert_str_eq(tmp->val, "");

    tmp = handlebars_string_ctor(context, HBS_STRL("blah1"));
    handlebars_ast_helper_strip_comment(tmp);
    ck_assert_str_eq(tmp->val, "blah1");

    tmp = handlebars_string_ctor(context, HBS_STRL("{"));
    handlebars_ast_helper_strip_comment(tmp);
    ck_assert_str_eq(tmp->val, "{");

    tmp = handlebars_string_ctor(context, HBS_STRL("{{!"));
    handlebars_ast_helper_strip_comment(tmp);
    ck_assert_str_eq(tmp->val, "");

    tmp = handlebars_string_ctor(context, HBS_STRL("{{~!--"));
    handlebars_ast_helper_strip_comment(tmp);
    ck_assert_str_eq(tmp->val, "");

    tmp = handlebars_string_ctor(context, HBS_STRL("{{!-- blah"));
    handlebars_ast_helper_strip_comment(tmp);
    ck_assert_str_eq(tmp->val, " blah");

    tmp = handlebars_string_ctor(context, HBS_STRL("}}"));
    handlebars_ast_helper_strip_comment(tmp);
    ck_assert_str_eq(tmp->val, "");

    tmp = handlebars_string_ctor(context, HBS_STRL("--}}"));
    handlebars_ast_helper_strip_comment(tmp);
    ck_assert_str_eq(tmp->val, "");

    tmp = handlebars_string_ctor(context, HBS_STRL("{{!}}"));
    handlebars_ast_helper_strip_comment(tmp);
    ck_assert_str_eq(tmp->val, "");

    tmp = handlebars_string_ctor(context, HBS_STRL("{{! foo }}"));
    handlebars_ast_helper_strip_comment(tmp);
    ck_assert_str_eq(tmp->val, " foo ");

    tmp = handlebars_string_ctor(context, HBS_STRL("{{!-- bar --}}"));
    handlebars_ast_helper_strip_comment(tmp);
    ck_assert_str_eq(tmp->val, " bar ");

    tmp = handlebars_string_ctor(context, HBS_STRL("{{~!-- baz --~}}"));
    handlebars_ast_helper_strip_comment(tmp);
    ck_assert_str_eq(tmp->val, " baz ");
}
END_TEST

Suite * parser_suite(void)
{
    Suite * s = suite_create("AST Helpers");

    REGISTER_TEST_FIXTURE(s, test_ast_helper_set_strip_flags, "Set strip flags");
    REGISTER_TEST_FIXTURE(s, test_ast_helper_strip_comment, "Strip comment");

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
