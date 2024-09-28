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

#include "handlebars.h"
#include "handlebars_memory.h"
#include "handlebars_string.h"
#include "handlebars_token.h"
#include "handlebars.tab.h"
#include "utils.h"



START_TEST(test_token_ctor)
{
	struct handlebars_string * string = handlebars_string_ctor(context, HBS_STRL("{{"));
    struct handlebars_token * token = handlebars_token_ctor(HBSCTX(parser), OPEN, string);

    ck_assert_ptr_ne(NULL, token);
    ck_assert_ptr_ne(NULL, handlebars_token_get_text(token));
    ck_assert_int_eq(OPEN, handlebars_token_get_type(token));
    ck_assert_hbs_str_eq_cstr(handlebars_token_get_text(token), "{{");
    ck_assert_uint_eq(sizeof("{{") - 1, hbs_str_len(handlebars_token_get_text(token)));

    handlebars_token_dtor(token);
}
END_TEST

START_TEST(test_token_ctor_failed_alloc)
{
#ifdef HANDLEBARS_MEMORY
	struct handlebars_string * string;
	jmp_buf buf;
    struct handlebars_token * token;

    if( handlebars_setjmp_ex(context, &buf) ) {
		ck_assert(1);
		return;
	}

	string = handlebars_string_ctor(context, HBS_STRL("{{"));

    handlebars_memory_fail_enable();
    token = handlebars_token_ctor(context, OPEN, string);
    (void) token;
    handlebars_memory_fail_disable();

    ck_assert(0);
#else
	fprintf(stderr, "Skipped, memory testing functions are disabled\n");
#endif
}
END_TEST

START_TEST(test_token_dtor)
{
	struct handlebars_string * string = handlebars_string_ctor(context, HBS_STRL("{{"));
    struct handlebars_token * token = handlebars_token_ctor(context, OPEN, string);
    handlebars_token_dtor(token);
}
END_TEST

START_TEST(test_token_get_type)
{
	struct handlebars_string * string = handlebars_string_ctor(context, HBS_STRL("{{"));
    struct handlebars_token * token = handlebars_token_ctor(context, OPEN, string);

    ck_assert_int_eq(OPEN, handlebars_token_get_type(token));

    handlebars_token_dtor(token);
}
END_TEST

START_TEST(test_token_get_text)
{
	struct handlebars_string * string = handlebars_string_ctor(context, HBS_STRL("{{"));
    struct handlebars_token * token = handlebars_token_ctor(context, OPEN, string);

    ck_assert_cstr_eq_hbs_str("{{", handlebars_token_get_text(token));
    ck_assert_uint_eq(sizeof("{{") - 1, hbs_str_len(handlebars_token_get_text(token)));

    handlebars_token_dtor(token);
}
END_TEST

START_TEST(test_token_readable_type)
{
#define _RTYPE_STR(str) #str
#define _RTYPE_TEST(str) \
        do { \
			const char * expected = _RTYPE_STR(str); \
			const char * actual = handlebars_token_readable_type(str); \
			ck_assert_str_eq(expected, actual); \
		} while(0)

	_RTYPE_TEST(BOOLEAN);
	_RTYPE_TEST(CLOSE);
	_RTYPE_TEST(CLOSE_RAW_BLOCK);
	_RTYPE_TEST(CLOSE_SEXPR);
	_RTYPE_TEST(CLOSE_UNESCAPED);
	_RTYPE_TEST(COMMENT);
	_RTYPE_TEST(CONTENT);
	_RTYPE_TEST(DATA);
	_RTYPE_TEST(END);
	_RTYPE_TEST(END_RAW_BLOCK);
	_RTYPE_TEST(EQUALS);
	_RTYPE_TEST(ID);
	_RTYPE_TEST(INVALID);
	_RTYPE_TEST(INVERSE);
	_RTYPE_TEST(NUMBER);
	_RTYPE_TEST(OPEN);
	_RTYPE_TEST(OPEN_BLOCK);
	_RTYPE_TEST(OPEN_ENDBLOCK);
	_RTYPE_TEST(OPEN_INVERSE);
	_RTYPE_TEST(OPEN_PARTIAL);
	_RTYPE_TEST(OPEN_RAW_BLOCK);
	_RTYPE_TEST(OPEN_SEXPR);
	_RTYPE_TEST(OPEN_UNESCAPED);
	_RTYPE_TEST(SEP);
	_RTYPE_TEST(STRING);
	ck_assert_str_eq("UNKNOWN", handlebars_token_readable_type(-1));

	// Added in v3
	_RTYPE_TEST(CLOSE_BLOCK_PARAMS);
	_RTYPE_TEST(OPEN_BLOCK_PARAMS);
	_RTYPE_TEST(OPEN_INVERSE_CHAIN);
	_RTYPE_TEST(UNDEFINED);
	ck_assert_str_eq("NULL", handlebars_token_readable_type(NUL));

	// Added in v4
	_RTYPE_TEST(OPEN_PARTIAL_BLOCK);
}
END_TEST

