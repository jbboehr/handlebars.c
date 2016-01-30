
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
	tcase_add_checked_fixture(tc_ ## name, setup, teardown); \
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

struct handlebars_value;

const int MOD_ADLER;

typedef void (*scan_directory_cb)(char * filename);

int file_get_contents(const char * filename, char ** buf, size_t * len);
int scan_directory_callback(char * dirname, scan_directory_cb cb);
int regex_compare(const char * regex, const char * string, char ** error);
uint32_t adler32(unsigned char *data, size_t len);

void load_fixtures(struct handlebars_value * value);

#endif
