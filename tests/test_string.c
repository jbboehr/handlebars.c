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
#include <stdint.h>
#include <string.h>

#include "handlebars.h"
#include "handlebars_memory.h"
#include "handlebars_rc.h"
#include "handlebars_string.h"
#include "utils.h"

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
_Static_assert(
    _Generic(
        &hbs_str_val,
        const char * (*)(const struct handlebars_string *): 1,
        default: 0
    ),
    "hbs_str_val must accept a const string and expose read-only data"
);
_Static_assert(
    _Generic(
        &hbs_str_len,
        size_t (*)(const struct handlebars_string *): 1,
        default: 0
    ),
    "hbs_str_len must accept a const string"
);
#endif



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

START_TEST(test_handlebars_string_read_accessors_preserve_binary_bytes)
{
    static const char embedded[] = {'a', '\0', 'b'};
    const struct handlebars_string * empty = handlebars_string_ctor(context, HBS_STRL(""));
    const struct handlebars_string * binary = handlebars_string_ctor(
        context,
        embedded,
        sizeof(embedded)
    );

    ck_assert_uint_eq(hbs_str_len(empty), 0);
    ck_assert_int_eq(hbs_str_val(empty)[0], '\0');
    ck_assert_uint_eq(hbs_str_len(binary), sizeof(embedded));
    ck_assert_int_eq(memcmp(hbs_str_val(binary), embedded, sizeof(embedded)), 0);
    ck_assert_int_eq(hbs_str_val(binary)[sizeof(embedded)], '\0');
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

START_TEST(test_handlebars_strnstr_needle_longer_than_haystack)
{
    const char string[] = "a";
    const char * res = handlebars_strnstr(HBS_STRL(string), HBS_STRL("longer"));
    ck_assert_ptr_eq(res, NULL);
}
END_TEST

START_TEST(test_handlebars_string_hash_collision_is_not_equal)
{
    struct handlebars_string * string1 = handlebars_string_ctor(context, HBS_STRL("lwaaaa"));
    struct handlebars_string * string2 = handlebars_string_ctor(context, HBS_STRL("tnsaaa"));

    ck_assert_uint_eq(hbs_str_hash(string1), hbs_str_hash(string2));
    ck_assert(!handlebars_string_eq(string1, string2));

    handlebars_talloc_free(string1);
    handlebars_talloc_free(string2);
}
END_TEST

static unsigned rc_dtor_calls;

static void test_rc_dtor(struct handlebars_rc * rc)
{
    (void) rc;
    rc_dtor_calls++;
}

START_TEST(test_handlebars_rc_exceeds_byte_range)
{
    struct handlebars_rc rc;
    handlebars_rc_init(&rc);
    rc_dtor_calls = 0;

    for( size_t i = 0; i < 300; i++ ) {
        handlebars_rc_addref(&rc);
    }
    ck_assert_uint_eq(handlebars_rc_refcount(&rc), 300);

    for( size_t i = 0; i < 300; i++ ) {
        handlebars_rc_delref(&rc, test_rc_dtor);
    }
    ck_assert_uint_eq(rc_dtor_calls, 1);
}
END_TEST

START_TEST(test_handlebars_string_size_rejects_overflow)
{
    size_t maximum_length = SIZE_MAX - HANDLEBARS_STRING_SIZE - 1;

    ck_assert_uint_eq(handlebars_string_size(maximum_length), SIZE_MAX);
    ck_assert_uint_eq(handlebars_string_size(maximum_length + 1), 0);
    ck_assert_uint_eq(handlebars_string_size(SIZE_MAX), 0);
    ck_assert_uint_eq(HBS_STR_SIZE(SIZE_MAX), 0);
}
END_TEST

START_TEST(test_handlebars_string_init_rejects_overflow)
{
    jmp_buf * prev = context->e->jmp;
    jmp_buf buf;

    if( handlebars_setjmp_ex(context, &buf) ) {
        context->e->jmp = prev;
        ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_NOMEM);
        return;
    }

    handlebars_talloc_free(handlebars_string_init(context, SIZE_MAX));
    context->e->jmp = prev;
    ck_abort_msg("Expected an overflowing string capacity to be rejected");
}
END_TEST

