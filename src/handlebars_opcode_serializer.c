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
#define SERIALIZER_SIZE_ERROR "Serialized module is too large"
#define SERIALIZER_LOCAL_FRAME_COUNT 64

const size_t HANDLEBARS_MODULE_SIZE = sizeof(struct handlebars_module);
const size_t HANDLEBARS_MODULE_TABLE_ENTRY_SIZE = sizeof(struct handlebars_module_table_entry);
static const unsigned char HANDLEBARS_MODULE_HEADER[8] = "HBSCM";

static size_t serialize_size_add(
    struct handlebars_context * context,
    size_t left,
    size_t right
)
{
    if( unlikely(right > SIZE_MAX - left) ) {
        handlebars_throw(context, HANDLEBARS_NOMEM, SERIALIZER_SIZE_ERROR);
    }
    return left + right;
}

static size_t serialize_size_multiply(
    struct handlebars_context * context,
    size_t count,
    size_t item_size
)
{
    if( unlikely(item_size != 0 && count > SIZE_MAX / item_size) ) {
        handlebars_throw(context, HANDLEBARS_NOMEM, SERIALIZER_SIZE_ERROR);
    }
    return count * item_size;
}

static size_t serialize_size_align(
    struct handlebars_context * context,
    size_t size
)
{
    if( unlikely(size > SIZE_MAX - (sizeof(void *) - 1)) ) {
        handlebars_throw(context, HANDLEBARS_NOMEM, SERIALIZER_SIZE_ERROR);
    }
    return align_size(size);
}

static size_t serialize_string_size(
    struct handlebars_context * context,
    struct handlebars_string * string
)
{
    size_t size;

    if( unlikely(string == NULL) ) {
        handlebars_throw(context, HANDLEBARS_ERROR, "Invalid string operand");
    }
    size = HBS_STR_SIZE(hbs_str_len(string));
    if( unlikely(size == 0) ) {
        handlebars_throw(context, HANDLEBARS_NOMEM, SERIALIZER_SIZE_ERROR);
    }
    return size;
}

static void serialize_count_increment(
    struct handlebars_context * context,
    size_t * count
)
{
    if( unlikely(*count == SIZE_MAX) ) {
        handlebars_throw(context, HANDLEBARS_NOMEM, SERIALIZER_SIZE_ERROR);
    }
    (*count)++;
}

struct handlebars_serialize_state {
    struct handlebars_context * context;
    struct handlebars_module * module;
    size_t program_limit;
    size_t opcode_limit;
};

struct handlebars_size_program_frame {
    struct handlebars_program * program;
    size_t child_index;
};

struct handlebars_active_program_slot {
    struct handlebars_program * program;
    unsigned char active;
};

struct handlebars_active_program_set {
    struct handlebars_active_program_slot * slots;
    size_t count;
    size_t capacity;
};

struct handlebars_serialize_program_frame {
    struct handlebars_program * program;
    struct handlebars_module_table_entry * entry;
    struct handlebars_module_table_entry ** children;
    size_t child_index;
};

static void * append(
    struct handlebars_serialize_state * state,
    void * source,
    size_t size
)
{
    struct handlebars_module * module = state->module;
    size_t aligned_size = serialize_size_align(state->context, size);
    size_t data_capacity;
    void * addr;

    if( unlikely(size != 0 && source == NULL) ) {
        handlebars_throw(state->context, HANDLEBARS_ERROR, "Invalid serialized module source");
    }
    if( unlikely(module->size < sizeof(struct handlebars_module)) ) {
        handlebars_throw(state->context, HANDLEBARS_ERROR, "Invalid serialized module size");
    }
    data_capacity = module->size - sizeof(struct handlebars_module);
    if( unlikely(module->data_offset > data_capacity
            || aligned_size > data_capacity - module->data_offset) ) {
        handlebars_throw(state->context, HANDLEBARS_ERROR, "Serialized module write exceeds allocated size");
    }
    addr = &module->data[module->data_offset];
#ifdef HANDLEBARS_ENABLE_DEBUG
    if (NULL != getenv("HANDLEBARS_OPCODE_SERIALIZE_DEBUG")) {
        fprintf(stderr, "Data offset: %zu, Append size: %zu, Buffer size: %zu, Aligned size: %zu\n", module->data_offset, size, module->size, aligned_size);
    }
    assert(((uintptr_t) addr) % sizeof(void *) == 0);
#endif
    if( size != 0 ) {
        memcpy(addr, source, size);
    }
    if (aligned_size != size) {
        memset((char *) addr + size, 0, aligned_size - size);
    }
    module->data_offset += aligned_size;
    return addr;
}

static inline void patch_string(struct handlebars_string * str) {
    handlebars_string_immortalize(str);
}

