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
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif

#define HANDLEBARS_COMPILER_PRIVATE
#define HANDLEBARS_OPCODE_SERIALIZER_PRIVATE
#define HANDLEBARS_OPCODES_PRIVATE

#include "handlebars.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"

#include "handlebars_compiler.h"
#include "handlebars_opcodes.h"
#include "handlebars_opcode_serializer.h"
#include "handlebars_string.h"

#define PATCH(ptr, baseaddr) ptr = (void *) ((uintptr_t) (ptr) - (uintptr_t) module->addr + (uintptr_t) (baseaddr))
#define align_size(size) handlebars_align_size(size, sizeof(void *))

const size_t HANDLEBARS_MODULE_SIZE = sizeof(struct handlebars_module);
const size_t HANDLEBARS_MODULE_TABLE_ENTRY_SIZE = sizeof(struct handlebars_module_table_entry);
static const unsigned char HANDLEBARS_MODULE_HEADER[8] = "HBSCM";

static void * append(struct handlebars_module * module, void * source, size_t size)
{
    size_t aligned_size = align_size(size);
    void * addr = &module->data[module->data_offset];
#ifdef HANDLEBARS_ENABLE_DEBUG
    if (NULL != getenv("HANDLEBARS_OPCODE_SERIALIZE_DEBUG")) {
        fprintf(stderr, "Data offset: %zu, Append size: %zu, Buffer size: %zu, Aligned size: %zu\n", module->data_offset, size, module->size, aligned_size);
    }
    assert(module->data_offset < module->size - sizeof(struct handlebars_module));
    assert(((uintptr_t) addr) % sizeof(void *) == 0);
#endif
    memcpy(addr, source, size);
    if (aligned_size != size) {
        memset((char *) addr + size, 0, aligned_size - size);
    }
    module->data_offset += aligned_size;
    return addr;
}

static inline void patch_string(struct handlebars_string * str) {
    handlebars_string_immortalize(str);
}

static size_t calculate_size_operand(struct handlebars_module * module, struct handlebars_operand * operand)
{
    size_t i;
    size_t size = 0;

    // Increment for children
    switch( operand->type ) {
        case handlebars_operand_type_string:
            size += align_size(HBS_STR_SIZE(hbs_str_len(operand->data.string.string)));
            break;
        case handlebars_operand_type_array:
            size += align_size(sizeof(struct handlebars_operand_string) * operand->data.array.count);
            for( i = 0; i < operand->data.array.count; i++ ) {
                size += align_size(HBS_STR_SIZE(hbs_str_len(operand->data.array.array[i].string)));
            }
            break;
        default:
            // nothing
            break;
    }

    return size;
}

static size_t calculate_size_opcode(struct handlebars_module * module, struct handlebars_opcode * opcode)
{
    size_t size = 0;

    size += sizeof(struct handlebars_opcode);
    module->opcode_count++;

    size += calculate_size_operand(module, &opcode->op1);
    size += calculate_size_operand(module, &opcode->op2);
    size += calculate_size_operand(module, &opcode->op3);
    size += calculate_size_operand(module, &opcode->op4);

    return size;
}

static size_t calculate_size_program(struct handlebars_module * module, struct handlebars_program * program)
{
    size_t i;
    size_t size = 0;

    // Increment for self
    size += sizeof(struct handlebars_module_table_entry);
    module->program_count++;

    // Increment for children
    for( i = 0; i < program->children_length; i++ ) {
        size += calculate_size_program(module, program->children[i]);
    }

    // Increment for opcodes
    for( i = 0; i < program->opcodes_length; i++ ) {
        size += calculate_size_opcode(module, program->opcodes[i]);
    }

    // Insert return opcode
    struct handlebars_opcode opcode = {0};
    opcode.type = handlebars_opcode_type_return;
    size += calculate_size_opcode(module, &opcode);

    return size;
}

static void serialize_operand(struct handlebars_module * module, struct handlebars_operand * operand)
{
    size_t i;
    size_t size;

    // Increment for children
    switch( operand->type ) {
        case handlebars_operand_type_string:
            // Make sure hash is computed
            hbs_str_hash(operand->data.string.string);

            size = HBS_STR_SIZE(hbs_str_len(operand->data.string.string));
            operand->data.string.string = append(module, operand->data.string.string, size);
            patch_string(operand->data.string.string);
            break;
        case handlebars_operand_type_array:
            operand->data.array.array = append(module, operand->data.array.array, sizeof(struct handlebars_operand_string) * operand->data.array.count);
            for( i = 0; i < operand->data.array.count; i++ ) {
                // Make sure hash is computed
                hbs_str_hash(operand->data.array.array[i].string);

                size = HBS_STR_SIZE(hbs_str_len(operand->data.array.array[i].string));
                operand->data.array.array[i].string = append(module, operand->data.array.array[i].string, size);
                patch_string(operand->data.array.array[i].string);
            }
            break;
        default:
            // nothing
            break;
    }
}

