
#include <check.h>
#include <string.h>
#include <talloc.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "handlebars.h"
#include "handlebars_memory.h"
#include "handlebars_opcode_printer.h"
#include "handlebars_opcodes.h"
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

START_TEST(test_operand_print_append_null)
{
    struct handlebars_operand op;
    char * str = handlebars_talloc_strdup(ctx, "");
    
    handlebars_operand_set_null(&op);
    str = handlebars_operand_print_append(str, &op);
    ck_assert_ptr_ne(NULL, str);
    ck_assert_str_eq("[NULL]", str);
}
END_TEST

START_TEST(test_operand_print_append_boolean)
{
    struct handlebars_operand op;
    char * str = handlebars_talloc_strdup(ctx, "");
    
    handlebars_operand_set_boolval(&op, 1);
    str = handlebars_operand_print_append(str, &op);
    ck_assert_ptr_ne(NULL, str);
    ck_assert_str_eq("[BOOLEAN:1]", str);
}
END_TEST

START_TEST(test_operand_print_append_long)
{
    struct handlebars_operand op;
    char * str = handlebars_talloc_strdup(ctx, "");
    
    handlebars_operand_set_longval(&op, 2358);
    str = handlebars_operand_print_append(str, &op);
    ck_assert_ptr_ne(NULL, str);
    ck_assert_str_eq("[LONG:2358]", str);
}
END_TEST

START_TEST(test_operand_print_append_string)
{
    struct handlebars_operand op;
    char * str = handlebars_talloc_strdup(ctx, "");
    
    handlebars_operand_set_stringval(ctx, &op, "baz");
    str = handlebars_operand_print_append(str, &op);
    ck_assert_ptr_ne(NULL, str);
    ck_assert_str_eq("[STRING:baz]", str);
}
END_TEST

START_TEST(test_operand_print_append_array)
{
    // @todo
}
END_TEST

START_TEST(test_opcode_print_1)
{
    struct handlebars_opcode * opcode = handlebars_opcode_ctor(ctx, handlebars_opcode_type_ambiguous_block_value);
    char * str;
    char * expected = "ambiguousBlockValue";
    
    str = handlebars_opcode_print(ctx, opcode);
    
    ck_assert_str_eq(expected, str);
}
END_TEST

START_TEST(test_opcode_print_2)
{
    struct handlebars_opcode * opcode = handlebars_opcode_ctor(ctx, handlebars_opcode_type_get_context);
    char * str;
    char * expected = "getContext[LONG:2358]";
    
    handlebars_operand_set_longval(&opcode->op1, 2358);
    str = handlebars_opcode_print(ctx, opcode);
    
    ck_assert_str_eq(expected, str);
}
END_TEST

START_TEST(test_opcode_print_3)
{
    struct handlebars_opcode * opcode = handlebars_opcode_ctor(ctx, handlebars_opcode_type_invoke_helper);
    char * str;
    char * expected = "invokeHelper[LONG:123][STRING:baz][LONG:456]";
    
    handlebars_operand_set_longval(&opcode->op1, 123);
    handlebars_operand_set_stringval(ctx, &opcode->op2, "baz");
    handlebars_operand_set_longval(&opcode->op3, 456);
    str = handlebars_opcode_print(ctx, opcode);
    
    ck_assert_str_eq(expected, str);
}
END_TEST

START_TEST(test_opcode_array_print)
{
    struct handlebars_opcode * opcode1 = handlebars_opcode_ctor(ctx, handlebars_opcode_type_get_context);
    struct handlebars_opcode * opcode2 = handlebars_opcode_ctor(ctx, handlebars_opcode_type_push_context);
    struct handlebars_opcode ** opcodes = talloc_array(ctx, struct handlebars_opcode *, 3);
    const char * expected = "getContext[LONG:2] pushContext";
    char * str;
    
    handlebars_operand_set_longval(&opcode1->op1, 2);
    opcodes[0] = opcode1;
    opcodes[1] = opcode2;
    opcodes[2] = NULL;
    str = handlebars_opcode_array_print(ctx, opcodes, 2);
    ck_assert_ptr_ne(NULL, str);
    ck_assert_str_eq(expected, str);
}
END_TEST

Suite * parser_suite(void)
{
    Suite * s = suite_create("Opcode Printer");
    
	REGISTER_TEST_FIXTURE(s, test_operand_print_append_null, "Operand Print Append (null)");
	REGISTER_TEST_FIXTURE(s, test_operand_print_append_boolean, "Operand Print Append (boolean)");
	REGISTER_TEST_FIXTURE(s, test_operand_print_append_long, "Operand Print Append (long)");
	REGISTER_TEST_FIXTURE(s, test_operand_print_append_string, "Operand Print Append (string)");
	REGISTER_TEST_FIXTURE(s, test_operand_print_append_array, "Operand Print Append (array)");
	REGISTER_TEST_FIXTURE(s, test_opcode_print_1, "Opcode Print (1)");
	REGISTER_TEST_FIXTURE(s, test_opcode_print_2, "Opcode Print (2)");
	REGISTER_TEST_FIXTURE(s, test_opcode_print_3, "Opcode Print (3)");
	REGISTER_TEST_FIXTURE(s, test_opcode_array_print, "Opcode Array Print");
	
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