static struct handlebars_string * append_string(
    struct handlebars_serialize_state * state,
    struct handlebars_string * string
)
{
    size_t length = hbs_str_len(string);
    size_t size = serialize_string_size(state->context, string);
    size_t value_offset = (size_t) (hbs_str_val(string) - (char *) string);
    size_t initialized_size = serialize_size_add(
        state->context,
        value_offset,
        serialize_size_add(state->context, length, 1)
    );
    struct handlebars_string * serialized;

    if( unlikely(initialized_size > size) ) {
        handlebars_throw(state->context, HANDLEBARS_ERROR, "Invalid serialized string layout");
    }

    // Make sure hash is computed before copying the complete string header.
    hbs_str_hash(string);
    serialized = append(state, string, size);

    // Flexible array members may leave trailing representation padding after
    // the NUL terminator. Canonicalize it before hashing or persisting modules.
    memset((char *) serialized + initialized_size, 0, size - initialized_size);
    patch_string(serialized);
    return serialized;
}

static void calculate_size_operand(
    struct handlebars_context * context,
    struct handlebars_operand * operand,
    size_t * size
)
{
    size_t i;
    size_t child_size;

    // Increment for children
    switch( operand->type ) {
        case handlebars_operand_type_string:
            child_size = serialize_string_size(context, operand->data.string.string);
            *size = serialize_size_add(context, *size, serialize_size_align(context, child_size));
            break;
        case handlebars_operand_type_array:
            if( unlikely(operand->data.array.count != 0
                    && operand->data.array.array == NULL) ) {
                handlebars_throw(context, HANDLEBARS_ERROR, "Invalid array operand");
            }
            child_size = serialize_size_multiply(
                context,
                operand->data.array.count,
                sizeof(struct handlebars_operand_string)
            );
            *size = serialize_size_add(context, *size, serialize_size_align(context, child_size));
            for( i = 0; i < operand->data.array.count; i++ ) {
                child_size = serialize_string_size(context, operand->data.array.array[i].string);
                *size = serialize_size_add(context, *size, serialize_size_align(context, child_size));
            }
            break;
        case handlebars_operand_type_null:
        case handlebars_operand_type_boolean:
        case handlebars_operand_type_long:
            break;
        default:
            handlebars_throw(context, HANDLEBARS_ERROR, "Invalid operand type: %d", operand->type);
    }
}

static void calculate_size_opcode(
    struct handlebars_context * context,
    struct handlebars_module * module,
    struct handlebars_opcode * opcode,
    size_t * size
)
{
    *size = serialize_size_add(context, *size, sizeof(struct handlebars_opcode));
    serialize_count_increment(context, &module->opcode_count);

    calculate_size_operand(context, &opcode->op1, size);
    calculate_size_operand(context, &opcode->op2, size);
    calculate_size_operand(context, &opcode->op3, size);
    calculate_size_operand(context, &opcode->op4, size);
}

static void validate_program(
    struct handlebars_context * context,
    struct handlebars_program * program
)
{
    if( unlikely(program == NULL) ) {
        handlebars_throw(context, HANDLEBARS_ERROR, "Invalid child program");
    }
    if( unlikely(
        program->children_length > program->children_size
        || (program->children_length != 0 && program->children == NULL)
    ) ) {
        handlebars_throw(context, HANDLEBARS_ERROR, "Invalid child program array");
    }
    if( unlikely(
        program->opcodes_length > program->opcodes_size
        || (program->opcodes_length != 0 && program->opcodes == NULL)
    ) ) {
        handlebars_throw(context, HANDLEBARS_ERROR, "Invalid opcode array");
    }
}

static size_t active_program_hash(struct handlebars_program * program)
{
    uintptr_t address = (uintptr_t) program;

    return (size_t) handlebars_hash_xxh3((const char *) &address, sizeof(address));
}

static void active_program_set_grow(
    struct handlebars_context * context,
    struct handlebars_module * module,
    struct handlebars_active_program_set * set
)
{
    struct handlebars_active_program_slot * slots;
    size_t capacity;
    size_t bytes;

    if( set->capacity == 0 ) {
        capacity = SERIALIZER_LOCAL_FRAME_COUNT * 2;
    } else {
        if( unlikely(set->capacity > SIZE_MAX / 2) ) {
            handlebars_throw(context, HANDLEBARS_NOMEM, SERIALIZER_SIZE_ERROR);
        }
        capacity = set->capacity * 2;
    }
    bytes = serialize_size_multiply(
        context,
        capacity,
        sizeof(struct handlebars_active_program_slot)
    );
    slots = handlebars_talloc_zero_size(module, bytes);
    HANDLEBARS_MEMCHECK(slots, context);

    for( size_t i = 0; i < set->capacity; i++ ) {
        struct handlebars_active_program_slot * old_slot = &set->slots[i];
        size_t index;

        if( old_slot->program == NULL ) {
            continue;
        }
        index = active_program_hash(old_slot->program) & (capacity - 1);
        while( slots[index].program != NULL ) {
            index = (index + 1) & (capacity - 1);
        }
        slots[index] = *old_slot;
    }

    handlebars_talloc_free(set->slots);
    set->slots = slots;
    set->capacity = capacity;
}

