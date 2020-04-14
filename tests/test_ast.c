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
#include "handlebars_ast.h"
#include "handlebars_memory.h"
#include "handlebars.tab.h"
#include "utils.h"



START_TEST(test_ast_node_ctor)
{
    struct handlebars_ast_node * node = handlebars_ast_node_ctor(HBSCTX(parser), HANDLEBARS_AST_NODE_PROGRAM);

    ck_assert_ptr_ne(NULL, node);
    ck_assert_int_eq(HANDLEBARS_AST_NODE_PROGRAM, node->type);

    handlebars_talloc_free(node);
}
END_TEST

START_TEST(test_ast_node_ctor_failed_alloc)
{
#if HANDLEBARS_MEMORY
    jmp_buf buf;

    context->e->jmp = &buf;
    if( setjmp(buf) ) {
        // Should get here
        ck_assert(1);
        return;
    }

    handlebars_memory_fail_enable();
    handlebars_ast_node_ctor(HBSCTX(parser), HANDLEBARS_AST_NODE_PROGRAM);
    handlebars_memory_fail_disable();

    ck_assert(0);
#else
    fprintf(stderr, "Skipped, memory testing functions are disabled\n");
#endif
}
END_TEST

START_TEST(test_ast_node_dtor)
{
    struct handlebars_ast_node * node = handlebars_ast_node_ctor(HBSCTX(parser), HANDLEBARS_AST_NODE_PROGRAM);
    handlebars_ast_node_dtor(node);
}
END_TEST

START_TEST(test_ast_node_dtor_failed_alloc)
{
#if HANDLEBARS_MEMORY
    struct handlebars_ast_node * node = handlebars_ast_node_ctor(HBSCTX(parser), HANDLEBARS_AST_NODE_PROGRAM);
    int call_count;

    handlebars_memory_fail_enable();
    handlebars_ast_node_dtor(node);
    call_count = handlebars_memory_get_call_counter();
    handlebars_memory_fail_disable();

    ck_assert_int_eq(1, call_count);

    handlebars_talloc_free(node);
#else
    fprintf(stderr, "Skipped, memory testing functions are disabled\n");
#endif
}
END_TEST

START_TEST(test_ast_node_readable_type)
{
#define _RTYPE_STR(str) #str
#define _RTYPE_MK(str) HANDLEBARS_AST_NODE_ ## str
#define _RTYPE_TEST(str, name) \
        do { \
			const char * expected = _RTYPE_STR(name); \
			const char * actual = handlebars_ast_node_readable_type(_RTYPE_MK(str)); \
			ck_assert_str_eq(expected, actual); \
		} while(0)

	_RTYPE_TEST(NIL, NIL);
    _RTYPE_TEST(BLOCK, block);
    _RTYPE_TEST(BOOLEAN, BooleanLiteral);
    _RTYPE_TEST(COMMENT, comment);
    _RTYPE_TEST(CONTENT, content);
    _RTYPE_TEST(HASH, hash);
    _RTYPE_TEST(HASH_PAIR, HASH_PAIR);
    _RTYPE_TEST(INTERMEDIATE, INTERMEDIATE);
    _RTYPE_TEST(INVERSE, INVERSE);
    _RTYPE_TEST(MUSTACHE, mustache);
    _RTYPE_TEST(NUMBER, NumberLiteral);
    _RTYPE_TEST(PARTIAL, partial);
    _RTYPE_TEST(PARTIAL_BLOCK, PartialBlockStatement);
    _RTYPE_TEST(PATH, PathExpression);
    _RTYPE_TEST(PATH_SEGMENT, PATH_SEGMENT);
	_RTYPE_TEST(PROGRAM, program);
    _RTYPE_TEST(RAW_BLOCK, raw_block);
    _RTYPE_TEST(SEXPR, SubExpression);
    _RTYPE_TEST(STRING, StringLiteral);
    _RTYPE_TEST(UNDEFINED, UNDEFINED);
    ck_assert_str_eq("NULL", handlebars_ast_node_readable_type(HANDLEBARS_AST_NODE_NUL));
    ck_assert_str_eq("UNKNOWN", handlebars_ast_node_readable_type(-1));
}
END_TEST


Suite * parser_suite(void)
{
    Suite * s = suite_create("AST Node");

    REGISTER_TEST_FIXTURE(s, test_ast_node_ctor, "Constructor");
    REGISTER_TEST_FIXTURE(s, test_ast_node_ctor_failed_alloc, "Constructor (failed alloc)");
    REGISTER_TEST_FIXTURE(s, test_ast_node_dtor, "Destructor");
    REGISTER_TEST_FIXTURE(s, test_ast_node_dtor_failed_alloc, "Destructor (failed alloc)");
    REGISTER_TEST_FIXTURE(s, test_ast_node_readable_type, "Readable Type");

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
