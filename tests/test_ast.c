
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
    ck_assert_str_eq("UNKNOWN", handlebars_ast_node_readable_type(-1));
}
END_TEST

START_TEST(test_check_open_close)
{
    struct YYLTYPE loc;
    struct handlebars_context * context = handlebars_context_ctor();
    struct handlebars_ast_node * block = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_BLOCK, ctx);
    struct handlebars_ast_node * mustache = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_MUSTACHE, ctx);
    struct handlebars_ast_node * sexpr = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_SEXPR, ctx);
    struct handlebars_ast_node * id = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_ID, ctx);
    struct handlebars_ast_node * id_close = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_ID, ctx);
    const char * original1 = "foo";
    const char * original2 = "bar";
    int ret;
    
    block->node.block.mustache = mustache;
    mustache->node.mustache.sexpr = sexpr;
    sexpr->node.sexpr.id = id;
    block->node.block.close = id_close;
    
    memset(&loc, 0, sizeof(loc));
    
    // Test match
    id->node.id.original = handlebars_talloc_strdup(ctx, original1);
    id_close->node.id.original = handlebars_talloc_strdup(ctx, original1);
    ret = handlebars_check_open_close(block, context, &loc);
    ck_assert_int_eq(0, ret);
    
    // Test match failure
    id->node.id.original = handlebars_talloc_strdup(ctx, original1);
    id_close->node.id.original = handlebars_talloc_strdup(ctx, original2);
    ret = handlebars_check_open_close(block, context, &loc);
    ck_assert_int_ne(0, ret);
    ck_assert_ptr_ne(NULL, context->error);
    
    // Test missing fields
    id_close->node.id.original = NULL;
    ret = handlebars_check_open_close(block, context, &loc);
    ck_assert_int_eq(1, ret);
    
    block->node.block.close = NULL;
    ret = handlebars_check_open_close(block, context, &loc);
    ck_assert_int_eq(1, ret);
    
    id->node.id.original = NULL;
    ret = handlebars_check_open_close(block, context, &loc);
    ck_assert_int_eq(1, ret);
    
    sexpr->node.sexpr.id = NULL;
    ret = handlebars_check_open_close(block, context, &loc);
    ck_assert_int_eq(1, ret);

    mustache->node.mustache.sexpr = NULL;
    ret = handlebars_check_open_close(block, context, &loc);
    ck_assert_int_eq(1, ret);

    block->node.block.mustache = NULL;
    ret = handlebars_check_open_close(block, context, &loc);
    ck_assert_int_eq(1, ret);

    ret = handlebars_check_open_close(NULL, context, &loc);
    ck_assert_int_eq(1, ret);
}
END_TEST

START_TEST(test_check_raw_open_close)
{
    struct YYLTYPE loc;
    struct handlebars_context * context = handlebars_context_ctor();
    struct handlebars_ast_node * raw_block = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_RAW_BLOCK, ctx);
    struct handlebars_ast_node * mustache = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_MUSTACHE, ctx);
    struct handlebars_ast_node * sexpr = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_SEXPR, ctx);
    struct handlebars_ast_node * id = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_ID, ctx);
    const char * original1 = "foo";
    const char * original2 = "bar";
    int ret;
    
    raw_block->node.raw_block.mustache = mustache;
    mustache->node.mustache.sexpr = sexpr;
    sexpr->node.sexpr.id = id;
    //raw_block->node.raw_block.close = id_close;
    
    memset(&loc, 0, sizeof(loc));
    
    // Test match
    id->node.id.original = handlebars_talloc_strdup(ctx, original1);
    raw_block->node.raw_block.close = handlebars_talloc_strdup(ctx, original1);
    ret = handlebars_check_raw_open_close(raw_block, context, &loc);
    ck_assert_int_eq(0, ret);
    
    // Test match failure
    id->node.id.original = handlebars_talloc_strdup(ctx, original1);
    raw_block->node.raw_block.close = handlebars_talloc_strdup(ctx, original2);
    ret = handlebars_check_raw_open_close(raw_block, context, &loc);
    ck_assert_int_ne(0, ret);
    ck_assert_ptr_ne(NULL, context->error);
    
    // Test missing fields
    raw_block->node.raw_block.close = NULL;
    ret = handlebars_check_raw_open_close(raw_block, context, &loc);
    ck_assert_int_eq(1, ret);
    
    id->node.id.original = NULL;
    ret = handlebars_check_raw_open_close(raw_block, context, &loc);
    ck_assert_int_eq(1, ret);
    
    sexpr->node.sexpr.id = NULL;
    ret = handlebars_check_raw_open_close(raw_block, context, &loc);
    ck_assert_int_eq(1, ret);
    
    mustache->node.mustache.sexpr = NULL;
    ret = handlebars_check_raw_open_close(raw_block, context, &loc);
    ck_assert_int_eq(1, ret);
    
    raw_block->node.raw_block.mustache = NULL;
    ret = handlebars_check_raw_open_close(raw_block, context, &loc);
    ck_assert_int_eq(1, ret);
    
    raw_block = NULL;
    ret = handlebars_check_raw_open_close(raw_block, context, &loc);
    ck_assert_int_eq(1, ret);
}
END_TEST

Suite * parser_suite(void)
{
    Suite * s = suite_create("AST Node");
    
    REGISTER_TEST_FIXTURE(s, test_ast_node_ctor, "Constructor");
    REGISTER_TEST_FIXTURE(s, test_ast_node_ctor_failed_alloc, "Constructor (failed alloc)");
    REGISTER_TEST_FIXTURE(s, test_ast_node_dtor, "Destructor");
    REGISTER_TEST_FIXTURE(s, test_ast_node_dtor_failed_alloc, "Destructor (failed alloc)");
    REGISTER_TEST_FIXTURE(s, test_ast_node_readable_type, "Append");
    REGISTER_TEST_FIXTURE(s, test_check_open_close, "Check open/close");
    REGISTER_TEST_FIXTURE(s, test_check_raw_open_close, "Check raw open/close");
    
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