static bool active_program_set_enter(
    struct handlebars_context * context,
    struct handlebars_module * module,
    struct handlebars_active_program_set * set,
    struct handlebars_program * program
)
{
    for( ;; ) {
        size_t index;

        if( set->capacity == 0 ) {
            active_program_set_grow(context, module, set);
        }
        index = active_program_hash(program) & (set->capacity - 1);
        while( set->slots[index].program != NULL ) {
            if( set->slots[index].program == program ) {
                if( set->slots[index].active ) {
                    return false;
                }
                set->slots[index].active = 1;
                return true;
            }
            index = (index + 1) & (set->capacity - 1);
        }

        if( set->count < set->capacity - set->capacity / 4 ) {
            set->slots[index].program = program;
            set->slots[index].active = 1;
            set->count++;
            return true;
        }
        active_program_set_grow(context, module, set);
    }
}

static void active_program_set_leave(
    struct handlebars_active_program_set * set,
    struct handlebars_program * program
)
{
    size_t index = active_program_hash(program) & (set->capacity - 1);

    while( set->slots[index].program != NULL ) {
        if( set->slots[index].program == program ) {
            assert(set->slots[index].active);
            set->slots[index].active = 0;
            return;
        }
        index = (index + 1) & (set->capacity - 1);
    }
    assert(false);
}

static struct handlebars_size_program_frame * grow_size_program_frames(
    struct handlebars_context * context,
    struct handlebars_module * module,
    struct handlebars_size_program_frame * frames,
    struct handlebars_size_program_frame * local_frames,
    size_t length,
    size_t * capacity
)
{
    struct handlebars_size_program_frame * new_frames;
    size_t new_capacity;
    size_t bytes;

    if( unlikely(*capacity > SIZE_MAX / 2) ) {
        handlebars_throw(context, HANDLEBARS_NOMEM, SERIALIZER_SIZE_ERROR);
    }
    new_capacity = *capacity * 2;
    bytes = serialize_size_multiply(
        context,
        new_capacity,
        sizeof(struct handlebars_size_program_frame)
    );

    if( frames == local_frames ) {
        new_frames = handlebars_talloc_size(module, bytes);
        HANDLEBARS_MEMCHECK(new_frames, context);
        memcpy(
            new_frames,
            frames,
            serialize_size_multiply(
                context,
                length,
                sizeof(struct handlebars_size_program_frame)
            )
        );
    } else {
        new_frames = handlebars_talloc_realloc_size(module, frames, bytes);
        HANDLEBARS_MEMCHECK(new_frames, context);
    }

    *capacity = new_capacity;
    return new_frames;
}

static void calculate_size_program(
    struct handlebars_context * context,
    struct handlebars_module * module,
    struct handlebars_program * program,
    size_t * size,
    size_t * max_depth
)
{
    struct handlebars_size_program_frame local_frames[SERIALIZER_LOCAL_FRAME_COUNT];
    struct handlebars_size_program_frame * frames = local_frames;
    struct handlebars_active_program_set active_programs = {0};
    size_t capacity = SERIALIZER_LOCAL_FRAME_COUNT;
    size_t length = 1;
    struct handlebars_opcode opcode = {0};

    validate_program(context, program);
    frames[0].program = program;
    frames[0].child_index = 0;
    *max_depth = 1;

    // Increment for self
    *size = serialize_size_add(context, *size, sizeof(struct handlebars_module_table_entry));
    serialize_count_increment(context, &module->program_count);

    while( length != 0 ) {
        struct handlebars_size_program_frame * frame = &frames[length - 1];

        if( frame->child_index < frame->program->children_length ) {
            struct handlebars_program * child = frame->program->children[frame->child_index++];

            if( unlikely(child == NULL) ) {
                handlebars_throw(context, HANDLEBARS_ERROR, "Invalid child program");
            }
            if( active_programs.slots == NULL
                    && length < SERIALIZER_LOCAL_FRAME_COUNT ) {
                for( size_t i = 0; i < length; i++ ) {
                    if( unlikely(frames[i].program == child) ) {
                        handlebars_throw(context, HANDLEBARS_ERROR, "Cyclic child program reference");
                    }
                }
            } else {
                if( active_programs.slots == NULL ) {
                    for( size_t i = 0; i < length; i++ ) {
                        if( unlikely(!active_program_set_enter(
                                context,
                                module,
                                &active_programs,
                                frames[i].program
                        )) ) {
                            handlebars_throw(
                                context,
                                HANDLEBARS_ERROR,
                                "Cyclic child program reference"
                            );
                        }
                    }
                }
                if( unlikely(!active_program_set_enter(
                        context,
                        module,
                        &active_programs,
                        child
                )) ) {
                    handlebars_throw(context, HANDLEBARS_ERROR, "Cyclic child program reference");
                }
            }
            validate_program(context, child);

            if( length == capacity ) {
                frames = grow_size_program_frames(
                    context,
                    module,
                    frames,
                    local_frames,
                    length,
                    &capacity
                );
            }

            frames[length].program = child;
            frames[length].child_index = 0;
            length++;
            if( length > *max_depth ) {
                *max_depth = length;
            }

            *size = serialize_size_add(context, *size, sizeof(struct handlebars_module_table_entry));
            serialize_count_increment(context, &module->program_count);
            continue;
        }

        // Increment for opcodes
        for( size_t i = 0; i < frame->program->opcodes_length; i++ ) {
            if( unlikely(frame->program->opcodes[i] == NULL) ) {
                handlebars_throw(context, HANDLEBARS_ERROR, "Invalid opcode");
            }
            calculate_size_opcode(context, module, frame->program->opcodes[i], size);
        }

        // Insert return opcode
        opcode.type = handlebars_opcode_type_return;
        calculate_size_opcode(context, module, &opcode, size);
        if( active_programs.slots != NULL ) {
            active_program_set_leave(&active_programs, frame->program);
        }
        length--;
    }

    handlebars_talloc_free(active_programs.slots);
    if( frames != local_frames ) {
        handlebars_talloc_free(frames);
    }
}

