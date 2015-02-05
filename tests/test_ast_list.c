
#include <check.h>
#include <talloc.h>

#include "handlebars.h"
#include "handlebars_ast.h"
#include "handlebars_ast_list.h"
#include "handlebars_memory.h"
#include "utils.h"

static TALLOC_CTX * ctx;

static void setup(void)
{
    handlebars_memory_fail_disable();
    ctx = talloc_init(NULL);
}

static void teardown(void)
{
    handlebars_memory_fail_disable();
    talloc_free(ctx);
    ctx = NULL;
}

START_TEST(test_ast_list_append)
{
    struct handlebars_ast_list * list = handlebars_ast_list_ctor(ctx);
    struct handlebars_ast_node * node1 = handlebars_talloc(list, struct handlebars_ast_node);
    struct handlebars_ast_node * node2 = handlebars_talloc(list, struct handlebars_ast_node);
    handlebars_ast_list_append(list, node1);
    handlebars_ast_list_append(list, node2);
    
    ck_assert_ptr_eq(list->first->data, node1);
    ck_assert_ptr_eq(list->first->next->data, node2);
    ck_assert_ptr_eq(list->last->data, node2);
    
    handlebars_ast_list_dtor(list);
}
END_TEST

START_TEST(test_ast_list_append_failed_alloc)
{
    struct handlebars_ast_list * list = handlebars_ast_list_ctor(ctx);
    struct handlebars_ast_node * node1 = handlebars_talloc(list, struct handlebars_ast_node);
    int retval;
    
    handlebars_memory_fail_enable();
    retval = handlebars_ast_list_append(list, node1);
    handlebars_memory_fail_disable();
    
    ck_assert_int_eq(retval, HANDLEBARS_NOMEM);
    ck_assert_ptr_eq(list->first, NULL);
    ck_assert_ptr_eq(list->last, NULL);
    
    handlebars_ast_list_dtor(list);
}
END_TEST

START_TEST(test_ast_list_append_null)
{
    handlebars_ast_list_append(NULL, NULL);
}
END_TEST

START_TEST(test_ast_list_ctor)
{
    struct handlebars_ast_list * list = handlebars_ast_list_ctor(ctx);
    
    ck_assert_ptr_ne(list, NULL);
    
    handlebars_ast_list_dtor(list);
}
END_TEST

START_TEST(test_ast_list_ctor_failed_alloc)
{
    struct handlebars_ast_list * list;
    
    handlebars_memory_fail_enable();
    list = handlebars_ast_list_ctor(NULL);
    handlebars_memory_fail_disable();
    
    ck_assert_ptr_eq(list, NULL);
}
END_TEST

START_TEST(test_ast_list_prepend)
{
    struct handlebars_ast_list * list = handlebars_ast_list_ctor(ctx);
    struct handlebars_ast_node * node1 = handlebars_talloc(list, struct handlebars_ast_node);
    struct handlebars_ast_node * node2 = handlebars_talloc(list, struct handlebars_ast_node);
    handlebars_ast_list_prepend(list, node1);
    handlebars_ast_list_prepend(list, node2);
    
    ck_assert_ptr_eq(list->first->data, node2);
    ck_assert_ptr_eq(list->first->next->data, node1);
    ck_assert_ptr_eq(list->last->data, node1);
    
    handlebars_ast_list_dtor(list);
}
END_TEST

START_TEST(test_ast_list_prepend_failed_alloc)
{
    struct handlebars_ast_list * list = handlebars_ast_list_ctor(ctx);
    struct handlebars_ast_node * node1 = handlebars_talloc(list, struct handlebars_ast_node);
    int retval;
    
    handlebars_memory_fail_enable();
    retval = handlebars_ast_list_prepend(list, node1);
    handlebars_memory_fail_disable();
    
    ck_assert_int_eq(retval, HANDLEBARS_NOMEM);
    ck_assert_ptr_eq(list->first, NULL);
    ck_assert_ptr_eq(list->last, NULL);
    
    handlebars_ast_list_dtor(list);
}
END_TEST

START_TEST(test_ast_list_prepend_null)
{
    handlebars_ast_list_prepend(NULL, NULL);
}
END_TEST

