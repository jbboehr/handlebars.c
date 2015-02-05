
#include <check.h>
#include <talloc.h>

#include "handlebars.h"
#include "handlebars_ast.h"
#include "handlebars_context.h"
#include "handlebars_memory.h"
#include "handlebars.tab.h"
#include "utils.h"

static TALLOC_CTX * ctx;

static void setup(void)
{
    handlebars_memory_fail_disable();
    ctx = talloc_init(NULL);
}

static void teardown(void)
{
    handlebars_memory_fail_disable();
    talloc_free(ctx);
    ctx = NULL;
}

START_TEST(test_ast_node_ctor)
{
    struct handlebars_ast_node * node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_PROGRAM, ctx);
    
    ck_assert_ptr_ne(NULL, node);
    ck_assert_int_eq(HANDLEBARS_AST_NODE_PROGRAM, node->type);
}
END_TEST

START_TEST(test_ast_node_ctor_failed_alloc)
{
    struct handlebars_ast_node * node;
    
    handlebars_memory_fail_enable();
    node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_PROGRAM, ctx);
    handlebars_memory_fail_disable();
    
    ck_assert_ptr_eq(NULL, node);
}
END_TEST

START_TEST(test_ast_node_dtor)
{
    struct handlebars_ast_node * node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_PROGRAM, ctx);
    handlebars_ast_node_dtor(node);
}
END_TEST

START_TEST(test_ast_node_dtor_failed_alloc)
{
    struct handlebars_ast_node * node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_PROGRAM, ctx);
    int call_count;
    
    handlebars_memory_fail_enable();
    handlebars_ast_node_dtor(node);
    call_count = handlebars_memory_get_call_counter();
    handlebars_memory_fail_disable();
    
    ck_assert_int_eq(1, call_count);
}
END_TEST
    
START_TEST(test_ast_node_readable_type)
{
#define _RTYPE_STR(str) #str
#define _RTYPE_MK(str) HANDLEBARS_AST_NODE_ ## str
#define _RTYPE_TEST(str) \
        do { \
			const char * expected = _RTYPE_STR(str); \
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
    ck_assert_str_eq("UNKNOWN", handlebars_ast_node_readable_type(-1));
}
END_TEST


Suite * parser_suite(void)
{
    Suite * s = suite_create("AST Node");
    
    REGISTER_TEST_FIXTURE(s, test_ast_node_ctor, "Constructor");
    REGISTER_TEST_FIXTURE(s, test_ast_node_ctor_failed_alloc, "Constructor (failed alloc)");
    REGISTER_TEST_FIXTURE(s, test_ast_node_dtor, "Destructor");
    REGISTER_TEST_FIXTURE(s, test_ast_node_dtor_failed_alloc, "Destructor (failed alloc)");
    REGISTER_TEST_FIXTURE(s, test_ast_node_readable_type, "Readable Type");
    
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