static void serialize_operand(
    struct handlebars_serialize_state * state,
    struct handlebars_operand * operand
)
{
    size_t i;
    size_t size;

    // Increment for children
    switch( operand->type ) {
        case handlebars_operand_type_string:
            operand->data.string.string = append_string(
                state,
                operand->data.string.string
            );
            break;
        case handlebars_operand_type_array:
            size = serialize_size_multiply(
                state->context,
                operand->data.array.count,
                sizeof(struct handlebars_operand_string)
            );
            operand->data.array.array = append(state, operand->data.array.array, size);
            for( i = 0; i < operand->data.array.count; i++ ) {
                operand->data.array.array[i].string = append_string(
                    state,
                    operand->data.array.array[i].string
                );
            }
            break;
        case handlebars_operand_type_null:
        case handlebars_operand_type_boolean:
        case handlebars_operand_type_long:
            break;
        default:
            handlebars_throw(state->context, HANDLEBARS_ERROR, "Invalid operand type: %d", operand->type);
    }
}

static void serialize_opcode(
    struct handlebars_serialize_state * state,
    struct handlebars_opcode * opcode,
    struct handlebars_module_table_entry ** table,
    size_t table_count,
    bool inline_partial
)
{
    struct handlebars_module * module = state->module;
    struct handlebars_opcode * new_opcode;
    size_t guid;

    if( unlikely(module->opcode_count >= state->opcode_limit) ) {
        handlebars_throw(state->context, HANDLEBARS_ERROR, "Serialized opcode count exceeds allocated table");
    }
    guid = module->opcode_count++;
    new_opcode = &module->opcodes[guid];

    // Copy
    *new_opcode = *opcode;
    if( inline_partial ) {
        handlebars_operand_set_boolval(&new_opcode->op3, true);
    }

    // Serialize operands
    serialize_operand(state, &new_opcode->op1);
    serialize_operand(state, &new_opcode->op2);
    serialize_operand(state, &new_opcode->op3);
    serialize_operand(state, &new_opcode->op4);

    // Patch push_program opcode
    if( new_opcode->type == handlebars_opcode_type_push_program ) {
        if( new_opcode->op1.type == handlebars_operand_type_long && !new_opcode->op4.data.boolval ) {
            long child_index = new_opcode->op1.data.longval;
            if( unlikely(child_index < 0
                    || table == NULL
                    || (size_t) child_index >= table_count
                    || table[child_index] == NULL) ) {
                handlebars_throw(state->context, HANDLEBARS_ERROR, "Invalid child program index: %ld", child_index);
            }
            new_opcode->op1.data.longval = table[child_index]->guid;
            new_opcode->op4.data.boolval = 1;
        }
    }
}

#define INLINE_PARTIAL_MIN_OPCODE_COUNT 5

static bool inline_partial_name_opcode_is_valid(
    const struct handlebars_opcode * opcode
)
{
    if( opcode->type == handlebars_opcode_type_push_string ) {
        return opcode->op1.type == handlebars_operand_type_string;
    }
    if( opcode->type != handlebars_opcode_type_push_literal ) {
        return false;
    }
    return opcode->op1.type == handlebars_operand_type_boolean
        || opcode->op1.type == handlebars_operand_type_long
        || opcode->op1.type == handlebars_operand_type_string;
}

