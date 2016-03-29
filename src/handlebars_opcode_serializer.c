
#include "handlebars.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"

#include "handlebars_compiler.h"
#include "handlebars_opcodes.h"
#include "handlebars_opcode_serializer.h"
#include "handlebars_string.h"

#define GET_PROGRAMS(header) (struct handlebars_program *) ((void *)header + header->program_offset)
#define GET_OPCODES(header) (struct handlebars_opcode *) ((void *)header + header->opcode_offset)
#define GET_DATA(header) (unsigned char *) ((void *)header + header->data_offset)

#define GET_PROGRAM(header, index) ((GET_PROGRAMS(header)) + index)
#define GET_OPCODE(header, index) ((GET_OPCODES(header)) + index)

struct serialize_ctx {
    size_t program_index;
    size_t opcode_index;
};


void count_programs(struct handlebars_program_header * header, struct handlebars_program * program)
{
    int i;

    // Increment for self
    header->program_count++;
    header->opcode_count += program->opcodes_length;

    // Increment for children
    for( i = 0; i < program->children_length; i++ ) {
        count_programs(header, program->children[i]);
    }

    // Increment for decorators?
//    for( i = 0; i < program->decorators_length; i++ ) {
//        count_programs(header, program->decorators[i]);
//    }
}

void serialize_program(struct serialize_ctx * ctx, struct handlebars_program_header * header, struct handlebars_program * program)
{
    struct handlebars_program * output = GET_PROGRAM(header, ctx->program_index);
    *output = *program;

    output->guid = ctx->program_index++;


}

struct handlebars_program_header * handlebars_program_serialize(
    struct handlebars_context * context,
    struct handlebars_program * program
) {
    struct serialize_ctx ctx = {0};

    // Allocate initial buffer
    struct handlebars_program_header * header = handlebars_talloc_zero(context, struct handlebars_program_header);

    // Reallocate buffer
    header->size = sizeof(struct handlebars_program_header) + talloc_total_size(program);
    header = handlebars_talloc_realloc_size(context, header, header->size);

    // Get counts
    count_programs(header, program);

    // Calculate sizes and offsets
    size_t programs_size = sizeof(struct handlebars_program) * header->program_count;
    size_t opcodes_size = sizeof(struct handlebars_opcode) * header->opcode_count;

    header->program_offset = sizeof(struct handlebars_program_header);
    header->opcode_offset = header->program_offset + programs_size;
    header->data_offset = header->opcode_offset + opcodes_size;

    serialize_program(&ctx, header, program);

    return header;
}
