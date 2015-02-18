
/**
 * @file
 * @brief Compiler
 */

#ifndef HANDLEBARS_COMPILER_H
#define HANDLEBARS_COMPILER_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stddef.h>

enum handlebars_opcode_type {
    // Take no arguments
    handlebars_opcode_type_nil = 0,
    handlebars_opcode_type_ambiguous_block_value,
    handlebars_opcode_type_append,
    handlebars_opcode_type_append_escaped,
    handlebars_opcode_type_empty_hash,
    handlebars_opcode_type_pop_hash,
    handlebars_opcode_type_push_context,
    handlebars_opcode_type_push_hash,
    handlebars_opcode_type_resolve_possible_lambda,
    
    // Take one integer argument
    handlebars_opcode_type_get_context,
    handlebars_opcode_type_push_program,
    
    // Takes one string argument
    handlebars_opcode_type_append_content,
    handlebars_opcode_type_assign_to_hash,
    handlebars_opcode_type_block_value,
    handlebars_opcode_type_push,
    handlebars_opcode_type_push_literal,
    handlebars_opcode_type_push_string,
    
    // Takes two string arguments
    handlebars_opcode_type_invoke_partial,
    handlebars_opcode_type_push_id,
    handlebars_opcode_type_push_string_param,
    
    // Takes one string, one integer/boolean argument
    handlebars_opcode_type_invoke_ambiguous,
    
    // Takes one integer, one string argument
    handlebars_opcode_type_invoke_known_helper,
    
    // Takes one integer, one string, and one boolean argument
    handlebars_opcode_type_invoke_helper,
    
    // Takes one array, two boolean arguments
    handlebars_opcode_type_lookup_on_context,
    
    // Takes one integer, one array argument
    handlebars_opcode_type_lookup_data
};

enum handlebars_operand_type {
    handlebars_operand_type_nil = 0,
    handlebars_operand_type_long,
    handlebars_operand_type_string,
    handlebars_operand_type_array,
};

union handlebars_operand_internals {
    long intval;   // these might only need int
    char * strval;
    void * arrval; // maybe concat to string for string arrays
};

struct handlebars_operand {
    enum handlebars_operand_type type;
    union handlebars_operand_internals data;
};

struct handlebars_opcode {
    enum handlebars_opcode_type type;
    struct handlebars_operand op1;
    struct handlebars_operand op2;
    struct handlebars_operand op3;
};

struct handlebars_compiler_ctx {
    
};

#ifdef	__cplusplus
}
#endif

#endif
