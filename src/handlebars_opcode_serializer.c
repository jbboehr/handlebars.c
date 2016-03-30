
#include "handlebars.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"

#include "handlebars_compiler.h"
#include "handlebars_opcodes.h"
#include "handlebars_opcode_serializer.h"
#include "handlebars_string.h"



static void * append(struct handlebars_module * module, void * source, size_t size)
{
    void * addr = module->data + module->data_size;
#ifndef NDEBUG
    size_t data_offset = module->data - ((void *) module);
    //fprintf(stderr, "Data offset: %ld, Data size: %ld, Append size: %ld, Buffer size: %ld\n", data_offset, module->data_size, size, module->size);
    assert(data_offset + module->data_size + size <= module->size);
#endif
    memcpy(addr, source, size);
    module->data_size += size;
    return addr;
}

static void calculate_size_operand(struct handlebars_module * module, struct handlebars_operand * operand)
{
    int i;

    // Increment for children
    switch( operand->type ) {
        case handlebars_operand_type_string:
            module->data_size += HANDLEBARS_STRING_SIZE(operand->data.string.string->len);
            break;
        case handlebars_operand_type_array:
            for( i = 0; i < operand->data.array.count; i++ ) {
                module->data_size += sizeof(struct handlebars_operand_string);
                module->data_size += HANDLEBARS_STRING_SIZE(operand->data.array.array[i].string->len);
            }
            break;
        default:
            // nothing
            break;
    }
}

static void calculate_size_opcode(struct handlebars_module * module, struct handlebars_opcode * opcode)
{
    module->size += sizeof(struct handlebars_opcode);
    module->opcode_count++;

    calculate_size_operand(module, &opcode->op1);
    calculate_size_operand(module, &opcode->op2);
    calculate_size_operand(module, &opcode->op3);
    calculate_size_operand(module, &opcode->op4);
}

static void calculate_size_program(struct handlebars_module * module, struct handlebars_program * program)
{
    int i;

    // Increment for self
    module->size += sizeof(struct handlebars_module_table_entry);
    module->program_count++;

    // Increment for children
    for( i = 0; i < program->children_length; i++ ) {
        calculate_size_program(module, program->children[i]);
    }

    // Increment for opcodes
    for( i = 0; i < program->opcodes_length; i++ ) {
        calculate_size_opcode(module, program->opcodes[i]);
    }
}

static void serialize_operand(struct handlebars_module * module, struct handlebars_operand * operand)
{
    int i;
    size_t size;

    // Increment for children
    switch( operand->type ) {
        case handlebars_operand_type_string:
            size = HANDLEBARS_STRING_SIZE(operand->data.string.string->len);
            operand->data.string.string = append(module, operand->data.string.string, size);
            break;
        case handlebars_operand_type_array:
            operand->data.array.array = append(module, operand->data.array.array, sizeof(struct handlebars_operand_string) * operand->data.array.count);
            for( i = 0; i < operand->data.array.count; i++ ) {
                size = HANDLEBARS_STRING_SIZE(operand->data.array.array[i].string->len);
                operand->data.array.array[i].string = append(module, operand->data.array.array[i].string, size);
            }
            break;
        default:
            // nothing
            break;
    }
}

static void serialize_opcode(struct handlebars_module * module, struct handlebars_opcode * opcode, size_t program_offset)
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
            new_opcode->op1.data.longval += program_offset;
            new_opcode->op4.data.boolval = 1;
        }
    }
}

static void serialize_program(struct handlebars_module * module, struct handlebars_program * program)
{
    size_t guid = module->program_count++;
    struct handlebars_module_table_entry * entry = &module->programs[guid];
    size_t i;

    entry->guid = guid;
    entry->opcode_count = program->opcodes_length;
    entry->opcodes = module->opcodes + module->opcode_count;
    entry->child_count = program->children_length;
    entry->children = module->programs + module->program_count;

    // Serialize opcodes
    for( i = 0 ; i < program->opcodes_length; i++ ) {
        serialize_opcode(module, program->opcodes[i], guid + 1);
    }

    // Serialize children
    for( i = 0; i < program->children_length; i++ ) {
        serialize_program(module, program->children[i]);
    }
}

struct handlebars_module * handlebars_program_serialize(
    struct handlebars_context * context,
    struct handlebars_program * program
) {
    // Allocate initial buffer
    struct handlebars_module * module = handlebars_talloc_zero(context, struct handlebars_module);
    module->size = sizeof(struct handlebars_module);

    // Calculate size
    calculate_size_program(module, program);
    module->size += module->data_size;

    // Reallocate buffer
    module = handlebars_talloc_realloc_size(context, module, module->size);

    // Setup pointers
    module->programs = ((void *) module) + sizeof(struct handlebars_module);
    module->opcodes = ((void *) module->programs) + (sizeof(struct handlebars_module_table_entry) * module->program_count);
    module->data = ((void *) module->opcodes) + (sizeof(struct handlebars_opcode) * module->opcode_count);

    // Reset counts - use as index
    size_t program_count = module->program_count;
    size_t opcode_count = module->opcode_count;
    size_t data_size = module->data_size;
    module->program_count = module->opcode_count = module->data_size = 0;

    // Copy data
    serialize_program(module, program);

    assert(module->program_count == program_count);
    assert(module->opcode_count == opcode_count);
    assert(module->data_size == data_size);

    return module;
}
