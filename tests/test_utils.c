
#include <check.h>
#include <talloc.h>

#include "handlebars.h"
#include "handlebars_memory.h"
#include "handlebars_utils.h"
#include "utils.h"

static void setup(void)
{
	 handlebars_memory_fail_disable();
}

static void teardown(void)
{
	 handlebars_memory_fail_disable();
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

START_TEST(test_rtrim)
{
    char * in = handlebars_talloc_strdup(NULL, "test \n \r ");
    handlebars_rtrim(in, " \t\r\n");
    ck_assert_str_eq(in, "test");
    handlebars_talloc_free(in);
}
{
    char * in = handlebars_talloc_strdup(NULL, "");
    handlebars_rtrim(in, "");
    ck_assert_str_eq(in, "");
    handlebars_talloc_free(in);
}
END_TEST

START_TEST(test_stripcslashes)
{
	char * input = strdup("\\n\\r");
	size_t input_len = strlen(input);
	const char * expected = "\n\r";
	handlebars_stripcslashes(input, &input_len);
	ck_assert_str_eq(expected, input);
	free(input);
}
{
	char * input = strdup("\\065\\x64");
	size_t input_len = strlen(input);
	const char * expected = "5d";
	handlebars_stripcslashes(input, &input_len);
	ck_assert_str_eq(expected, input);
	free(input);
}
{
	char * input = strdup("");
	size_t input_len = strlen(input);
	const char * expected = "";
	handlebars_stripcslashes(input, &input_len);
	ck_assert_str_eq(expected, input);
	free(input);
}
{
	char * input = strdup("\\{");
	size_t input_len = strlen(input);
	const char * expected = "{";
	handlebars_stripcslashes(input, &input_len);
	ck_assert_str_eq(expected, input);
	free(input);
}
END_TEST

Suite * parser_suite(void)
{
	Suite * s = suite_create("Utils");

	REGISTER_TEST_FIXTURE(s, test_addcslashes, "addcslashes");
	REGISTER_TEST_FIXTURE(s, test_rtrim, "rtrim");
	REGISTER_TEST_FIXTURE(s, test_stripcslashes, "stripcslashes");

	return s;
}

int main(void)
{
    int number_failed;
    int memdebug;
    int error;
    
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
    srunner_run_all(sr, CK_VERBOSE);
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
