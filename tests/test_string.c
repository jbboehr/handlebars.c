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
#include <talloc.h>

#include "handlebars.h"
#include "handlebars_memory.h"

#include "handlebars_string.h"

#include "utils.h"


START_TEST(test_handlebars_string_hash)
    ck_assert_uint_eq(229466050689405, handlebars_string_hash(HBS_STRL("foobar\xFF")));
END_TEST

START_TEST(test_handlebars_strnstr_1)
    const char string[] = "";
    const char * res = handlebars_strnstr(HBS_STRL(string), HBS_STRL(""));
    ck_assert_ptr_eq(res, NULL);
END_TEST

START_TEST(test_handlebars_strnstr_2)
    const char string[] = "abcdefgh";
    const char * res = handlebars_strnstr(HBS_STRL(string), HBS_STRL("def"));
    ck_assert_ptr_eq(res, string + 3);
END_TEST

START_TEST(test_handlebars_strnstr_3)
    const char string[] = "a\0bcdefgh";
    const char * res = handlebars_strnstr(HBS_STRL(string), HBS_STRL("def"));
    ck_assert_ptr_eq(res, string + 4);
END_TEST

START_TEST(test_handlebars_strnstr_4)
    const char string[] = "abcdefgh";
    const char * res = handlebars_strnstr(string, 4, HBS_STRL("fgh"));
    ck_assert_ptr_eq(res, NULL);
END_TEST

START_TEST(test_handlebars_strnstr_5)
    const char string[] = "[foo\\\\]";
    const char * res = handlebars_strnstr(HBS_STRL(string), HBS_STRL("\\]"));
    ck_assert_ptr_eq(res, string + 5);
END_TEST

START_TEST(test_handlebars_string_reduce_1)
    struct handlebars_string * input = handlebars_string_ctor(context, HBS_STRL("abcdef"));
    input = handlebars_str_reduce(input, HBS_STRL("bcd"), HBS_STRL("qq"));
    ck_assert_str_eq("aqqef", input->val);
    handlebars_talloc_free(input);
END_TEST

START_TEST(test_handlebars_string_reduce_2)
    struct handlebars_string * input = handlebars_string_ctor(context, HBS_STRL(""));
    input = handlebars_str_reduce(input, HBS_STRL("a"), HBS_STRL(""));
    ck_assert_str_eq("", input->val);
    handlebars_talloc_free(input);
END_TEST

START_TEST(test_handlebars_string_reduce_3)
    struct handlebars_string * input = handlebars_string_ctor(context, HBS_STRL("QQQ"));
    input = handlebars_str_reduce(input, HBS_STRL("Q"), HBS_STRL("W"));
    ck_assert_str_eq("WWW", input->val);
    handlebars_talloc_free(input);
END_TEST

START_TEST(test_handlebars_string_addcslashes_1)
    struct handlebars_string * input = handlebars_string_ctor(context, HBS_STRL(""));
    struct handlebars_string * actual = handlebars_string_addcslashes(context, input, HBS_STRL(""));
    ck_assert_str_eq("", actual->val);
    ck_assert_ptr_ne(input, actual);
    handlebars_talloc_free(input);
    handlebars_talloc_free(actual);
END_TEST

START_TEST(test_handlebars_string_addcslashes_2)
    struct handlebars_string * input = handlebars_string_ctor(context, HBS_STRL("\ttest\rlines\n"));
    struct handlebars_string * actual = handlebars_string_addcslashes(context, input, HBS_STRL("\r\n\t"));
    ck_assert_str_eq("\\ttest\\rlines\\n", actual->val);
    ck_assert_ptr_ne(input, actual);
    handlebars_talloc_free(input);
    handlebars_talloc_free(actual);
END_TEST

START_TEST(test_handlebars_string_addcslashes_3)
    struct handlebars_string * input = handlebars_string_ctor(context, HBS_STRL("amazing biscuit circus"));
    struct handlebars_string * actual = handlebars_string_addcslashes(context, input, HBS_STRL("abc"));
    ck_assert_str_eq("\\am\\azing \\bis\\cuit \\cir\\cus", actual->val);
    ck_assert_ptr_ne(input, actual);
    handlebars_talloc_free(input);
    handlebars_talloc_free(actual);
END_TEST

START_TEST(test_handlebars_string_addcslashes_4)
    struct handlebars_string * input = handlebars_string_ctor(context, HBS_STRL("kaboemkara!"));
    struct handlebars_string * actual = handlebars_string_addcslashes(context, input, HBS_STRL(""));
    ck_assert_str_eq("kaboemkara!", actual->val);
    ck_assert_ptr_ne(input, actual);
    handlebars_talloc_free(input);
    handlebars_talloc_free(actual);
