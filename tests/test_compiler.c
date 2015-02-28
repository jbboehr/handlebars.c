
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
    ctx = talloc_new(NULL);
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
    
    compiler = handlebars_compiler_ctor(ctx);
    
    ck_assert_ptr_ne(NULL, compiler);
    
    handlebars_compiler_dtor(compiler);
}
END_TEST

START_TEST(test_compiler_ctor_failed_alloc)
{
    struct handlebars_compiler * compiler;
    
    handlebars_memory_fail_enable();
    compiler = handlebars_compiler_ctor(ctx);
    handlebars_memory_fail_disable();
    
    ck_assert_ptr_eq(NULL, compiler);
}
END_TEST

START_TEST(test_compiler_dtor)
{
    struct handlebars_compiler * compiler;
    
    compiler = handlebars_compiler_ctor(ctx);
    handlebars_compiler_dtor(compiler);
}
END_TEST

START_TEST(test_compiler_get_flags)
{
    struct handlebars_compiler * compiler;
    
    compiler = handlebars_compiler_ctor(ctx);
    
    ck_assert_int_eq(0, handlebars_compiler_get_flags(compiler));
    
    compiler->flags = handlebars_compiler_flag_all;
    
    ck_assert_int_eq(handlebars_compiler_flag_all, handlebars_compiler_get_flags(compiler));
    
    handlebars_compiler_dtor(compiler);
}
END_TEST

START_TEST(test_compiler_set_flags)
{
    struct handlebars_compiler * compiler;
    
    compiler = handlebars_compiler_ctor(ctx);
    
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
    
    handlebars_compiler_dtor(compiler);
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
    
    compiler = handlebars_compiler_ctor(ctx);
    //ck_assert_int_eq(0, handlebars_compiler_is_known_helper(compiler, NULL));
    
    id = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_ID, compiler);
    ck_assert_int_eq(0, handlebars_compiler_is_known_helper(compiler, id));
    
    id->node.id.parts = parts = handlebars_ast_list_ctor(compiler);
    ck_assert_int_eq(0, handlebars_compiler_is_known_helper(compiler, id));
    
    path_segment = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_ID, compiler);
    handlebars_ast_list_append(parts, path_segment);
    ck_assert_int_eq(0, handlebars_compiler_is_known_helper(compiler, id));
    
    path_segment->node.path_segment.part = handlebars_talloc_strdup(compiler, helper1);
    path_segment->node.path_segment.part_length = strlen(helper1);
    ck_assert_int_eq(1, handlebars_compiler_is_known_helper(compiler, id));
    
    path_segment->node.path_segment.part = handlebars_talloc_strdup(compiler, helper2);
    path_segment->node.path_segment.part_length = strlen(helper2);
    ck_assert_int_eq(1, handlebars_compiler_is_known_helper(compiler, id));
    
    path_segment->node.path_segment.part = handlebars_talloc_strdup(compiler, helper3);
    path_segment->node.path_segment.part_length = strlen(helper3);
    ck_assert_int_eq(0, handlebars_compiler_is_known_helper(compiler, id));
    
    path_segment->node.path_segment.part = handlebars_talloc_strdup(compiler, helper4);
    path_segment->node.path_segment.part_length = strlen(helper4);
    ck_assert_int_eq(0, handlebars_compiler_is_known_helper(compiler, id));
    
    handlebars_compiler_dtor(compiler);
}
END_TEST

START_TEST(test_compiler_classify_sexpr)
{
    struct handlebars_compiler * compiler;
    struct handlebars_ast_node * sexpr;
    enum handlebars_compiler_sexpr_type ret;
    const char * helper1 = "if";
    const char * helper2 = "foo";
    struct handlebars_ast_node * id;
    struct handlebars_ast_list * parts;
    struct handlebars_ast_node * path_segment;
    
    compiler = handlebars_compiler_ctor(ctx);
    sexpr = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_SEXPR, ctx);
    
    ret = handlebars_compiler_classify_sexpr(compiler, sexpr);
    //ck_assert_int_eq(handlebars_compiler_sexpr_type_simple, ret);
    
    id = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_ID, compiler);
    
    sexpr->node.sexpr.eligible_helper = 1;
    ck_assert_int_eq(handlebars_compiler_sexpr_type_simple, ret);
    
    id->node.id.parts = parts = handlebars_ast_list_ctor(compiler);
    path_segment = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_ID, compiler);
    handlebars_ast_list_append(parts, path_segment);
    path_segment->node.path_segment.part = handlebars_talloc_strdup(compiler, helper1);
    path_segment->node.path_segment.part_length = strlen(helper1);
    sexpr->node.sexpr.id = id;
    
    ret = handlebars_compiler_classify_sexpr(compiler, sexpr);
    ck_assert_int_eq(handlebars_compiler_sexpr_type_helper, ret);
    
    path_segment->node.path_segment.part = handlebars_talloc_strdup(compiler, helper2);
    path_segment->node.path_segment.part_length = strlen(helper2);
    
    ret = handlebars_compiler_classify_sexpr(compiler, sexpr);
    ck_assert_int_eq(handlebars_compiler_sexpr_type_ambiguous, ret);
    
    compiler->known_helpers_only = 1;
    ret = handlebars_compiler_classify_sexpr(compiler, sexpr);
    ck_assert_int_eq(handlebars_compiler_sexpr_type_simple, ret);
    compiler->known_helpers_only = 0;
    
    sexpr->node.sexpr.is_helper = 1;
    ret = handlebars_compiler_classify_sexpr(compiler, sexpr);
    ck_assert_int_eq(handlebars_compiler_sexpr_type_helper, ret);
    
    handlebars_compiler_dtor(compiler);
}
END_TEST

START_TEST(test_compiler_opcode)
{
    struct handlebars_compiler * compiler;
    struct handlebars_opcode * op1;
    struct handlebars_opcode * op2;
    compiler = handlebars_compiler_ctor(ctx);
    
    op1 = handlebars_opcode_ctor(ctx, handlebars_opcode_type_append);
    op2 = handlebars_opcode_ctor(ctx, handlebars_opcode_type_append_escaped);
    
    handlebars_compiler_opcode(compiler, op1);
    ck_assert_ptr_ne(NULL, compiler->opcodes);
    ck_assert_int_ne(0, compiler->opcodes_size);
    ck_assert_int_eq(1, compiler->opcodes_length);
    ck_assert_ptr_eq(op1, *compiler->opcodes);
    
    handlebars_compiler_opcode(compiler, op2);
    ck_assert_ptr_ne(NULL, compiler->opcodes);
    ck_assert_int_ne(0, compiler->opcodes_size);
    ck_assert_int_eq(2, compiler->opcodes_length);
    ck_assert_ptr_eq(op2, *(compiler->opcodes + 1));
    
    handlebars_compiler_dtor(compiler);
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
	REGISTER_TEST_FIXTURE(s, test_compiler_classify_sexpr, "Classify Sexpr");
	REGISTER_TEST_FIXTURE(s, test_compiler_opcode, "Push opcode");
	
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
