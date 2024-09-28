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

#ifndef HANDLEBARS_TESTS_UTILS_H
#define HANDLEBARS_TESTS_UTILS_H

#include <stdlib.h>
#include <stdint.h>
#include <check.h>

#ifdef HANDLEBARS_HAVE_VALGRIND
#include <valgrind/valgrind.h>
#include <valgrind/memcheck.h>
#endif

#if defined(_WIN64) || defined(_WIN32) || defined(__WIN32__) || defined(__CYGWIN32__)
#define IS_WIN 1
#else
#define IS_WIN 0
#endif

#define REGISTER_TEST(s, name, title) \
	TCase * tc_ ## name = tcase_create(title); \
	tcase_add_test(tc_ ## name, name); \
	suite_add_tcase(s, tc_ ## name);

#define REGISTER_TEST_FIXTURE(s, name, title) \
	TCase * tc_ ## name = tcase_create(title); \
	tcase_add_checked_fixture(tc_ ## name, default_setup, default_teardown); \
	tcase_add_test(tc_ ## name, name); \
	suite_add_tcase(s, tc_ ## name);

#define _ck_assert_str_msg(X, OP, Y, msg) do { \
	const char* _ck_x = (X); \
	const char* _ck_y = (Y); \
	ck_assert_msg(0 OP strcmp(_ck_y, _ck_x), \
	"Assertion '"#X#OP#Y"' failed: "#X"==\"%s\", "#Y"==\"%s\" with message \"%s\"", _ck_x, _ck_y, msg); \
} while (0)
#define ck_assert_str_eq_msg(X, Y, msg) _ck_assert_str_msg(X, ==, Y, msg)
#define ck_assert_str_ne_msg(X, Y, msg) _ck_assert_str_msg(X, !=, Y, msg)
#define ck_assert_str_lt_msg(X, Y, msg) _ck_assert_str_msg(X, <, Y, msg)
#define ck_assert_str_le_msg(X, Y, msg) _ck_assert_str_msg(X, <=, Y, msg)
#define ck_assert_str_gt_msg(X, Y, msg) _ck_assert_str_msg(X, >, Y, msg)
#define ck_assert_str_ge_msg(X, Y, msg) _ck_assert_str_msg(X, >=, Y, msg)

#if !defined(ck_assert_ptr_eq)
#define ck_assert_ptr_eq(a, b) ck_assert_int_eq(a, b)
#endif
#if !defined(ck_assert_ptr_ne)
#define ck_assert_ptr_ne(a, b) ck_assert_int_ne(a, b)
#endif
#if !defined(ck_assert_uint_eq)
#define ck_assert_uint_eq(a, b) ck_assert_int_eq(a, b)
#endif
#if !defined(ck_assert_uint_ne)
#define ck_assert_uint_ne(a, b) ck_assert_int_ne(a, b)
#endif

#define ck_assert_hbs_str_eq(a, b) ck_assert_str_eq(hbs_str_val(a), hbs_str_val(b))
#define ck_assert_hbs_str_eq_cstr(a, b) ck_assert_str_eq(hbs_str_val(a), b)
#define ck_assert_cstr_eq_hbs_str(a, b) ck_assert_str_eq(a, hbs_str_val(b))

#if !defined(HANDLEBARS_NO_REFCOUNT)
#define ASSERT_INIT_BLOCKS() \
    do { \
        if (init_blocks != talloc_total_blocks(context)) { \
            talloc_report_full(context, stderr); \
        } \
        ck_assert_int_eq(init_blocks, talloc_total_blocks(context)); \
    } while(0)
#else
#define ASSERT_INIT_BLOCKS() do { ; } while (0)
#endif

struct handlebars_value;
struct handlebars_string;

typedef void (*scan_directory_cb)(char * filename);

int file_get_contents(const char * filename, char ** buf, size_t * len);
int scan_directory_callback(char * dirname, scan_directory_cb cb);
int regex_compare(const char * regex, const char * string, char ** error);
uint32_t adler32(unsigned char *data, size_t len);

void load_fixtures(struct handlebars_value * value);

char * normalize_template_whitespace(TALLOC_CTX *ctx, struct handlebars_string * str);

#ifdef HANDLEBARS_HAVE_JSON
struct json_object;

struct hbs_test_json_holder {
    struct json_object * obj;
};

int hbs_test_json_dtor(struct hbs_test_json_holder * holder);

#define HBS_TEST_JSON_DTOR(ctx, o) \
    do { \
        if( o ) { \
            struct hbs_test_json_holder * holder = talloc(ctx, struct hbs_test_json_holder); \
            holder->obj = (void *) o; \
            talloc_set_destructor(holder, hbs_test_json_dtor); \
        } \
    } while (0)
#endif

// Common
extern TALLOC_CTX * root;
extern struct handlebars_context * context;
extern struct handlebars_parser * parser;
extern struct handlebars_compiler * compiler;
extern struct handlebars_vm * vm;
extern size_t init_blocks;
void default_setup(void);
void default_teardown(void);
typedef Suite * (*suite_ctor_func)(void);
int default_main(suite_ctor_func suite_ctor);

#endif
