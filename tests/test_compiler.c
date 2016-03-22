
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <check.h>
#include <string.h>
#include <talloc.h>

#include "handlebars.h"
#include "handlebars_ast.h"
#include "handlebars_ast_list.h"
#include "handlebars_compiler.h"
#include "handlebars_memory.h"
#include "utils.h"

// Include compiler source file so we can test the inline functions
#include "handlebars_compiler.c"



START_TEST(test_compiler_ctor)
{
    struct handlebars_compiler * compiler;
    
    compiler = handlebars_compiler_ctor(context);
    
    ck_assert_ptr_ne(NULL, compiler);
    
    handlebars_compiler_dtor(compiler);
}
END_TEST

START_TEST(test_compiler_ctor_failed_alloc)
{
    jmp_buf buf;

    context->jmp = &buf;
    if( setjmp(buf) ) {
        ck_assert(1);
        return;
    }

    handlebars_memory_fail_enable();
    handlebars_compiler_ctor(context);
    handlebars_memory_fail_disable();

    ck_assert(0);
}
END_TEST

START_TEST(test_compiler_dtor)
{
    struct handlebars_compiler * compiler;
    
    compiler = handlebars_compiler_ctor(context);
    handlebars_compiler_dtor(compiler);
}
END_TEST

START_TEST(test_compiler_get_flags)
{
    ck_assert_int_eq(0, handlebars_compiler_get_flags(compiler));
    
    compiler->flags = handlebars_compiler_flag_all;
    
    ck_assert_int_eq(handlebars_compiler_flag_all, handlebars_compiler_get_flags(compiler));
}
END_TEST

START_TEST(test_compiler_set_flags)
{
    // Make sure it changes option flags
    handlebars_compiler_set_flags(compiler, handlebars_compiler_flag_string_params);
    ck_assert_int_eq(handlebars_compiler_flag_string_params, compiler->flags);
    ck_assert_int_eq(1, compiler->string_params);
    ck_assert_int_eq(0, compiler->track_ids);
    
    handlebars_compiler_set_flags(compiler, handlebars_compiler_flag_track_ids);
    ck_assert_int_eq(handlebars_compiler_flag_track_ids, compiler->flags);
    ck_assert_int_eq(0, compiler->string_params);
    ck_assert_int_eq(1, compiler->track_ids);
}
END_TEST

START_TEST(test_compiler_is_known_helper)
{
    struct handlebars_ast_node * id;
    struct handlebars_ast_list * parts;
    struct handlebars_ast_node * path_segment;

    //ck_assert_int_eq(0, handlebars_compiler_is_known_helper(compiler, NULL));
    
    id = handlebars_ast_node_ctor(parser, HANDLEBARS_AST_NODE_PATH);
    ck_assert_int_eq(0, handlebars_compiler_is_known_helper(compiler, id));
    
    id->node.path.parts = parts = handlebars_ast_list_ctor(parser);
    ck_assert_int_eq(0, handlebars_compiler_is_known_helper(compiler, id));
    
    path_segment = handlebars_ast_node_ctor(parser, HANDLEBARS_AST_NODE_PATH);
    handlebars_ast_list_append(parts, path_segment);
    ck_assert_int_eq(0, handlebars_compiler_is_known_helper(compiler, id));
    
    path_segment->node.path_segment.part = handlebars_string_ctor(HBSCTX(compiler), HBS_STRL("if"));
    ck_assert_int_eq(1, handlebars_compiler_is_known_helper(compiler, id));
    
    path_segment->node.path_segment.part = handlebars_string_ctor(HBSCTX(compiler), HBS_STRL("unless"));
    ck_assert_int_eq(1, handlebars_compiler_is_known_helper(compiler, id));
    
    path_segment->node.path_segment.part = handlebars_string_ctor(HBSCTX(compiler), HBS_STRL("foobar"));
    ck_assert_int_eq(0, handlebars_compiler_is_known_helper(compiler, id));
    
    path_segment->node.path_segment.part = handlebars_string_ctor(HBSCTX(compiler), HBS_STRL(""));
    ck_assert_int_eq(0, handlebars_compiler_is_known_helper(compiler, id));
}
END_TEST

START_TEST(test_compiler_opcode)
{
    struct handlebars_opcode * op1;
    struct handlebars_opcode * op2;
    
    op1 = handlebars_opcode_ctor(compiler, handlebars_opcode_type_append);
    op2 = handlebars_opcode_ctor(compiler, handlebars_opcode_type_append_escaped);
    
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
	REGISTER_TEST_FIXTURE(s, test_compiler_opcode, "Push opcode");
	
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
