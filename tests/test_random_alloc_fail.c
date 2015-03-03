
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <check.h>
#include <stdio.h>
#include <talloc.h>

#include "handlebars.h"
#include "handlebars_compiler.h"
#include "handlebars_context.h"
#include "handlebars_memory.h"
#include "handlebars.tab.h"
#include "handlebars.lex.h"
#include "utils.h"

static TALLOC_CTX * rootctx;

// @todo try to get this to include every language feature
static const char * tmpl = "{{#if foo}} {{bar}} {{/if}}  {{{blah}}} {{{{raw}}}} "
        "{{#a}}{{^}}{{/a}} {{bar baz=foo}} {{> partial}} {{../depth}} {{this}} "
        "{{#unless}}{{else}}{{/unless}} {{&unescaped}} {{~strip~}} ";

static void setup(void)
{
    handlebars_memory_fail_disable();
}

static void teardown(void)
{
    handlebars_memory_fail_disable();
}

START_TEST(test_random_alloc_fail_tokenizer)
{
    size_t i;
    int retval;
    struct handlebars_token_list * list;

    for( i = 1; i < 200; i++ ) {
        struct handlebars_context * ctx = handlebars_context_ctor();
        talloc_steal(rootctx, ctx);
        ctx->tmpl = tmpl;

        // For now, don't do yy alloc
        handlebars_memory_fail_set_flags(handlebars_memory_fail_flag_alloc);
        handlebars_memory_fail_counter(i);
        list = handlebars_lex(ctx);
        handlebars_memory_fail_disable();

        handlebars_context_dtor(ctx);
        ctx = NULL;
    }
}
END_TEST

START_TEST(test_random_alloc_fail_parser)
{
    size_t i;
    int retval;

    for( i = 1; i < 200; i++ ) {
        struct handlebars_context * ctx = handlebars_context_ctor();
        talloc_steal(rootctx, ctx);
        ctx->tmpl = tmpl;

        // For now, don't do yy alloc
        handlebars_memory_fail_set_flags(handlebars_memory_fail_flag_alloc);
        handlebars_memory_fail_counter(i);
        retval = handlebars_yy_parse(ctx);
        handlebars_memory_fail_disable();

        handlebars_context_dtor(ctx);
        ctx = NULL;
    }
}
END_TEST

START_TEST(test_random_alloc_fail_compiler)
{
    size_t i;
    int retval;
    
    for( i = 1; i < 200; i++ ) {
        struct handlebars_context * ctx = handlebars_context_ctor();
        struct handlebars_compiler * compiler = handlebars_compiler_ctor(ctx);
        talloc_steal(rootctx, ctx);
        ctx->tmpl = tmpl;
        retval = handlebars_yy_parse(ctx);

        // For now, don't do yy alloc
        handlebars_memory_fail_set_flags(handlebars_memory_fail_flag_alloc);
        handlebars_memory_fail_counter(i);
        handlebars_compiler_compile(compiler, ctx->program);
        handlebars_memory_fail_disable();

        handlebars_context_dtor(ctx);
    }
}
END_TEST

Suite * parser_suite(void)
{
    Suite * s = suite_create("Random Memory Allocation Failures");

    REGISTER_TEST_FIXTURE(s, test_random_alloc_fail_tokenizer, "Random Memory Allocation Failures (Tokenizer)");
    REGISTER_TEST_FIXTURE(s, test_random_alloc_fail_parser, "Random Memory Allocation Failures (Parser)");
    REGISTER_TEST_FIXTURE(s, test_random_alloc_fail_compiler, "Random Memory Allocation Failures (Compiler)");
    
    return s;
}

int main(void)
{
    int number_failed;
    Suite * s;
    SRunner * sr;
    int memdebug = 0;
    int iswin = 0;
    int error = 0;
    
#if defined(_WIN64) || defined(_WIN32) || defined(__WIN32__) || defined(__CYGWIN32__)
    iswin = 1;
#endif
    memdebug = getenv("MEMDEBUG") ? atoi(getenv("MEMDEBUG")) : 0;
    
    if( memdebug ) {
        talloc_enable_leak_report_full();
    }
    rootctx = talloc_new(NULL);
    
    s = parser_suite();
    sr = srunner_create(s);
    if( iswin || memdebug ) {
        srunner_set_fork_status(sr, CK_NOFORK);
    }
    srunner_run_all(sr, CK_ENV);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    error = (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
    
error:
    talloc_free(rootctx);
    if( memdebug ) {
        talloc_report_full(NULL, stderr);
    }
    return error;
}
