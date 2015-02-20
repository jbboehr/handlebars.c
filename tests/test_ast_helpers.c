
#include <check.h>
#include <talloc.h>

#include "handlebars.h"
#include "handlebars_ast.h"
#include "handlebars_ast_helpers.h"
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

START_TEST(test_ast_helper_check_block)
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
    ret = handlebars_ast_helper_check_block(block, context, &loc);
    ck_assert_int_eq(0, ret);
    
    // Test match failure
    id->node.id.original = handlebars_talloc_strdup(ctx, original1);
    id_close->node.id.original = handlebars_talloc_strdup(ctx, original2);
    ret = handlebars_ast_helper_check_block(block, context, &loc);
    ck_assert_int_ne(0, ret);
    ck_assert_ptr_ne(NULL, context->error);
    
    // Test missing fields
    id_close->node.id.original = NULL;
    ret = handlebars_ast_helper_check_block(block, context, &loc);
    ck_assert_int_eq(1, ret);
    
    block->node.block.close = NULL;
    ret = handlebars_ast_helper_check_block(block, context, &loc);
    ck_assert_int_eq(1, ret);
    
    id->node.id.original = NULL;
    ret = handlebars_ast_helper_check_block(block, context, &loc);
    ck_assert_int_eq(1, ret);
    
    sexpr->node.sexpr.id = NULL;
    ret = handlebars_ast_helper_check_block(block, context, &loc);
    ck_assert_int_eq(1, ret);

    mustache->node.mustache.sexpr = NULL;
    ret = handlebars_ast_helper_check_block(block, context, &loc);
    ck_assert_int_eq(1, ret);

    block->node.block.mustache = NULL;
    ret = handlebars_ast_helper_check_block(block, context, &loc);
    ck_assert_int_eq(1, ret);

    ret = handlebars_ast_helper_check_block(NULL, context, &loc);
    ck_assert_int_eq(1, ret);
}
END_TEST

START_TEST(test_ast_helper_check_raw_block)
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
    ret = handlebars_ast_helper_check_raw_block(raw_block, context, &loc);
    ck_assert_int_eq(0, ret);
    
    // Test match failure
    id->node.id.original = handlebars_talloc_strdup(ctx, original1);
    raw_block->node.raw_block.close = handlebars_talloc_strdup(ctx, original2);
    ret = handlebars_ast_helper_check_raw_block(raw_block, context, &loc);
    ck_assert_int_ne(0, ret);
    ck_assert_ptr_ne(NULL, context->error);
    
    // Test missing fields
    raw_block->node.raw_block.close = NULL;
    ret = handlebars_ast_helper_check_raw_block(raw_block, context, &loc);
    ck_assert_int_eq(1, ret);
    
    id->node.id.original = NULL;
    ret = handlebars_ast_helper_check_raw_block(raw_block, context, &loc);
    ck_assert_int_eq(1, ret);
    
    sexpr->node.sexpr.id = NULL;
    ret = handlebars_ast_helper_check_raw_block(raw_block, context, &loc);
    ck_assert_int_eq(1, ret);
    
    mustache->node.mustache.sexpr = NULL;
    ret = handlebars_ast_helper_check_raw_block(raw_block, context, &loc);
    ck_assert_int_eq(1, ret);
    
    raw_block->node.raw_block.mustache = NULL;
    ret = handlebars_ast_helper_check_raw_block(raw_block, context, &loc);
    ck_assert_int_eq(1, ret);
    
    raw_block = NULL;
    ret = handlebars_ast_helper_check_raw_block(raw_block, context, &loc);
    ck_assert_int_eq(1, ret);
}
END_TEST

Suite * parser_suite(void)
{
    Suite * s = suite_create("AST Helpers");
    
    REGISTER_TEST_FIXTURE(s, test_ast_helper_check_block, "Check block AST node");
    REGISTER_TEST_FIXTURE(s, test_ast_helper_check_raw_block, "Check raw block AST node");
    
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