static void serialize_opcode(
    struct handlebars_context * context,
    struct handlebars_module * module,
    struct handlebars_opcode * opcode,
    struct handlebars_module_table_entry ** table,
    size_t table_count
)
{
    size_t guid = module->opcode_count++;
    struct handlebars_opcode * new_opcode = &module->opcodes[guid];

    // Copy
    *new_opcode = *opcode;

    // Serialize operands
    serialize_operand(module, &new_opcode->op1);
    serialize_operand(module, &new_opcode->op2);
    serialize_operand(module, &new_opcode->op3);
    serialize_operand(module, &new_opcode->op4);

    // Patch push_program opcode
    if( new_opcode->type == handlebars_opcode_type_push_program ) {
        if( new_opcode->op1.type == handlebars_operand_type_long && !new_opcode->op4.data.boolval ) {
            long child_index = new_opcode->op1.data.longval;
            if( child_index < 0
                    || table == NULL
                    || (size_t) child_index >= table_count
                    || table[child_index] == NULL ) {
                handlebars_throw(context, HANDLEBARS_ERROR, "Invalid child program index: %ld", child_index);
            }
            new_opcode->op1.data.longval = table[child_index]->guid;
            new_opcode->op4.data.boolval = 1;
        }
    }
}

static struct handlebars_module_table_entry * serialize_program_shallow(struct handlebars_module * module, struct handlebars_program * program)
{
    size_t guid = module->program_count++;
    struct handlebars_module_table_entry * entry = &module->programs[guid];

    entry->guid = guid;

    return entry;
}

static void serialize_program2(
    struct handlebars_context * context,
    struct handlebars_module * module,
    struct handlebars_program * program,
    struct handlebars_module_table_entry * entry
)
{
    size_t i;
    struct handlebars_module_table_entry ** children = NULL;

    if( program->children_length > 0 ) {
        children = handlebars_talloc_array(module, struct handlebars_module_table_entry *, program->children_length);
        HANDLEBARS_MEMCHECK(children, HBSCTX(talloc_parent(module)));
    }

    // Serialize children (shallow)
    for( i = 0; i < program->children_length; i++ ) {
        children[i] = serialize_program_shallow(module, program->children[i]);
    }

    // Serialize opcodes
    entry->opcode_count = program->opcodes_length;
    entry->opcode_offset = module->opcode_count;
    for( i = 0 ; i < program->opcodes_length; i++ ) {
        serialize_opcode(context, module, program->opcodes[i], children, program->children_length);
    }

    // Insert return opcode
    struct handlebars_opcode opcode = {0};
    opcode.type = handlebars_opcode_type_return;
    serialize_opcode(context, module, &opcode, children, program->children_length);
    entry->opcode_count++;

    // Serialize children
    for( i = 0; i < program->children_length; i++ ) {
        serialize_program2(context, module, program->children[i], children[i]);
    }

    handlebars_talloc_free(children);
}

static void serialize_program(
    struct handlebars_context * context,
    struct handlebars_module * module,
    struct handlebars_program * program
)
{
    struct handlebars_module_table_entry * entry = serialize_program_shallow(module, program);
    serialize_program2(context, module, program, entry);
}

struct handlebars_module * handlebars_program_serialize(
    struct handlebars_context * context,
    struct handlebars_program * program
) {
    // Allocate initial buffer
    struct handlebars_module * module = handlebars_talloc_zero(context, struct handlebars_module);
    memcpy(&module->header, HANDLEBARS_MODULE_HEADER, sizeof(module->header));
    module->version = handlebars_version();
    module->flags = program->flags;
    time(&module->ts);

    // Calculate size
    module->size = sizeof(struct handlebars_module) + calculate_size_program(module, program);

    // Reallocate buffer
    module = handlebars_talloc_realloc_size(context, module, module->size);
    module->addr = (void *) module;
    talloc_set_type(module, struct handlebars_module);

    // Setup pointers
    size_t offset = 0;
    module->programs = (void *) &module->data[offset];
    offset += sizeof(struct handlebars_module_table_entry) * module->program_count;
    module->opcodes = (void *) &module->data[offset];
    offset += sizeof(struct handlebars_opcode) * module->opcode_count;

    // Reset counts - use as index
#ifndef NDEBUG
    size_t program_count = module->program_count;
    size_t opcode_count = module->opcode_count;
#endif

    module->program_count = module->opcode_count = 0;
    module->data_offset = offset;

    // Copy data
    serialize_program(context, module, program);

#ifndef NDEBUG
    assert(module->program_count == program_count);
    assert(module->opcode_count == opcode_count);
    assert(module->data_offset + sizeof(struct handlebars_module) == module->size);
#endif

    return module;
}




