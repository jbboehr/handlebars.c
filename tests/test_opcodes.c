
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <check.h>
#include <string.h>
#include <talloc.h>

#include "handlebars.h"
#include "handlebars_compiler.h"
#include "handlebars_context.h"
#include "handlebars_memory.h"
#include "handlebars_opcodes.h"
#include "utils.h"



START_TEST(test_opcode_ctor)
{
    struct handlebars_opcode * opcode = handlebars_opcode_ctor(compiler, handlebars_opcode_type_append);
    
    ck_assert_ptr_ne(NULL, opcode);
    ck_assert_int_eq(handlebars_opcode_type_append, opcode->type);
    
    handlebars_talloc_free(opcode);
}
END_TEST

START_TEST(test_opcode_ctor_failed_alloc)
{
    jmp_buf buf;

    context->e.jmp = &buf;
    if( setjmp(buf) ) {
        ck_assert(1);
        return;
    }

    handlebars_memory_fail_enable();
    handlebars_opcode_ctor(compiler, handlebars_opcode_type_append);
    handlebars_memory_fail_disable();

    ck_assert(0);
}
END_TEST

START_TEST(test_opcode_ctor_long)
{
    struct handlebars_opcode * opcode;
    
    opcode = handlebars_opcode_ctor_long(compiler, handlebars_opcode_type_get_context, 1);
    ck_assert_ptr_ne(NULL, opcode);
    ck_assert_int_eq(handlebars_opcode_type_get_context, opcode->type);
    ck_assert_int_eq(1, opcode->op1.data.longval);
    
    handlebars_talloc_free(opcode);
}
END_TEST

START_TEST(test_opcode_ctor_long_failed_alloc)
{
    jmp_buf buf;

    context->e.jmp = &buf;
    if( setjmp(buf) ) {
        ck_assert(1);
        return;
    }

    handlebars_memory_fail_enable();
    handlebars_opcode_ctor_long(compiler, handlebars_opcode_type_get_context, 1);
    handlebars_memory_fail_disable();

    ck_assert(0);
}
END_TEST

START_TEST(test_opcode_ctor_long_string)
{
    struct handlebars_opcode * opcode;
    const char * str = "blah";
    
    opcode = handlebars_opcode_ctor_long_string(compiler, handlebars_opcode_type_get_context, 1, str);
    ck_assert_ptr_ne(NULL, opcode);
    ck_assert_int_eq(handlebars_opcode_type_get_context, opcode->type);
    ck_assert_int_eq(1, opcode->op1.data.longval);
    ck_assert_ptr_ne(NULL, opcode->op2.data.stringval);
    ck_assert_str_eq(str, opcode->op2.data.stringval);
    
    handlebars_talloc_free(opcode);
}
END_TEST

START_TEST(test_opcode_ctor_long_string_failed_alloc)
{
    struct handlebars_opcode * opcode;
    const char * str = "blah";
    jmp_buf buf;

    context->e.jmp = &buf;
    if( setjmp(buf) ) {
        ck_assert(1);
        return;
    }
    
    handlebars_memory_fail_enable();
    handlebars_opcode_ctor_long_string(compiler, handlebars_opcode_type_get_context, 1, str);
    handlebars_memory_fail_disable();

    ck_assert(0);
}
END_TEST

START_TEST(test_opcode_ctor_string)
{
    struct handlebars_opcode * opcode;
    const char * str = "foo";
    
    opcode = handlebars_opcode_ctor_string(compiler, handlebars_opcode_type_append_content, str);
    ck_assert_ptr_ne(NULL, opcode);
    ck_assert_int_eq(handlebars_opcode_type_append_content, opcode->type);
    ck_assert_str_eq(str, opcode->op1.data.stringval);
    ck_assert_ptr_ne(str, opcode->op1.data.stringval);
    
    handlebars_talloc_free(opcode);
}
END_TEST

START_TEST(test_opcode_ctor_string_failed_alloc)
{
    struct handlebars_opcode * opcode;
    const char * str = "foo";
    jmp_buf buf;

    context->e.jmp = &buf;
    if( setjmp(buf) ) {
        ck_assert(1);
        return;
    }

    handlebars_memory_fail_enable();
    opcode = handlebars_opcode_ctor_string(compiler, handlebars_opcode_type_append_content, str);
    handlebars_memory_fail_disable();

    ck_assert(0);
}
END_TEST

START_TEST(test_opcode_ctor_string2)
{
    struct handlebars_opcode * opcode;
    const char * str1 = "foo";
    const char * str2 = "bar";
    
    opcode = handlebars_opcode_ctor_string2(compiler, handlebars_opcode_type_append_content, str1, str2);
    ck_assert_ptr_ne(NULL, opcode);
    ck_assert_int_eq(handlebars_opcode_type_append_content, opcode->type);
    ck_assert_str_eq(str1, opcode->op1.data.stringval);
    ck_assert_ptr_ne(str1, opcode->op1.data.stringval);
    ck_assert_str_eq(str2, opcode->op2.data.stringval);
    ck_assert_ptr_ne(str2, opcode->op2.data.stringval);
    
    handlebars_talloc_free(opcode);
}
END_TEST

