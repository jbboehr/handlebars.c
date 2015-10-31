
#include <check.h>
#include <talloc.h>

#include "handlebars.h"
#include "handlebars_ast.h"
#include "handlebars_context.h"
#include "handlebars_memory.h"
#include "handlebars_utils.h"
#include "handlebars.tab.h"
#include "utils.h"

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

START_TEST(test_addcslashes)
{
    const char * what = "";
    const char * input = "";
    const char * expected = "";
    char * actual = handlebars_addcslashes(input, what);
    ck_assert_str_eq(expected, actual);
    handlebars_talloc_free(actual);
}
{
    const char * what = "\r\n\t";
    const char * input = "\ttest\rlines\n";
    const char * expected = "\\ttest\\rlines\\n";
    char * actual = handlebars_addcslashes(input, what);
    ck_assert_str_eq(expected, actual);
    handlebars_talloc_free(actual);
}
{
    const char * what = "abc";
    const char * input = "amazing biscuit circus";
    const char * expected = "\\am\\azing \\bis\\cuit \\cir\\cus";
    char * actual = handlebars_addcslashes(input, what);
    ck_assert_str_eq(expected, actual);
    handlebars_talloc_free(actual);
}
{
    const char * what = "";
    const char * input = "kaboemkara!";
    const char * expected = "kaboemkara!";
    char * actual = handlebars_addcslashes(input, what);
    ck_assert_str_eq(expected, actual);
    handlebars_talloc_free(actual);
}
{
    const char * what = "bar";
    const char * input = "foobarbaz";
    const char * expected = "foo\\b\\a\\r\\b\\az";
    char * actual = handlebars_addcslashes(input, what);
    ck_assert_str_eq(expected, actual);
    handlebars_talloc_free(actual);
}
{
    const char * what = "\a\v\b\f\x3";
    const char * input = "\a\v\b\f\x3";
    const char * expected = "\\a\\v\\b\\f\\003";
    char * actual = handlebars_addcslashes(input, what);
    ck_assert_str_eq(expected, actual);
    handlebars_talloc_free(actual);
}
END_TEST

START_TEST(test_ltrim)
{
    char * in = handlebars_talloc_strdup(ctx, " \n \r test ");
    char * ret = handlebars_ltrim(in, " \t\r\n");
    ck_assert_str_eq(in, "test ");
    ck_assert_ptr_eq(in, ret);
    handlebars_talloc_free(in);
}
{
    char * in = handlebars_talloc_strdup(ctx, "\n  ");
    char * ret = handlebars_ltrim(in, " \t");
    ck_assert_str_eq(in, "\n  ");
    ck_assert_ptr_eq(in, ret);
    handlebars_talloc_free(in);
}
{
    char * in = handlebars_talloc_strdup(ctx, "");
    char * ret = handlebars_ltrim(in, "");
    ck_assert_str_eq(in, "");
    ck_assert_ptr_eq(in, ret);
    handlebars_talloc_free(in);
}
END_TEST

START_TEST(test_rtrim)
{
    char * in = handlebars_talloc_strdup(ctx, "test \n \r ");
    handlebars_rtrim(in, " \t\r\n");
    ck_assert_str_eq(in, "test");
    handlebars_talloc_free(in);
}
{
    char * in = handlebars_talloc_strdup(ctx, "\n");
    handlebars_rtrim(in, " \v\t\r\n");
    ck_assert_str_eq(in, "");
    handlebars_talloc_free(in);
}
{
    char * in = handlebars_talloc_strdup(ctx, "");
    handlebars_rtrim(in, "");
    ck_assert_str_eq(in, "");
    handlebars_talloc_free(in);
}
END_TEST

START_TEST(test_stripcslashes)
{
    char * input = handlebars_talloc_strdup(ctx, "\\n\\r");
    size_t input_len = strlen(input);
    const char * expected = "\n\r";
    handlebars_stripcslashes_ex(input, &input_len);
    ck_assert_str_eq(expected, input);
    handlebars_talloc_free(input);
}
{
    char * input = handlebars_talloc_strdup(ctx, "\\065\\x64");
    size_t input_len = strlen(input);
    const char * expected = "5d";
    handlebars_stripcslashes_ex(input, &input_len);
    ck_assert_str_eq(expected, input);
    handlebars_talloc_free(input);
}
{
    char * input = handlebars_talloc_strdup(ctx, "");
    size_t input_len = strlen(input);
    const char * expected = "";
    handlebars_stripcslashes_ex(input, &input_len);
    ck_assert_str_eq(expected, input);
    handlebars_talloc_free(input);
}
{
    char * input = handlebars_talloc_strdup(ctx, "\\{");
    size_t input_len = strlen(input);
    const char * expected = "{";
    handlebars_stripcslashes_ex(input, &input_len);
    ck_assert_str_eq(expected, input);
    handlebars_talloc_free(input);
}
{
    char * input = handlebars_talloc_strdup(ctx, "\\a\\t\\v\\b\\f\\\\");
    size_t input_len = strlen(input);
    const char * expected = "\a\t\v\b\f\\";
    handlebars_stripcslashes_ex(input, &input_len);
    ck_assert_str_eq(expected, input);
    handlebars_talloc_free(input);
}
{
    char * input = handlebars_talloc_strdup(ctx, "\\x3");
    size_t input_len = strlen(input);
    const char * expected = "\x3";
    handlebars_stripcslashes_ex(input, &input_len);
    ck_assert_str_eq(expected, input);
    handlebars_talloc_free(input);
}
{
    char * input = handlebars_talloc_strdup(ctx, "\\0test");
    size_t input_len = 6;

    handlebars_stripcslashes_ex(input, &input_len);

    ck_assert_int_eq(0, input[0]);
    ck_assert_int_eq('t', input[1]);
    ck_assert_int_eq(0, input[5]);

    handlebars_talloc_free(input);
}
END_TEST

