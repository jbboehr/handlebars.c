
#include <check.h>
#include <string.h>
#include <talloc.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "handlebars.h"
#include "handlebars_ast.h"
#include "handlebars_context.h"
#include "handlebars_memory.h"
#include "handlebars.tab.h"
#include "handlebars.lex.h"
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

START_TEST(test_context_ctor_dtor)
{
    struct handlebars_context * context = handlebars_context_ctor();
    ck_assert_ptr_ne(NULL, context);
    handlebars_context_dtor(context);
}
END_TEST

START_TEST(test_context_ctor_failed_alloc)
{
    struct handlebars_context * context;
    
    handlebars_memory_fail_enable();
    context = handlebars_context_ctor();
    handlebars_memory_fail_disable();
    
    ck_assert_ptr_eq(NULL, context);
}
END_TEST

START_TEST(test_context_ctor_failed_alloc2)
{
    struct handlebars_context * context;
    
    handlebars_memory_fail_counter(2);
    context = handlebars_context_ctor();
    handlebars_memory_fail_disable();
    
    ck_assert_ptr_eq(NULL, context);
}
END_TEST

START_TEST(test_context_get_errmsg)
{
    struct handlebars_context * context = handlebars_context_ctor();
    struct YYLTYPE loc;
    char * actual;
    loc.last_line = 1;
    loc.last_column = 2;
    
    context->error = "test";
    context->errloc = &loc;
    actual = handlebars_context_get_errmsg(context);
    
    ck_assert_ptr_ne(NULL, actual);
    ck_assert_ptr_ne(NULL, strstr(actual, "test"));
    ck_assert_ptr_ne(NULL, strstr(actual, "line 1"));
    ck_assert_ptr_ne(NULL, strstr(actual, "column 2"));
    
    handlebars_context_dtor(context);
}
{
    ck_assert_ptr_eq(NULL, handlebars_context_get_errmsg(NULL));
}
END_TEST

START_TEST(test_context_get_errmsg_failed_alloc)
{
    struct handlebars_context * context = handlebars_context_ctor();
    struct YYLTYPE loc;
    char * actual;
    loc.last_line = 1;
    loc.last_column = 2;
    
    context->error = "test";
    context->errloc = &loc;
    
    handlebars_memory_fail_enable();
    actual = handlebars_context_get_errmsg(context);
    handlebars_memory_fail_disable();
    
    //ck_assert_ptr_eq(NULL, actual);
    ck_assert_ptr_eq(context->error, actual);
    
    handlebars_context_dtor(context);
}
END_TEST

START_TEST(test_context_get_errmsg_js)
{
    struct handlebars_context * context = handlebars_context_ctor();
    struct YYLTYPE loc;
    char * actual;
    loc.last_line = 1;
    loc.last_column = 2;
    
    context->error = "test";
    context->errloc = &loc;
    actual = handlebars_context_get_errmsg_js(context);
    
    ck_assert_ptr_ne(NULL, actual);
    ck_assert_ptr_ne(NULL, strstr(actual, "test"));
    ck_assert_ptr_ne(NULL, strstr(actual, "line 1"));
    ck_assert_ptr_ne(NULL, strstr(actual, "column 2"));
    ck_assert_ptr_ne(NULL, strstr(actual, "Parse error"));
    
    handlebars_context_dtor(context);
}
{
    ck_assert_ptr_eq(NULL, handlebars_context_get_errmsg_js(NULL));
}
END_TEST

START_TEST(test_context_get_errmsg_js_failed_alloc)
{
    struct handlebars_context * context = handlebars_context_ctor();
    struct YYLTYPE loc;
    char * actual;
    loc.last_line = 1;
    loc.last_column = 2;
    
    context->error = "test";
    context->errloc = &loc;
    
    handlebars_memory_fail_enable();
    actual = handlebars_context_get_errmsg_js(context);
    handlebars_memory_fail_disable();
    
    //ck_assert_ptr_eq(NULL, actual);
    ck_assert_ptr_eq(context->error, actual);
    
    handlebars_context_dtor(context);
}
END_TEST

Suite * parser_suite(void)
{
    Suite * s = suite_create("Context");
    REGISTER_TEST_FIXTURE(s, test_context_ctor_dtor, "Constructor/Destructor");
    REGISTER_TEST_FIXTURE(s, test_context_ctor_failed_alloc, "Constructor (failed alloc)");
    REGISTER_TEST_FIXTURE(s, test_context_ctor_failed_alloc2, "Constructor (failed alloc 2)");
    REGISTER_TEST_FIXTURE(s, test_context_get_errmsg, "Get error message");
    REGISTER_TEST_FIXTURE(s, test_context_get_errmsg_failed_alloc, "Get error message (failed alloc)");
    REGISTER_TEST_FIXTURE(s, test_context_get_errmsg_js, "Get error message (js compat)");
    REGISTER_TEST_FIXTURE(s, test_context_get_errmsg_js_failed_alloc, "Get error message (js compat) (failed alloc)");
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
