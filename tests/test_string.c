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
#include <limits.h>

#include "handlebars.h"
#include "handlebars_memory.h"
#include "handlebars_string.h"
#include "utils.h"



START_TEST(test_handlebars_string_hash)
{
#if 0
    // DJBX33A
    ck_assert_uint_eq(3127933309ul, handlebars_string_hash(HBS_STRL("foobar\xFF")));
#elif 1
    // XXH3LOW
    ck_assert_uint_eq(1811779989ul, handlebars_string_hash(HBS_STRL("")));
    ck_assert_uint_eq(813235675ul, handlebars_string_hash(HBS_STRL("foobar\xFF")));
#endif
}
END_TEST

START_TEST(test_handlebars_strnstr_1)
{
    const char string[] = "";
    const char * res = handlebars_strnstr(HBS_STRL(string), HBS_STRL(""));
    ck_assert_ptr_eq(res, NULL);
}
END_TEST

START_TEST(test_handlebars_strnstr_2)
{
    const char string[] = "abcdefgh";
    const char * res = handlebars_strnstr(HBS_STRL(string), HBS_STRL("def"));
    ck_assert_ptr_eq(res, string + 3);
}
END_TEST

START_TEST(test_handlebars_strnstr_3)
{
    const char string[] = "a\0bcdefgh";
    const char * res = handlebars_strnstr(HBS_STRL(string), HBS_STRL("def"));
    ck_assert_ptr_eq(res, string + 4);
}
END_TEST

START_TEST(test_handlebars_strnstr_4)
{
    const char string[] = "abcdefgh";
    const char * res = handlebars_strnstr(string, 4, HBS_STRL("fgh"));
    ck_assert_ptr_eq(res, NULL);
}
END_TEST

START_TEST(test_handlebars_strnstr_5)
{
    const char string[] = "[foo\\\\]";
    const char * res = handlebars_strnstr(HBS_STRL(string), HBS_STRL("\\]"));
    ck_assert_ptr_eq(res, string + 5);
}
END_TEST

START_TEST(test_handlebars_string_reduce_1)
{
    struct handlebars_string * input = handlebars_string_ctor(context, HBS_STRL("abcdef"));
    input = handlebars_str_reduce(input, HBS_STRL("bcd"), HBS_STRL("qq"));
    ck_assert_hbs_str_eq_cstr(input, "aqqef");
    handlebars_talloc_free(input);
}
END_TEST

START_TEST(test_handlebars_string_reduce_2)
{
    struct handlebars_string * input = handlebars_string_ctor(context, HBS_STRL(""));
    input = handlebars_str_reduce(input, HBS_STRL("a"), HBS_STRL(""));
    ck_assert_hbs_str_eq_cstr(input, "");
    handlebars_talloc_free(input);
}
END_TEST

START_TEST(test_handlebars_string_reduce_3)
{
    struct handlebars_string * input = handlebars_string_ctor(context, HBS_STRL("QQQ"));
    input = handlebars_str_reduce(input, HBS_STRL("Q"), HBS_STRL("W"));
    ck_assert_hbs_str_eq_cstr(input, "WWW");
    handlebars_talloc_free(input);
}
END_TEST

START_TEST(test_handlebars_string_replace_1)
{
    struct handlebars_string * input = handlebars_string_ctor(context, HBS_STRL("abcdef"));
    input = handlebars_str_replace(context, input, HBS_STRL("bcd"), HBS_STRL("qq"));
    ck_assert_hbs_str_eq_cstr(input, "aqqef");
    handlebars_talloc_free(input);
}
END_TEST

START_TEST(test_handlebars_string_replace_2)
{
    struct handlebars_string * input = handlebars_string_ctor(context, HBS_STRL(""));
    input = handlebars_str_replace(context, input, HBS_STRL("a"), HBS_STRL(""));
    ck_assert_hbs_str_eq_cstr(input, "");
    handlebars_talloc_free(input);
}
END_TEST

