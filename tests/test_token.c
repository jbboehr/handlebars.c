
#include <check.h>
#include <talloc.h>

#include "handlebars.h"
#include "handlebars_memory.h"
#include "handlebars_token.h"
#include "handlebars.tab.h"
#include "utils.h"

static void setup(void)
{
	 handlebars_memory_fail_disable();
}

static void teardown(void)
{
	 handlebars_memory_fail_disable();
}

START_TEST(test_token_readable_type)
{
#define _RTYPE_TEST(str) \
        do { \
			const char * expected = handlebars_stringify(str); \
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
}
END_TEST

START_TEST(test_token_revere_readable_type)
{
#define _RTYPE_REV_TEST(str) \
        do { \
    		int expected = str; \
    		const char * actual_str = handlebars_stringify(str); \
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
}
END_TEST
	
Suite * parser_suite(void)
{
	Suite * s = suite_create("Token");
	
	REGISTER_TEST_FIXTURE(s, test_token_readable_type, "Readable Type");
	REGISTER_TEST_FIXTURE(s, test_token_revere_readable_type, "Reverse Readable Type");
	
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
