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

#include "handlebars.h"
#include "handlebars_compiler.h"
#include "handlebars_memory.h"
#include "handlebars_opcodes.h"
#include "handlebars_string.h"
#include "utils.h"



START_TEST(test_opcode_ctor)
{
    struct handlebars_opcode * opcode = handlebars_opcode_ctor(context, handlebars_opcode_type_append);

    ck_assert_ptr_ne(NULL, opcode);
    ck_assert_int_eq(handlebars_opcode_type_append, opcode->type);

    handlebars_talloc_free(opcode);
}
END_TEST

START_TEST(test_opcode_ctor_failed_alloc)
{
#ifdef HANDLEBARS_MEMORY
    jmp_buf buf;
    struct handlebars_opcode * opcode;

    if( handlebars_setjmp_ex(compiler, &buf) ) {
        ck_assert(1);
        return;
    }

    handlebars_memory_fail_enable();
    opcode = handlebars_opcode_ctor(HBSCTX(compiler), handlebars_opcode_type_append);
    (void) opcode;
    handlebars_memory_fail_disable();

    ck_assert(0);
#else
    fprintf(stderr, "Skipped, memory testing functions are disabled\n");
#endif
}
END_TEST

START_TEST(test_opcode_readable_type)
{
#define _RTYPE_STR(x) #x
#define _RTYPE_MK(type) handlebars_opcode_type_ ## type
#define _RTYPE_TEST(type, name) \
        do { \
			const char * expected = _RTYPE_STR(name); \
			const char * actual = handlebars_opcode_readable_type(_RTYPE_MK(type)); \
			ck_assert_str_eq(expected, actual); \
		} while(0)

    _RTYPE_TEST(nil, nil);
    _RTYPE_TEST(ambiguous_block_value, ambiguousBlockValue);
    _RTYPE_TEST(append, append);
    _RTYPE_TEST(append_escaped, appendEscaped);
    _RTYPE_TEST(empty_hash, emptyHash);
    _RTYPE_TEST(pop_hash, popHash);
    _RTYPE_TEST(push_context, pushContext);
    _RTYPE_TEST(push_hash, pushHash);
    _RTYPE_TEST(resolve_possible_lambda, resolvePossibleLambda);

    _RTYPE_TEST(get_context, getContext);
    _RTYPE_TEST(push_program, pushProgram);

    _RTYPE_TEST(append_content, appendContent);
    _RTYPE_TEST(assign_to_hash, assignToHash);
    _RTYPE_TEST(block_value, blockValue);
    _RTYPE_TEST(push, push);
    _RTYPE_TEST(push_literal, pushLiteral);
    _RTYPE_TEST(push_string, pushString);

    _RTYPE_TEST(invoke_partial, invokePartial);
    _RTYPE_TEST(push_id, pushId);
    _RTYPE_TEST(push_string_param, pushStringParam);

    _RTYPE_TEST(invoke_ambiguous, invokeAmbiguous);

    _RTYPE_TEST(invoke_known_helper, invokeKnownHelper);

    _RTYPE_TEST(invoke_helper, invokeHelper);

    _RTYPE_TEST(lookup_on_context, lookupOnContext);

    _RTYPE_TEST(lookup_data, lookupData);

    _RTYPE_TEST(invalid, invalid);

    ck_assert_str_eq("invalid", handlebars_opcode_readable_type(13434534));
}
END_TEST

START_TEST(test_operand_set_null)
{
    struct handlebars_operand op;

    handlebars_operand_set_null(&op);
    ck_assert_int_eq(handlebars_operand_type_null, op.type);
    ck_assert_int_eq(0, op.data.boolval);
    ck_assert_int_eq(0, op.data.longval);
    ck_assert_ptr_eq(NULL, op.data.string.string);
}
END_TEST

START_TEST(test_operand_set_boolval)
{
    struct handlebars_operand op;

    handlebars_operand_set_boolval(&op, 1);
    ck_assert_int_eq(handlebars_operand_type_boolean, op.type);
    ck_assert_int_eq(1, op.data.boolval);
}
END_TEST

START_TEST(test_operand_set_longval)
{
    struct handlebars_operand op;

    handlebars_operand_set_longval(&op, 12);
    ck_assert_int_eq(handlebars_operand_type_long, op.type);
    ck_assert_int_eq(12, op.data.longval);

    handlebars_operand_set_longval(&op, -65);
    ck_assert_int_eq(handlebars_operand_type_long, op.type);
    ck_assert_int_eq(-65, op.data.longval);
}
END_TEST