static size_t inline_partial_opcode_range_length(
    struct handlebars_program * program,
    size_t offset
)
{
    struct handlebars_opcode ** opcodes;
    size_t opcode_count;

    if( offset > program->opcodes_length
            || program->opcodes_length - offset < INLINE_PARTIAL_MIN_OPCODE_COUNT ) {
        return 0;
    }

    opcodes = &program->opcodes[offset];
    opcode_count = program->opcodes_length - offset;
    if( !inline_partial_name_opcode_is_valid(opcodes[0]) ) {
        return 0;
    }

    for( size_t program_offset = 1;
            program_offset <= opcode_count - 4;
            program_offset++ ) {
        size_t hash_offset;
        size_t registration_offset;

        if( opcodes[program_offset]->type != handlebars_opcode_type_push_program
                || opcodes[program_offset]->op1.type != handlebars_operand_type_long
                || opcodes[program_offset]->op1.data.longval < 0
                || opcodes[program_offset + 1]->type != handlebars_opcode_type_push_program
                || opcodes[program_offset + 1]->op1.type != handlebars_operand_type_null ) {
            continue;
        }

        hash_offset = program_offset + 2;
        if( opcodes[hash_offset]->type == handlebars_opcode_type_empty_hash ) {
            registration_offset = hash_offset + 1;
        } else if( opcodes[hash_offset]->type == handlebars_opcode_type_push_hash ) {
            size_t hash_depth = 1;
            bool found_pop_hash = false;

            for( size_t i = hash_offset + 1; i < opcode_count; i++ ) {
                if( opcodes[i]->type == handlebars_opcode_type_push_hash ) {
                    hash_depth++;
                } else if( opcodes[i]->type == handlebars_opcode_type_pop_hash ) {
                    if( hash_depth == 1 ) {
                        registration_offset = i + 1;
                        found_pop_hash = true;
                        break;
                    }
                    hash_depth--;
                }
            }
            if( !found_pop_hash ) {
                continue;
            }
        } else {
            continue;
        }

        if( registration_offset >= opcode_count ) {
            continue;
        }

        if( opcodes[registration_offset]->type == handlebars_opcode_type_register_decorator
                && opcodes[registration_offset]->op1.type == handlebars_operand_type_long
                && opcodes[registration_offset]->op1.data.longval >= 1
                && opcodes[registration_offset]->op2.type == handlebars_operand_type_string
                && hbs_str_eq_strl(
                    opcodes[registration_offset]->op2.data.string.string,
                    HBS_STRL("inline")
                )
                && opcodes[registration_offset]->op3.type == handlebars_operand_type_null
                && opcodes[registration_offset]->op4.type == handlebars_operand_type_null ) {
            return registration_offset + 1;
        }
    }

    return 0;
}

static struct handlebars_module_table_entry * serialize_program_shallow(
    struct handlebars_serialize_state * state,
    struct handlebars_program * program
)
{
    struct handlebars_module * module = state->module;
    struct handlebars_module_table_entry * entry;
    size_t guid;

    if( unlikely(module->program_count >= state->program_limit) ) {
        handlebars_throw(state->context, HANDLEBARS_ERROR, "Serialized program count exceeds allocated table");
    }
    guid = module->program_count++;
    entry = &module->programs[guid];

    if( unlikely(program->block_params < 0 || program->block_params > 2) ) {
        handlebars_throw(
            state->context,
            HANDLEBARS_ERROR,
            "Invalid block parameter count: %d",
            program->block_params
        );
    }
    entry->guid = guid;
    entry->block_params = (size_t) program->block_params;

    return entry;
}

static void serialize_program_frame_enter(
    struct handlebars_serialize_state * state,
    struct handlebars_serialize_program_frame * frame
)
{
    struct handlebars_module * module = state->module;

    frame->children = NULL;
    frame->child_index = 0;
    if( frame->program->children_length > 0 ) {
        frame->children = handlebars_talloc_array(
            module,
            struct handlebars_module_table_entry *,
            frame->program->children_length
        );
        HANDLEBARS_MEMCHECK(frame->children, state->context);
    }

    // Serialize children (shallow)
    for( size_t i = 0; i < frame->program->children_length; i++ ) {
        frame->children[i] = serialize_program_shallow(state, frame->program->children[i]);
    }

    // Inline partial declarations are decorators in the upstream opcode
    // format, so they apply to the whole program even when written after
    // their first use. Move only the compiler's built-in sequence into
    // a marked prologue; generic decorators remain in source order.
    frame->entry->opcode_count = frame->program->opcodes_length;
    frame->entry->opcode_offset = module->opcode_count;
    for( size_t i = 0; i < frame->program->opcodes_length; ) {
        size_t range_length = inline_partial_opcode_range_length(frame->program, i);

        if( range_length > 0 ) {
            for( size_t j = 0; j < range_length; j++ ) {
                serialize_opcode(
                    state,
                    frame->program->opcodes[i + j],
                    frame->children,
                    frame->program->children_length,
                    j == range_length - 1
                );
            }
            i += range_length;
        } else {
            i++;
        }
    }
    for( size_t i = 0 ; i < frame->program->opcodes_length; ) {
        size_t range_length = inline_partial_opcode_range_length(frame->program, i);

        if( range_length > 0 ) {
            i += range_length;
            continue;
        }
        serialize_opcode(
            state,
            frame->program->opcodes[i],
            frame->children,
            frame->program->children_length,
            false
        );
        i++;
    }

    // Insert return opcode
    struct handlebars_opcode opcode = {0};
    opcode.type = handlebars_opcode_type_return;
    serialize_opcode(state, &opcode, frame->children, frame->program->children_length, false);
    serialize_count_increment(state->context, &frame->entry->opcode_count);
}