START_TEST(test_token_reverse_readable_type)
{
#define _RTYPE_REV_STR(str) #str
#define _RTYPE_REV_TEST(str) \
        do { \
    		int expected = str; \
    		const char * actual_str = _RTYPE_REV_STR(str); \
    		int actual = handlebars_token_reverse_readable_type(actual_str); \
    		ck_assert_int_eq(expected, actual); \
    	} while(0)

	_RTYPE_REV_TEST(BOOLEAN);
	_RTYPE_REV_TEST(CLOSE);
	_RTYPE_REV_TEST(CLOSE_RAW_BLOCK);
	_RTYPE_REV_TEST(CLOSE_SEXPR);
	_RTYPE_REV_TEST(CLOSE_UNESCAPED);
	_RTYPE_REV_TEST(COMMENT);
	_RTYPE_REV_TEST(CONTENT);
	_RTYPE_REV_TEST(DATA);
	_RTYPE_REV_TEST(END);
	_RTYPE_REV_TEST(END_RAW_BLOCK);
	_RTYPE_REV_TEST(EQUALS);
	_RTYPE_REV_TEST(ID);
	_RTYPE_REV_TEST(INVALID);
	_RTYPE_REV_TEST(INVERSE);
	_RTYPE_REV_TEST(NUMBER);
	_RTYPE_REV_TEST(OPEN);
	_RTYPE_REV_TEST(OPEN_BLOCK);
	_RTYPE_REV_TEST(OPEN_ENDBLOCK);
	_RTYPE_REV_TEST(OPEN_INVERSE);
	_RTYPE_REV_TEST(OPEN_PARTIAL);
	_RTYPE_REV_TEST(OPEN_RAW_BLOCK);
	_RTYPE_REV_TEST(OPEN_SEXPR);
	_RTYPE_REV_TEST(OPEN_UNESCAPED);
	_RTYPE_REV_TEST(SEP);
	_RTYPE_REV_TEST(STRING);
	ck_assert_int_eq(-1, handlebars_token_reverse_readable_type("UNKNOWN"));

	// Added in v3
	_RTYPE_REV_TEST(CLOSE_BLOCK_PARAMS);
	_RTYPE_REV_TEST(OPEN_BLOCK_PARAMS);
	_RTYPE_REV_TEST(OPEN_INVERSE_CHAIN);
	_RTYPE_REV_TEST(UNDEFINED);
	ck_assert_int_eq(NUL, handlebars_token_reverse_readable_type("NULL"));

	// Added in v4
	_RTYPE_REV_TEST(OPEN_PARTIAL_BLOCK);
}
END_TEST

START_TEST(test_token_print)
{
    struct handlebars_string * string = handlebars_string_ctor(context, HBS_STRL("{{"));
    struct handlebars_token * tok = handlebars_token_ctor(context, OPEN, string);
    struct handlebars_string * actual = handlebars_token_print(context, tok, 0);
    ck_assert_cstr_eq_hbs_str("OPEN [{{] ", actual);
    handlebars_talloc_free(tok);
    handlebars_talloc_free(actual);
}
END_TEST

START_TEST(test_token_print2)
{
    struct handlebars_string * string = handlebars_string_ctor(context, HBS_STRL("this\nis\ra\ttest"));
    struct handlebars_token * tok = handlebars_token_ctor(context, CONTENT, string);
    struct handlebars_string * actual = handlebars_token_print(context, tok, 0);
    ck_assert_cstr_eq_hbs_str("CONTENT [this\\nis\\ra\\ttest] ", actual);
    handlebars_talloc_free(tok);
    handlebars_talloc_free(actual);
}
END_TEST

START_TEST(test_token_print3)
{
    struct handlebars_string * string = handlebars_string_ctor(context, HBS_STRL("this\nis\ra\ttest"));
    struct handlebars_token * tok = handlebars_token_ctor(context, CONTENT, string);
    struct handlebars_string * actual = handlebars_token_print(context, tok, handlebars_token_print_flag_newlines);
    ck_assert_cstr_eq_hbs_str("CONTENT [this\\nis\\ra\\ttest]\n", actual);
    handlebars_talloc_free(tok);
    handlebars_talloc_free(actual);
}
END_TEST

START_TEST(test_token_print_failed_alloc)
{
#ifdef HANDLEBARS_MEMORY
    struct handlebars_string * string = handlebars_string_ctor(context, HBS_STRL("tok1"));
    struct handlebars_token * tok = handlebars_token_ctor(context, CONTENT, string);
	struct handlebars_string * actual;
    jmp_buf buf;

    if( !handlebars_setjmp_ex(context, &buf) ) {
        handlebars_token_dtor(tok);
        return;
    }

    handlebars_memory_fail_enable();
    actual = handlebars_token_print(context, tok, 0);
    (void) actual;
    handlebars_memory_fail_disable();

    ck_assert(0);

#else
    fprintf(stderr, "Skipped, memory testing functions are disabled\n");
#endif
}
END_TEST

static Suite * suite(void);
static Suite * suite(void)
{
	Suite * s = suite_create("Token");

	REGISTER_TEST_FIXTURE(s, test_token_ctor, "Constructor");
	REGISTER_TEST_FIXTURE(s, test_token_ctor_failed_alloc, "Constructor (failed alloc)");
	REGISTER_TEST_FIXTURE(s, test_token_dtor, "Destructor");
	REGISTER_TEST_FIXTURE(s, test_token_get_type, "Get type");
	REGISTER_TEST_FIXTURE(s, test_token_get_text, "Get text");
	REGISTER_TEST_FIXTURE(s, test_token_readable_type, "Readable Type");
	REGISTER_TEST_FIXTURE(s, test_token_reverse_readable_type, "Reverse Readable Type");
	REGISTER_TEST_FIXTURE(s, test_token_print, "Print Token");
	REGISTER_TEST_FIXTURE(s, test_token_print2, "Print Token (2)");
	REGISTER_TEST_FIXTURE(s, test_token_print3, "Print Token (3)");
	REGISTER_TEST_FIXTURE(s, test_token_print_failed_alloc, "Print Token (failed alloc)");

	return s;
}

int main(void)
{
    return default_main(&suite);
}