START_TEST(test_handlebars_string_extend_rejects_overflow)
{
    struct handlebars_string * string = handlebars_string_ctor(context, HBS_STRL("original"));
    jmp_buf * prev = context->e->jmp;
    jmp_buf buf;

    if( handlebars_setjmp_ex(context, &buf) ) {
        context->e->jmp = prev;
        ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_NOMEM);
        ck_assert_hbs_str_eq_cstr(string, "original");
        handlebars_talloc_free(string);
        return;
    }

    string = handlebars_string_extend(context, string, SIZE_MAX);
    context->e->jmp = prev;
    handlebars_talloc_free(string);
    ck_abort_msg("Expected an overflowing string extension to be rejected");
}
END_TEST

START_TEST(test_handlebars_string_append_self)
{
    struct handlebars_string * string = handlebars_string_ctor(context, HBS_STRL("abcdef"));

    string = handlebars_string_append_str(context, string, string);

    ck_assert_hbs_str_eq_cstr(string, "abcdefabcdef");
    handlebars_talloc_free(string);
}
END_TEST

START_TEST(test_handlebars_string_append_substring)
{
    struct handlebars_string * string = handlebars_string_ctor(context, HBS_STRL("abcdef"));

    string = handlebars_string_append(context, string, hbs_str_val(string) + 2, 3);

    ck_assert_hbs_str_eq_cstr(string, "abcdefcde");
    handlebars_talloc_free(string);
}
END_TEST

#ifndef HANDLEBARS_NO_REFCOUNT
START_TEST(test_handlebars_string_shared_append_preserves_parent)
{
    struct handlebars_context * owner = handlebars_context_ctor_ex(context);
    struct handlebars_context * auxiliary = handlebars_context_ctor_ex(context);
    struct handlebars_string * input;
    struct handlebars_string * shared;
    struct handlebars_string * actual;

    ck_assert_ptr_nonnull(owner);
    ck_assert_ptr_nonnull(auxiliary);
    input = handlebars_string_ctor(owner, HBS_STRL("abc"));
    shared = input;
    handlebars_string_addref(input);
    handlebars_string_addref(input);

    actual = handlebars_string_append(auxiliary, input, HBS_STRL("def"));

    ck_assert_ptr_eq(talloc_parent(actual), owner);
    handlebars_context_dtor(auxiliary);
    ck_assert_hbs_str_eq_cstr(actual, "abcdef");
    ck_assert_hbs_str_eq_cstr(shared, "abc");
    handlebars_string_delref(actual);
    handlebars_string_delref(shared);
    handlebars_context_dtor(owner);
}
END_TEST
#endif

#if defined(HANDLEBARS_MEMORY) && !defined(HANDLEBARS_NO_REFCOUNT)
static void assert_shared_append_allocation_is_safe(int fail_at)
{
    struct handlebars_string * input = handlebars_string_ctor(context, HBS_STRL("abc"));
    struct handlebars_string * shared = input;
    struct handlebars_string * actual;
    jmp_buf * prev = context->e->jmp;
    jmp_buf buf;

    handlebars_string_addref(input);
    handlebars_string_addref(input);

    if( handlebars_setjmp_ex(context, &buf) ) {
        handlebars_memory_fail_disable();
        context->e->jmp = prev;
        ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_NOMEM);
        ck_assert_hbs_str_eq_cstr(input, "abc");
        ck_assert_hbs_str_eq_cstr(shared, "abc");
        handlebars_string_delref(input);
        handlebars_string_delref(shared);
        return;
    }

    handlebars_memory_fail_set_flags(handlebars_memory_fail_flag_alloc);
    handlebars_memory_fail_counter(fail_at);
    actual = handlebars_string_append(context, input, HBS_STRL("def"));
    handlebars_memory_fail_disable();
    context->e->jmp = prev;

    ck_assert_hbs_str_eq_cstr(actual, "abcdef");
    ck_assert_hbs_str_eq_cstr(shared, "abc");
    handlebars_string_delref(actual);
    handlebars_string_delref(shared);
}

