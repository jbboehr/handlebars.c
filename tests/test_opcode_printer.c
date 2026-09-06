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

#define HANDLEBARS_OPCODES_PRIVATE
#define HANDLEBARS_COMPILER_PRIVATE

#include "handlebars.h"
#include "handlebars_compiler.h"
#include "handlebars_memory.h"
#include "handlebars_opcode_printer.h"
#include "handlebars_opcodes.h"
#include "handlebars_string.h"
#include "utils.h"



START_TEST(test_operand_print_append_null)
{
    struct handlebars_operand op;
    struct handlebars_string * string;
    handlebars_operand_set_null(&op);
    string = handlebars_operand_print(context, &op);
    ck_assert_ptr_ne(NULL, string);
    ck_assert_str_eq("[NULL]", hbs_str_val(string));
    handlebars_talloc_free(string);
}
END_TEST

START_TEST(test_operand_print_append_boolean)
{
    struct handlebars_operand op;
    struct handlebars_string * string;
    handlebars_operand_set_boolval(&op, 1);
    string = handlebars_operand_print(context, &op);
    ck_assert_ptr_ne(NULL, string);
    ck_assert_str_eq("[BOOLEAN:1]", hbs_str_val(string));
    handlebars_talloc_free(string);
}
END_TEST

START_TEST(test_operand_print_append_long)
{
    struct handlebars_operand op;
    struct handlebars_string * string;
    handlebars_operand_set_longval(&op, 2358);
    string = handlebars_operand_print(context, &op);
    ck_assert_ptr_ne(NULL, string);
    ck_assert_str_eq("[LONG:2358]", hbs_str_val(string));
    handlebars_talloc_free(string);
}
END_TEST

START_TEST(test_operand_print_append_string)
{
    struct handlebars_operand op;
    struct handlebars_string * string;
    struct handlebars_opcode * opcode = handlebars_opcode_ctor(context, handlebars_opcode_type_nil);
    handlebars_operand_set_stringval(context, opcode, &op, handlebars_string_ctor(context, HBS_STRL("baz")));
    string = handlebars_operand_print(context, &op);
    ck_assert_ptr_ne(NULL, string);
    ck_assert_str_eq("[STRING:baz]", hbs_str_val(string));
    handlebars_talloc_free(string);
}
END_TEST

START_TEST(test_operand_print_append_array)
{
    // @todo
}
END_TEST

START_TEST(test_opcode_print_1)
{
    struct handlebars_opcode * opcode = handlebars_opcode_ctor(context, handlebars_opcode_type_ambiguous_block_value);
    const char * expected = "ambiguousBlockValue";
    struct handlebars_string * string = handlebars_opcode_print(context, opcode, 0);
    ck_assert_str_eq(expected, hbs_str_val(string));
    handlebars_talloc_free(opcode);
    handlebars_talloc_free(string);
}
END_TEST

START_TEST(test_opcode_print_2)
{
    struct handlebars_opcode * opcode = handlebars_opcode_ctor(context, handlebars_opcode_type_get_context);
    const char * expected = "getContext[LONG:2358]";
    struct handlebars_string * string;
    handlebars_operand_set_longval(&opcode->op1, 2358);
    string = handlebars_opcode_print(context, opcode, 0);
    ck_assert_str_eq(expected, hbs_str_val(string));
    handlebars_talloc_free(opcode);
    handlebars_talloc_free(string);
}
END_TEST

START_TEST(test_opcode_print_3)
{
    struct handlebars_opcode * opcode = handlebars_opcode_ctor(context, handlebars_opcode_type_invoke_helper);
    const char * expected = "invokeHelper[LONG:123][STRING:baz][LONG:456]";
    struct handlebars_string * string;

    handlebars_operand_set_longval(&opcode->op1, 123);
    handlebars_operand_set_stringval(context, opcode, &opcode->op2, handlebars_string_ctor(context, HBS_STRL("baz")));
    handlebars_operand_set_longval(&opcode->op3, 456);

    string = handlebars_opcode_print(context, opcode, 0);
    ck_assert_str_eq(expected, hbs_str_val(string));
    handlebars_talloc_free(opcode);
    handlebars_talloc_free(string);
}
END_TEST

START_TEST(test_opcode_print_4)
{
    struct handlebars_opcode * opcode = handlebars_opcode_ctor(context, handlebars_opcode_type_lookup_on_context);
    const char * expected = "lookupOnContext[LONG:123][STRING:baz][LONG:456][STRING:bat]";
    struct handlebars_string * string;

    handlebars_operand_set_longval(&opcode->op1, 123);
    handlebars_operand_set_stringval(context, opcode, &opcode->op2, handlebars_string_ctor(context, HBS_STRL("baz")));
    handlebars_operand_set_longval(&opcode->op3, 456);
    handlebars_operand_set_stringval(context, opcode, &opcode->op4, handlebars_string_ctor(context, HBS_STRL("bat")));

    string = handlebars_opcode_print(context, opcode, 0);
    ck_assert_str_eq(expected, hbs_str_val(string));
    handlebars_talloc_free(opcode);
    handlebars_talloc_free(string);
}
END_TEST