static void serialize_program(
    struct handlebars_serialize_state * state,
    struct handlebars_program * program,
    size_t max_depth
)
{
    struct handlebars_serialize_program_frame local_frames[SERIALIZER_LOCAL_FRAME_COUNT];
    struct handlebars_serialize_program_frame * frames = local_frames;
    size_t capacity = SERIALIZER_LOCAL_FRAME_COUNT;
    size_t length = 1;

    if( max_depth > capacity ) {
        size_t bytes = serialize_size_multiply(
            state->context,
            max_depth,
            sizeof(struct handlebars_serialize_program_frame)
        );
        frames = handlebars_talloc_size(state->module, bytes);
        HANDLEBARS_MEMCHECK(frames, state->context);
        capacity = max_depth;
    }

    frames[0].program = program;
    frames[0].entry = serialize_program_shallow(state, program);
    serialize_program_frame_enter(state, &frames[0]);

    while( length != 0 ) {
        struct handlebars_serialize_program_frame * frame = &frames[length - 1];

        if( frame->child_index < frame->program->children_length ) {
            size_t child_index = frame->child_index++;

            if( unlikely(length >= capacity) ) {
                handlebars_throw(
                    state->context,
                    HANDLEBARS_ERROR,
                    "Serialized program depth exceeds calculated traversal depth"
                );
            }
            frames[length].program = frame->program->children[child_index];
            frames[length].entry = frame->children[child_index];
            serialize_program_frame_enter(state, &frames[length]);
            length++;
            continue;
        }

        handlebars_talloc_free(frame->children);
        length--;
    }

    if( frames != local_frames ) {
        handlebars_talloc_free(frames);
    }
}