END_TEST

START_TEST(test_handlebars_string_addcslashes_5)
    struct handlebars_string * input = handlebars_string_ctor(context, HBS_STRL("foobarbaz"));
    struct handlebars_string * actual = handlebars_string_addcslashes(context, input, HBS_STRL("bar"));
    ck_assert_str_eq("foo\\b\\a\\r\\b\\az", actual->val);
    ck_assert_ptr_ne(input, actual);
    handlebars_talloc_free(input);
    handlebars_talloc_free(actual);
END_TEST

START_TEST(test_handlebars_string_addcslashes_6)
    struct handlebars_string * input = handlebars_string_ctor(context, HBS_STRL("\a\v\b\f\x3"));
    struct handlebars_string * actual = handlebars_string_addcslashes(context, input, HBS_STRL("\a\v\b\f\x3"));
    ck_assert_str_eq("\\a\\v\\b\\f\\003", actual->val);
    ck_assert_ptr_ne(input, actual);
    handlebars_talloc_free(input);
    handlebars_talloc_free(actual);
END_TEST

START_TEST(test_handlebars_string_stripcslashes_1)
    struct handlebars_string * input = handlebars_string_ctor(context, HBS_STRL("\\n\\r"));
    struct handlebars_string * actual = handlebars_string_stripcslashes(input);
    ck_assert_str_eq("\n\r", actual->val);
    ck_assert_ptr_eq(input, actual);
    handlebars_talloc_free(input);
END_TEST

START_TEST(test_handlebars_string_stripcslashes_2)
    struct handlebars_string * input = handlebars_string_ctor(context, HBS_STRL("\\065\\x64"));
    struct handlebars_string * actual = handlebars_string_stripcslashes(input);
    ck_assert_str_eq("5d", actual->val);
    ck_assert_ptr_eq(input, actual);
    handlebars_talloc_free(input);
END_TEST

START_TEST(test_handlebars_string_stripcslashes_3)
    struct handlebars_string * input = handlebars_string_ctor(context, HBS_STRL(""));
    struct handlebars_string * actual = handlebars_string_stripcslashes(input);
    ck_assert_str_eq("", actual->val);
    ck_assert_ptr_eq(input, actual);
    handlebars_talloc_free(input);
END_TEST

START_TEST(test_handlebars_string_stripcslashes_4)
    struct handlebars_string * input = handlebars_string_ctor(context, HBS_STRL("\\{"));
    struct handlebars_string * actual = handlebars_string_stripcslashes(input);
    ck_assert_str_eq("{", actual->val);
    ck_assert_ptr_eq(input, actual);
    handlebars_talloc_free(input);
END_TEST

START_TEST(test_handlebars_string_stripcslashes_5)
    struct handlebars_string * input = handlebars_string_ctor(context, HBS_STRL("\\a\\t\\v\\b\\f\\\\"));
    struct handlebars_string * actual = handlebars_string_stripcslashes(input);
    ck_assert_str_eq("\a\t\v\b\f\\", actual->val);
    ck_assert_ptr_eq(input, actual);
    handlebars_talloc_free(input);
END_TEST

START_TEST(test_handlebars_string_stripcslashes_6)
    struct handlebars_string * input = handlebars_string_ctor(context, HBS_STRL("\\x3"));
    struct handlebars_string * actual = handlebars_string_stripcslashes(input);
    ck_assert_str_eq("\x3", actual->val);
    ck_assert_ptr_eq(input, actual);
    handlebars_talloc_free(input);
END_TEST

START_TEST(test_handlebars_string_stripcslashes_7)
    struct handlebars_string * input = handlebars_string_ctor(context, HBS_STRL("\\0test"));
    struct handlebars_string * actual = handlebars_string_stripcslashes(input);
    ck_assert_str_eq("", actual->val);
    ck_assert_uint_eq(5, actual->len);
    ck_assert_int_eq(0, actual->val[0]);
    ck_assert_int_eq('t', actual->val[1]);
    ck_assert_int_eq(0, actual->val[5]);
    ck_assert_ptr_eq(input, actual);
    handlebars_talloc_free(input);
END_TEST

START_TEST(test_handlebars_string_asprintf)
    struct handlebars_string * actual = handlebars_string_asprintf(context, "|%d|%c|%s|", 148, 56, "1814");
    ck_assert_str_eq(actual->val, "|148|8|1814|");
    handlebars_talloc_free(actual);