START_TEST(test_handlebars_string_shared_append_nomem_is_safe)
{
    assert_shared_append_allocation_is_safe(1);
    assert_shared_append_allocation_is_safe(2);
}
END_TEST
#endif

#ifdef HANDLEBARS_MEMORY
START_TEST(test_handlebars_string_compact_nomem_preserves_string)
{
    struct handlebars_string * string = handlebars_string_init(context, 64);
    struct handlebars_string * actual;

    string = handlebars_string_append(context, string, HBS_STRL("abc"));
    handlebars_memory_fail_set_flags(handlebars_memory_fail_flag_alloc);
    handlebars_memory_fail_counter(1);
    actual = handlebars_string_compact(string);
    handlebars_memory_fail_disable();

    ck_assert_ptr_eq(actual, string);
    ck_assert_hbs_str_eq_cstr(actual, "abc");
    handlebars_talloc_free(actual);
}
END_TEST
#endif

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

START_TEST(test_handlebars_string_reduce_refreshes_cached_hash)
{
    static const char input_bytes[] = {'a', '/', 'b', '\0', '/', 'c'};
    static const char expected_bytes[] = {'a', '.', 'b', '\0', '.', 'c'};
    struct handlebars_string * input = handlebars_string_ctor(
        context,
        input_bytes,
        sizeof(input_bytes)
    );
    uint32_t original_hash = hbs_str_hash(input);

    ck_assert_uint_eq(
        original_hash,
        handlebars_string_hash(input_bytes, sizeof(input_bytes))
    );

    input = handlebars_str_reduce(input, HBS_STRL("/"), HBS_STRL("."));

    ck_assert_uint_eq(hbs_str_len(input), sizeof(expected_bytes));
    ck_assert_int_eq(memcmp(hbs_str_val(input), expected_bytes, sizeof(expected_bytes)), 0);
    ck_assert_uint_eq(
        hbs_str_hash(input),
        handlebars_string_hash(expected_bytes, sizeof(expected_bytes))
    );
    handlebars_talloc_free(input);
}
END_TEST

#ifndef HANDLEBARS_NO_REFCOUNT
START_TEST(test_handlebars_string_reduce_with_separation)
{
    struct handlebars_string * input = handlebars_string_ctor(context, HBS_STRL("abcdef"));
    struct handlebars_string * shared = input;
    handlebars_string_addref(input);
    handlebars_string_addref(input);

    struct handlebars_string * actual = handlebars_str_reduce(input, HBS_STRL("bcd"), HBS_STRL("qq"));

    ck_assert_ptr_ne(actual, shared);
    ck_assert_hbs_str_eq_cstr(actual, "aqqef");
    ck_assert_hbs_str_eq_cstr(shared, "abcdef");
    handlebars_string_delref(actual);
    handlebars_string_delref(shared);
}
END_TEST
#endif

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

START_TEST(test_handlebars_string_replace_expanding)
{
    struct handlebars_string * input = handlebars_string_ctor(context, HBS_STRL("aaaa"));
    struct handlebars_string * actual = handlebars_str_replace(context, input, HBS_STRL("a"), HBS_STRL("bb"));

    ck_assert_hbs_str_eq_cstr(actual, "bbbbbbbb");
    handlebars_talloc_free(actual);
    handlebars_talloc_free(input);
}
END_TEST

#ifdef HANDLEBARS_MEMORY
START_TEST(test_handlebars_string_replace_expanding_allocates_once)
{
    struct handlebars_string * input = handlebars_string_ctor(context, HBS_STRL("aaaa"));
    struct handlebars_string * actual;

    handlebars_memory_fail_set_flags(handlebars_memory_fail_flag_alloc);
    handlebars_memory_fail_counter(2);
    actual = handlebars_str_replace(context, input, HBS_STRL("a"), HBS_STRL("bb"));
    handlebars_memory_fail_disable();

    ck_assert_hbs_str_eq_cstr(actual, "bbbbbbbb");
    handlebars_talloc_free(actual);
    handlebars_talloc_free(input);
}
END_TEST
#endif

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