START_TEST(test_ast_list_remove_single)
{
    // Only item
    struct handlebars_ast_list * list = handlebars_ast_list_ctor(ctx);
    struct handlebars_ast_node * node1 = handlebars_talloc(list, struct handlebars_ast_node);
    handlebars_ast_list_append(list, node1);
    
    handlebars_ast_list_remove(list, node1);
    
    ck_assert_ptr_eq(list->first, NULL);
    ck_assert_ptr_eq(list->last, NULL);
    
    handlebars_ast_list_dtor(list);
}
END_TEST

START_TEST(test_ast_list_remove_first)
{
    // Not only item
    struct handlebars_ast_list * list = handlebars_ast_list_ctor(ctx);
    struct handlebars_ast_node * node1 = handlebars_talloc(list, struct handlebars_ast_node);
    struct handlebars_ast_node * node2 = handlebars_talloc(list, struct handlebars_ast_node);
    handlebars_ast_list_append(list, node1);
    handlebars_ast_list_append(list, node2);
    
    handlebars_ast_list_remove(list, node1);
    
    ck_assert_ptr_eq(list->first->data, node2);
    ck_assert_ptr_eq(list->last->data, node2);
    
    handlebars_ast_list_dtor(list);
}
END_TEST

START_TEST(test_ast_list_remove_middle)
{
    struct handlebars_ast_list * list = handlebars_ast_list_ctor(ctx);
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
    
    handlebars_ast_list_dtor(list);
}
END_TEST

START_TEST(test_ast_list_remove_last)
{
    struct handlebars_ast_list * list = handlebars_ast_list_ctor(ctx);
    struct handlebars_ast_node * node1 = handlebars_talloc(list, struct handlebars_ast_node);
    struct handlebars_ast_node * node2 = handlebars_talloc(list, struct handlebars_ast_node);
    handlebars_ast_list_append(list, node1);
    handlebars_ast_list_append(list, node2);
    
    handlebars_ast_list_remove(list, node2);
    
    ck_assert_ptr_eq(list->first->data, node1);
    ck_assert_ptr_eq(list->last->data, node1);
    
    handlebars_ast_list_dtor(list);
}
END_TEST

START_TEST(test_ast_list_remove_empty)
{
    struct handlebars_ast_list * list = handlebars_ast_list_ctor(ctx);
    struct handlebars_ast_node * node1 = handlebars_talloc(list, struct handlebars_ast_node);
    
    ck_assert_int_eq(0, handlebars_ast_list_remove(list, node1));
    
    handlebars_ast_list_dtor(list);
}
END_TEST

Suite * parser_suite(void)
{
    Suite * s = suite_create("AST Node List");
    
    REGISTER_TEST_FIXTURE(s, test_ast_list_append, "Append");
    REGISTER_TEST_FIXTURE(s, test_ast_list_append_failed_alloc, "Append with failed alloc");
    REGISTER_TEST_FIXTURE(s, test_ast_list_append_null, "Append with null argument");
    REGISTER_TEST_FIXTURE(s, test_ast_list_ctor, "Constructor");
    REGISTER_TEST_FIXTURE(s, test_ast_list_ctor_failed_alloc, "Constructor with failed alloc");
    REGISTER_TEST_FIXTURE(s, test_ast_list_prepend, "Prepend");
    REGISTER_TEST_FIXTURE(s, test_ast_list_prepend_failed_alloc, "Prepend with failed alloc");
    REGISTER_TEST_FIXTURE(s, test_ast_list_prepend_null, "Prepend with null argument");
    REGISTER_TEST_FIXTURE(s, test_ast_list_remove_single, "Remove (single)");
    REGISTER_TEST_FIXTURE(s, test_ast_list_remove_first, "Remove (first)");
    REGISTER_TEST_FIXTURE(s, test_ast_list_remove_middle, "Remove (middle)");
    REGISTER_TEST_FIXTURE(s, test_ast_list_remove_last, "Remove (last)");
    REGISTER_TEST_FIXTURE(s, test_ast_list_remove_empty, "Remove (empty)");
    
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
