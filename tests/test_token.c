
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <check.h>
#include <talloc.h>

#include "handlebars.h"
#include "handlebars_memory.h"

#include "handlebars_string.h"
#include "handlebars_token.h"
#include "handlebars.tab.h"
#include "utils.h"



START_TEST(test_token_ctor)
{
	struct handlebars_string * string = handlebars_string_ctor(context, HBS_STRL("{{"));
    struct handlebars_token * token = handlebars_token_ctor(HBSCTX(parser), OPEN, string);
    
    ck_assert_ptr_ne(NULL, token);
    ck_assert_ptr_ne(NULL, token->string);
    ck_assert_int_eq(OPEN, token->token);
    ck_assert_str_eq(token->string->val, "{{");
    ck_assert_uint_eq(sizeof("{{") - 1, token->string->len);
    
    handlebars_token_dtor(token);
}
END_TEST

START_TEST(test_token_ctor_failed_alloc)
{
#if HANDLEBARS_MEMORY
	struct handlebars_string * string;
	jmp_buf buf;

    if( handlebars_setjmp_ex(context, &buf) ) {
		ck_assert(1);
		return;
	}

	string = handlebars_string_ctor(context, HBS_STRL("{{"));

    handlebars_memory_fail_enable();
    handlebars_token_ctor(context, OPEN, string);
    handlebars_memory_fail_disable();

    ck_assert(0);
#else
	fprintf(stderr, "Skipped, memory testing functions are disabled\n");
#endif
}
END_TEST

START_TEST(test_token_dtor)
{
	struct handlebars_string * string = handlebars_string_ctor(context, HBS_STRL("{{"));
    struct handlebars_token * token = handlebars_token_ctor(context, OPEN, string);
    handlebars_token_dtor(token);
}
END_TEST

START_TEST(test_token_get_type)
{
	struct handlebars_string * string = handlebars_string_ctor(context, HBS_STRL("{{"));
    struct handlebars_token * token = handlebars_token_ctor(context, OPEN, string);
    
    ck_assert_int_eq(OPEN, handlebars_token_get_type(token));
    ck_assert_int_eq(-1, handlebars_token_get_type(NULL));
    
    handlebars_token_dtor(token);
}
END_TEST

START_TEST(test_token_get_text)
{
	struct handlebars_string * string = handlebars_string_ctor(context, HBS_STRL("{{"));
    struct handlebars_token * token = handlebars_token_ctor(context, OPEN, string);

    ck_assert_str_eq("{{", handlebars_token_get_text(token)->val);
    ck_assert_uint_eq(sizeof("{{") - 1, handlebars_token_get_text(token)->len);
    ck_assert_ptr_eq(NULL, handlebars_token_get_text(NULL));
    
    handlebars_token_dtor(token);
}
END_TEST

START_TEST(test_token_readable_type)
{
#define _RTYPE_STR(str) #str
#define _RTYPE_TEST(str) \
        do { \
			const char * expected = _RTYPE_STR(str); \
			const char * actual = handlebars_token_readable_type(str); \
			ck_assert_str_eq(expected, actual); \
		} while(0)
	
	_RTYPE_TEST(BOOLEAN);
	_RTYPE_TEST(CLOSE);
	_RTYPE_TEST(CLOSE_RAW_BLOCK);
	_RTYPE_TEST(CLOSE_SEXPR);
	_RTYPE_TEST(CLOSE_UNESCAPED);
	_RTYPE_TEST(COMMENT);
	_RTYPE_TEST(CONTENT);
	_RTYPE_TEST(DATA);
	_RTYPE_TEST(END);
	_RTYPE_TEST(END_RAW_BLOCK);
	_RTYPE_TEST(EQUALS);
	_RTYPE_TEST(ID);
	_RTYPE_TEST(INVALID);
	_RTYPE_TEST(INVERSE);
	_RTYPE_TEST(NUMBER);
	_RTYPE_TEST(OPEN);
	_RTYPE_TEST(OPEN_BLOCK);
	_RTYPE_TEST(OPEN_ENDBLOCK);
	_RTYPE_TEST(OPEN_INVERSE);
	_RTYPE_TEST(OPEN_PARTIAL);
	_RTYPE_TEST(OPEN_RAW_BLOCK);
	_RTYPE_TEST(OPEN_SEXPR);
	_RTYPE_TEST(OPEN_UNESCAPED);
	_RTYPE_TEST(SEP);
	_RTYPE_TEST(STRING);
	ck_assert_str_eq("UNKNOWN", handlebars_token_readable_type(-1));
	
	// Added in v3
	_RTYPE_TEST(CLOSE_BLOCK_PARAMS);
	_RTYPE_TEST(OPEN_BLOCK_PARAMS);
	_RTYPE_TEST(OPEN_INVERSE_CHAIN);
	_RTYPE_TEST(UNDEFINED);
	ck_assert_str_eq("NULL", handlebars_token_readable_type(NUL));

	// Added in v4
	_RTYPE_TEST(OPEN_PARTIAL_BLOCK);
}
END_TEST

