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
    ck_assert_str_eq(hbs_str_val(tmp), "");

    tmp = handlebars_string_ctor(context, HBS_STRL("blah1"));
    handlebars_ast_helper_strip_comment(tmp);
    ck_assert_str_eq(hbs_str_val(tmp), "blah1");

    tmp = handlebars_string_ctor(context, HBS_STRL("{"));
    handlebars_ast_helper_strip_comment(tmp);
    ck_assert_str_eq(hbs_str_val(tmp), "{");

    tmp = handlebars_string_ctor(context, HBS_STRL("{{!"));
    handlebars_ast_helper_strip_comment(tmp);
    ck_assert_str_eq(hbs_str_val(tmp), "");

    tmp = handlebars_string_ctor(context, HBS_STRL("{{~!--"));
    handlebars_ast_helper_strip_comment(tmp);
    ck_assert_str_eq(hbs_str_val(tmp), "");

    tmp = handlebars_string_ctor(context, HBS_STRL("{{!-- blah"));
    handlebars_ast_helper_strip_comment(tmp);
    ck_assert_str_eq(hbs_str_val(tmp), " blah");

    tmp = handlebars_string_ctor(context, HBS_STRL("}}"));
    handlebars_ast_helper_strip_comment(tmp);
    ck_assert_str_eq(hbs_str_val(tmp), "");

    tmp = handlebars_string_ctor(context, HBS_STRL("--}}"));
    handlebars_ast_helper_strip_comment(tmp);
    ck_assert_str_eq(hbs_str_val(tmp), "");

    tmp = handlebars_string_ctor(context, HBS_STRL("{{!}}"));
    handlebars_ast_helper_strip_comment(tmp);
    ck_assert_str_eq(hbs_str_val(tmp), "");

    tmp = handlebars_string_ctor(context, HBS_STRL("{{! foo }}"));
    handlebars_ast_helper_strip_comment(tmp);
    ck_assert_str_eq(hbs_str_val(tmp), " foo ");

    tmp = handlebars_string_ctor(context, HBS_STRL("{{!-- bar --}}"));
    handlebars_ast_helper_strip_comment(tmp);
    ck_assert_str_eq(hbs_str_val(tmp), " bar ");

    tmp = handlebars_string_ctor(context, HBS_STRL("{{~!-- baz --~}}"));
    handlebars_ast_helper_strip_comment(tmp);
    ck_assert_str_eq(hbs_str_val(tmp), " baz ");
}
END_TEST

static Suite * suite(void);
static Suite * suite(void)
{
    Suite * s = suite_create("AST Helpers");

    REGISTER_TEST_FIXTURE(s, test_ast_helper_set_strip_flags, "Set strip flags");
    REGISTER_TEST_FIXTURE(s, test_ast_helper_strip_comment, "Strip comment");

    return s;
}

int main(void)
{
    return default_main(&suite);
}