END_TEST

START_TEST(test_handlebars_string_asprintf_append)
    struct handlebars_string * input = handlebars_string_ctor(context, HBS_STRL("PREFIX"));
    input = handlebars_string_asprintf_append(context, input, "|%d|%c|%s|", 148, 56, "1814");
    ck_assert_str_eq(input->val, "PREFIX|148|8|1814|");
    handlebars_talloc_free(input);
END_TEST

START_TEST(test_handlebars_string_htmlspecialchars_1)
    struct handlebars_string * actual = handlebars_string_htmlspecialchars(context, HBS_STRL("&"));
    ck_assert_str_eq("&amp;", actual->val);
    handlebars_talloc_free(actual);
END_TEST

START_TEST(test_handlebars_string_htmlspecialchars_2)
    struct handlebars_string * actual = handlebars_string_htmlspecialchars(context, HBS_STRL("<"));
    ck_assert_str_eq("&lt;", actual->val);
    handlebars_talloc_free(actual);
END_TEST

START_TEST(test_handlebars_string_htmlspecialchars_3)
    struct handlebars_string * actual = handlebars_string_htmlspecialchars(context, HBS_STRL(">"));
    ck_assert_str_eq("&gt;", actual->val);
    handlebars_talloc_free(actual);
END_TEST

START_TEST(test_handlebars_string_htmlspecialchars_4)
    struct handlebars_string * actual = handlebars_string_htmlspecialchars(context, HBS_STRL("'"));
    ck_assert_str_eq("&#x27;", actual->val);
    handlebars_talloc_free(actual);
END_TEST

START_TEST(test_handlebars_string_htmlspecialchars_5)
    struct handlebars_string * actual = handlebars_string_htmlspecialchars(context, HBS_STRL("\""));
    ck_assert_str_eq("&quot;", actual->val);
    handlebars_talloc_free(actual);
END_TEST

START_TEST(test_handlebars_string_htmlspecialchars_6)
    struct handlebars_string * actual = handlebars_string_htmlspecialchars(context, HBS_STRL("a&b<c>d\'e\"f"));
    ck_assert_str_eq("a&amp;b&lt;c&gt;d&#x27;e&quot;f", actual->val);
    handlebars_talloc_free(actual);
END_TEST

START_TEST(test_handlebars_string_implode_1)
    struct handlebars_string ** parts = handlebars_talloc_array(context, struct handlebars_string *, 1);
    parts[0] = NULL;
    struct handlebars_string * actual = handlebars_string_implode(context, HBS_STRL("!!!"), parts);
    ck_assert_str_eq(actual->val, "");
    handlebars_talloc_free(parts);
    handlebars_talloc_free(actual);
END_TEST

START_TEST(test_handlebars_string_implode_2)
    struct handlebars_string ** parts = handlebars_talloc_array(context, struct handlebars_string *, 3);
    parts[0] = handlebars_string_ctor(context, HBS_STRL("one"));
    parts[1] = handlebars_string_ctor(context, HBS_STRL("two"));
    parts[2] = NULL;
    struct handlebars_string * actual = handlebars_string_implode(context, HBS_STRL("!"), parts);
    ck_assert_str_eq(actual->val, "one!two");
    handlebars_talloc_free(parts);
    handlebars_talloc_free(actual);
END_TEST

START_TEST(test_handlebars_string_ltrim_1)
    struct handlebars_string * in = handlebars_string_ctor(context, HBS_STRL(" \n \r test "));
    struct handlebars_string * ret = handlebars_string_ltrim(in, HBS_STRL(" \t\r\n"));
    ck_assert_str_eq(ret->val, "test ");
    ck_assert_ptr_eq(in, ret);
    handlebars_talloc_free(in);
END_TEST

START_TEST(test_handlebars_string_ltrim_2)
    struct handlebars_string * in = handlebars_string_ctor(context, HBS_STRL("\n  "));
    struct handlebars_string * ret = handlebars_string_ltrim(in, HBS_STRL(" \t"));
    ck_assert_str_eq(ret->val, "\n  ");
    ck_assert_ptr_eq(in, ret);
    handlebars_talloc_free(in);
END_TEST

START_TEST(test_handlebars_string_ltrim_3)
    struct handlebars_string * in = handlebars_string_ctor(context, HBS_STRL(""));
    struct handlebars_string * ret = handlebars_string_ltrim(in, HBS_STRL(""));
    ck_assert_str_eq(ret->val, "");
    ck_assert_ptr_eq(in, ret);
    handlebars_talloc_free(in);
