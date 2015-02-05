
#include <check.h>
#include <talloc.h>

#include "handlebars.h"
#include "handlebars_memory.h"
#include "handlebars_token.h"
#include "handlebars_token_list.h"
#include "handlebars_token_printer.h"
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

START_TEST(test_token_print)
{
	struct handlebars_token * tok = talloc(NULL, struct handlebars_token);
	tok->token = OPEN;
	tok->text = "{{";
	tok->length = strlen(tok->text);

	char * actual = handlebars_token_print(tok, 0);
	char * expected = "OPEN [{{] ";
	ck_assert_str_eq(expected, actual);

	talloc_free(tok);
}
END_TEST

START_TEST(test_token_print2)
{
	struct handlebars_token * tok = talloc(NULL, struct handlebars_token);
	tok->token = CONTENT;
	tok->text = "this\nis\ra\ttest";
	tok->length = strlen(tok->text);

	char * actual = handlebars_token_print(tok, 0);
	char * expected = "CONTENT [this\\nis\\ra\\ttest] ";
	ck_assert_str_eq(expected, actual);

	talloc_free(tok);
}
END_TEST

START_TEST(test_token_print3)
{
	struct handlebars_token * tok = talloc(NULL, struct handlebars_token);
	tok->token = CONTENT;
	tok->text = "this\nis\ra\ttest";
	tok->length = strlen(tok->text);

	char * actual = handlebars_token_print(tok, handlebars_token_printer_flag_newlines);
	char * expected = "CONTENT [this\\nis\\ra\\ttest]\n";
	ck_assert_str_eq(expected, actual);

	talloc_free(tok);
}
END_TEST

START_TEST(test_token_print_failed_alloc)
{
    struct handlebars_token * tok = handlebars_token_ctor(CONTENT, "tok1", strlen("tok1"), ctx);
	char * expected;
    
    handlebars_memory_fail_enable();
    expected = handlebars_token_print(tok, 0);
    handlebars_memory_fail_disable();
    
    ck_assert_ptr_eq(NULL, expected);
    
	talloc_free(tok);
}
END_TEST

START_TEST(test_token_print_null_arg)
{
    handlebars_token_print(NULL, 0);
}
END_TEST

START_TEST(test_token_list_print)
{
    struct handlebars_token_list * list = handlebars_token_list_ctor(ctx);
    struct handlebars_token * token1 = handlebars_token_ctor(CONTENT, "tok1", strlen("tok1"), ctx);
    struct handlebars_token * token2 = handlebars_token_ctor(CONTENT, "tok2", strlen("tok1"), ctx);
    
    handlebars_token_list_append(list, token1);
    handlebars_token_list_append(list, token2);
    
	char * actual = handlebars_token_list_print(list, 0);
	char * expected = "CONTENT [tok1] CONTENT [tok2]";
	ck_assert_str_eq(expected, actual);
}
END_TEST

START_TEST(test_token_list_print_null_item)
{
    struct handlebars_token_list * list = handlebars_token_list_ctor(ctx);
    struct handlebars_token * token1 = handlebars_token_ctor(CONTENT, "tok1", strlen("tok1"), ctx);
    struct handlebars_token * token2 = handlebars_token_ctor(CONTENT, "tok2", strlen("tok1"), ctx);
    
    handlebars_token_list_append(list, token1);
    handlebars_token_list_append(list, token2);
    list->last->data = NULL;
    
	char * actual = handlebars_token_list_print(list, 0);
	char * expected = "CONTENT [tok1]";
	ck_assert_str_eq(expected, actual);
}
END_TEST

Suite * parser_suite(void)
{
	Suite * s = suite_create("Token Printer");
	
	REGISTER_TEST_FIXTURE(s, test_token_print, "Print Token");
	REGISTER_TEST_FIXTURE(s, test_token_print2, "Print Token (2)");
	REGISTER_TEST_FIXTURE(s, test_token_print3, "Print Token (3)");
	REGISTER_TEST_FIXTURE(s, test_token_print_failed_alloc, "Print Token (failed alloc)");
	REGISTER_TEST_FIXTURE(s, test_token_print_null_arg, "Print Token (null arg)");
	REGISTER_TEST_FIXTURE(s, test_token_list_print, "Print Token List");
	REGISTER_TEST_FIXTURE(s, test_token_list_print_null_item, "Print Token List (null item)");
	
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
