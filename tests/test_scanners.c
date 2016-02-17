
#include <check.h>
#include <string.h>
#include <talloc.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "handlebars_memory.h"
#include "handlebars_scanners.h"
#include "utils.h"



START_TEST(test_scanner_next_whitespace)
{
    ck_assert_int_eq(0, handlebars_scanner_next_whitespace(" \t\r", 0));
    ck_assert_int_eq(0, handlebars_scanner_next_whitespace(" \t\rfoo\n", 0));
    ck_assert_int_eq(1, handlebars_scanner_next_whitespace(" \t\r\n", 0));
    
    ck_assert_int_eq(0, handlebars_scanner_next_whitespace(" \t\rz", 1));
    ck_assert_int_eq(1, handlebars_scanner_next_whitespace(" \t\r", 1));
}
END_TEST

START_TEST(test_scanner_prev_whitespace)
{
    ck_assert_int_eq(1, handlebars_scanner_prev_whitespace("\n \t\r", 0));
    ck_assert_int_eq(0, handlebars_scanner_prev_whitespace("\n foo", 0));
    
    ck_assert_int_eq(0, handlebars_scanner_prev_whitespace("foo \t\r", 1));
    ck_assert_int_eq(1, handlebars_scanner_prev_whitespace(" \t\r", 1));
    
}
END_TEST

Suite * parser_suite(void)
{
    Suite * s = suite_create("Scanners");
    
	REGISTER_TEST_FIXTURE(s, test_scanner_next_whitespace, "Prev Whitespace");
	REGISTER_TEST_FIXTURE(s, test_scanner_prev_whitespace, "Next Whitespace");
	
	
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