START_TEST(test_opcode_print_register_decorator)
{
    struct handlebars_opcode * opcode = handlebars_opcode_ctor(context, handlebars_opcode_type_register_decorator);
    struct handlebars_string * string;
    const char * expected[] = {
        "registerDecorator[LONG:1][STRING:inline]",
        "registerDecorator[LONG:1][STRING:inline][BOOLEAN:0]",
        "registerDecorator[LONG:1][STRING:inline][BOOLEAN:1]"
    };

    ck_assert_int_eq(handlebars_opcode_num_operands(opcode->type), 3);
    handlebars_operand_set_longval(&opcode->op1, 1);
    handlebars_operand_set_stringval(context, opcode, &opcode->op2, handlebars_string_ctor(context, HBS_STRL("inline")));
    for( size_t i = 0; i < sizeof(expected) / sizeof(expected[0]); i++ ) {
        if( i != 0 ) {
            handlebars_operand_set_boolval(&opcode->op3, i == 2);
        }
        string = handlebars_opcode_print(context, opcode, 0);
        ck_assert_str_eq(hbs_str_val(string), expected[i]);
        handlebars_talloc_free(string);
    }
    handlebars_talloc_free(opcode);
}
END_TEST

START_TEST(test_program_print_separators)
{
    struct handlebars_opcode * root_opcodes[] = {
        handlebars_opcode_ctor(context, handlebars_opcode_type_append_content),
        handlebars_opcode_ctor(context, handlebars_opcode_type_append)
    };
    struct handlebars_opcode * child_opcodes[] = {
        handlebars_opcode_ctor(context, handlebars_opcode_type_return)
    };
    struct handlebars_opcode * decorator_opcodes[] = {
        handlebars_opcode_ctor(context, handlebars_opcode_type_append_escaped)
    };
    struct handlebars_program child = {.opcodes = child_opcodes, .opcodes_length = 1};
    struct handlebars_program decorator = {.opcodes = decorator_opcodes, .opcodes_length = 1};
    struct handlebars_program * children[] = {&child};
    struct handlebars_program * decorators[] = {&decorator};
    struct handlebars_program program = {
        .opcodes = root_opcodes, .opcodes_length = 2,
        .children = children, .children_length = 1,
        .decorators = decorators, .decorators_length = 1
    };
    struct handlebars_string * string;

    handlebars_operand_set_stringval(context, root_opcodes[0], &root_opcodes[0]->op1,
        handlebars_string_ctor(context, HBS_STRL("root\ntext")));

    string = handlebars_program_print(context, &program, 0);
    ck_assert_str_eq(hbs_str_val(string),
        "appendContent[STRING:root\\ntext]\nappend\n-----\n"
        "  DECORATOR\n  appendEscaped\n  -----\n  return\n  -----\n");
    handlebars_talloc_free(string);

    string = handlebars_program_print(context, &program, handlebars_opcode_printer_flag_no_newlines);
    ck_assert_str_eq(hbs_str_val(string),
        "appendContent[STRING:root\\ntext] append ----- "
        "  DECORATOR   appendEscaped   -----   return   ----- ");
    handlebars_talloc_free(string);
    handlebars_talloc_free(root_opcodes[0]);
    handlebars_talloc_free(root_opcodes[1]);
    handlebars_talloc_free(child_opcodes[0]);
    handlebars_talloc_free(decorator_opcodes[0]);
}
END_TEST

START_TEST(test_program_print_empty_separators)
{
    struct handlebars_program program = {0};
    struct handlebars_string * string = handlebars_program_print(context, &program, 0);
    ck_assert_str_eq(hbs_str_val(string), "-----\n");
    handlebars_talloc_free(string);
    string = handlebars_program_print(context, &program, handlebars_opcode_printer_flag_no_newlines);
    ck_assert_str_eq(hbs_str_val(string), "----- ");
    handlebars_talloc_free(string);
}
END_TEST

START_TEST(test_program_print_locations_without_newlines)
{
    struct handlebars_opcode * opcode = handlebars_opcode_ctor(context, handlebars_opcode_type_append);
    struct handlebars_program program = {.opcodes = &opcode, .opcodes_length = 1};
    struct handlebars_locinfo loc = {1, 2, 3, 4};
    struct handlebars_string * string;

    handlebars_opcode_set_loc(opcode, loc);
    string = handlebars_program_print(context, &program,
        handlebars_opcode_printer_flag_no_newlines | handlebars_opcode_printer_flag_locations);
    ck_assert_str_eq(hbs_str_val(string), "append [1:2-3:4] ----- ");
    handlebars_talloc_free(string);
    handlebars_talloc_free(opcode);
}
END_TEST

static Suite * suite(void);
static Suite * suite(void)
{
    Suite * s = suite_create("Opcode Printer");

    REGISTER_TEST_FIXTURE(s, test_operand_print_append_null, "Operand Print Append (null)");
    REGISTER_TEST_FIXTURE(s, test_operand_print_append_boolean, "Operand Print Append (boolean)");
    REGISTER_TEST_FIXTURE(s, test_operand_print_append_long, "Operand Print Append (long)");
    REGISTER_TEST_FIXTURE(s, test_operand_print_append_string, "Operand Print Append (string)");
    REGISTER_TEST_FIXTURE(s, test_operand_print_append_array, "Operand Print Append (array)");
    REGISTER_TEST_FIXTURE(s, test_opcode_print_1, "Opcode Print (1)");
    REGISTER_TEST_FIXTURE(s, test_opcode_print_2, "Opcode Print (2)");
    REGISTER_TEST_FIXTURE(s, test_opcode_print_3, "Opcode Print (3)");
    REGISTER_TEST_FIXTURE(s, test_opcode_print_4, "Opcode Print (4)");
    REGISTER_TEST_FIXTURE(s, test_opcode_print_register_decorator, "Print decorator registration operands");
    REGISTER_TEST_FIXTURE(s, test_program_print_separators, "Program separators with children and decorators");
    REGISTER_TEST_FIXTURE(s, test_program_print_empty_separators, "Empty program separators");
    REGISTER_TEST_FIXTURE(s, test_program_print_locations_without_newlines, "Program locations without newlines");

    return s;
}

int main(void)
{
    return default_main(&suite);
}