static inline void normalize_operand(struct handlebars_module * module, struct handlebars_operand * operand, void * baseaddr)
{
    size_t i;

    switch( operand->type ) {
        case handlebars_operand_type_string:
            PATCH(operand->data.string.string, baseaddr);
            break;
        case handlebars_operand_type_array:
            for( i = 0; i < operand->data.array.count; i++ ) {
                PATCH(operand->data.array.array[i].string, baseaddr);
            }
            PATCH(operand->data.array.array, baseaddr);
            break;
        default:
            // nothing
            break;
    }
}

void handlebars_module_normalize_pointers(struct handlebars_module * module, void *baseaddr)
{
    size_t i;

    if( module->addr == baseaddr ) {
        return;
    }

    for( i = 0; i < module->opcode_count; i++ ) {
        normalize_operand(module, &module->opcodes[i].op1, baseaddr);
        normalize_operand(module, &module->opcodes[i].op2, baseaddr);
        normalize_operand(module, &module->opcodes[i].op3, baseaddr);
        normalize_operand(module, &module->opcodes[i].op4, baseaddr);
    }

    PATCH(module->programs, baseaddr);
    PATCH(module->opcodes, baseaddr);

    module->addr = baseaddr;
}

static inline void patch_operand(struct handlebars_module * module, struct handlebars_operand * operand, void * baseaddr)
{
    size_t i;

    switch( operand->type ) {
        case handlebars_operand_type_string:
            PATCH(operand->data.string.string, baseaddr);
            patch_string(operand->data.string.string);
            break;
        case handlebars_operand_type_array:
            PATCH(operand->data.array.array, baseaddr);
            for( i = 0; i < operand->data.array.count; i++ ) {
                PATCH(operand->data.array.array[i].string, baseaddr);
                patch_string(operand->data.array.array[i].string);
            }
            break;
        default:
            // nothing
            break;
    }
}

void handlebars_module_patch_pointers(struct handlebars_module * module)
{
    size_t i;
    void *baseaddr = (void *) module;

    PATCH(module->programs, baseaddr);
    PATCH(module->opcodes, baseaddr);

    for( i = 0; i < module->opcode_count; i++ ) {
        patch_operand(module, &module->opcodes[i].op1, baseaddr);
        patch_operand(module, &module->opcodes[i].op2, baseaddr);
        patch_operand(module, &module->opcodes[i].op3, baseaddr);
        patch_operand(module, &module->opcodes[i].op4, baseaddr);
    }

    module->addr = baseaddr;
}

size_t handlebars_module_get_size(struct handlebars_module * module)
{
    return module->size;
}

int handlebars_module_get_version(struct handlebars_module * module)
{
    return module->version;
}

time_t handlebars_module_get_ts(struct handlebars_module * module)
{
    return module->ts;
}

long handlebars_module_get_flags(struct handlebars_module * module)
{
    return module->flags;
}

uint64_t handlebars_module_get_hash(struct handlebars_module * module)
{
    return module->hash;
}

static uint64_t calculate_hash(struct handlebars_module * module)
{
    void * start = &module->version;
    size_t size = module->size - offsetof(struct handlebars_module, version);
    return handlebars_hash_xxh3((const char *) start, size);
}

uint64_t handlebars_module_generate_hash(
    struct handlebars_module * module
) {
    return module->hash = calculate_hash(module);
}

struct handlebars_module_verify_state {
    struct handlebars_module * module;
    size_t size;
    size_t data_offset;
};

static bool module_verify_error(struct handlebars_context * ctx, const char * message)
{
    if( ctx != NULL ) {
        handlebars_throw(ctx, HANDLEBARS_ERROR, "%s", message);
    }
    return false;
}

