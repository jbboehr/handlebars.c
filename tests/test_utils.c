/**
 * Copyright (C) 2016 John Boehr
 *
 * This file is part of handlebars.c.
 *
 * handlebars.c is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * handlebars.c is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with handlebars.c.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <check.h>
#include <talloc.h>

#include "handlebars.h"
#include "handlebars_memory.h"

#include "handlebars_ast.h"
#include "handlebars_string.h"
#include "handlebars_utils.h"
#include "handlebars.tab.h"
#include "utils.h"



START_TEST(test_yy_error)
{
    struct handlebars_locinfo loc;
    const char * err = "sample error message";
    jmp_buf buf;

    loc.first_line = 1;
    loc.first_column = 2;
    loc.last_line = 3;
    loc.last_column = 4;

    if( !handlebars_setjmp_ex(parser, &buf) ) {
        handlebars_yy_error(&loc, parser, err);
    }

    struct handlebars_locinfo loc2 = handlebars_error_loc(HBSCTX(parser));

    ck_assert_int_eq(handlebars_error_num(HBSCTX(parser)), HANDLEBARS_PARSEERR);
    ck_assert_str_eq(handlebars_error_msg(HBSCTX(parser)), err);
    ck_assert_int_eq(loc2.first_line, loc.first_line);
    ck_assert_int_eq(loc2.first_column, loc.first_column);
    ck_assert_int_eq(loc2.last_line, loc.last_line);
    ck_assert_int_eq(loc2.last_column, loc.last_column);
}
END_TEST

START_TEST(test_yy_fatal_error)
{
    const char * err = "sample error message";
    jmp_buf buf;

    if( handlebars_setjmp_ex(parser, &buf) ) {
        ck_assert_str_eq(err, handlebars_error_msg(parser));
        return;
    }

    handlebars_yy_fatal_error(err, parser);
    ck_assert(0);
}
END_TEST

START_TEST(test_yy_free)
{
#if HANDLEBARS_MEMORY
    char * tmp = handlebars_talloc_strdup(root, "");
    int count;
    
    handlebars_memory_fail_enable();
    handlebars_yy_free(tmp, NULL);
    count = handlebars_memory_get_call_counter();
    handlebars_memory_fail_disable();
    
    ck_assert_int_eq(1, count);
    
    handlebars_talloc_free(tmp);
#else
    fprintf(stderr, "Skipped, memory testing functions are disabled\n");
#endif
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
    char * tmp = handlebars_talloc_strdup(root, "");
    
    tmp = handlebars_yy_realloc(tmp, 10, NULL);
    
    // This should segfault on failure
    tmp[8] = '0';
    
    ck_assert_int_eq(10, talloc_get_size(tmp));
    
    handlebars_talloc_free(tmp);
}
END_TEST

START_TEST(test_yy_realloc_failed_alloc)
{
#if HANDLEBARS_MEMORY
    char * tmp = handlebars_talloc_strdup(root, "");
    char * tmp2;
    
    handlebars_memory_fail_enable();
    tmp2 = handlebars_yy_realloc(tmp, 10, NULL);
    handlebars_memory_fail_disable();
    
    ck_assert_ptr_eq(NULL, tmp2);
    
    // (it's not actually getting freed in the realloc)
    handlebars_talloc_free(tmp);
#else
    fprintf(stderr, "Skipped, memory testing functions are disabled\n");
#endif
}
END_TEST


Suite * parser_suite(void)
{
    Suite * s = suite_create("Utils");

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
