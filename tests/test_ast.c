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
#ifdef HANDLEBARS_MEMORY
    jmp_buf buf;

    context->e->jmp = &buf;
    if( setjmp(buf) ) {
        // Should get here
        ck_assert(1);
        return;
    }

    handlebars_memory_fail_enable();
    struct handlebars_ast_node * ast = handlebars_ast_node_ctor(HBSCTX(parser), HANDLEBARS_AST_NODE_PROGRAM);
    (void) ast;
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
#ifdef HANDLEBARS_MEMORY
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

static Suite * suite(void);
static Suite * suite(void)
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
    return default_main(&suite);
}