static bool module_pointer_matches_offset(
    struct handlebars_module * module,
    const void * pointer,
    size_t expected_offset
) {
    uintptr_t original_base = (uintptr_t) module->addr;
    uintptr_t serialized_pointer = (uintptr_t) pointer;

    return serialized_pointer >= original_base
        && serialized_pointer - original_base == expected_offset;
}

static bool module_advance(
    size_t * offset,
    size_t count,
    size_t item_size,
    size_t limit
) {
    if( *offset > limit || (item_size != 0 && count > (limit - *offset) / item_size) ) {
        return false;
    }
    *offset += count * item_size;
    return true;
}

static bool module_advance_aligned(
    size_t * offset,
    size_t size,
    size_t limit
) {
    if( size > SIZE_MAX - (sizeof(void *) - 1) ) {
        return false;
    }
    return module_advance(offset, 1, align_size(size), limit);
}

static bool module_verify_string(
    struct handlebars_module_verify_state * state,
    struct handlebars_string * serialized_string
) {
    struct handlebars_string * string;
    size_t string_size;
    size_t length;

    if( !module_pointer_matches_offset(state->module, serialized_string, state->data_offset)
            || state->data_offset > state->size
            || HANDLEBARS_STRING_SIZE > state->size - state->data_offset ) {
        return false;
    }

    string = (void *) ((unsigned char *) state->module + state->data_offset);
    length = hbs_str_len(string);
    if( length > SIZE_MAX - HANDLEBARS_STRING_SIZE - 1 ) {
        return false;
    }
    string_size = HBS_STR_SIZE(length);
    if( string_size > state->size - state->data_offset
            || hbs_str_val(string)[length] != '\0' ) {
        return false;
    }

    return module_advance_aligned(&state->data_offset, string_size, state->size);
}

static bool module_verify_boolean(const bool * value)
{
    static const bool false_value = false;
    static const bool true_value = true;

    return memcmp(value, &false_value, sizeof(bool)) == 0
        || memcmp(value, &true_value, sizeof(bool)) == 0;
}

static bool module_verify_false_boolean(const bool * value)
{
    static const bool false_value = false;

    return memcmp(value, &false_value, sizeof(bool)) == 0;
}

static bool module_verify_operand(
    struct handlebars_module_verify_state * state,
    struct handlebars_operand * operand
) {
    size_t array_offset;
    struct handlebars_operand_string * array;

    switch( operand->type ) {
        case handlebars_operand_type_null:
        case handlebars_operand_type_long:
            return true;

        case handlebars_operand_type_boolean:
            return module_verify_boolean(&operand->data.boolval);

        case handlebars_operand_type_string:
            return module_verify_string(state, operand->data.string.string);

        case handlebars_operand_type_array:
            array_offset = state->data_offset;
            if( operand->data.array.count > SIZE_MAX / sizeof(struct handlebars_operand_string)
                    || !module_pointer_matches_offset(state->module, operand->data.array.array, array_offset)
                    || !module_advance_aligned(
                        &state->data_offset,
                        operand->data.array.count * sizeof(struct handlebars_operand_string),
                        state->size
                    ) ) {
                return false;
            }
            array = (void *) ((unsigned char *) state->module + array_offset);
            for( size_t i = 0; i < operand->data.array.count; i++ ) {
                if( !module_verify_string(state, array[i].string) ) {
                    return false;
                }
            }
            return true;

        default:
            return false;
    }
}

#define OPERAND_TYPE_MASK(type) (1U << (type))

static bool module_operand_type_is(
    const struct handlebars_operand * operand,
    unsigned int allowed_types
) {
    return operand->type >= handlebars_operand_type_null
        && operand->type <= handlebars_operand_type_array
        && (allowed_types & OPERAND_TYPE_MASK(operand->type)) != 0;
}