START_TEST(test_token_reverse_readable_type)
{
#define _RTYPE_REV_STR(str) #str
#define _RTYPE_REV_TEST(str) \
        do { \
    		int expected = str; \
    		const char * actual_str = _RTYPE_REV_STR(str); \
    		int actual = handlebars_token_reverse_readable_type(actual_str); \
    		ck_assert_int_eq(expected, actual); \
    	} while(0)
	
	_RTYPE_REV_TEST(BOOLEAN);
	_RTYPE_REV_TEST(CLOSE);
	_RTYPE_REV_TEST(CLOSE_RAW_BLOCK);
	_RTYPE_REV_TEST(CLOSE_SEXPR);
	_RTYPE_REV_TEST(CLOSE_UNESCAPED);
	_RTYPE_REV_TEST(COMMENT);
	_RTYPE_REV_TEST(CONTENT);
	_RTYPE_REV_TEST(DATA);
	_RTYPE_REV_TEST(END);
	_RTYPE_REV_TEST(END_RAW_BLOCK);
	_RTYPE_REV_TEST(EQUALS);
	_RTYPE_REV_TEST(ID);
	_RTYPE_REV_TEST(INVALID);
	_RTYPE_REV_TEST(INVERSE);
	_RTYPE_REV_TEST(NUMBER);
	_RTYPE_REV_TEST(OPEN);
	_RTYPE_REV_TEST(OPEN_BLOCK);
	_RTYPE_REV_TEST(OPEN_ENDBLOCK);
	_RTYPE_REV_TEST(OPEN_INVERSE);
	_RTYPE_REV_TEST(OPEN_PARTIAL);
	_RTYPE_REV_TEST(OPEN_RAW_BLOCK);
	_RTYPE_REV_TEST(OPEN_SEXPR);
	_RTYPE_REV_TEST(OPEN_UNESCAPED);
	_RTYPE_REV_TEST(SEP);
	_RTYPE_REV_TEST(STRING);
	ck_assert_int_eq(-1, handlebars_token_reverse_readable_type("UNKNOWN"));
	
	// Added in v3
	_RTYPE_REV_TEST(CLOSE_BLOCK_PARAMS);
	_RTYPE_REV_TEST(OPEN_BLOCK_PARAMS);
	_RTYPE_REV_TEST(OPEN_INVERSE_CHAIN);
	_RTYPE_REV_TEST(UNDEFINED);
	ck_assert_int_eq(NUL, handlebars_token_reverse_readable_type("NULL"));

	// Added in v4
	_RTYPE_REV_TEST(OPEN_PARTIAL_BLOCK);
}
END_TEST

START_TEST(test_token_print)
    struct handlebars_string * string = handlebars_string_ctor(context, HBS_STRL("{{"));
    struct handlebars_token * tok = handlebars_token_ctor(context, OPEN, string);
    struct handlebars_string * actual = handlebars_token_print(context, tok, 0);
    ck_assert_str_eq("OPEN [{{] ", actual->val);
    handlebars_talloc_free(tok);
    handlebars_talloc_free(actual);
END_TEST

START_TEST(test_token_print2)
    struct handlebars_string * string = handlebars_string_ctor(context, HBS_STRL("this\nis\ra\ttest"));
    struct handlebars_token * tok = handlebars_token_ctor(context, CONTENT, string);
    struct handlebars_string * actual = handlebars_token_print(context, tok, 0);
    ck_assert_str_eq("CONTENT [this\\nis\\ra\\ttest] ", actual->val);
    handlebars_talloc_free(tok);
    handlebars_talloc_free(actual);
END_TEST

START_TEST(test_token_print3)
    struct handlebars_string * string = handlebars_string_ctor(context, HBS_STRL("this\nis\ra\ttest"));
    struct handlebars_token * tok = handlebars_token_ctor(context, CONTENT, string);
    struct handlebars_string * actual = handlebars_token_print(context, tok, handlebars_token_print_flag_newlines);
    ck_assert_str_eq("CONTENT [this\\nis\\ra\\ttest]\n", actual->val);
    handlebars_talloc_free(tok);
    handlebars_talloc_free(actual);
END_TEST

START_TEST(test_token_print_failed_alloc)
#if HANDLEBARS_MEMORY
    struct handlebars_string * string = handlebars_string_ctor(context, HBS_STRL("tok1"));
    struct handlebars_token * tok = handlebars_token_ctor(context, CONTENT, string);
	struct handlebars_string * expected = NULL;
    jmp_buf buf;

    if( !handlebars_setjmp_ex(context, &buf) ) {
        handlebars_token_dtor(tok);
        return;
    }

    handlebars_memory_fail_enable();
    handlebars_token_print(context, tok, 0);
    handlebars_memory_fail_disable();

    ck_assert(0);

#else
    fprintf(stderr, "Skipped, memory testing functions are disabled\n");
#endif
END_TEST

Suite * parser_suite(void)
{
	Suite * s = suite_create("Token");
	
	REGISTER_TEST_FIXTURE(s, test_token_ctor, "Constructor");
	REGISTER_TEST_FIXTURE(s, test_token_ctor_failed_alloc, "Constructor (failed alloc)");
	REGISTER_TEST_FIXTURE(s, test_token_dtor, "Destructor");
	REGISTER_TEST_FIXTURE(s, test_token_get_type, "Get type");
	REGISTER_TEST_FIXTURE(s, test_token_get_text, "Get text");
	REGISTER_TEST_FIXTURE(s, test_token_readable_type, "Readable Type");
	REGISTER_TEST_FIXTURE(s, test_token_reverse_readable_type, "Reverse Readable Type");
	REGISTER_TEST_FIXTURE(s, test_token_print, "Print Token");
	REGISTER_TEST_FIXTURE(s, test_token_print2, "Print Token (2)");
	REGISTER_TEST_FIXTURE(s, test_token_print3, "Print Token (3)");
	REGISTER_TEST_FIXTURE(s, test_token_print_failed_alloc, "Print Token (failed alloc)");

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
