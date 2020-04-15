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

#ifndef HANDLEBARS_TESTS_UTILS_H
#define HANDLEBARS_TESTS_UTILS_H

#include <stdlib.h>
#include <stdint.h>

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

struct handlebars_value;

typedef void (*scan_directory_cb)(char * filename);

int file_get_contents(const char * filename, char ** buf, size_t * len);
int scan_directory_callback(char * dirname, scan_directory_cb cb);
int regex_compare(const char * regex, const char * string, char ** error);
uint32_t adler32(unsigned char *data, size_t len);

long json_load_compile_flags(struct json_object * object);
char ** json_load_known_helpers(void * ctx, struct json_object * object);

void load_fixtures(struct handlebars_value * value);

char * normalize_template_whitespace(TALLOC_CTX *ctx, char *str, size_t len);


// Common
extern TALLOC_CTX * root;
extern struct handlebars_context * context;
extern struct handlebars_parser * parser;
extern struct handlebars_compiler * compiler;
extern struct handlebars_vm * vm;
extern int init_blocks;
void default_setup(void);
void default_teardown(void);

#endif
