
#include <check.h>
#include <string.h>
#include <talloc.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "handlebars.h"
#include "handlebars_ast.h"
#include "handlebars_ast_list.h"
#include "handlebars_compiler.h"
#include "handlebars_memory.h"
#include "utils.h"

// Include compiler source file so we can test the inline functions
#include "handlebars_compiler.c"



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

START_TEST(test_compiler_ctor)
{
    struct handlebars_compiler * compiler;
    
    compiler = handlebars_compiler_ctor();
    
    ck_assert_ptr_ne(NULL, compiler);
    
    handlebars_compiler_dtor(compiler);
}
END_TEST

START_TEST(test_compiler_ctor_failed_alloc)
{
    struct handlebars_compiler * compiler;
    
    handlebars_memory_fail_enable();
    compiler = handlebars_compiler_ctor();
    handlebars_memory_fail_disable();
    
    ck_assert_ptr_eq(NULL, compiler);
}
END_TEST

START_TEST(test_compiler_dtor)
{
    struct handlebars_compiler * compiler;
    
    compiler = handlebars_compiler_ctor();
    handlebars_compiler_dtor(compiler);
}
END_TEST

START_TEST(test_compiler_get_flags)
{
    struct handlebars_compiler * compiler;
    
    compiler = handlebars_compiler_ctor();
    
    ck_assert_int_eq(0, handlebars_compiler_get_flags(compiler));
    
    compiler->flags = handlebars_compiler_flag_all;
    
    ck_assert_int_eq(handlebars_compiler_flag_all, handlebars_compiler_get_flags(compiler));
}
END_TEST

START_TEST(test_compiler_set_flags)
{
    struct handlebars_compiler * compiler;
    
    compiler = handlebars_compiler_ctor();
    
    // Make sure it can't change result flags
    handlebars_compiler_set_flags(compiler, handlebars_compiler_flag_is_simple);
    ck_assert_int_eq(0, compiler->flags);
    
    // Make sure it changes option flags
    handlebars_compiler_set_flags(compiler, handlebars_compiler_flag_string_params);
    ck_assert_int_eq(handlebars_compiler_flag_string_params, compiler->flags);
    ck_assert_int_eq(1, compiler->string_params);
    ck_assert_int_eq(0, compiler->track_ids);
    
    handlebars_compiler_set_flags(compiler, handlebars_compiler_flag_track_ids);
    ck_assert_int_eq(handlebars_compiler_flag_track_ids, compiler->flags);
    ck_assert_int_eq(0, compiler->string_params);
    ck_assert_int_eq(1, compiler->track_ids);
    
    // Make sure it can't change result flags pt 2
    handlebars_compiler_set_flags(compiler, handlebars_compiler_flag_use_partial);
    ck_assert_int_eq(0, compiler->flags);
}
END_TEST

START_TEST(test_compiler_is_known_helper)
{
    struct handlebars_compiler * compiler;
    struct handlebars_ast_node * id;
    struct handlebars_ast_list * parts;
    struct handlebars_ast_node * path_segment;
    const char * helper1 = "if";
    const char * helper2 = "unless";
    const char * helper3 = "foobar";
    const char * helper4 = "";
    
    compiler = handlebars_compiler_ctor();
    ck_assert_int_eq(0, handlebars_compiler_is_known_helper(compiler, NULL));
    
    id = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_ID, compiler);
    ck_assert_int_eq(0, handlebars_compiler_is_known_helper(compiler, id));
    
    id->node.id.parts = parts = handlebars_ast_list_ctor(compiler);
    ck_assert_int_eq(0, handlebars_compiler_is_known_helper(compiler, id));
    
    path_segment = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_ID, compiler);
    handlebars_ast_list_append(parts, path_segment);
    ck_assert_int_eq(0, handlebars_compiler_is_known_helper(compiler, id));
    
    path_segment->node.path_segment.part = helper1;
    path_segment->node.path_segment.part_length = strlen(helper1);
    ck_assert_int_eq(1, handlebars_compiler_is_known_helper(compiler, id));
    
    path_segment->node.path_segment.part = helper2;
    path_segment->node.path_segment.part_length = strlen(helper2);
    ck_assert_int_eq(1, handlebars_compiler_is_known_helper(compiler, id));
    
    path_segment->node.path_segment.part = helper3;
    path_segment->node.path_segment.part_length = strlen(helper3);
    ck_assert_int_eq(0, handlebars_compiler_is_known_helper(compiler, id));
    
    path_segment->node.path_segment.part = helper4;
    path_segment->node.path_segment.part_length = strlen(helper4);
    ck_assert_int_eq(0, handlebars_compiler_is_known_helper(compiler, id));
}
END_TEST

Suite * parser_suite(void)
{
    Suite * s = suite_create("Compiler");
    
	REGISTER_TEST_FIXTURE(s, test_compiler_ctor, "Constructor");
	REGISTER_TEST_FIXTURE(s, test_compiler_ctor_failed_alloc, "Constructor (failed alloc)");
	REGISTER_TEST_FIXTURE(s, test_compiler_dtor, "Destructor");
	REGISTER_TEST_FIXTURE(s, test_compiler_get_flags, "Get Flags");
	REGISTER_TEST_FIXTURE(s, test_compiler_set_flags, "Set Flags");
	REGISTER_TEST_FIXTURE(s, test_compiler_is_known_helper, "Is Known Helper");
	
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
