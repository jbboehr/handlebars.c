
#include <check.h>
#include <string.h>
#include <talloc.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "handlebars.h"
#include "handlebars_compiler.h"
#include "handlebars_memory.h"
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
	
	REGISTER_TEST_FIXTURE(s, test_opcode_readable_type, "Opcode Readable Type");
	
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