#ifndef HANDLEBARS_NO_REFCOUNT
START_TEST(test_handlebars_string_stripcslashes_with_separation)
{
    struct handlebars_string * input = handlebars_string_ctor(context, HBS_STRL("\\n\\r"));
    struct handlebars_string * shared = input;
    handlebars_string_addref(input);
    handlebars_string_addref(input);

    struct handlebars_string * actual = handlebars_string_stripcslashes(input);

    ck_assert_ptr_ne(actual, shared);
    ck_assert_cstr_eq_hbs_str("\n\r", actual);
    ck_assert_hbs_str_eq_cstr(shared, "\\n\\r");
    handlebars_string_delref(actual);
    handlebars_string_delref(shared);
}
END_TEST
#endif

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

START_TEST(test_handlebars_string_asprintf_append_uses_spare_capacity)
{
    struct handlebars_string * input = handlebars_string_init(context, 64);
    struct handlebars_string * original;

    input = handlebars_string_append(context, input, HBS_STRL("prefix"));
    original = input;
    input = handlebars_string_asprintf_append(context, input, "-%s", "suffix");

    ck_assert_ptr_eq(input, original);
    ck_assert_hbs_str_eq_cstr(input, "prefix-suffix");
    handlebars_talloc_free(input);
}
END_TEST

START_TEST(test_handlebars_string_asprintf_append_preserves_parent)
{
    struct handlebars_context * owner = handlebars_context_ctor_ex(context);
    struct handlebars_context * auxiliary = handlebars_context_ctor_ex(context);
    struct handlebars_string * input;

    ck_assert_ptr_nonnull(owner);
    ck_assert_ptr_nonnull(auxiliary);
    input = handlebars_string_ctor(owner, HBS_STRL("prefix"));
    input = handlebars_string_asprintf_append(auxiliary, input, "-%s", "suffix");

    ck_assert_ptr_eq(talloc_parent(input), owner);
    handlebars_context_dtor(auxiliary);
    ck_assert_hbs_str_eq_cstr(input, "prefix-suffix");
    handlebars_talloc_free(input);
    handlebars_context_dtor(owner);
}
END_TEST

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
#pragma GCC diagnostic ignored "-Wsuggest-attribute=format"
#endif
static struct handlebars_string * test_string_vasprintf_append(
    struct handlebars_string * string,
    const char * fmt,
    ...
) {
    struct handlebars_string * result;
    va_list ap;

    va_start(ap, fmt);
    result = handlebars_string_vasprintf_append(context, string, fmt, ap);
    va_end(ap);
    return result;
}
#ifdef __clang__
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

START_TEST(test_handlebars_string_asprintf_append_with_aliased_arguments)
{
    struct handlebars_string * input = handlebars_string_ctor(context, HBS_STRL("%s:value"));

    input = test_string_vasprintf_append(
        input,
        hbs_str_val(input),
        hbs_str_val(input) + 3
    );

    ck_assert_hbs_str_eq_cstr(input, "%s:valuevalue:value");
    handlebars_talloc_free(input);
}
END_TEST

#ifndef HANDLEBARS_NO_REFCOUNT
START_TEST(test_handlebars_string_asprintf_append_with_separation)
{
    struct handlebars_string * input = handlebars_string_ctor(context, HBS_STRL("prefix"));
    struct handlebars_string * shared = input;
    struct handlebars_string * actual;

    handlebars_string_addref(input);
    handlebars_string_addref(input);
    actual = handlebars_string_asprintf_append(context, input, "-%s", "suffix");

    ck_assert_ptr_ne(actual, shared);
    ck_assert_hbs_str_eq_cstr(actual, "prefix-suffix");
    ck_assert_hbs_str_eq_cstr(shared, "prefix");
    handlebars_string_delref(actual);
    handlebars_string_delref(shared);
}
END_TEST
#endif

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

