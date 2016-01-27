
#include <check.h>
#include <talloc.h>

#include "handlebars.h"
#include "handlebars_ast.h"
#include "handlebars_ast_helpers.h"
#include "handlebars_context.h"
#include "handlebars_memory.h"
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

START_TEST(test_ast_helper_set_strip_flags)
{
    struct handlebars_ast_node node;
    const char * str1 = "{{";
    const char * str2 = "{{~";
    const char * str3 = "}}";
    const char * str4 = "~}}";
    
    memset(&node, 0, sizeof(struct handlebars_ast_node));
    
    handlebars_ast_helper_set_strip_flags(&node, str1, str3);
    ck_assert_int_eq(1, node.strip);
    
    handlebars_ast_helper_set_strip_flags(&node, str2, str4);
    ck_assert_int_eq(7, node.strip);
    
    handlebars_ast_helper_set_strip_flags(&node, NULL, NULL);
    ck_assert_int_eq(1, node.strip);
}
END_TEST

START_TEST(test_ast_helper_strip_comment)
{
    char * tmp;

    tmp = handlebars_talloc_strdup(ctx, "");
    handlebars_ast_helper_strip_comment(tmp);
    ck_assert_str_eq(tmp, "");

    tmp = handlebars_talloc_strdup(ctx, "blah");
    handlebars_ast_helper_strip_comment(tmp);
    ck_assert_str_eq(tmp, "blah");

    tmp = handlebars_talloc_strdup(ctx, "{");
    handlebars_ast_helper_strip_comment(tmp);
    ck_assert_str_eq(tmp, "{");

    tmp = handlebars_talloc_strdup(ctx, "{{!");
    handlebars_ast_helper_strip_comment(tmp);
    ck_assert_str_eq(tmp, "");

    tmp = handlebars_talloc_strdup(ctx, "{{~!--");
    handlebars_ast_helper_strip_comment(tmp);
    ck_assert_str_eq(tmp, "");

    tmp = handlebars_talloc_strdup(ctx, "{{!-- blah");
    handlebars_ast_helper_strip_comment(tmp);
    ck_assert_str_eq(tmp, " blah");

    tmp = handlebars_talloc_strdup(ctx, "}}");
    handlebars_ast_helper_strip_comment(tmp);
    ck_assert_str_eq(tmp, "");

    tmp = handlebars_talloc_strdup(ctx, "--}}");
    handlebars_ast_helper_strip_comment(tmp);
    ck_assert_str_eq(tmp, "");

    tmp = handlebars_talloc_strdup(ctx, "{{!}}");
    handlebars_ast_helper_strip_comment(tmp);
    ck_assert_str_eq(tmp, "");

    tmp = handlebars_talloc_strdup(ctx, "{{! foo }}");
    handlebars_ast_helper_strip_comment(tmp);
    ck_assert_str_eq(tmp, " foo ");

    tmp = handlebars_talloc_strdup(ctx, "{{!-- bar --}}");
    handlebars_ast_helper_strip_comment(tmp);
    ck_assert_str_eq(tmp, " bar ");

    tmp = handlebars_talloc_strdup(ctx, "{{~!-- baz --~}}");
    handlebars_ast_helper_strip_comment(tmp);
    ck_assert_str_eq(tmp, " baz ");
}
END_TEST

Suite * parser_suite(void)
{
    Suite * s = suite_create("AST Helpers");
    
    REGISTER_TEST_FIXTURE(s, test_ast_helper_set_strip_flags, "Set strip flags");
    REGISTER_TEST_FIXTURE(s, test_ast_helper_strip_comment, "Strip comment");
    
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