START_TEST(test_handlebars_string_replace_3)
{
    struct handlebars_string * input = handlebars_string_ctor(context, HBS_STRL("QQQ"));
    input = handlebars_str_replace(context, input, HBS_STRL("Q"), HBS_STRL("W"));
    ck_assert_hbs_str_eq_cstr(input, "WWW");
    handlebars_talloc_free(input);
}
END_TEST

START_TEST(test_handlebars_string_addcslashes_1)
{
    struct handlebars_string * input = handlebars_string_ctor(context, HBS_STRL(""));
    struct handlebars_string * actual = handlebars_string_addcslashes(context, input, HBS_STRL(""));
    ck_assert_hbs_str_eq_cstr(actual, "");
    ck_assert_ptr_ne(input, actual);
    handlebars_talloc_free(input);
    handlebars_talloc_free(actual);
}
END_TEST

START_TEST(test_handlebars_string_addcslashes_2)
{
    struct handlebars_string * input = handlebars_string_ctor(context, HBS_STRL("\ttest\rlines\n"));
    struct handlebars_string * actual = handlebars_string_addcslashes(context, input, HBS_STRL("\r\n\t"));
    ck_assert_hbs_str_eq_cstr(actual, "\\ttest\\rlines\\n");
    ck_assert_ptr_ne(input, actual);
    handlebars_talloc_free(input);
    handlebars_talloc_free(actual);
}
END_TEST

START_TEST(test_handlebars_string_addcslashes_3)
{
    struct handlebars_string * input = handlebars_string_ctor(context, HBS_STRL("amazing biscuit circus"));
    struct handlebars_string * actual = handlebars_string_addcslashes(context, input, HBS_STRL("abc"));
    ck_assert_hbs_str_eq_cstr(actual, "\\am\\azing \\bis\\cuit \\cir\\cus");
    ck_assert_ptr_ne(input, actual);
    handlebars_talloc_free(input);
    handlebars_talloc_free(actual);
}
END_TEST

START_TEST(test_handlebars_string_addcslashes_4)
{
    struct handlebars_string * input = handlebars_string_ctor(context, HBS_STRL("kaboemkara!"));
    struct handlebars_string * actual = handlebars_string_addcslashes(context, input, HBS_STRL(""));
    ck_assert_hbs_str_eq_cstr(actual, "kaboemkara!");
    ck_assert_ptr_ne(input, actual);
    handlebars_talloc_free(input);
    handlebars_talloc_free(actual);
}
END_TEST

START_TEST(test_handlebars_string_addcslashes_5)
{
    struct handlebars_string * input = handlebars_string_ctor(context, HBS_STRL("foobarbaz"));
    struct handlebars_string * actual = handlebars_string_addcslashes(context, input, HBS_STRL("bar"));
    ck_assert_hbs_str_eq_cstr(actual, "foo\\b\\a\\r\\b\\az");
    ck_assert_ptr_ne(input, actual);
    handlebars_talloc_free(input);
    handlebars_talloc_free(actual);
}
END_TEST

START_TEST(test_handlebars_string_addcslashes_6)
{
    struct handlebars_string * input = handlebars_string_ctor(context, HBS_STRL("\a\v\b\f\x3"));
    struct handlebars_string * actual = handlebars_string_addcslashes(context, input, HBS_STRL("\a\v\b\f\x3"));
    ck_assert_hbs_str_eq_cstr(actual, "\\a\\v\\b\\f\\003");
    ck_assert_ptr_ne(input, actual);
    handlebars_talloc_free(input);
    handlebars_talloc_free(actual);
}
END_TEST

START_TEST(test_handlebars_string_stripcslashes_1)
{
    struct handlebars_string * input = handlebars_string_ctor(context, HBS_STRL("\\n\\r"));
    struct handlebars_string * actual = handlebars_string_stripcslashes(input);
    ck_assert_cstr_eq_hbs_str("\n\r", actual);
    ck_assert_ptr_eq(input, actual);
    handlebars_talloc_free(input);
}
END_TEST