START_TEST(test_handlebars_string_htmlspecialchars_append_self)
{
    struct handlebars_string * string = handlebars_string_ctor(context, HBS_STRL("a&"));

    string = handlebars_string_htmlspecialchars_append(
        context,
        string,
        HBS_STR_STRL(string)
    );

    ck_assert_hbs_str_eq_cstr(string, "a&a&amp;");
    handlebars_talloc_free(string);
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

START_TEST(test_handlebars_string_ltrim_refreshes_cached_hash)
{
    struct handlebars_string * string = handlebars_string_ctor(context, HBS_STRL("."));

    (void) hbs_str_hash(string);
    string = handlebars_string_ltrim(string, HBS_STRL("./"));

    ck_assert_uint_eq(hbs_str_len(string), 0);
    ck_assert_uint_eq(hbs_str_hash(string), handlebars_string_hash(HBS_STRL("")));
    handlebars_talloc_free(string);
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

START_TEST(test_handlebars_string_rtrim_refreshes_cached_hash)
{
    struct handlebars_string * string = handlebars_string_ctor(context, HBS_STRL("a."));

    (void) hbs_str_hash(string);
    string = handlebars_string_rtrim(string, HBS_STRL("./"));

    ck_assert_hbs_str_eq_cstr(string, "a");
    ck_assert_uint_eq(hbs_str_hash(string), handlebars_string_hash(HBS_STRL("a")));
    handlebars_talloc_free(string);
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

START_TEST(test_handlebars_string_truncate_invalid_range)
{
    struct handlebars_string * str = handlebars_string_ctor(context, HBS_STRL("abc"));
    str = handlebars_string_truncate(str, 5, 2);
    ck_assert_hbs_str_eq_cstr(str, "");
    handlebars_talloc_free(str);
}
END_TEST

START_TEST(test_handlebars_string_indent_empty)
{
    struct handlebars_string * input = handlebars_string_ctor(context, HBS_STRL(""));
    struct handlebars_string * indent = handlebars_string_ctor(context, HBS_STRL("  "));
    struct handlebars_string * actual = handlebars_string_indent(context, input, indent);

    ck_assert_hbs_str_eq_cstr(actual, "  ");
    handlebars_talloc_free(indent);
    handlebars_talloc_free(actual);
}
END_TEST

START_TEST(test_handlebars_string_indent_append_self)
{
    struct handlebars_string * string = handlebars_string_ctor(context, HBS_STRL("a\n"));
    struct handlebars_string * indent = handlebars_string_ctor(context, HBS_STRL(">"));

    string = handlebars_string_indent_append(context, string, string, indent);

    ck_assert_hbs_str_eq_cstr(string, "a\n>a\n");
    handlebars_talloc_free(indent);
    handlebars_talloc_free(string);
}
END_TEST

static Suite * suite(void);
static Suite * suite(void)
{
    Suite * s = suite_create("String");

    REGISTER_TEST_FIXTURE(s, test_handlebars_string_hash, "handlebars_string_hash");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_read_accessors_preserve_binary_bytes, "read accessors preserve binary bytes");
    REGISTER_TEST_FIXTURE(s, test_handlebars_strnstr_1, "handlebars_strnstr 1");
    REGISTER_TEST_FIXTURE(s, test_handlebars_strnstr_2, "handlebars_strnstr 2");
    REGISTER_TEST_FIXTURE(s, test_handlebars_strnstr_3, "handlebars_strnstr 3");
    REGISTER_TEST_FIXTURE(s, test_handlebars_strnstr_4, "handlebars_strnstr 4");
    REGISTER_TEST_FIXTURE(s, test_handlebars_strnstr_5, "handlebars_strnstr 5");
    REGISTER_TEST_FIXTURE(s, test_handlebars_strnstr_needle_longer_than_haystack, "handlebars_strnstr needle longer than haystack");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_hash_collision_is_not_equal, "hash collision is not string equality");
    REGISTER_TEST_FIXTURE(s, test_handlebars_rc_exceeds_byte_range, "reference count exceeds byte range");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_size_rejects_overflow, "handlebars_string_size rejects overflow");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_init_rejects_overflow, "handlebars_string_init rejects overflow");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_extend_rejects_overflow, "handlebars_string_extend rejects overflow");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_append_self, "handlebars_string_append self");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_append_substring, "handlebars_string_append substring");
