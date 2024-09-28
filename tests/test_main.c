/**
 * Copyright (c) anno Domini nostri Jesu Christi MMXVI-MMXXIV John Boehr & contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <check.h>
#include <talloc.h>

#include "handlebars.h"
#include "handlebars_memory.h"
#include "handlebars_parser.h"
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

    tokens = handlebars_lex_ex(parser, handlebars_string_ctor(context, HBS_STRL("{{foo}}")));

    ck_assert_ptr_ne(NULL, tokens[0]);
    ck_assert_int_eq(OPEN, handlebars_token_get_type(tokens[0]));
    ck_assert_str_eq("{{", hbs_str_val(handlebars_token_get_text(tokens[0])));

    ck_assert_ptr_ne(NULL, tokens[1]);
    ck_assert_int_eq(ID, handlebars_token_get_type(tokens[1]));
    ck_assert_str_eq("foo", hbs_str_val(handlebars_token_get_text(tokens[1])));

    ck_assert_ptr_ne(NULL, tokens[2]);
    ck_assert_int_eq(CLOSE, handlebars_token_get_type(tokens[2]));
    ck_assert_str_eq("}}", hbs_str_val(handlebars_token_get_text(tokens[2])));

    ck_assert_ptr_eq(NULL, tokens[3]);
}
END_TEST

START_TEST(test_context_ctor_dtor)
{
    struct handlebars_context * mycontext = handlebars_context_ctor();
    ck_assert_ptr_ne(NULL, mycontext);
    handlebars_context_dtor(mycontext);
}
END_TEST

START_TEST(test_context_ctor_failed_alloc)
{
#ifdef HANDLEBARS_MEMORY
    struct handlebars_context * mycontext;

    handlebars_memory_fail_enable();
    mycontext = handlebars_context_ctor();
    handlebars_memory_fail_disable();

    ck_assert_ptr_eq(NULL, mycontext);
#else
    fprintf(stderr, "Skipped, memory testing functions are disabled\n");
#endif
}
END_TEST

START_TEST(test_context_ctor_ex_failed_alloc)
{
#ifdef HANDLEBARS_MEMORY
    struct handlebars_context * mycontext;

    handlebars_memory_fail_enable();
    handlebars_memory_fail_counter(2);
    mycontext = handlebars_context_ctor_ex(context);
    handlebars_memory_fail_disable();

    ck_assert_ptr_eq(NULL, mycontext);
#else
    fprintf(stderr, "Skipped, memory testing functions are disabled\n");
#endif
}
END_TEST

START_TEST(test_context_get_errmsg)
{
    struct handlebars_locinfo loc;
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
#ifdef HANDLEBARS_MEMORY
    struct handlebars_locinfo loc;
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
    struct handlebars_locinfo loc;
    char * actual;
    loc.first_line = 1;
    loc.first_column = 2;
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
#ifdef HANDLEBARS_MEMORY
    struct handlebars_locinfo loc;
    char * actual;
    loc.first_line = 1;
    loc.first_column = 2;
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

START_TEST(test_context_bind_failure)
{
    jmp_buf buf;
    struct handlebars_context * mycontext = handlebars_context_ctor();
    struct handlebars_context * mycontext2 = handlebars_context_ctor();

    if( handlebars_setjmp_ex(mycontext, &buf) ) {
        ck_assert(1);
        handlebars_context_dtor(mycontext2);
        handlebars_context_dtor(mycontext);
        return;
    }

    handlebars_context_bind(mycontext, mycontext2);
    handlebars_context_bind(mycontext, mycontext2);
    ck_assert(0);
}
END_TEST

static Suite * suite(void);
static Suite * suite(void)
{
    Suite * s = suite_create("Handlebars");

    REGISTER_TEST_FIXTURE(s, test_version, "Version");
    REGISTER_TEST_FIXTURE(s, test_version_string, "Version String");
    REGISTER_TEST_FIXTURE(s, test_handlebars_spec_version_string, "Handlebars Spec Version String");
    REGISTER_TEST_FIXTURE(s, test_mustache_spec_version_string, "Mustache Spec Version String");
    REGISTER_TEST_FIXTURE(s, test_lex, "Lex Convenience Function");
    REGISTER_TEST_FIXTURE(s, test_context_ctor_dtor, "Constructor/Destructor");
    REGISTER_TEST_FIXTURE(s, test_context_ctor_failed_alloc, "Constructor (failed alloc)");
    REGISTER_TEST_FIXTURE(s, test_context_ctor_ex_failed_alloc, "Constructor ex (failed alloc)");
    REGISTER_TEST_FIXTURE(s, test_context_get_errmsg, "Get error message");
    REGISTER_TEST_FIXTURE(s, test_context_get_errmsg_failed_alloc, "Get error message (failed alloc)");
    REGISTER_TEST_FIXTURE(s, test_context_get_errmsg_js, "Get error message (js compat)");
    REGISTER_TEST_FIXTURE(s, test_context_get_errmsg_js_failed_alloc, "Get error message (js compat) (failed alloc)");
    REGISTER_TEST_FIXTURE(s, test_context_bind_failure, "Get context bind (failed)");

    return s;
}

int main(void)
{
    return default_main(&suite);
}
