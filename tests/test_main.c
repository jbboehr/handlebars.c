
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
#include "handlebars.lex.h"

#include "utils.h"



START_TEST(test_version)
{
    ck_assert_int_eq(handlebars_version(), HANDLEBARS_VERSION_PATCH
            + (HANDLEBARS_VERSION_MINOR * 100)
            + (HANDLEBARS_VERSION_MAJOR * 10000));
}
END_TEST

START_TEST(test_version_string)
{
    int maj = 0, min = 0, rev = 0;
    const char * version_string = handlebars_version_string();

    sscanf(version_string, "%3d.%3d.%3d", &maj, &min, &rev);
    ck_assert_int_eq(maj, HANDLEBARS_VERSION_MAJOR);
    ck_assert_int_eq(min, HANDLEBARS_VERSION_MINOR);
    ck_assert_int_eq(rev, HANDLEBARS_VERSION_PATCH);
}
END_TEST

START_TEST(test_handlebars_spec_version_string)
{
    ck_assert_ptr_ne(NULL, handlebars_spec_version_string());
}
END_TEST

START_TEST(test_mustache_spec_version_string)
{
    ck_assert_ptr_ne(NULL, handlebars_mustache_spec_version_string());
}
END_TEST

START_TEST(test_lex)
{
    struct handlebars_token ** tokens;
    
    parser->tmpl = handlebars_string_ctor(context, HBS_STRL("{{foo}}"));

    tokens = handlebars_lex(parser);
    
    ck_assert_ptr_ne(NULL, tokens[0]);
    ck_assert_int_eq(OPEN, tokens[0]->token);
    ck_assert_str_eq("{{", tokens[0]->string->val);
    
    ck_assert_ptr_ne(NULL, tokens[1]);
    ck_assert_int_eq(ID, tokens[1]->token);
    ck_assert_str_eq("foo", tokens[1]->string->val);
    
    ck_assert_ptr_ne(NULL, tokens[2]);
    ck_assert_int_eq(CLOSE, tokens[2]->token);
    ck_assert_str_eq("}}", tokens[2]->string->val);

    ck_assert_ptr_eq(NULL, tokens[3]);
}
END_TEST

START_TEST(test_context_ctor_dtor)
{
    struct handlebars_context * context = handlebars_context_ctor();
    ck_assert_ptr_ne(NULL, context);
    handlebars_context_dtor(context);
}
END_TEST

START_TEST(test_context_ctor_failed_alloc)
{
#if HANDLEBARS_MEMORY
    struct handlebars_context * context;

    handlebars_memory_fail_enable();
    context = handlebars_context_ctor();
    handlebars_memory_fail_disable();

    ck_assert_ptr_eq(NULL, context);
#else
    fprintf(stderr, "Skipped, memory testing functions are disabled\n");
#endif
}
END_TEST

START_TEST(test_context_get_errmsg)
{
    struct YYLTYPE loc;
    char * actual;
    loc.last_line = 1;
    loc.last_column = 2;

    context->e->msg = "test";
    context->e->loc = loc;
    actual = handlebars_error_message(context);

    ck_assert_ptr_ne(NULL, actual);
    ck_assert_ptr_ne(NULL, strstr(actual, "test"));
    ck_assert_ptr_ne(NULL, strstr(actual, "line 1"));
    ck_assert_ptr_ne(NULL, strstr(actual, "column 2"));
}
END_TEST

START_TEST(test_context_get_errmsg_failed_alloc)
{
#if HANDLEBARS_MEMORY
    struct YYLTYPE loc;
    char * actual;
    loc.last_line = 1;
    loc.last_column = 2;

    context->e->msg = "test";
    context->e->loc = loc;

    handlebars_memory_fail_enable();
    actual = handlebars_error_message(context);
    handlebars_memory_fail_disable();

    //ck_assert_ptr_eq(NULL, actual);
    ck_assert_ptr_eq(handlebars_error_msg(context), actual);
#else
        fprintf(stderr, "Skipped, memory testing functions are disabled\n");
#endif
}
END_TEST

START_TEST(test_context_get_errmsg_js)
{
    struct YYLTYPE loc;
    char * actual;
    loc.last_line = 1;
    loc.last_column = 2;

    context->e->msg = "test";
    context->e->loc = loc;
    actual = handlebars_error_message_js(context);

    ck_assert_ptr_ne(NULL, actual);
    ck_assert_ptr_ne(NULL, strstr(actual, "test"));
    ck_assert_ptr_ne(NULL, strstr(actual, "line 1"));
    ck_assert_ptr_ne(NULL, strstr(actual, "column 2"));
    ck_assert_ptr_ne(NULL, strstr(actual, "Parse error"));
}
END_TEST

START_TEST(test_context_get_errmsg_js_failed_alloc)
{
#if HANDLEBARS_MEMORY
    struct YYLTYPE loc;
    char * actual;
    loc.last_line = 1;
    loc.last_column = 2;

    context->e->msg = "test";
    context->e->loc = loc;

    handlebars_memory_fail_enable();
    actual = handlebars_error_message_js(context);
    handlebars_memory_fail_disable();

    //ck_assert_ptr_eq(NULL, actual);
    ck_assert_ptr_eq(handlebars_error_msg(context), actual);
#else
        fprintf(stderr, "Skipped, memory testing functions are disabled\n");
#endif
}
END_TEST

Suite * parser_suite(void)
{
    Suite * s = suite_create("Handlebars");

    REGISTER_TEST_FIXTURE(s, test_version, "Version");
    REGISTER_TEST_FIXTURE(s, test_version_string, "Version String");
    REGISTER_TEST_FIXTURE(s, test_handlebars_spec_version_string, "Handlebars Spec Version String");
    REGISTER_TEST_FIXTURE(s, test_mustache_spec_version_string, "Mustache Spec Version String");
    REGISTER_TEST_FIXTURE(s, test_lex, "Lex Convenience Function");
    REGISTER_TEST_FIXTURE(s, test_context_ctor_dtor, "Constructor/Destructor");
    REGISTER_TEST_FIXTURE(s, test_context_ctor_failed_alloc, "Constructor (failed alloc)");
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
