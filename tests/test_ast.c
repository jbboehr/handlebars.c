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
#include "handlebars_compiler.h"
#include "handlebars_memory.h"
#include "handlebars_parser.h"
#include "handlebars_string.h"
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

START_TEST(test_ast_path_segment_owns_strings)
{
    struct handlebars_locinfo loc = {0};
    struct handlebars_string * part = handlebars_string_ctor(HBSCTX(parser), HBS_STRL("[part]"));
    struct handlebars_string * separator = handlebars_string_ctor(HBSCTX(parser), HBS_STRL("/"));
    struct handlebars_ast_node * node = handlebars_ast_node_ctor_path_segment(parser, part, separator, &loc);

    ck_assert_ptr_eq(talloc_parent(node->node.path_segment.original), node);
    ck_assert_ptr_eq(talloc_parent(node->node.path_segment.part), node);
    ck_assert_ptr_eq(talloc_parent(node->node.path_segment.separator), node);
    ck_assert_hbs_str_eq_cstr(node->node.path_segment.part, "part");
}
END_TEST

START_TEST(test_ast_tree_outlives_parser_when_reparented)
{
    struct handlebars_parser * local_parser = handlebars_parser_ctor(context);
    struct handlebars_string * tmpl = handlebars_string_ctor(
        context,
        HBS_STRL("{{#each users as |user index|}}{{user.name}} {{helper (lookup ../map index) key=\"value\"}}{{else}}{{> fallback user}}{{/each}}")
    );
    struct handlebars_ast_node * ast = handlebars_parse_ex(local_parser, tmpl, 0);
    struct handlebars_program * program;

    ck_assert_ptr_nonnull(ast);
    talloc_steal(context, ast);
    handlebars_parser_dtor(local_parser);

    program = handlebars_compiler_compile_ex(compiler, ast);
    ck_assert_ptr_nonnull(program);
}
END_TEST

START_TEST(test_ast_standalone_partial_indent_outlives_parser)
{
    struct handlebars_parser * local_parser = handlebars_parser_ctor(context);
    struct handlebars_string * tmpl = handlebars_string_ctor(context, HBS_STRL("  {{> foo}}\n"));
    struct handlebars_ast_node * ast = handlebars_parse_ex(local_parser, tmpl, 0);
    struct handlebars_ast_list_item * item;
    struct handlebars_ast_node * partial = NULL;
    struct handlebars_program * program;

    ck_assert_ptr_nonnull(ast);
    for( item = ast->node.program.statements->first; item; item = item->next ) {
        if( item->data->type == HANDLEBARS_AST_NODE_PARTIAL ) {
            partial = item->data;
            break;
        }
    }

    ck_assert_ptr_nonnull(partial);
    ck_assert_hbs_str_eq_cstr(partial->node.partial.indent, "  ");
    ck_assert_ptr_eq(talloc_parent(partial->node.partial.indent), partial);

    talloc_steal(context, ast);
    handlebars_parser_dtor(local_parser);

    program = handlebars_compiler_compile_ex(compiler, ast);
    ck_assert_ptr_nonnull(program);
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
    REGISTER_TEST_FIXTURE(s, test_ast_path_segment_owns_strings, "Path segment owns strings");
    REGISTER_TEST_FIXTURE(s, test_ast_tree_outlives_parser_when_reparented, "Reparented tree outlives parser");
    REGISTER_TEST_FIXTURE(s, test_ast_standalone_partial_indent_outlives_parser, "Standalone partial indent outlives parser");
    REGISTER_TEST_FIXTURE(s, test_ast_node_readable_type, "Readable Type");

    return s;
}

int main(void)
{
    return default_main(&suite);
}