START_TEST(test_operand_set_stringval)
{
    struct handlebars_operand op;
    struct handlebars_opcode * opcode = handlebars_opcode_ctor(context, handlebars_opcode_type_nil);
    struct handlebars_string * string = handlebars_string_ctor(context, HBS_STRL("bar"));

    handlebars_operand_set_stringval(context, opcode, &op, string);

    ck_assert_int_eq(handlebars_operand_type_string, op.type);
    ck_assert_ptr_ne(NULL, op.data.string.string);
    ck_assert_hbs_str_eq(string, op.data.string.string);
    //ck_assert_ptr_ne(str, op.data.stringval);
}
END_TEST

/*
START_TEST(test_operand_set_stringval_failed_alloc)
{
    struct handlebars_operand op;
    struct handlebars_string * string = handlebars_string_ctor(context, HBS_STRL("bar"));
    jmp_buf buf;

    if( handlebars_setjmp_ex(compiler, &buf) ) {
        ck_assert(1);
        return;
    }

    handlebars_memory_fail_enable();
    handlebars_operand_set_stringval(compiler, &op, string);
    handlebars_memory_fail_disable();

    ck_assert(0);
}
END_TEST
*/

START_TEST(test_operand_set_arrayval)
{
    struct handlebars_operand op;
    struct handlebars_opcode * opcode = handlebars_opcode_ctor(context, handlebars_opcode_type_nil);
    const char * strs[] = {
            "foo", "bar", "baz", "helicopter", NULL
    };
    const char ** ptr1;
    struct handlebars_operand_string * ptr2;

    handlebars_operand_set_arrayval(context, opcode, &op, strs);

    ck_assert_int_eq(handlebars_operand_type_array, op.type);
    ck_assert_ptr_ne(NULL, op.data.array.array);

    // Compare arrays
    for( ptr1 = strs, ptr2 = op.data.array.array; *ptr1 /*|| *ptr2*/; ptr1++, ptr2++ ) {
        ck_assert_hbs_str_eq_cstr(ptr2->string, *ptr1);
    }
}
END_TEST

START_TEST(test_operand_set_arrayval_string)
{
    struct handlebars_string * strings[5];
    struct handlebars_opcode * opcode = handlebars_opcode_ctor(context, handlebars_opcode_type_invalid);

    strings[0] = handlebars_string_ctor(context, HBS_STRL("foo"));
    strings[1] = handlebars_string_ctor(context, HBS_STRL("bar"));
    strings[2] = handlebars_string_ctor(context, HBS_STRL("baz"));
    strings[3] = handlebars_string_ctor(context, HBS_STRL("helicopter"));
    strings[4] = NULL;

    struct handlebars_string ** ptr1;
    struct handlebars_operand_string * ptr2;

    handlebars_operand_set_arrayval_string(context, opcode, &opcode->op1, strings);

    ck_assert_int_eq(handlebars_operand_type_array, opcode->op1.type);
    ck_assert_ptr_ne(NULL, opcode->op1.data.array.array);

    // Compare arrays
    for( ptr1 = strings, ptr2 = opcode->op1.data.array.array; *ptr1 /* || *ptr2*/; ptr1++, ptr2++ ) {
        ck_assert_hbs_str_eq(*ptr1, ptr2->string);
    }
}
END_TEST

static Suite * suite(void);
static Suite * suite(void)
{
    Suite * s = suite_create("Opcodes");

	REGISTER_TEST_FIXTURE(s, test_opcode_ctor, "Constructor");
	REGISTER_TEST_FIXTURE(s, test_opcode_ctor_failed_alloc, "Constructor (failed alloc)");
	REGISTER_TEST_FIXTURE(s, test_opcode_readable_type, "Readable Type");

	REGISTER_TEST_FIXTURE(s, test_operand_set_null, "Set operand null");
	REGISTER_TEST_FIXTURE(s, test_operand_set_boolval, "Set operand boolval");
	REGISTER_TEST_FIXTURE(s, test_operand_set_longval, "Set operand longval");
	REGISTER_TEST_FIXTURE(s, test_operand_set_stringval, "Set operand stringval");
	//REGISTER_TEST_FIXTURE(s, test_operand_set_stringval_failed_alloc, "Set operand stringval (failed alloc)");
    REGISTER_TEST_FIXTURE(s, test_operand_set_arrayval, "Set operand arrayval");
    REGISTER_TEST_FIXTURE(s, test_operand_set_arrayval_string, "operand_set_arrayval_string");


    return s;
}

int main(void)
{
    return default_main(&suite);
}