START_TEST(test_handlebars_string_stripcslashes_2)
{
    struct handlebars_string * input = handlebars_string_ctor(context, HBS_STRL("\\065\\x64"));
    struct handlebars_string * actual = handlebars_string_stripcslashes(input);
    ck_assert_cstr_eq_hbs_str("5d", actual);
    ck_assert_ptr_eq(input, actual);
    handlebars_talloc_free(input);
}
END_TEST

START_TEST(test_handlebars_string_stripcslashes_3)
{
    struct handlebars_string * input = handlebars_string_ctor(context, HBS_STRL(""));
    struct handlebars_string * actual = handlebars_string_stripcslashes(input);
    ck_assert_cstr_eq_hbs_str("", actual);
    ck_assert_ptr_eq(input, actual);
    handlebars_talloc_free(input);
}
END_TEST

START_TEST(test_handlebars_string_stripcslashes_4)
{
    struct handlebars_string * input = handlebars_string_ctor(context, HBS_STRL("\\{"));
    struct handlebars_string * actual = handlebars_string_stripcslashes(input);
    ck_assert_cstr_eq_hbs_str("{", actual);
    ck_assert_ptr_eq(input, actual);
    handlebars_talloc_free(input);
}
END_TEST

START_TEST(test_handlebars_string_stripcslashes_5)
{
    struct handlebars_string * input = handlebars_string_ctor(context, HBS_STRL("\\a\\t\\v\\b\\f\\\\"));
    struct handlebars_string * actual = handlebars_string_stripcslashes(input);
    ck_assert_cstr_eq_hbs_str("\a\t\v\b\f\\", actual);
    ck_assert_ptr_eq(input, actual);
    handlebars_talloc_free(input);
}
END_TEST

START_TEST(test_handlebars_string_stripcslashes_6)
{
    struct handlebars_string * input = handlebars_string_ctor(context, HBS_STRL("\\x3"));
    struct handlebars_string * actual = handlebars_string_stripcslashes(input);
    ck_assert_cstr_eq_hbs_str("\x3", actual);
    ck_assert_ptr_eq(input, actual);
    handlebars_talloc_free(input);
}
END_TEST

START_TEST(test_handlebars_string_stripcslashes_7)
{
    struct handlebars_string * input = handlebars_string_ctor(context, HBS_STRL("\\0test"));
    struct handlebars_string * actual = handlebars_string_stripcslashes(input);
    ck_assert_cstr_eq_hbs_str("", actual);
    ck_assert_uint_eq(5, hbs_str_len(actual));
    ck_assert_int_eq(0, hbs_str_val(actual)[0]);
    ck_assert_int_eq('t', hbs_str_val(actual)[1]);
    ck_assert_int_eq(0, hbs_str_val(actual)[5]);
    ck_assert_ptr_eq(input, actual);
    handlebars_talloc_free(input);
}
END_TEST

START_TEST(test_handlebars_string_asprintf)
{
    struct handlebars_string * actual = handlebars_string_asprintf(context, "|%d|%c|%s|", 148, 56, "1814");
    ck_assert_hbs_str_eq_cstr(actual, "|148|8|1814|");
    handlebars_talloc_free(actual);
}
END_TEST

START_TEST(test_handlebars_string_asprintf_append)
{
    struct handlebars_string * input = handlebars_string_ctor(context, HBS_STRL("PREFIX"));
    input = handlebars_string_asprintf_append(context, input, "|%d|%c|%s|", 148, 56, "1814");
    ck_assert_hbs_str_eq_cstr(input, "PREFIX|148|8|1814|");
    handlebars_talloc_free(input);
}
END_TEST