START_TEST(test_opcode_ctor_string2_failed_alloc)
{
    const char * str1 = "foo";
    const char * str2 = "bar";
    jmp_buf buf;

    context->e.jmp = &buf;
    if( setjmp(buf) ) {
        ck_assert(1);
        return;
    }

    handlebars_memory_fail_enable();
    handlebars_opcode_ctor_string2(compiler, handlebars_opcode_type_append_content, str1, str2);
    handlebars_memory_fail_disable();

    ck_assert(0);
}
END_TEST

START_TEST(test_opcode_ctor_string_long)
{
    struct handlebars_opcode * opcode;
    const char * str = "foo";
    
    opcode = handlebars_opcode_ctor_string_long(compiler, handlebars_opcode_type_append_content, str, 3);
    ck_assert_ptr_ne(NULL, opcode);
    ck_assert_int_eq(handlebars_opcode_type_append_content, opcode->type);
    ck_assert_str_eq(str, opcode->op1.data.stringval);
    ck_assert_ptr_ne(str, opcode->op1.data.stringval);
    ck_assert_int_eq(3, opcode->op2.data.longval);
    
    handlebars_talloc_free(opcode);
}
END_TEST

START_TEST(test_opcode_ctor_string_long_failed_alloc)
{
    const char * str = "foo";
    jmp_buf buf;

    context->e.jmp = &buf;
    if( setjmp(buf) ) {
        ck_assert(1);
        return;
    }

    handlebars_memory_fail_enable();
    handlebars_opcode_ctor_string_long(compiler, handlebars_opcode_type_append_content, str, 3);
    handlebars_memory_fail_disable();

    ck_assert(0);
}
END_TEST

START_TEST(test_opcode_readable_type)
{
#define _RTYPE_STR(x) #x
#define _RTYPE_MK(type) handlebars_opcode_type_ ## type
#define _RTYPE_TEST(type, name) \
        do { \
			const char * expected = _RTYPE_STR(name); \
			const char * actual = handlebars_opcode_readable_type(_RTYPE_MK(type)); \
			ck_assert_str_eq(expected, actual); \
		} while(0)
		
    _RTYPE_TEST(nil, nil);
    _RTYPE_TEST(ambiguous_block_value, ambiguousBlockValue);
    _RTYPE_TEST(append, append);
    _RTYPE_TEST(append_escaped, appendEscaped);
    _RTYPE_TEST(empty_hash, emptyHash);
    _RTYPE_TEST(pop_hash, popHash);
    _RTYPE_TEST(push_context, pushContext);
    _RTYPE_TEST(push_hash, pushHash);
    _RTYPE_TEST(resolve_possible_lambda, resolvePossibleLambda);
    
    _RTYPE_TEST(get_context, getContext);
    _RTYPE_TEST(push_program, pushProgram);
    
    _RTYPE_TEST(append_content, appendContent);
    _RTYPE_TEST(assign_to_hash, assignToHash);
    _RTYPE_TEST(block_value, blockValue);
    _RTYPE_TEST(push, push);
    _RTYPE_TEST(push_literal, pushLiteral);
    _RTYPE_TEST(push_string, pushString);
    
    _RTYPE_TEST(invoke_partial, invokePartial);
    _RTYPE_TEST(push_id, pushId);
    _RTYPE_TEST(push_string_param, pushStringParam);
    
    _RTYPE_TEST(invoke_ambiguous, invokeAmbiguous);
    
    _RTYPE_TEST(invoke_known_helper, invokeKnownHelper);
    
    _RTYPE_TEST(invoke_helper, invokeHelper);
    
    _RTYPE_TEST(lookup_on_context, lookupOnContext);
    
    _RTYPE_TEST(lookup_data, lookupData);
    
    _RTYPE_TEST(invalid, invalid);
    
    ck_assert_str_eq("invalid", handlebars_opcode_readable_type(13434534));
}
END_TEST

START_TEST(test_operand_set_null)
{
    struct handlebars_operand op;
    
    handlebars_operand_set_null(&op);
    ck_assert_int_eq(handlebars_operand_type_null, op.type);
    ck_assert_int_eq(0, op.data.boolval);
    ck_assert_int_eq(0, op.data.longval);
    ck_assert_ptr_eq(NULL, op.data.stringval);
}
END_TEST

START_TEST(test_operand_set_boolval)
{
    struct handlebars_operand op;
    
    handlebars_operand_set_boolval(&op, 1);
    ck_assert_int_eq(handlebars_operand_type_boolean, op.type);
    ck_assert_int_eq(1, op.data.boolval);
}
END_TEST

