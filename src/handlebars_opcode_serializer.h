
#ifndef HANDLEBARS_OPCODE_SERIALIZER_H
#define HANDLEBARS_OPCODE_SERIALIZER_H

#include <time.h>
#include "handlebars.h"

#ifdef	__cplusplus
extern "C" {
#endif

struct handlebars_program;
struct handlebars_opcode;

struct handlebars_module_table_entry {
    size_t guid;
    size_t opcode_count;
    size_t child_count;
    struct handlebars_opcode * opcodes;
    struct handlebars_module_table_entry * children;
};

struct handlebars_module {
    int version;
    void * addr;
    size_t size;
    time_t ts;
    long flags;

    size_t program_count;
    struct handlebars_module_table_entry * programs;

    size_t opcode_count;
    struct handlebars_opcode * opcodes;

    size_t data_size;
    void * data;
};

struct handlebars_module * handlebars_program_serialize(
    struct handlebars_context * context,
    struct handlebars_program * program
);

void handlebars_module_patch_pointers(struct handlebars_module * module);

#ifdef	__cplusplus
}
#endif

#endif