START_TEST(test_handlebars_string_htmlspecialchars_1)
{
    struct handlebars_string * actual = handlebars_string_htmlspecialchars(context, HBS_STRL("&"));
    ck_assert_cstr_eq_hbs_str("&amp;", actual);
    handlebars_talloc_free(actual);
}
END_TEST

START_TEST(test_handlebars_string_htmlspecialchars_2)
{
    struct handlebars_string * actual = handlebars_string_htmlspecialchars(context, HBS_STRL("<"));
    ck_assert_cstr_eq_hbs_str("&lt;", actual);
    handlebars_talloc_free(actual);
}
END_TEST

START_TEST(test_handlebars_string_htmlspecialchars_3)
{
    struct handlebars_string * actual = handlebars_string_htmlspecialchars(context, HBS_STRL(">"));
    ck_assert_cstr_eq_hbs_str("&gt;", actual);
    handlebars_talloc_free(actual);
}
END_TEST

START_TEST(test_handlebars_string_htmlspecialchars_4)
{
    struct handlebars_string * actual = handlebars_string_htmlspecialchars(context, HBS_STRL("'"));
    ck_assert_cstr_eq_hbs_str("&#x27;", actual);
    handlebars_talloc_free(actual);
}
END_TEST

START_TEST(test_handlebars_string_htmlspecialchars_5)
{
    struct handlebars_string * actual = handlebars_string_htmlspecialchars(context, HBS_STRL("\""));
    ck_assert_cstr_eq_hbs_str("&quot;", actual);
    handlebars_talloc_free(actual);
}
END_TEST

START_TEST(test_handlebars_string_htmlspecialchars_6)
{
    struct handlebars_string * actual = handlebars_string_htmlspecialchars(context, HBS_STRL("a&b<c>d\'e\"f"));
    ck_assert_cstr_eq_hbs_str("a&amp;b&lt;c&gt;d&#x27;e&quot;f", actual);
    handlebars_talloc_free(actual);
}
END_TEST

START_TEST(test_handlebars_string_implode_1)
{
    struct handlebars_string ** parts = handlebars_talloc_array(context, struct handlebars_string *, 1);
    parts[0] = NULL;
    struct handlebars_string * actual = handlebars_string_implode(context, HBS_STRL("!!!"), parts);
    ck_assert_hbs_str_eq_cstr(actual, "");
    handlebars_talloc_free(parts);
    handlebars_talloc_free(actual);
}
END_TEST

START_TEST(test_handlebars_string_implode_2)
{
    struct handlebars_string ** parts = handlebars_talloc_array(context, struct handlebars_string *, 3);
    parts[0] = handlebars_string_ctor(context, HBS_STRL("one"));
    parts[1] = handlebars_string_ctor(context, HBS_STRL("two"));
    parts[2] = NULL;
    struct handlebars_string * actual = handlebars_string_implode(context, HBS_STRL("!"), parts);
    ck_assert_hbs_str_eq_cstr(actual, "one!two");
    handlebars_talloc_free(parts);
    handlebars_talloc_free(actual);
}
END_TEST

START_TEST(test_handlebars_string_ltrim_1)
{
    struct handlebars_string * in = handlebars_string_ctor(context, HBS_STRL(" \n \r test "));
    struct handlebars_string * ret = handlebars_string_ltrim(in, HBS_STRL(" \t\r\n"));
    ck_assert_hbs_str_eq_cstr(ret, "test ");
    ck_assert_ptr_eq(in, ret);
    handlebars_talloc_free(in);
}
END_TEST

START_TEST(test_handlebars_string_ltrim_2)
{
    struct handlebars_string * in = handlebars_string_ctor(context, HBS_STRL("\n  "));
    struct handlebars_string * ret = handlebars_string_ltrim(in, HBS_STRL(" \t"));
    ck_assert_hbs_str_eq_cstr(ret, "\n  ");
    ck_assert_ptr_eq(in, ret);
    handlebars_talloc_free(in);
}
END_TEST