static bool module_verify_opcode_shape(
    struct handlebars_module * module,
    struct handlebars_opcode * opcode
) {
    static const unsigned int null_type = OPERAND_TYPE_MASK(handlebars_operand_type_null);
    static const unsigned int bool_type = OPERAND_TYPE_MASK(handlebars_operand_type_boolean);
    static const unsigned int long_type = OPERAND_TYPE_MASK(handlebars_operand_type_long);
    static const unsigned int string_type = OPERAND_TYPE_MASK(handlebars_operand_type_string);
    static const unsigned int array_type = OPERAND_TYPE_MASK(handlebars_operand_type_array);
    unsigned int allowed[4] = {null_type, null_type, null_type, null_type};
    struct handlebars_operand * operands[4] = {
        &opcode->op1, &opcode->op2, &opcode->op3, &opcode->op4
    };

    switch( opcode->type ) {
        case handlebars_opcode_type_nil:
        case handlebars_opcode_type_ambiguous_block_value:
        case handlebars_opcode_type_append:
        case handlebars_opcode_type_append_escaped:
        case handlebars_opcode_type_pop_hash:
        case handlebars_opcode_type_push_context:
        case handlebars_opcode_type_push_hash:
        case handlebars_opcode_type_resolve_possible_lambda:
        case handlebars_opcode_type_return:
            break;

        case handlebars_opcode_type_empty_hash:
            allowed[0] |= bool_type;
            break;

        case handlebars_opcode_type_get_context:
            allowed[0] = long_type;
            break;

        case handlebars_opcode_type_push_program:
            allowed[0] |= long_type;
            break;

        case handlebars_opcode_type_append_content:
        case handlebars_opcode_type_assign_to_hash:
        case handlebars_opcode_type_block_value:
        case handlebars_opcode_type_push:
        case handlebars_opcode_type_push_string:
            allowed[0] = string_type;
            break;

        case handlebars_opcode_type_push_literal:
            allowed[0] |= bool_type | long_type | string_type;
            break;

        case handlebars_opcode_type_invoke_partial:
            allowed[0] = bool_type;
            allowed[1] |= long_type | string_type;
            allowed[2] = string_type;
            break;

        case handlebars_opcode_type_push_id:
            allowed[0] = string_type;
            allowed[1] = bool_type | long_type | string_type | array_type;
            allowed[2] |= string_type;
            break;

        case handlebars_opcode_type_push_string_param:
            allowed[0] = bool_type | string_type;
            allowed[1] = string_type;
            break;

        case handlebars_opcode_type_invoke_ambiguous:
            allowed[0] = string_type;
            allowed[1] = bool_type;
            allowed[2] = module->flags & handlebars_compiler_flag_mustache_style_lambdas
                ? string_type
                : null_type;
            break;

        case handlebars_opcode_type_invoke_known_helper:
        case handlebars_opcode_type_register_decorator:
            allowed[0] = long_type;
            allowed[1] = string_type;
            break;

        case handlebars_opcode_type_invoke_helper:
            allowed[0] = long_type;
            allowed[1] = string_type;
            allowed[2] = bool_type;
            break;

        case handlebars_opcode_type_lookup_block_param:
            allowed[0] = array_type;
            allowed[1] = array_type;
            break;

        case handlebars_opcode_type_lookup_data:
            allowed[0] = long_type;
            allowed[1] = array_type;
            allowed[2] |= bool_type;
            break;

        case handlebars_opcode_type_lookup_on_context:
            allowed[0] = array_type;
            allowed[1] |= bool_type;
            allowed[2] |= bool_type;
            allowed[3] |= bool_type;
            break;

        default:
            return false;
    }

    for( size_t i = 0; i < 4; i++ ) {
        if( !module_operand_type_is(operands[i], allowed[i]) ) {
            return false;
        }
    }

    switch( opcode->type ) {
        case handlebars_opcode_type_get_context:
            return opcode->op1.data.longval >= 0;

        case handlebars_opcode_type_push_program:
            if( !module_verify_boolean(&opcode->op4.data.boolval) ) {
                return false;
            }
            if( opcode->op1.type == handlebars_operand_type_null ) {
                return module_verify_false_boolean(&opcode->op4.data.boolval);
            }
            return opcode->op4.data.boolval
                && opcode->op1.data.longval >= 0
                && (size_t) opcode->op1.data.longval < module->program_count;

        case handlebars_opcode_type_invoke_known_helper:
        case handlebars_opcode_type_invoke_helper:
        case handlebars_opcode_type_register_decorator:
            return opcode->op1.data.longval >= 0
                && (size_t) opcode->op1.data.longval <= module->opcode_count;

        case handlebars_opcode_type_lookup_block_param:
            return opcode->op1.data.array.count >= 2;

        case handlebars_opcode_type_lookup_data:
            return opcode->op1.data.longval >= 0
                && opcode->op2.data.array.count > 0
                && (opcode->op3.type != handlebars_operand_type_null
                    || module_verify_false_boolean(&opcode->op3.data.boolval));

        case handlebars_opcode_type_lookup_on_context:
            return opcode->op1.data.array.count > 0
                && (opcode->op2.type != handlebars_operand_type_null
                    || module_verify_false_boolean(&opcode->op2.data.boolval))
                && (opcode->op3.type != handlebars_operand_type_null
                    || module_verify_false_boolean(&opcode->op3.data.boolval))
                && (opcode->op4.type != handlebars_operand_type_null
                    || module_verify_false_boolean(&opcode->op4.data.boolval));

        default:
            return true;
    }
}

