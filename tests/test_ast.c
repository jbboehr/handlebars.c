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
#include "handlebars_ast_printer.h"
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

typedef struct handlebars_string * (*ast_printer_func)(
    struct handlebars_context *,
    struct handlebars_ast_node *
);

static struct handlebars_string * ast_printer_nested_template(size_t levels)
{
    struct handlebars_string * tmpl = handlebars_string_init(context, levels * 20);

    for( size_t i = 0; i < levels; i++ ) {
        tmpl = handlebars_string_append(context, tmpl, HBS_STRL("{{#if a}}"));
    }
    for( size_t i = 0; i < levels; i++ ) {
        tmpl = handlebars_string_append(context, tmpl, HBS_STRL("{{/if}}"));
    }
    return tmpl;
}

static void assert_ast_printer_depth_limit(ast_printer_func printer_func)
{
    struct handlebars_string * tmpl = ast_printer_nested_template(HANDLEBARS_AST_PRINTER_STACK_SIZE);
    struct handlebars_ast_node * ast = handlebars_parse_ex(parser, tmpl, 0);
    jmp_buf * volatile previous = context->e->jmp;
    const size_t blocks_before = talloc_total_blocks(context);
    jmp_buf buf;

    ck_assert_ptr_nonnull(ast);
    if( handlebars_setjmp_ex(context, &buf) ) {
        context->e->jmp = previous;
        ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_STACK_OVERFLOW);
        ck_assert_str_eq(handlebars_error_msg(context), "AST printer stack overflow");
        ck_assert_uint_le(talloc_total_blocks(context), blocks_before + 1);
        return;
    }

    struct handlebars_string * output = printer_func(context, ast);
    (void) output;
    context->e->jmp = previous;
    ck_abort_msg("Expected deeply nested AST printing to hit the stack limit");
}

START_TEST(test_ast_print_depth_limit)
{
    assert_ast_printer_depth_limit(handlebars_ast_print);
}
END_TEST

START_TEST(test_ast_to_string_depth_limit)
{
    assert_ast_printer_depth_limit(handlebars_ast_to_string);
}
END_TEST

START_TEST(test_ast_to_string_block_parameter_spacing)
{
    struct handlebars_string * tmpl = handlebars_string_ctor(
        context,
        HBS_STRL("{{#each users as |user index|}}{{user}}{{/each}}")
    );
    struct handlebars_ast_node * ast = handlebars_parse_ex(parser, tmpl, 0);

    ck_assert_ptr_nonnull(ast);
    struct handlebars_string * output = handlebars_ast_to_string(context, ast);

    ck_assert_hbs_str_eq_cstr(output, "{{#each users as |user index|}}{{user}}{{/each}}");
}
END_TEST

START_TEST(test_ast_to_string_partial_block_parameter_spacing)
{
    struct handlebars_string * tmpl = handlebars_string_ctor(
        context,
        HBS_STRL("{{#> layout foo bar baz=qux}}x{{/layout}}")
    );
    struct handlebars_ast_node * ast = handlebars_parse_ex(parser, tmpl, 0);

    ck_assert_ptr_nonnull(ast);
    struct handlebars_string * output = handlebars_ast_to_string(context, ast);

    ck_assert_hbs_str_eq_cstr(output, "{{#> layout foo bar baz=qux}}x{{/layout}}");
}
END_TEST

#ifdef HANDLEBARS_MEMORY
static bool ast_printer_fails_at_allocation(
    ast_printer_func printer_func,
    struct handlebars_ast_node * ast,
    int fail_at
)
{
    jmp_buf * volatile previous = context->e->jmp;
    const size_t blocks_before = talloc_total_blocks(context);
    jmp_buf buf;

    if( handlebars_setjmp_ex(context, &buf) ) {
        handlebars_memory_fail_disable();
        context->e->jmp = previous;
        ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_NOMEM);
        ck_assert_uint_le(talloc_total_blocks(context), blocks_before + 1);
        return true;
    }

    handlebars_memory_fail_set_flags(handlebars_memory_fail_flag_alloc);
    handlebars_memory_fail_counter(fail_at);
    struct handlebars_string * output = printer_func(context, ast);
    handlebars_memory_fail_disable();
    context->e->jmp = previous;
    handlebars_talloc_free(output);
    return false;
}

static void assert_ast_printer_allocation_failure_cleanup(ast_printer_func printer_func)
{
    struct handlebars_string * tmpl = handlebars_string_ctor(
        context,
        HBS_STRL("content {{foo \"bar\"}}")
    );
    struct handlebars_ast_node * ast = handlebars_parse_ex(parser, tmpl, 0);
    int fail_at;

    ck_assert_ptr_nonnull(ast);
    for( fail_at = 1; fail_at < 32; fail_at++ ) {
        if( !ast_printer_fails_at_allocation(printer_func, ast, fail_at) ) {
            break;
        }
    }

    ck_assert_int_lt(fail_at, 32);
}

START_TEST(test_ast_printer_allocation_failure_cleanup)
{
    assert_ast_printer_allocation_failure_cleanup(handlebars_ast_print);
    assert_ast_printer_allocation_failure_cleanup(handlebars_ast_to_string);
}
END_TEST
#endif

static Suite * suite(void);
static Suite * suite(void)
{
    Suite * s = suite_create("AST Node");

    REGISTER_TEST_FIXTURE(s, test_ast_node_ctor, "Constructor");
    REGISTER_MEMORY_TEST_FIXTURE(s, test_ast_node_ctor_failed_alloc, "Constructor (failed alloc)");
    REGISTER_TEST_FIXTURE(s, test_ast_node_dtor, "Destructor");
    REGISTER_MEMORY_TEST_FIXTURE(s, test_ast_node_dtor_failed_alloc, "Destructor (failed alloc)");
    REGISTER_TEST_FIXTURE(s, test_ast_path_segment_owns_strings, "Path segment owns strings");
    REGISTER_TEST_FIXTURE(s, test_ast_tree_outlives_parser_when_reparented, "Reparented tree outlives parser");
    REGISTER_TEST_FIXTURE(s, test_ast_standalone_partial_indent_outlives_parser, "Standalone partial indent outlives parser");
    REGISTER_TEST_FIXTURE(s, test_ast_node_readable_type, "Readable Type");
    REGISTER_TEST_FIXTURE(s, test_ast_print_depth_limit, "Printer depth limit");
    REGISTER_TEST_FIXTURE(s, test_ast_to_string_depth_limit, "Source printer depth limit");
    REGISTER_TEST_FIXTURE(s, test_ast_to_string_block_parameter_spacing, "Source block parameter spacing");
    REGISTER_TEST_FIXTURE(s, test_ast_to_string_partial_block_parameter_spacing, "Source partial block parameter spacing");
#ifdef HANDLEBARS_MEMORY
    REGISTER_TEST_FIXTURE(s, test_ast_printer_allocation_failure_cleanup, "Printer allocation failure cleanup");
#endif

    return s;
}

int main(void)
{
    return default_main(&suite);
}