#ifndef HANDLEBARS_NO_REFCOUNT
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_shared_append_preserves_parent, "handlebars_string_append preserves parent");
#endif
#if defined(HANDLEBARS_MEMORY) && !defined(HANDLEBARS_NO_REFCOUNT)
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_shared_append_nomem_is_safe, "handlebars_string_append shared allocation failure");
#endif
#ifdef HANDLEBARS_MEMORY
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_compact_nomem_preserves_string, "handlebars_string_compact allocation failure");
#endif
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_reduce_1, "handlebars_string_reduce 1");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_reduce_2, "handlebars_string_reduce 2");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_reduce_3, "handlebars_string_reduce 3");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_reduce_refreshes_cached_hash, "handlebars_string_reduce refreshes cached hash");
#ifndef HANDLEBARS_NO_REFCOUNT
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_reduce_with_separation, "handlebars_string_reduce with separation");
#endif
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_replace_1, "handlebars_string_replace 1");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_replace_2, "handlebars_string_replace 2");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_replace_3, "handlebars_string_replace 3");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_replace_expanding, "handlebars_string_replace expanding");
#ifdef HANDLEBARS_MEMORY
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_replace_expanding_allocates_once, "handlebars_string_replace expanding allocation count");
#endif
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
#ifndef HANDLEBARS_NO_REFCOUNT
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_stripcslashes_with_separation, "handlebars_string_stripcslashes with separation");
#endif
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_asprintf, "handlebars_string_asprintf");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_asprintf_append, "handlebars_string_asprintf_append");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_asprintf_append_uses_spare_capacity, "handlebars_string_asprintf_append uses spare capacity");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_asprintf_append_preserves_parent, "handlebars_string_asprintf_append preserves parent");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_asprintf_append_with_aliased_arguments, "handlebars_string_asprintf_append aliased arguments");
#ifndef HANDLEBARS_NO_REFCOUNT
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_asprintf_append_with_separation, "handlebars_string_asprintf_append with separation");
#endif

    REGISTER_TEST_FIXTURE(s, test_handlebars_string_htmlspecialchars_1, "handlebars_string_htmlspecialchars 1");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_htmlspecialchars_2, "handlebars_string_htmlspecialchars 2");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_htmlspecialchars_3, "handlebars_string_htmlspecialchars 3");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_htmlspecialchars_4, "handlebars_string_htmlspecialchars 4");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_htmlspecialchars_5, "handlebars_string_htmlspecialchars 5");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_htmlspecialchars_6, "handlebars_string_htmlspecialchars 6");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_htmlspecialchars_append_self, "handlebars_string_htmlspecialchars append self");

    REGISTER_TEST_FIXTURE(s, test_handlebars_string_implode_1, "handlebars_string_implode 1");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_implode_2, "handlebars_string_implode 2");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_ltrim_1, "test_handlebars_string_ltrim 1");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_ltrim_2, "test_handlebars_string_ltrim 2");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_ltrim_3, "test_handlebars_string_ltrim 3");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_ltrim_refreshes_cached_hash, "handlebars_string_ltrim refreshes cached hash");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_rtrim_1, "test_handlebars_string_rtrim 1");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_rtrim_2, "test_handlebars_string_rtrim 2");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_rtrim_3, "test_handlebars_string_rtrim 3");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_rtrim_refreshes_cached_hash, "handlebars_string_rtrim refreshes cached hash");

    REGISTER_TEST_FIXTURE(s, test_handlebars_string_truncate_1, "handlebars_string_truncate 1");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_truncate_2, "handlebars_string_truncate 2");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_truncate_3, "handlebars_string_truncate 3");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_truncate_4, "handlebars_string_truncate 4");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_truncate_invalid_range, "handlebars_string_truncate invalid range");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_indent_empty, "handlebars_string_indent empty input");
    REGISTER_TEST_FIXTURE(s, test_handlebars_string_indent_append_self, "handlebars_string_indent append self");

    return s;
}

int main(void)
{
    return default_main(&suite);
}