static bool module_verify_structure(struct handlebars_module * module, size_t size)
{
    struct handlebars_module_verify_state state = {module, size, sizeof(struct handlebars_module)};
    struct handlebars_module_table_entry * programs;
    struct handlebars_opcode * opcodes;
    unsigned char * opcode_owners;
    size_t programs_offset = state.data_offset;
    size_t opcodes_offset;
    size_t covered_opcode_count = 0;

    if( module->program_count == 0 || module->opcode_count == 0
            || !module_advance(
                &state.data_offset,
                module->program_count,
                sizeof(struct handlebars_module_table_entry),
                size
            )
            || !module_pointer_matches_offset(module, module->programs, programs_offset) ) {
        return false;
    }

    opcodes_offset = state.data_offset;
    if( !module_advance(
            &state.data_offset,
            module->opcode_count,
            sizeof(struct handlebars_opcode),
            size
        )
            || !module_pointer_matches_offset(module, module->opcodes, opcodes_offset) ) {
        return false;
    }

    programs = (void *) ((unsigned char *) module + programs_offset);
    opcodes = (void *) ((unsigned char *) module + opcodes_offset);

    for( size_t i = 0; i < module->opcode_count; i++ ) {
        if( !module_verify_operand(&state, &opcodes[i].op1)
                || !module_verify_operand(&state, &opcodes[i].op2)
                || !module_verify_operand(&state, &opcodes[i].op3)
                || !module_verify_operand(&state, &opcodes[i].op4)
                || !module_verify_opcode_shape(module, &opcodes[i]) ) {
            return false;
        }
    }

    if( state.data_offset != size
            || module->data_offset != size - sizeof(struct handlebars_module) ) {
        return false;
    }

    opcode_owners = calloc(module->opcode_count, sizeof(*opcode_owners));
    if( opcode_owners == NULL ) {
        return false;
    }

    for( size_t i = 0; i < module->program_count; i++ ) {
        struct handlebars_module_table_entry * program = &programs[i];
        size_t program_end;

        if( program->guid != i
                || program->opcode_count == 0
                || program->opcode_offset > module->opcode_count
                || program->opcode_count > module->opcode_count - program->opcode_offset ) {
            free(opcode_owners);
            return false;
        }
        program_end = program->opcode_offset + program->opcode_count;
        for( size_t j = program->opcode_offset; j < program_end; j++ ) {
            if( opcode_owners[j]
                    || (j + 1 < program_end
                        && opcodes[j].type == handlebars_opcode_type_return) ) {
                free(opcode_owners);
                return false;
            }
            opcode_owners[j] = 1;
        }
        if( opcodes[program_end - 1].type != handlebars_opcode_type_return ) {
            free(opcode_owners);
            return false;
        }
        covered_opcode_count += program->opcode_count;
    }

    free(opcode_owners);
    return covered_opcode_count == module->opcode_count;
}

bool handlebars_module_verify_ex(
    struct handlebars_module * module,
    size_t size,
    struct handlebars_context * ctx
) {
    if( size < sizeof(struct handlebars_module) ) {
        return module_verify_error(ctx, "Invalid module buffer size");
    }
    if( module->size != size
            || memcmp(module->header, HANDLEBARS_MODULE_HEADER, sizeof(module->header)) != 0 ) {
        return module_verify_error(ctx, "Invalid module header or size");
    }
    uint64_t hash = calculate_hash(module);
    if (hash != module->hash) {
        if (ctx != NULL) {
            handlebars_throw(
                ctx,
                HANDLEBARS_ERROR,
                "Invalid module hash expected=%llu actual=%llu",
                (unsigned long long) module->hash,
                (unsigned long long) hash
            );
        }
        return false;
    }
    if (handlebars_version() != module->version) {
        if (ctx != NULL) {
            handlebars_throw(
                ctx,
                HANDLEBARS_ERROR,
                "Invalid module version expected=%llu actual=%llu",
                (unsigned long long) module->version,
                (unsigned long long) handlebars_version()
            );
        }
        return false;
    }
    if( !module_verify_structure(module, size) ) {
        return module_verify_error(ctx, "Invalid module structure");
    }
    return true;
}

bool handlebars_module_verify(
    struct handlebars_module * module,
    struct handlebars_context * ctx
) {
    return handlebars_module_verify_ex(module, module->size, ctx);
}
