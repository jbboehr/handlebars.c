
#include <check.h>
#include <talloc.h>

#include "handlebars.h"
#include "handlebars_ast.h"
#include "handlebars_memory.h"
#include "utils.h"

static void setup(void)
{
    handlebars_memory_fail_disable();
}

static void teardown(void)
{
    handlebars_memory_fail_disable();
}

START_TEST(test_ast_node_readable_type)
{
#define _RTYPE_MK(str) HANDLEBARS_AST_NODE_ ## str
#define _RTYPE_TEST(str) \
        do { \
			const char * expected = handlebars_stringify(str); \
			const char * actual = handlebars_ast_node_readable_type(_RTYPE_MK(str)); \
			ck_assert_str_eq(expected, actual); \
		} while(0)
    
	_RTYPE_TEST(NIL);
	_RTYPE_TEST(PROGRAM);
    _RTYPE_TEST(PROGRAM);
    _RTYPE_TEST(MUSTACHE);
    _RTYPE_TEST(SEXPR);
    _RTYPE_TEST(PARTIAL);
    _RTYPE_TEST(BLOCK);
    _RTYPE_TEST(RAW_BLOCK);
    _RTYPE_TEST(CONTENT);
    _RTYPE_TEST(HASH);
    _RTYPE_TEST(HASH_SEGMENT);
    _RTYPE_TEST(ID);
    _RTYPE_TEST(PARTIAL_NAME);
    _RTYPE_TEST(DATA);
    _RTYPE_TEST(STRING);
    _RTYPE_TEST(NUMBER);
    _RTYPE_TEST(BOOLEAN);
    _RTYPE_TEST(COMMENT);
    _RTYPE_TEST(PATH_SEGMENT);
}
END_TEST

Suite * parser_suite(void)
{
    Suite * s = suite_create("AST Node");
    
    REGISTER_TEST_FIXTURE(s, test_ast_node_readable_type, "Append");
    
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
