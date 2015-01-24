
#include <check.h>
#include <talloc.h>

#include "handlebars.h"
#include "handlebars_utils.h"
#include "utils.h"

static void setup(void)
{
//   handlebars_alloc_failure(0);
    ;
}

static void teardown(void)
{
  ;
}

START_TEST(test_addcslashes)
{
	const char * what = "";
	const char * input = "";
	const char * expected = "";
	char * actual = handlebars_addcslashes(input, strlen(input), what, strlen(what));
	ck_assert_str_eq(expected, actual);
}
{
	const char * what = "\r\n\t";
	const char * input = "\ttest\rlines\n";
	const char * expected = "\\ttest\\rlines\\n";
	char * actual = handlebars_addcslashes(input, strlen(input), what, strlen(what));
	ck_assert_str_eq(expected, actual);
}
{
	const char * what = "abc";
	const char * input = "amazing biscuit circus";
	const char * expected = "\\am\\azing \\bis\\cuit \\cir\\cus";
	char * actual = handlebars_addcslashes(input, strlen(input), what, strlen(what));
	ck_assert_str_eq(expected, actual);
}
{
	const char * what = "";
	const char * input = "kaboemkara!";
	const char * expected = "kaboemkara!";
	char * actual = handlebars_addcslashes(input, strlen(input), what, strlen(what));
	ck_assert_str_eq(expected, actual);
}
{
	const char * what = "bar";
	const char * input = "foobarbaz";
	const char * expected = "foo\\b\\a\\r\\b\\az";
	char * actual = handlebars_addcslashes(input, strlen(input), what, strlen(what));
	ck_assert_str_eq(expected, actual);
}
END_TEST

Suite * parser_suite(void)
{
  Suite * s = suite_create("Utils");

  REGISTER_TEST_FIXTURE(s, test_addcslashes, "addcslashes");

  return s;
}

int main(void)
{
  int number_failed;
  Suite * s = parser_suite();
  SRunner * sr = srunner_create(s);
#if defined(_WIN64) || defined(_WIN32) || defined(__WIN32__) || defined(__CYGWIN32__)
  srunner_set_fork_status(sr, CK_NOFORK);
#endif
  //srunner_set_log(sr, "test_token.log");
  srunner_run_all(sr, CK_VERBOSE);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