START_TEST(test_handlebars_str_reduce)
{
    char * input = handlebars_talloc_strdup(ctx, "abcdef");
    const char * search = "bcd";
    const char * replace = "qq";
    const char * expected = "aqqef";
    input = handlebars_str_reduce(input, search, replace);
    ck_assert_str_eq(expected, input);
    handlebars_talloc_free(input);
}
{
    char * input = handlebars_talloc_strdup(ctx, "");
    const char * search = "a";
    const char * replace = "";
    const char * expected = "";
    input = handlebars_str_reduce(input, search, replace);
    ck_assert_str_eq(expected, input);
    handlebars_talloc_free(input);
}
/*{
    char * input = handlebars_talloc_strdup(ctx, "");
    const char * search = "asd";
    const char * replace = "asdasd";
    input = handlebars_str_reduce(input, search, replace);
    ck_assert_ptr_eq(NULL, input);
    handlebars_talloc_free(input);
}*/
END_TEST

START_TEST(test_yy_error)
{
    struct handlebars_context * context = handlebars_context_ctor();
    struct YYLTYPE loc;
    const char * err = "sample error message";
    loc.first_line = 1;
    loc.first_column = 2;
    loc.last_line = 3;
    loc.last_column = 4;
    
    handlebars_yy_error(&loc, context, err);
    
    ck_assert_int_eq(context->errnum, HANDLEBARS_PARSEERR);
    ck_assert_str_eq(context->error, err);
    ck_assert_int_eq(context->errloc->first_line, loc.first_line);
    ck_assert_int_eq(context->errloc->first_column, loc.first_column);
    ck_assert_int_eq(context->errloc->last_line, loc.last_line);
    ck_assert_int_eq(context->errloc->last_column, loc.last_column);
    
    handlebars_context_dtor(context);
}
END_TEST

START_TEST(test_yy_fatal_error)
{
    const char * err = "sample error message";
    int exit_code;
    
    handlebars_memory_fail_enable();
    handlebars_yy_fatal_error(err, NULL);
    exit_code = handlebars_memory_get_last_exit_code();
    handlebars_memory_fail_disable();
    
    ck_assert_int_eq(exit_code, 2);
}
END_TEST

START_TEST(test_yy_free)
{
    char * tmp = handlebars_talloc_strdup(ctx, "");
    int count;
    
    handlebars_memory_fail_enable();
    handlebars_yy_free(tmp, NULL);
    count = handlebars_memory_get_call_counter();
    handlebars_memory_fail_disable();
    
    ck_assert_int_eq(1, count);
    
    handlebars_talloc_free(tmp);
}
END_TEST

START_TEST(test_yy_print)
{
    union YYSTYPE t;
    handlebars_yy_print(stderr, 1, t);
}
END_TEST

START_TEST(test_yy_realloc)
{
    char * tmp = handlebars_talloc_strdup(ctx, "");
    
    tmp = handlebars_yy_realloc(tmp, 10, NULL);
    
    // This should segfault on failure
    tmp[8] = '0';
    
    ck_assert_int_eq(10, talloc_get_size(tmp));
    
    handlebars_talloc_free(tmp);
}
END_TEST

START_TEST(test_yy_realloc_failed_alloc)
{
    char * tmp = handlebars_talloc_strdup(ctx, "");
    char * tmp2;
    
    handlebars_memory_fail_enable();
    tmp2 = handlebars_yy_realloc(tmp, 10, NULL);
    handlebars_memory_fail_disable();
    
    ck_assert_ptr_eq(NULL, tmp2);
    
    // (it's not actually getting freed in the realloc)
    handlebars_talloc_free(tmp);
}
END_TEST


Suite * parser_suite(void)
{
    Suite * s = suite_create("Utils");

    REGISTER_TEST_FIXTURE(s, test_addcslashes, "addcslashes");
    REGISTER_TEST_FIXTURE(s, test_ltrim, "ltrim");
    REGISTER_TEST_FIXTURE(s, test_rtrim, "rtrim");
    REGISTER_TEST_FIXTURE(s, test_stripcslashes, "stripcslashes");
    REGISTER_TEST_FIXTURE(s, test_handlebars_str_reduce, "str_reduce");
    REGISTER_TEST_FIXTURE(s, test_yy_error, "yy_error");
    REGISTER_TEST_FIXTURE(s, test_yy_fatal_error, "yy_fatal_error");
    REGISTER_TEST_FIXTURE(s, test_yy_free, "yy_free");
    REGISTER_TEST_FIXTURE(s, test_yy_print, "yy_print");
    REGISTER_TEST_FIXTURE(s, test_yy_realloc, "yy_realloc");
    REGISTER_TEST_FIXTURE(s, test_yy_realloc_failed_alloc, "yy_realloc (failed alloc)");

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