START_TEST(test_operand_set_longval)
{
    struct handlebars_operand op;
    
    handlebars_operand_set_longval(&op, 12);
    ck_assert_int_eq(handlebars_operand_type_long, op.type);
    ck_assert_int_eq(12, op.data.longval);
    
    handlebars_operand_set_longval(&op, -65);
    ck_assert_int_eq(handlebars_operand_type_long, op.type);
    ck_assert_int_eq(-65, op.data.longval);
}
END_TEST

START_TEST(test_operand_set_stringval)
{
    struct handlebars_operand op;
    const char * str = "bar";
    int ret;
    
    ret = handlebars_operand_set_stringval(compiler, &op, str);
    
    ck_assert_int_eq(HANDLEBARS_SUCCESS, ret);
    ck_assert_int_eq(handlebars_operand_type_string, op.type);
    ck_assert_ptr_ne(NULL, op.data.stringval);
    ck_assert_str_eq(str, op.data.stringval);
    ck_assert_ptr_ne(str, op.data.stringval);
}
END_TEST

START_TEST(test_operand_set_stringval_failed_alloc)
{
    struct handlebars_operand op;
    const char * str = "bar";
    jmp_buf buf;

    context->e.jmp = &buf;
    if( setjmp(buf) ) {
        ck_assert(1);
        return;
    }

    handlebars_memory_fail_enable();
    handlebars_operand_set_stringval(compiler, &op, str);
    handlebars_memory_fail_disable();

    ck_assert(0);
}
END_TEST

START_TEST(test_operand_set_arrayval)
{
    struct handlebars_operand op;
    const char * strs[] = {
        "foo", "bar", "baz", "helicopter", NULL
    };
    int ret;
    const char ** ptr1;
    char ** ptr2;
    
    ret = handlebars_operand_set_arrayval(compiler, &op, strs);
    
    ck_assert_int_eq(HANDLEBARS_SUCCESS, ret);
    ck_assert_int_eq(handlebars_operand_type_array, op.type);
    ck_assert_ptr_ne(NULL, op.data.arrayval);
    
    // Compare arrays
    for( ptr1 = strs, ptr2 = op.data.arrayval; *ptr1 || *ptr2; ptr1++, ptr2++ ) {
        ck_assert_ptr_ne(*ptr1, *ptr2);
        ck_assert_str_eq(*ptr1, *ptr2);
    }
}
END_TEST

Suite * parser_suite(void)
{
    Suite * s = suite_create("Opcodes");
    
	REGISTER_TEST_FIXTURE(s, test_opcode_ctor, "Constructor");
	REGISTER_TEST_FIXTURE(s, test_opcode_ctor_failed_alloc, "Constructor (failed alloc)");
	REGISTER_TEST_FIXTURE(s, test_opcode_ctor_long, "Constructor (long) ");
	REGISTER_TEST_FIXTURE(s, test_opcode_ctor_long_failed_alloc, "Constructor (long) (failed alloc)");
	REGISTER_TEST_FIXTURE(s, test_opcode_ctor_long_string, "Constructor (long/string) ");
	REGISTER_TEST_FIXTURE(s, test_opcode_ctor_long_string_failed_alloc, "Constructor (long/string) (failed alloc)");
	REGISTER_TEST_FIXTURE(s, test_opcode_ctor_string, "Constructor (string) ");
	REGISTER_TEST_FIXTURE(s, test_opcode_ctor_string_failed_alloc, "Constructor (string) (failed alloc)");
	REGISTER_TEST_FIXTURE(s, test_opcode_ctor_string2, "Constructor (string) ");
	REGISTER_TEST_FIXTURE(s, test_opcode_ctor_string2_failed_alloc, "Constructor (string) (failed alloc)");
	REGISTER_TEST_FIXTURE(s, test_opcode_ctor_string_long, "Constructor (string/long) ");
	REGISTER_TEST_FIXTURE(s, test_opcode_ctor_string_long_failed_alloc, "Constructor (string/long) (failed alloc)");
	REGISTER_TEST_FIXTURE(s, test_opcode_readable_type, "Readable Type");
	
	REGISTER_TEST_FIXTURE(s, test_operand_set_null, "Set operand null");
	REGISTER_TEST_FIXTURE(s, test_operand_set_boolval, "Set operand boolval");
	REGISTER_TEST_FIXTURE(s, test_operand_set_longval, "Set operand longval");
	REGISTER_TEST_FIXTURE(s, test_operand_set_stringval, "Set operand stringval");
	REGISTER_TEST_FIXTURE(s, test_operand_set_stringval_failed_alloc, "Set operand stringval (failed alloc)");
	REGISTER_TEST_FIXTURE(s, test_operand_set_arrayval, "Set operand arrayval");
	
	
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
