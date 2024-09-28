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
#include <string.h>
#include <talloc.h>

#define HANDLEBARS_AST_PRIVATE
#define HANDLEBARS_COMPILER_PRIVATE

#include "handlebars.h"
#include "handlebars_ast.h"
#include "handlebars_ast_list.h"
#include "handlebars_compiler.h"
#include "handlebars_opcodes.h"
#include "handlebars_string.h"
#include "handlebars_memory.h"
#include "utils.h"



START_TEST(test_compiler_ctor)
{
    struct handlebars_compiler * mycompiler;

    mycompiler = handlebars_compiler_ctor(context);

    ck_assert_ptr_ne(NULL, mycompiler);

    handlebars_compiler_dtor(mycompiler);
}
END_TEST

START_TEST(test_compiler_ctor_failed_alloc)
{
#ifdef HANDLEBARS_MEMORY
    jmp_buf buf;
    struct handlebars_compiler * mycompiler;

    context->e->jmp = &buf;
    if( setjmp(buf) ) {
        ck_assert(1);
        return;
    }

    handlebars_memory_fail_enable();
    mycompiler = handlebars_compiler_ctor(context);
    (void) mycompiler;
    handlebars_memory_fail_disable();

    ck_assert(0);
#else
    fprintf(stderr, "Skipped, memory testing functions are disabled\n");
#endif
}
END_TEST

START_TEST(test_compiler_dtor)
{
    struct handlebars_compiler * mycompiler;

    mycompiler = handlebars_compiler_ctor(context);
    handlebars_compiler_dtor(mycompiler);
}
END_TEST

START_TEST(test_compiler_get_flags)
{
    ck_assert_int_eq(0, handlebars_compiler_get_flags(compiler));

    handlebars_compiler_set_flags(compiler, handlebars_compiler_flag_all);

    ck_assert_int_eq(handlebars_compiler_flag_all, handlebars_compiler_get_flags(compiler));
}
END_TEST

START_TEST(test_compiler_set_flags)
{
    // Make sure it changes option flags
    handlebars_compiler_set_flags(compiler, handlebars_compiler_flag_string_params);
    ck_assert_int_eq(handlebars_compiler_flag_string_params, handlebars_compiler_get_flags(compiler));

    handlebars_compiler_set_flags(compiler, handlebars_compiler_flag_track_ids);
    ck_assert_int_eq(handlebars_compiler_flag_track_ids, handlebars_compiler_get_flags(compiler));
}
END_TEST

#ifdef HANDLEBARS_TESTING_EXPORTS
START_TEST(test_compiler_is_known_helper)
{
    struct handlebars_ast_node * id;
    struct handlebars_ast_list * parts;
    struct handlebars_ast_node * path_segment;

    //ck_assert_int_eq(0, handlebars_compiler_is_known_helper(compiler, NULL));

    id = handlebars_ast_node_ctor(HBSCTX(parser), HANDLEBARS_AST_NODE_PATH);
    ck_assert_int_eq(0, handlebars_compiler_is_known_helper(compiler, id));

    id->node.path.parts = parts = handlebars_ast_list_ctor(HBSCTX(parser));
    ck_assert_int_eq(0, handlebars_compiler_is_known_helper(compiler, id));

    path_segment = handlebars_ast_node_ctor(HBSCTX(parser), HANDLEBARS_AST_NODE_PATH);
    handlebars_ast_list_append(parts, path_segment);
    ck_assert_int_eq(0, handlebars_compiler_is_known_helper(compiler, id));

    path_segment->node.path_segment.part = handlebars_string_ctor(HBSCTX(compiler), HBS_STRL("if"));
    ck_assert_int_eq(1, handlebars_compiler_is_known_helper(compiler, id));

    path_segment->node.path_segment.part = handlebars_string_ctor(HBSCTX(compiler), HBS_STRL("unless"));
    ck_assert_int_eq(1, handlebars_compiler_is_known_helper(compiler, id));

    path_segment->node.path_segment.part = handlebars_string_ctor(HBSCTX(compiler), HBS_STRL("foobar"));
    ck_assert_int_eq(0, handlebars_compiler_is_known_helper(compiler, id));

    path_segment->node.path_segment.part = handlebars_string_ctor(HBSCTX(compiler), HBS_STRL(""));
    ck_assert_int_eq(0, handlebars_compiler_is_known_helper(compiler, id));
}
END_TEST

START_TEST(test_compiler_opcode)
{
    struct handlebars_opcode * op1;
    struct handlebars_opcode * op2;
    struct handlebars_program *program;

    op1 = handlebars_opcode_ctor(HBSCTX(compiler), handlebars_opcode_type_append);
    op2 = handlebars_opcode_ctor(HBSCTX(compiler), handlebars_opcode_type_append_escaped);

    handlebars_compiler_opcode(compiler, op1);
    program = handlebars_compiler_get_program(compiler);

    ck_assert_ptr_ne(NULL, program->opcodes);
    ck_assert_int_ne(0, program->opcodes_size);
    ck_assert_int_eq(1, program->opcodes_length);
    ck_assert_ptr_eq(op1, *program->opcodes);

    handlebars_compiler_opcode(compiler, op2);
    program = handlebars_compiler_get_program(compiler);

    ck_assert_ptr_ne(NULL, program->opcodes);
    ck_assert_int_ne(0, program->opcodes_size);
    ck_assert_int_eq(2, program->opcodes_length);
    ck_assert_ptr_eq(op2, *(program->opcodes + 1));
}
END_TEST
#endif

static Suite * suite(void);
static Suite * suite(void)
{
    Suite * s = suite_create("Compiler");

	REGISTER_TEST_FIXTURE(s, test_compiler_ctor, "Constructor");
	REGISTER_TEST_FIXTURE(s, test_compiler_ctor_failed_alloc, "Constructor (failed alloc)");
	REGISTER_TEST_FIXTURE(s, test_compiler_dtor, "Destructor");
	REGISTER_TEST_FIXTURE(s, test_compiler_get_flags, "Get Flags");
	REGISTER_TEST_FIXTURE(s, test_compiler_set_flags, "Set Flags");
#ifdef HANDLEBARS_TESTING_EXPORTS
	REGISTER_TEST_FIXTURE(s, test_compiler_is_known_helper, "Is Known Helper");
	REGISTER_TEST_FIXTURE(s, test_compiler_opcode, "Push opcode");
#endif

    return s;
}

int main(void)
{
    return default_main(&suite);
}