START_TEST(test_handlebars_string_ltrim_3)
{
    struct handlebars_string * in = handlebars_string_ctor(context, HBS_STRL(""));
    struct handlebars_string * ret = handlebars_string_ltrim(in, HBS_STRL(""));
    ck_assert_hbs_str_eq_cstr(ret, "");
    ck_assert_ptr_eq(in, ret);
    handlebars_talloc_free(in);
}
END_TEST

START_TEST(test_handlebars_string_rtrim_1)
{
    struct handlebars_string * in = handlebars_string_ctor(context, HBS_STRL("test \n \r "));
    struct handlebars_string * ret = handlebars_string_rtrim(in, HBS_STRL(" \t\r\n"));
    ck_assert_hbs_str_eq_cstr(ret, "test");
    ck_assert_ptr_eq(in, ret);
    handlebars_talloc_free(in);
}
END_TEST

START_TEST(test_handlebars_string_rtrim_2)
{
    struct handlebars_string * in = handlebars_string_ctor(context, HBS_STRL("\n"));
    struct handlebars_string * ret = handlebars_string_rtrim(in, HBS_STRL(" \v\t\r\n"));
    ck_assert_hbs_str_eq_cstr(ret, "");
    ck_assert_ptr_eq(in, ret);
    handlebars_talloc_free(in);
}
END_TEST

START_TEST(test_handlebars_string_rtrim_3)
{
    struct handlebars_string * in = handlebars_string_ctor(context, HBS_STRL(""));
    struct handlebars_string * ret = handlebars_string_rtrim(in, HBS_STRL(""));
    ck_assert_hbs_str_eq_cstr(ret, "");
    ck_assert_ptr_eq(in, ret);
    handlebars_talloc_free(in);
}
END_TEST

START_TEST(test_handlebars_string_truncate_1)
{
    struct handlebars_string * str = handlebars_string_ctor(context, HBS_STRL(""));
    str = handlebars_string_truncate(str, 0, 0);
    ck_assert_str_eq(hbs_str_val(str), "");
    ck_assert_uint_eq(hbs_str_len(str), 0);
    handlebars_talloc_free(str);
}
END_TEST

START_TEST(test_handlebars_string_truncate_2)
{
    struct handlebars_string * str = handlebars_string_ctor(context, HBS_STRL("a"));
    str = handlebars_string_truncate(str, 0, 0);
    ck_assert_str_eq(hbs_str_val(str), "");
    ck_assert_uint_eq(hbs_str_len(str), 0);
    handlebars_talloc_free(str);
}
END_TEST

START_TEST(test_handlebars_string_truncate_3)
{
    struct handlebars_string * str = handlebars_string_ctor(context, HBS_STRL("a"));
    str = handlebars_string_truncate(str, 0, 1);
    ck_assert_str_eq(hbs_str_val(str), "a");
    ck_assert_uint_eq(hbs_str_len(str), 1);
    handlebars_talloc_free(str);
}
END_TEST

START_TEST(test_handlebars_string_truncate_4)
{
    struct handlebars_string * str = handlebars_string_ctor(context, HBS_STRL("abcde"));
    str = handlebars_string_truncate(str, 1, 4);
    ck_assert_str_eq(hbs_str_val(str), "bcd");
    ck_assert_uint_eq(hbs_str_len(str), 3);
    handlebars_talloc_free(str);
}
END_TEST

static Suite * suite(void);
static Suite * suite(void)
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
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_replace_1, "handlebars_string_replace 1");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_replace_2, "handlebars_string_replace 2");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_replace_3, "handlebars_string_replace 3");
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

    REGISTER_TEST_FIXTURE(s, test_handlebars_string_truncate_1, "handlebars_string_truncate 1");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_truncate_2, "handlebars_string_truncate 2");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_truncate_3, "handlebars_string_truncate 3");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_truncate_4, "handlebars_string_truncate 4");

    return s;
}

int main(void)
{
    return default_main(&suite);
}
