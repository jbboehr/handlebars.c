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

#define HANDLEBARS_AST_PRIVATE
#define HANDLEBARS_AST_LIST_PRIVATE

#include "handlebars.h"
#include "handlebars_ast.h"
#include "handlebars_ast_list.h"
#include "handlebars_memory.h"
#include "utils.h"



START_TEST(test_ast_list_append)
{
    struct handlebars_ast_list * list = handlebars_ast_list_ctor(HBSCTX(parser));
    struct handlebars_ast_node * node1 = handlebars_talloc(list, struct handlebars_ast_node);
    struct handlebars_ast_node * node2 = handlebars_talloc(list, struct handlebars_ast_node);
    handlebars_ast_list_append(list, node1);
    handlebars_ast_list_append(list, node2);

    ck_assert_ptr_eq(list->first->data, node1);
    ck_assert_ptr_eq(list->first->next->data, node2);
    ck_assert_ptr_eq(list->last->data, node2);
    ck_assert_ptr_eq(list->last->prev->data, node1);
    ck_assert_int_eq(list->count, 2);

    handlebars_ast_list_dtor(list);
}
END_TEST

START_TEST(test_ast_list_append_failed_alloc)
{
#ifdef HANDLEBARS_MEMORY
    struct handlebars_ast_list * list = handlebars_ast_list_ctor(HBSCTX(parser));
    struct handlebars_ast_node * node1 = handlebars_talloc(list, struct handlebars_ast_node);
    jmp_buf buf;

    if( handlebars_setjmp_ex(parser, &buf) ) {
        ck_assert(1);
        handlebars_ast_list_dtor(list);
        return;
    }

    handlebars_memory_fail_enable();
    handlebars_ast_list_append(list, node1);
    handlebars_memory_fail_disable();

    ck_assert(0);
#else
    fprintf(stderr, "Skipped, memory testing functions are disabled\n");
#endif
}
END_TEST

// START_TEST(test_ast_list_append_null)
// {
//     handlebars_ast_list_append(NULL, NULL);
// }
// END_TEST

START_TEST(test_ast_list_ctor)
{
    struct handlebars_ast_list * list = handlebars_ast_list_ctor(HBSCTX(parser));

    ck_assert_ptr_ne(list, NULL);

    handlebars_ast_list_dtor(list);
}
END_TEST

START_TEST(test_ast_list_ctor_failed_alloc)
{
#ifdef HANDLEBARS_MEMORY
    jmp_buf buf;

    if( handlebars_setjmp_ex(parser, &buf) ) {
        ck_assert(1);
        return;
    }

    handlebars_memory_fail_enable();
    struct handlebars_ast_list * list = handlebars_ast_list_ctor(HBSCTX(parser));
    (void) list;
    handlebars_memory_fail_disable();

    ck_assert(0);
#else
    fprintf(stderr, "Skipped, memory testing functions are disabled\n");
#endif
}
END_TEST

START_TEST(test_ast_list_prepend)
{
    struct handlebars_ast_list * list = handlebars_ast_list_ctor(HBSCTX(parser));
    struct handlebars_ast_node * node1 = handlebars_talloc(list, struct handlebars_ast_node);
    struct handlebars_ast_node * node2 = handlebars_talloc(list, struct handlebars_ast_node);
    handlebars_ast_list_prepend(list, node1);
    handlebars_ast_list_prepend(list, node2);

    ck_assert_ptr_eq(list->first->data, node2);
    ck_assert_ptr_eq(list->first->next->data, node1);
    ck_assert_ptr_eq(list->last->data, node1);
    ck_assert_ptr_eq(list->last->prev->data, node2);
    ck_assert_int_eq(list->count, 2);

    handlebars_ast_list_dtor(list);
}
END_TEST

START_TEST(test_ast_list_prepend_failed_alloc)
{
#ifdef HANDLEBARS_MEMORY
    struct handlebars_ast_list * list = handlebars_ast_list_ctor(HBSCTX(parser));
    struct handlebars_ast_node * node1 = handlebars_talloc(list, struct handlebars_ast_node);
    jmp_buf buf;

    if( handlebars_setjmp_ex(parser, &buf) ) {
        ck_assert(1);
        handlebars_ast_list_dtor(list);
        return;
    }

    handlebars_memory_fail_enable();
    handlebars_ast_list_prepend(list, node1);
    handlebars_memory_fail_disable();

    ck_assert(0);
#else
    fprintf(stderr, "Skipped, memory testing functions are disabled\n");
#endif
}
END_TEST

// START_TEST(test_ast_list_prepend_null)
// {
//     handlebars_ast_list_prepend(NULL, NULL);
// }
// END_TEST

START_TEST(test_ast_list_remove_single)
{
    // Only item
    struct handlebars_ast_list * list = handlebars_ast_list_ctor(HBSCTX(parser));
    struct handlebars_ast_node * node1 = handlebars_talloc(list, struct handlebars_ast_node);
    handlebars_ast_list_append(list, node1);

    handlebars_ast_list_remove(list, node1);

    ck_assert_ptr_eq(list->first, NULL);
    ck_assert_ptr_eq(list->last, NULL);
    ck_assert_int_eq(list->count, 0);

    handlebars_ast_list_dtor(list);
}
END_TEST

