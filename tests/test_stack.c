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
#include <setjmp.h>
#include <stdio.h>
#include <talloc.h>

#include "handlebars_memory.h"
#include "handlebars_stack.h"
#include "handlebars_string.h"
#include "handlebars_value.h"
#include "utils.h"


START_TEST(test_stack_copy_ctor)
{
    struct handlebars_stack * stack;
    struct handlebars_stack * stack_copy;
    HANDLEBARS_VALUE_DECL(tmp);

    stack = handlebars_stack_ctor(context, 3);

    handlebars_value_integer(tmp, 1);
    stack = handlebars_stack_push(stack, tmp);

    handlebars_value_integer(tmp, 2);
    stack = handlebars_stack_push(stack, tmp);

    handlebars_value_integer(tmp, 3);
    stack = handlebars_stack_push(stack, tmp);

    ck_assert_uint_eq(handlebars_stack_count(stack), 3);

    stack_copy = handlebars_stack_copy_ctor(stack, 0);

    ck_assert_ptr_ne(stack_copy, NULL);
    ck_assert_ptr_ne(stack, stack_copy);
    ck_assert_uint_eq(handlebars_stack_count(stack_copy), 3);
    ck_assert_int_eq(handlebars_value_get_intval(handlebars_stack_get(stack, 0)), handlebars_value_get_intval(handlebars_stack_get(stack_copy, 0)));
    ck_assert_int_eq(handlebars_value_get_intval(handlebars_stack_get(stack, 1)), handlebars_value_get_intval(handlebars_stack_get(stack_copy, 1)));
    ck_assert_int_eq(handlebars_value_get_intval(handlebars_stack_get(stack, 2)), handlebars_value_get_intval(handlebars_stack_get(stack_copy, 2)));

    handlebars_stack_delref(stack);
    handlebars_stack_delref(stack_copy);
    HANDLEBARS_VALUE_UNDECL(tmp);

    ASSERT_INIT_BLOCKS();
}
END_TEST

START_TEST(test_stack_push_preserves_aliased_value_during_resize)
{
    struct handlebars_stack * stack = handlebars_stack_ctor(context, 1);
    struct handlebars_value * source;
    HANDLEBARS_VALUE_DECL(tmp);

    handlebars_value_str(tmp, handlebars_string_ctor(context, HBS_STRL("source")));
    stack = handlebars_stack_push(stack, tmp);
    source = handlebars_stack_get(stack, 0);
    ck_assert_ptr_nonnull(source);

    stack = handlebars_stack_push(stack, source);

    ck_assert_uint_eq(handlebars_stack_count(stack), 2);
    ck_assert_hbs_str_eq_cstr(handlebars_value_get_string(handlebars_stack_get(stack, 0)), "source");
    ck_assert_hbs_str_eq_cstr(handlebars_value_get_string(handlebars_stack_get(stack, 1)), "source");

    HANDLEBARS_VALUE_UNDECL(tmp);
    handlebars_stack_delref(stack);
    ASSERT_INIT_BLOCKS();
}
END_TEST

#ifndef HANDLEBARS_NO_REFCOUNT
START_TEST(test_stack_push_with_separation)
{
    struct handlebars_stack * stack;
    struct handlebars_stack * stack_copy;
    HANDLEBARS_VALUE_DECL(tmp);

    stack = handlebars_stack_ctor(context, 1);
    handlebars_stack_addref(stack);

    handlebars_value_integer(tmp, 1);
    stack = handlebars_stack_push(stack, tmp);

    stack_copy = stack;
    handlebars_stack_addref(stack);

    handlebars_value_integer(tmp, 2);
    stack = handlebars_stack_push(stack, tmp);

    ck_assert_ptr_ne(stack_copy, NULL);
    ck_assert_ptr_ne(stack, stack_copy);
    ck_assert_uint_eq(handlebars_stack_count(stack), 2);
    ck_assert_uint_eq(handlebars_stack_count(stack_copy), 1);

    ck_assert_int_eq(handlebars_value_get_intval(handlebars_stack_get(stack, 0)), handlebars_value_get_intval(handlebars_stack_get(stack_copy, 0)));
    ck_assert_int_eq(handlebars_value_get_intval(handlebars_stack_get(stack, 1)), 2);

    HANDLEBARS_VALUE_UNDECL(tmp);
    handlebars_stack_delref(stack);
    handlebars_stack_delref(stack_copy);

    ASSERT_INIT_BLOCKS();
}
END_TEST
#endif

START_TEST(test_stack_pop_protected_boundary)
{
    struct handlebars_stack * stack = handlebars_stack_ctor(context, 1);
    HANDLEBARS_VALUE_DECL(tmp);
    handlebars_value_integer(tmp, 1);
    stack = handlebars_stack_push(stack, tmp);
    handlebars_stack_protect(stack, 1);
    jmp_buf buf;

    if( handlebars_setjmp_ex(context, &buf) ) {
        ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_STACK_OVERFLOW);
        handlebars_stack_delref(stack);
        HANDLEBARS_VALUE_UNDECL(tmp);
        return;
    }

    (void) handlebars_stack_pop(stack, tmp);
    ck_abort_msg("Expected pop at the protected boundary to fail");
}
END_TEST

static Suite * suite(void);
static Suite * suite(void)
{
    Suite * s = suite_create("Stack");

    REGISTER_TEST_FIXTURE(s, test_stack_copy_ctor, "Stack copy constructor");
    REGISTER_TEST_FIXTURE(s, test_stack_push_preserves_aliased_value_during_resize, "Stack push preserves aliased values during resize");
#ifndef HANDLEBARS_NO_REFCOUNT
    REGISTER_TEST_FIXTURE(s, test_stack_push_with_separation, "Stack push with separation");
#endif
    REGISTER_TEST_FIXTURE(s, test_stack_pop_protected_boundary, "Stack protected pop boundary");

    return s;
}

int main(void)
{
    return default_main(&suite);
}