END_TEST

START_TEST(test_handlebars_string_rtrim_1)
    struct handlebars_string * in = handlebars_string_ctor(context, HBS_STRL("test \n \r "));
    struct handlebars_string * ret = handlebars_string_rtrim(in, HBS_STRL(" \t\r\n"));
    ck_assert_str_eq(ret->val, "test");
    ck_assert_ptr_eq(in, ret);
    handlebars_talloc_free(in);
END_TEST

START_TEST(test_handlebars_string_rtrim_2)
    struct handlebars_string * in = handlebars_string_ctor(context, HBS_STRL("\n"));
    struct handlebars_string * ret = handlebars_string_rtrim(in, HBS_STRL(" \v\t\r\n"));
    ck_assert_str_eq(ret->val, "");
    ck_assert_ptr_eq(in, ret);
    handlebars_talloc_free(in);
END_TEST

START_TEST(test_handlebars_string_rtrim_3)
    struct handlebars_string * in = handlebars_string_ctor(context, HBS_STRL(""));
    struct handlebars_string * ret = handlebars_string_rtrim(in, HBS_STRL(""));
    ck_assert_str_eq(ret->val, "");
    ck_assert_ptr_eq(in, ret);
    handlebars_talloc_free(in);
END_TEST

Suite * parser_suite(void)
{
    Suite * s = suite_create("String");

    REGISTER_TEST_FIXTURE(s, test_handlebars_string_hash, "handlebars_string_hash");
    REGISTER_TEST_FIXTURE(s, test_handlebars_strnstr_1, "handlebars_strnstr 1");
    REGISTER_TEST_FIXTURE(s, test_handlebars_strnstr_2, "handlebars_strnstr 2");
    REGISTER_TEST_FIXTURE(s, test_handlebars_strnstr_3, "handlebars_strnstr 3");
    REGISTER_TEST_FIXTURE(s, test_handlebars_strnstr_4, "handlebars_strnstr 4");
    REGISTER_TEST_FIXTURE(s, test_handlebars_strnstr_5, "handlebars_strnstr 5");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_reduce_1, "handlebars_string_reduce 1");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_reduce_2, "handlebars_string_reduce 2");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_reduce_3, "handlebars_string_reduce 3");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_addcslashes_1, "handlebars_string_addcslashes 1");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_addcslashes_2, "handlebars_string_addcslashes 2");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_addcslashes_3, "handlebars_string_addcslashes 3");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_addcslashes_4, "handlebars_string_addcslashes 4");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_addcslashes_5, "handlebars_string_addcslashes 5");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_addcslashes_6, "handlebars_string_addcslashes 6");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_stripcslashes_1, "handlebars_string_addcslashes 1");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_stripcslashes_2, "handlebars_string_addcslashes 2");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_stripcslashes_3, "handlebars_string_addcslashes 3");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_stripcslashes_4, "handlebars_string_addcslashes 4");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_stripcslashes_5, "handlebars_string_addcslashes 5");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_stripcslashes_6, "handlebars_string_addcslashes 6");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_stripcslashes_7, "handlebars_string_addcslashes 7");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_asprintf, "handlebars_string_asprintf");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_asprintf_append, "handlebars_string_asprintf_append");

    REGISTER_TEST_FIXTURE(s, test_handlebars_string_htmlspecialchars_1, "handlebars_string_htmlspecialchars 1");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_htmlspecialchars_2, "handlebars_string_htmlspecialchars 2");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_htmlspecialchars_3, "handlebars_string_htmlspecialchars 3");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_htmlspecialchars_4, "handlebars_string_htmlspecialchars 4");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_htmlspecialchars_5, "handlebars_string_htmlspecialchars 5");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_htmlspecialchars_6, "handlebars_string_htmlspecialchars 6");

    REGISTER_TEST_FIXTURE(s, test_handlebars_string_implode_1, "handlebars_string_implode 1");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_implode_2, "handlebars_string_implode 2");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_ltrim_1, "test_handlebars_string_ltrim 1");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_ltrim_2, "test_handlebars_string_ltrim 2");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_ltrim_3, "test_handlebars_string_ltrim 3");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_rtrim_1, "test_handlebars_string_rtrim 1");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_rtrim_2, "test_handlebars_string_rtrim 2");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_rtrim_3, "test_handlebars_string_rtrim 3");

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