struct handlebars_module * handlebars_program_serialize(
    struct handlebars_context * context,
    struct handlebars_program * program
) {
    struct handlebars_serialize_state state;
    size_t data_size;
    size_t max_depth;
    size_t offset;

    // Allocate initial buffer
    struct handlebars_module * module = MC(handlebars_talloc_zero(context, struct handlebars_module));
    memcpy(&module->header, HANDLEBARS_MODULE_HEADER, sizeof(module->header));
    module->version = handlebars_version();
    module->flags = program->flags;
    time(&module->ts);

    // Calculate size
    data_size = 0;
    calculate_size_program(context, module, program, &data_size, &max_depth);
    module->size = serialize_size_add(context, sizeof(struct handlebars_module), data_size);

    // Reallocate buffer
    module = MC(handlebars_talloc_realloc_size(context, module, module->size));
    if( data_size != 0 ) {
        memset(module->data, 0, data_size);
    }
    module->addr = (void *) module;
    talloc_set_type(module, struct handlebars_module);

    // Setup pointers
    offset = 0;
    module->programs = (void *) &module->data[offset];
    offset = serialize_size_add(
        context,
        offset,
        serialize_size_multiply(context, module->program_count, sizeof(struct handlebars_module_table_entry))
    );
    module->opcodes = (void *) &module->data[offset];
    offset = serialize_size_add(
        context,
        offset,
        serialize_size_multiply(context, module->opcode_count, sizeof(struct handlebars_opcode))
    );

    if( unlikely(offset > data_size) ) {
        handlebars_throw(context, HANDLEBARS_ERROR, "Serialized module tables exceed allocated size");
    }

    state.context = context;
    state.module = module;
    state.program_limit = module->program_count;
    state.opcode_limit = module->opcode_count;

    // Reset counts - use as index
    module->program_count = module->opcode_count = 0;
    module->data_offset = offset;

    // Copy data
    serialize_program(&state, program, max_depth);

    if( unlikely(module->program_count != state.program_limit
            || module->opcode_count != state.opcode_limit
            || module->data_offset != data_size) ) {
        handlebars_throw(context, HANDLEBARS_ERROR, "Serialized module layout changed during serialization");
    }

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

static struct handlebars_string * module_resolve_string(
    struct handlebars_module * module,
    size_t size,
    struct handlebars_string * serialized_string
) {
    uintptr_t original_base = (uintptr_t) module->addr;
    uintptr_t serialized_pointer = (uintptr_t) serialized_string;
    uintptr_t serialized_offset;
    struct handlebars_string * string;
    size_t string_offset;
    size_t string_size;
    size_t length;

    if( unlikely(serialized_pointer < original_base) ) {
        return NULL;
    }
    serialized_offset = serialized_pointer - original_base;
    if( unlikely(serialized_offset > size) ) {
        return NULL;
    }
    string_offset = (size_t) serialized_offset;
    if( unlikely(HANDLEBARS_STRING_SIZE > size - string_offset) ) {
        return NULL;
    }

    string = (void *) ((unsigned char *) module + string_offset);
    length = hbs_str_len(string);
    if( unlikely(length > SIZE_MAX - HANDLEBARS_STRING_SIZE - 1) ) {
        return NULL;
    }
    string_size = HBS_STR_SIZE(length);
    if( unlikely(string_size > size - string_offset
            || hbs_str_val(string)[length] != '\0') ) {
        return NULL;
    }
    return string;
}

static bool module_string_eq_strl(
    struct handlebars_module * module,
    struct handlebars_string * serialized_string,
    const char * value,
    size_t length
) {
    struct handlebars_string * string = module_resolve_string(
        module,
        module->size,
        serialized_string
    );

    return string != NULL && hbs_str_eq_strl(string, value, length);
}

static bool module_advance(
    size_t * offset,
    size_t count,
    size_t item_size,
    size_t limit
) {
    if( unlikely(*offset > limit || (item_size != 0 && count > (limit - *offset) / item_size)) ) {
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
    if( unlikely(size > SIZE_MAX - (sizeof(void *) - 1)) ) {
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

    if( unlikely(!module_pointer_matches_offset(
                    state->module,
                    serialized_string,
                    state->data_offset
                )) ) {
        return false;
    }

    string = module_resolve_string(state->module, state->size, serialized_string);
    if( unlikely(string == NULL) ) {
        return false;
    }
    string_size = HBS_STR_SIZE(hbs_str_len(string));

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
            if( unlikely(operand->data.array.count > SIZE_MAX / sizeof(struct handlebars_operand_string)
                    || !module_pointer_matches_offset(state->module, operand->data.array.array, array_offset)
                    || !module_advance_aligned(
                        &state->data_offset,
                        operand->data.array.count * sizeof(struct handlebars_operand_string),
                        state->size
                    )) ) {
                return false;
            }
            array = (void *) ((unsigned char *) state->module + array_offset);
            for( size_t i = 0; i < operand->data.array.count; i++ ) {
                if( unlikely(!module_verify_string(state, array[i].string)) ) {
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
            allowed[0] = long_type;
            allowed[1] = string_type;
            break;

        case handlebars_opcode_type_register_decorator:
            allowed[0] = long_type;
            allowed[1] = string_type;
            allowed[2] |= bool_type;
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
        if( unlikely(!module_operand_type_is(operands[i], allowed[i])) ) {
            return false;
        }
    }

    switch( opcode->type ) {
        case handlebars_opcode_type_get_context:
            return opcode->op1.data.longval >= 0;

        case handlebars_opcode_type_push_program:
            if( unlikely(!module_verify_boolean(&opcode->op4.data.boolval)) ) {
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
            return opcode->op1.data.longval >= 0
                && (size_t) opcode->op1.data.longval <= module->opcode_count;

        case handlebars_opcode_type_register_decorator:
            if( opcode->op1.data.longval < 0
                    || (size_t) opcode->op1.data.longval > module->opcode_count ) {
                return false;
            }
            if( opcode->op3.type == handlebars_operand_type_null ) {
                return true;
            }
            return module_verify_boolean(&opcode->op3.data.boolval)
                && opcode->op3.data.boolval
                && opcode->op1.data.longval >= 1
                && module_string_eq_strl(
                    module,
                    opcode->op2.data.string.string,
                    HBS_STRL("inline")
                );

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

static bool module_opcode_is_inline_partial_marker(
    struct handlebars_opcode * opcode
)
{
    return opcode->type == handlebars_opcode_type_register_decorator
        && opcode->op3.type == handlebars_operand_type_boolean
        && opcode->op3.data.boolval;
}

static size_t module_inline_partial_opcode_range_length(
    struct handlebars_module * module,
    struct handlebars_opcode * opcodes,
    size_t opcode_count
)
{
    if( opcode_count < INLINE_PARTIAL_MIN_OPCODE_COUNT
            || !inline_partial_name_opcode_is_valid(&opcodes[0]) ) {
        return 0;
    }

    for( size_t program_offset = 1;
            program_offset <= opcode_count - 4;
            program_offset++ ) {
        size_t hash_offset;
        size_t registration_offset;

        if( opcodes[program_offset].type != handlebars_opcode_type_push_program
                || opcodes[program_offset].op1.type != handlebars_operand_type_long
                || opcodes[program_offset].op1.data.longval < 0
                || !opcodes[program_offset].op4.data.boolval
                || opcodes[program_offset + 1].type != handlebars_opcode_type_push_program
                || opcodes[program_offset + 1].op1.type != handlebars_operand_type_null ) {
            continue;
        }

        hash_offset = program_offset + 2;
        if( opcodes[hash_offset].type == handlebars_opcode_type_empty_hash ) {
            registration_offset = hash_offset + 1;
        } else if( opcodes[hash_offset].type == handlebars_opcode_type_push_hash ) {
            size_t hash_depth = 1;
            bool found_pop_hash = false;

            for( size_t i = hash_offset + 1; i < opcode_count; i++ ) {
                if( opcodes[i].type == handlebars_opcode_type_push_hash ) {
                    hash_depth++;
                } else if( opcodes[i].type == handlebars_opcode_type_pop_hash ) {
                    if( hash_depth == 1 ) {
                        registration_offset = i + 1;
                        found_pop_hash = true;
                        break;
                    }
                    hash_depth--;
                }
            }
            if( !found_pop_hash ) {
                continue;
            }
        } else {
            continue;
        }

        if( registration_offset >= opcode_count ) {
            continue;
        }

        if( opcodes[registration_offset].type == handlebars_opcode_type_register_decorator
                && opcodes[registration_offset].op1.type == handlebars_operand_type_long
                && opcodes[registration_offset].op1.data.longval >= 1
                && opcodes[registration_offset].op2.type == handlebars_operand_type_string
                && module_string_eq_strl(
                    module,
                    opcodes[registration_offset].op2.data.string.string,
                    HBS_STRL("inline")
                )
                && opcodes[registration_offset].op3.type == handlebars_operand_type_boolean
                && opcodes[registration_offset].op3.data.boolval ) {
            return registration_offset + 1;
        }
    }

    return 0;
}

static bool module_verify_inline_partial_program(
    struct handlebars_module * module,
    struct handlebars_opcode * opcodes,
    struct handlebars_module_table_entry * program
)
{
    size_t offset = program->opcode_offset;
    size_t end = offset + program->opcode_count;

    while( offset < end ) {
        size_t range_length = module_inline_partial_opcode_range_length(
            module,
            &opcodes[offset],
            end - offset
        );

        if( range_length == 0 ) {
            break;
        }
        offset += range_length;
    }

    for( ; offset < end; offset++ ) {
        if( unlikely(module_opcode_is_inline_partial_marker(&opcodes[offset])) ) {
            return false;
        }
    }
    return true;
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

    if( unlikely(module->program_count == 0 || module->opcode_count == 0
            || !module_advance(
                &state.data_offset,
                module->program_count,
                sizeof(struct handlebars_module_table_entry),
                size
            )
            || !module_pointer_matches_offset(module, module->programs, programs_offset)) ) {
        return false;
    }

    opcodes_offset = state.data_offset;
    if( unlikely(!module_advance(
            &state.data_offset,
            module->opcode_count,
            sizeof(struct handlebars_opcode),
            size
        )
            || !module_pointer_matches_offset(module, module->opcodes, opcodes_offset)) ) {
        return false;
    }

    programs = (void *) ((unsigned char *) module + programs_offset);
    opcodes = (void *) ((unsigned char *) module + opcodes_offset);

    for( size_t i = 0; i < module->opcode_count; i++ ) {
        if( unlikely(!module_verify_operand(&state, &opcodes[i].op1)
                || !module_verify_operand(&state, &opcodes[i].op2)
                || !module_verify_operand(&state, &opcodes[i].op3)
                || !module_verify_operand(&state, &opcodes[i].op4)
                || !module_verify_opcode_shape(module, &opcodes[i])) ) {
            return false;
        }
    }

    if( unlikely(state.data_offset != size
            || module->data_offset != size - sizeof(struct handlebars_module)) ) {
        return false;
    }

    opcode_owners = calloc(module->opcode_count, sizeof(*opcode_owners));
    if( unlikely(opcode_owners == NULL) ) {
        return false;
    }

    for( size_t i = 0; i < module->program_count; i++ ) {
        struct handlebars_module_table_entry * program = &programs[i];
        size_t program_end;

        if( unlikely(program->guid != i
                || program->block_params > 2
                || program->opcode_count == 0
                || program->opcode_offset > module->opcode_count
                || program->opcode_count > module->opcode_count - program->opcode_offset) ) {
            free(opcode_owners);
            return false;
        }
        program_end = program->opcode_offset + program->opcode_count;
        if( unlikely(!module_verify_inline_partial_program(module, opcodes, program)) ) {
            free(opcode_owners);
            return false;
        }
        for( size_t j = program->opcode_offset; j < program_end; j++ ) {
            if( unlikely(opcode_owners[j]
                    || (j + 1 < program_end
                        && opcodes[j].type == handlebars_opcode_type_return)) ) {
                free(opcode_owners);
                return false;
            }
            opcode_owners[j] = 1;
        }
        if( unlikely(opcodes[program_end - 1].type != handlebars_opcode_type_return) ) {
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
    if( unlikely(size < sizeof(struct handlebars_module)) ) {
        return module_verify_error(ctx, "Invalid module buffer size");
    }
    if( unlikely(module->size != size
            || memcmp(module->header, HANDLEBARS_MODULE_HEADER, sizeof(module->header)) != 0) ) {
        return module_verify_error(ctx, "Invalid module header or size");
    }
    uint64_t hash = calculate_hash(module);
    if( unlikely(hash != module->hash) ) {
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
    if( unlikely(handlebars_version() != module->version) ) {
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
    if( unlikely(!module_verify_structure(module, size)) ) {
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
