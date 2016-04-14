
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <check.h>
#include <stdio.h>
#include <talloc.h>

#include "handlebars.h"
#include "handlebars_compiler.h"
#include "handlebars_memory.h"
#include "handlebars_value.h"
#include "handlebars_vm.h"
#include "handlebars.tab.h"
#include "handlebars.lex.h"
#include "utils.h"


// @todo try to get this to include every language feature
static const char * tmpl = "{{#if foo}} {{bar}} {{/if}}  {{{blah}}} {{{{raw}}}} "
        "{{#a}}{{^}}{{/a}} {{bar baz=foo}} {{> partial}} {{../depth}} {{this}} "
        "{{#unless}}{{else}}{{/unless}} {{&unescaped}} {{~strip~}}  "
        "{{!-- comment --}} {{! coment }} {{blah (a (b)}}";

START_TEST(test_random_alloc_fail_tokenizer)
{
    size_t i;
    int retval;
    struct handlebars_token ** tokens;

    for( i = 1; i < 300; i++ ) {
        struct handlebars_context * ctx = handlebars_context_ctor_ex(root);
        struct handlebars_parser * parser = handlebars_parser_ctor(ctx);
        parser->tmpl = handlebars_string_ctor(HBSCTX(parser), HBS_STRL(tmpl));

        // For now, don't do yy alloc
        handlebars_memory_fail_set_flags(handlebars_memory_fail_flag_alloc);
        handlebars_memory_fail_counter(i);
        tokens = handlebars_lex(parser);
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

    for( i = 1; i < 300; i++ ) {
        struct handlebars_context * ctx = handlebars_context_ctor_ex(root);
        struct handlebars_parser * parser = handlebars_parser_ctor(ctx);
        parser->tmpl = handlebars_string_ctor(HBSCTX(parser), HBS_STRL(tmpl));

        // For now, don't do yy alloc
        handlebars_memory_fail_set_flags(handlebars_memory_fail_flag_alloc);
        handlebars_memory_fail_counter(i);
        handlebars_parse(parser);
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

        for( i = 1; i < 300; i++ ) {
            struct handlebars_context * ctx = handlebars_context_ctor_ex(root);
            struct handlebars_parser * parser = handlebars_parser_ctor(ctx);
            struct handlebars_compiler * compiler = handlebars_compiler_ctor(ctx);
            parser->tmpl = handlebars_string_ctor(HBSCTX(parser), HBS_STRL(tmpl));
            handlebars_parse(parser);

            // For now, don't do yy alloc
            handlebars_memory_fail_set_flags(handlebars_memory_fail_flag_alloc);
            handlebars_memory_fail_counter(i);
            handlebars_compiler_compile(compiler, parser->program);
            handlebars_memory_fail_disable();

            handlebars_context_dtor(ctx);
        }
    }
END_TEST

START_TEST(test_random_alloc_fail_vm)
    {
        size_t i;
        int retval;

        for( i = 1; i < 300; i++ ) {
            struct handlebars_context * ctx = handlebars_context_ctor_ex(root);
            struct handlebars_parser * parser = handlebars_parser_ctor(ctx);
            struct handlebars_compiler * compiler = handlebars_compiler_ctor(ctx);
            struct handlebars_vm * vm = handlebars_vm_ctor(ctx);
            struct handlebars_value * value = handlebars_value_from_json_string(ctx, "{\"foo\": {\"bar\": 2}}");
            handlebars_value_convert(value);

            parser->tmpl = handlebars_string_ctor(HBSCTX(parser), HBS_STRL(tmpl));
            handlebars_parse(parser);
            handlebars_compiler_compile(compiler, parser->program);

            // For now, don't do yy alloc
            handlebars_memory_fail_set_flags(handlebars_memory_fail_flag_alloc);
            handlebars_memory_fail_counter(i);
            handlebars_vm_execute(vm, compiler->program, value);
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
    REGISTER_TEST_FIXTURE(s, test_random_alloc_fail_vm, "Random Memory Allocation Failures (VM)");
    
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

    talloc_set_log_stderr();
    
#if defined(_WIN64) || defined(_WIN32) || defined(__WIN32__) || defined(__CYGWIN32__)
    iswin = 1;
#endif
    memdebug = getenv("MEMDEBUG") ? atoi(getenv("MEMDEBUG")) : 0;
    
    if( memdebug ) {
        talloc_enable_leak_report_full();
    }
    
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
    if( memdebug ) {
        talloc_report_full(NULL, stderr);
    }
    return error;
}