START_TEST(test_ast_list_remove_first)
{
    // Not only item
    struct handlebars_ast_list * list = handlebars_ast_list_ctor(HBSCTX(parser));
    struct handlebars_ast_node * node1 = handlebars_talloc(list, struct handlebars_ast_node);
    struct handlebars_ast_node * node2 = handlebars_talloc(list, struct handlebars_ast_node);
    handlebars_ast_list_append(list, node1);
    handlebars_ast_list_append(list, node2);

    handlebars_ast_list_remove(list, node1);

    ck_assert_ptr_eq(list->first->data, node2);
    ck_assert_ptr_eq(list->last->data, node2);
    ck_assert_ptr_eq(list->first->prev, NULL);
    ck_assert_ptr_eq(list->last->next, NULL);
    ck_assert_int_eq(list->count, 1);

    handlebars_ast_list_dtor(list);
}
END_TEST

START_TEST(test_ast_list_remove_middle)
{
    struct handlebars_ast_list * list = handlebars_ast_list_ctor(HBSCTX(parser));
    struct handlebars_ast_node * node1 = handlebars_talloc(list, struct handlebars_ast_node);
    struct handlebars_ast_node * node2 = handlebars_talloc(list, struct handlebars_ast_node);
    struct handlebars_ast_node * node3 = handlebars_talloc(list, struct handlebars_ast_node);
    handlebars_ast_list_append(list, node1);
    handlebars_ast_list_append(list, node2);
    handlebars_ast_list_append(list, node3);

    handlebars_ast_list_remove(list, node2);

    ck_assert_ptr_eq(list->first->data, node1);
    ck_assert_ptr_eq(list->last->data, node3);
    ck_assert_ptr_eq(list->first->next->data, node3);
    ck_assert_ptr_eq(list->last->prev->data, node1);
    ck_assert_int_eq(list->count, 2);

    handlebars_ast_list_dtor(list);
}
END_TEST

START_TEST(test_ast_list_remove_last)
{
    struct handlebars_ast_list * list = handlebars_ast_list_ctor(HBSCTX(parser));
    struct handlebars_ast_node * node1 = handlebars_talloc(list, struct handlebars_ast_node);
    struct handlebars_ast_node * node2 = handlebars_talloc(list, struct handlebars_ast_node);
    handlebars_ast_list_append(list, node1);
    handlebars_ast_list_append(list, node2);

    handlebars_ast_list_remove(list, node2);

    ck_assert_ptr_eq(list->first->data, node1);
    ck_assert_ptr_eq(list->last->data, node1);
    ck_assert_ptr_eq(list->first->prev, NULL);
    ck_assert_ptr_eq(list->last->next, NULL);
    ck_assert_int_eq(list->count, 1);

    handlebars_ast_list_dtor(list);
}
END_TEST

START_TEST(test_ast_list_remove_empty)
{
    struct handlebars_ast_list * list = handlebars_ast_list_ctor(HBSCTX(parser));
    struct handlebars_ast_node * node1 = handlebars_talloc(list, struct handlebars_ast_node);

    ck_assert_int_eq(0, handlebars_ast_list_remove(list, node1));

    handlebars_ast_list_dtor(list);
}
END_TEST

static Suite * suite(void);
static Suite * suite(void)
{
    Suite * s = suite_create("AST Node List");

    REGISTER_TEST_FIXTURE(s, test_ast_list_append, "Append");
    REGISTER_TEST_FIXTURE(s, test_ast_list_append_failed_alloc, "Append with failed alloc");
    //REGISTER_TEST_FIXTURE(s, test_ast_list_append_null, "Append with null argument");
    REGISTER_TEST_FIXTURE(s, test_ast_list_ctor, "Constructor");
    REGISTER_TEST_FIXTURE(s, test_ast_list_ctor_failed_alloc, "Constructor with failed alloc");
    REGISTER_TEST_FIXTURE(s, test_ast_list_prepend, "Prepend");
    REGISTER_TEST_FIXTURE(s, test_ast_list_prepend_failed_alloc, "Prepend with failed alloc");
    //REGISTER_TEST_FIXTURE(s, test_ast_list_prepend_null, "Prepend with null argument");
    REGISTER_TEST_FIXTURE(s, test_ast_list_remove_single, "Remove (single)");
    REGISTER_TEST_FIXTURE(s, test_ast_list_remove_first, "Remove (first)");
    REGISTER_TEST_FIXTURE(s, test_ast_list_remove_middle, "Remove (middle)");
    REGISTER_TEST_FIXTURE(s, test_ast_list_remove_last, "Remove (last)");
    REGISTER_TEST_FIXTURE(s, test_ast_list_remove_empty, "Remove (empty)");

    return s;
}

int main(void)
{
    return default_main(&suite);
}
