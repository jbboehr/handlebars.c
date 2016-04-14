
#ifndef HANDLEBARS_OPCODE_SERIALIZER_H
#define HANDLEBARS_OPCODE_SERIALIZER_H

#include <time.h>
#include "handlebars.h"

#ifdef	__cplusplus
extern "C" {
#endif

struct handlebars_program;
struct handlebars_opcode;

/**
 * @brief Program table entry
 */
struct handlebars_module_table_entry
{
    //! The unique ID of this program
    size_t guid;
    //! Number of opcodes
    size_t opcode_count;
    //! Number of child programs
    size_t child_count;
    //! Pointer to opcodes
    struct handlebars_opcode * opcodes;
    //! Pointer to child programs
    struct handlebars_module_table_entry * children;
};

/**
 * @brief Serialized program
 */
struct handlebars_module
{
    //! The handlebars version this program was compiled with
    int version;

    //! The original address of the struct used to fix internal pointers
    void * addr;

    //! The size of the struct and all children
    size_t size;

    //! The time at which the struct was created
    time_t ts;

    //! Compiler flags from handlebars_compiler#flags
    long flags;

    //! Number of programs
    size_t program_count;

    //! Array of programs
    struct handlebars_module_table_entry * programs;

    //! Number of opcodes
    size_t opcode_count;

    //! Array of opcodes
    struct handlebars_opcode * opcodes;

    //! Size of data segment
    size_t data_size;

    //! Pointer to the beginning of the data segment
    void * data;
};

/**
 * @brief Serialize a program into a single buffer. Adds return opcodes and globalizes program IDs.
 * @param[in] context
 * @param[in] program
 * @return The serialized program
 */
struct handlebars_module * handlebars_program_serialize(
    struct handlebars_context * context,
    struct handlebars_program * program
);

/**
 * @brief Fix any pointers by adjusting by the offset between the address of `module` and handlebars_module#addr
 * @param[in] module
 * @return void
 */
void handlebars_module_patch_pointers(struct handlebars_module * module);

#ifdef	__cplusplus
}
#endif

#endif
