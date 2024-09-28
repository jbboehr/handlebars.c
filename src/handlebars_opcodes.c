/**
 * Copyright (c) anno Domini nostri Jesu Christi MMXVI-MMXXIV John Boehr & contributors
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <string.h>

#define HANDLEBARS_OPCODES_PRIVATE

#include "handlebars.h"
#include "handlebars_compiler.h"
#include "handlebars_memory.h"
#include "handlebars_opcodes.h"
#include "handlebars_private.h"
#include "handlebars_string.h"



const size_t HANDLEBARS_OPCODE_SIZE = sizeof(struct handlebars_opcode);
const size_t HANDLEBARS_OPERAND_SIZE = sizeof(struct handlebars_operand);
const size_t HANDLEBARS_OPERAND_INTERNALS_SIZE = sizeof(union handlebars_operand_internals);

struct handlebars_opcode * handlebars_opcode_ctor(
        struct handlebars_context * context, enum handlebars_opcode_type type)
{
    struct handlebars_opcode * opcode = MC(handlebars_talloc_zero(context, struct handlebars_opcode));
    opcode->type = type;
    return opcode;
}

void handlebars_operand_set_null(struct handlebars_operand * operand)
{
    assert(operand != NULL);

    operand->type = handlebars_operand_type_null;
    memset(&operand->data, 0, sizeof(union handlebars_operand_internals));
}

void handlebars_operand_set_boolval(struct handlebars_operand * operand, bool arg)
{
    assert(operand != NULL);

    operand->type = handlebars_operand_type_boolean;
    operand->data.boolval = arg;
}

void handlebars_operand_set_longval(struct handlebars_operand * operand, long arg)
{
    assert(operand != NULL);

    operand->type = handlebars_operand_type_long;
    operand->data.longval = arg;
}

void handlebars_operand_set_stringval(
    struct handlebars_context * context,
    struct handlebars_opcode * opcode,
    struct handlebars_operand * operand,
    struct handlebars_string * string
) {
    assert(operand != NULL);

    operand->type = handlebars_operand_type_string;
    operand->data.string.string = talloc_steal(opcode, string);
}

void handlebars_operand_set_arrayval(
    struct handlebars_context * context,
    struct handlebars_opcode * opcode,
    struct handlebars_operand * operand,
    const char ** arg
) {
    struct handlebars_operand_string * arrptr;
    const char ** ptr;
    size_t num = 0;

    assert(operand != NULL);

    // Get number of items
    for( ptr = arg; *ptr; ++ptr, ++num );

    // Allocate
    operand->data.array.array = MC(handlebars_talloc_array(opcode, struct handlebars_operand_string, num + 1));
    operand->type = handlebars_operand_type_array;
    operand->data.array.count = num;

    // Copy each item
    ptr = arg;
    arrptr = operand->data.array.array;
    for( ; *ptr; ++ptr, ++arrptr ) {
        arrptr->string = talloc_steal(operand->data.array.array, handlebars_string_ctor(context, *ptr, strlen(*ptr)));
    }
}

void handlebars_operand_set_arrayval_string(
    struct handlebars_context * context,
    struct handlebars_opcode * opcode,
    struct handlebars_operand * operand,
    struct handlebars_string ** arg
) {
    struct handlebars_operand_string * arrptr;
    struct handlebars_string ** ptr;
    size_t num = 0;

    // Get number of items
    for( ptr = arg; *ptr; ++ptr, ++num );

    // Allocate
    operand->data.array.array = MC(handlebars_talloc_array(opcode, struct handlebars_operand_string, num + 1));
    operand->type = handlebars_operand_type_array;
    operand->data.array.count = num;

    // Copy each item
    ptr = arg;
    arrptr = operand->data.array.array;
    for( ; *ptr; ++ptr, ++arrptr ) {
        // arrptr->string = talloc_steal(operand->data.array.array, handlebars_string_ctor(context, (*ptr)->val, (*ptr)->len));
        arrptr->string = talloc_steal(operand->data.array.array, handlebars_string_copy_ctor(context, *ptr));
    }
}

const char * handlebars_opcode_readable_type(enum handlebars_opcode_type type)
{
#define _RTYPE_STR(x) #x
#define _RTYPE_MK(type) handlebars_opcode_type_ ## type
#define _RTYPE_CASE(type, name) \
        case _RTYPE_MK(type): return _RTYPE_STR(name); break

    switch( type ) {
        _RTYPE_CASE(nil, nil);
        _RTYPE_CASE(ambiguous_block_value, ambiguousBlockValue);
        _RTYPE_CASE(append, append);
        _RTYPE_CASE(append_escaped, appendEscaped);
        _RTYPE_CASE(empty_hash, emptyHash);
        _RTYPE_CASE(pop_hash, popHash);
        _RTYPE_CASE(push_context, pushContext);
        _RTYPE_CASE(push_hash, pushHash);
        _RTYPE_CASE(resolve_possible_lambda, resolvePossibleLambda);

        _RTYPE_CASE(get_context, getContext);
        _RTYPE_CASE(push_program, pushProgram);

        _RTYPE_CASE(append_content, appendContent);
        _RTYPE_CASE(assign_to_hash, assignToHash);
        _RTYPE_CASE(block_value, blockValue);
        _RTYPE_CASE(push, push);
        _RTYPE_CASE(push_literal, pushLiteral);
        _RTYPE_CASE(push_string, pushString);

        _RTYPE_CASE(invoke_partial, invokePartial);
        _RTYPE_CASE(push_id, pushId);
        _RTYPE_CASE(push_string_param, pushStringParam);

        _RTYPE_CASE(invoke_ambiguous, invokeAmbiguous);

        _RTYPE_CASE(invoke_known_helper, invokeKnownHelper);

        _RTYPE_CASE(invoke_helper, invokeHelper);

        _RTYPE_CASE(lookup_on_context, lookupOnContext);

        _RTYPE_CASE(lookup_data, lookupData);

        _RTYPE_CASE(invalid, invalid);

        // Added in v3
        _RTYPE_CASE(lookup_block_param, lookupBlockParam);

        // Added in v4
        _RTYPE_CASE(register_decorator, registerDecorator);

        // Special
        _RTYPE_CASE(return, return);

        default: return "invalid";
    }
}

enum handlebars_opcode_type handlebars_opcode_reverse_readable_type(const char * type)
{
#define _RTYPE_REV_STR(x) #x
#define _RTYPE_MK(type) handlebars_opcode_type_ ## type
#define _RTYPE_REV_CMP(str, str2) \
    if( strcmp(type, _RTYPE_REV_STR(str2)) == 0 ) { \
        return _RTYPE_MK(str); \
    }

    switch( type[0] ) {
        case 'a':
            _RTYPE_REV_CMP(ambiguous_block_value, ambiguousBlockValue);
            _RTYPE_REV_CMP(append, append);
            _RTYPE_REV_CMP(append_escaped, appendEscaped);
            _RTYPE_REV_CMP(append_content, appendContent);
            _RTYPE_REV_CMP(assign_to_hash, assignToHash);
            break;
        case 'b':
            _RTYPE_REV_CMP(block_value, blockValue);
            break;
        case 'e':
            _RTYPE_REV_CMP(empty_hash, emptyHash);
            break;
        case 'g':
            _RTYPE_REV_CMP(get_context, getContext);
            break;
        case 'i':
            _RTYPE_REV_CMP(invoke_partial, invokePartial);
            _RTYPE_REV_CMP(invoke_ambiguous, invokeAmbiguous);
            _RTYPE_REV_CMP(invoke_known_helper, invokeKnownHelper);
            _RTYPE_REV_CMP(invoke_helper, invokeHelper);
            _RTYPE_REV_CMP(invalid, invalid);
            break;
        case 'l':
            _RTYPE_REV_CMP(lookup_block_param, lookupBlockParam);
            _RTYPE_REV_CMP(lookup_on_context, lookupOnContext);
            _RTYPE_REV_CMP(lookup_data, lookupData);
            break;
        case 'n':
            _RTYPE_REV_CMP(nil, nil);
            break;
        case 'p':
            _RTYPE_REV_CMP(pop_hash, popHash);
            _RTYPE_REV_CMP(push_context, pushContext);
            _RTYPE_REV_CMP(push_hash, pushHash);
            _RTYPE_REV_CMP(push_program, pushProgram);
            _RTYPE_REV_CMP(push, push);
            _RTYPE_REV_CMP(push_literal, pushLiteral);
            _RTYPE_REV_CMP(push_string, pushString);
            _RTYPE_REV_CMP(push_id, pushId);
            _RTYPE_REV_CMP(push_string_param, pushStringParam);
            break;
        case 'r':
            _RTYPE_REV_CMP(resolve_possible_lambda, resolvePossibleLambda);
            _RTYPE_REV_CMP(register_decorator, registerDecorator);
            break;

        default: assert(0); break; // LCOV_EXCL_LINE
    }

    // Unknown :(
    return -1;
}

short handlebars_opcode_num_operands(enum handlebars_opcode_type type)
{
    switch( type ) {
        default:
        case handlebars_opcode_type_invalid:
        case handlebars_opcode_type_nil:
        case handlebars_opcode_type_ambiguous_block_value:
        case handlebars_opcode_type_append:
        case handlebars_opcode_type_append_escaped:
        case handlebars_opcode_type_pop_hash:
        case handlebars_opcode_type_push_context:
        case handlebars_opcode_type_push_hash:
        case handlebars_opcode_type_resolve_possible_lambda:
            return 0;

        case handlebars_opcode_type_get_context:
        case handlebars_opcode_type_push_program:
        case handlebars_opcode_type_append_content:
        case handlebars_opcode_type_assign_to_hash:
        case handlebars_opcode_type_block_value:
        case handlebars_opcode_type_push:
        case handlebars_opcode_type_push_literal:
        case handlebars_opcode_type_push_string:
        // In v3, empty_hash was changed from zero to zero or one
        case handlebars_opcode_type_empty_hash:
            return 1;

        case handlebars_opcode_type_push_string_param:
        case handlebars_opcode_type_invoke_known_helper:
        case handlebars_opcode_type_lookup_block_param:
        // Added in v4
        case handlebars_opcode_type_register_decorator:
            return 2;


        // @TODO make sure everything works. this one only really has two, but we're going to try to use the third one for mustache lambdas
        case handlebars_opcode_type_invoke_ambiguous:


        case handlebars_opcode_type_invoke_helper:
        // In v3 invoke_partial and push_id were changed from two to two or three
        case handlebars_opcode_type_invoke_partial:
        case handlebars_opcode_type_push_id:
        // In v4 lookup_data was changed from 2 to 3 operands
        case handlebars_opcode_type_lookup_data:
            return 3;

        // In v4 lookup_on_context was changed from 3 to 4 operands
        case handlebars_opcode_type_lookup_on_context:
            return 4;
    }
}

void handlebars_opcode_set_loc(struct handlebars_opcode * opcode, struct handlebars_locinfo loc) {
    opcode->loc = loc;
};
