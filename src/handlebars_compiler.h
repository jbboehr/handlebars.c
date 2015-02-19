
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
    handlebars_opcode_type_invalid = -1,
    
    // Takes no arguments
    handlebars_opcode_type_nil = 0,
    handlebars_opcode_type_ambiguous_block_value = 1,
    handlebars_opcode_type_append = 2,
    handlebars_opcode_type_append_escaped = 3,
    handlebars_opcode_type_empty_hash = 4,
    handlebars_opcode_type_pop_hash = 5,
    handlebars_opcode_type_push_context = 6,
    handlebars_opcode_type_push_hash = 7,
    handlebars_opcode_type_resolve_possible_lambda = 8,
    
    // Takes one integer argument
    handlebars_opcode_type_get_context = 9,
    handlebars_opcode_type_push_program = 10,
    
    // Takes one string argument
    handlebars_opcode_type_append_content = 11,
    handlebars_opcode_type_assign_to_hash = 12,
    handlebars_opcode_type_block_value = 13,
    handlebars_opcode_type_push = 14,
    handlebars_opcode_type_push_literal = 15,
    handlebars_opcode_type_push_string = 16,
    
    // Takes two string arguments
    handlebars_opcode_type_invoke_partial = 17,
    handlebars_opcode_type_push_id = 18,
    handlebars_opcode_type_push_string_param = 19,
    
    // Takes one string, one integer/boolean argument
    handlebars_opcode_type_invoke_ambiguous = 20,
    
    // Takes one integer, one string argument
    handlebars_opcode_type_invoke_known_helper = 21,
    
    // Takes one integer, one string, and one boolean argument
    handlebars_opcode_type_invoke_helper = 22,
    
    // Takes one array, two boolean arguments
    handlebars_opcode_type_lookup_on_context = 23,
    
    // Takes one integer, one array argument
    handlebars_opcode_type_lookup_data = 24
};

enum handlebars_operand_type {
    handlebars_operand_type_null = 0,
    handlebars_operand_type_long = 1,
    handlebars_operand_type_string = 2,
    handlebars_operand_type_array = 3,
    handlebars_operand_type_boolean = 4
};

union handlebars_operand_internals {
    long intval;   // these might only need int
    char * strval;
    void * arrval; // maybe concat to string for string arrays
    short boolval;
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

enum handlebars_compiler_flag {
    handlebars_compiler_flag_none = 0,
    
    // Option flags
    handlebars_compiler_flag_use_depths = 1,
    handlebars_compiler_flag_string_params = 2,
    handlebars_compiler_flag_track_ids = 4,
    
    // Result flags
    handlebars_compiler_flag_use_partial = 16,
    handlebars_compiler_flag_is_simple = 32,
    
    // Composite option flags
    handlebars_compiler_flag_compat = 1,
    handlebars_compiler_flag_all = 7
};

struct handlebars_compiler {
    
    // @todo opcodes children depths
    
    // Symbol counter
    size_t guid;
    
    // Flags
    int flags;
    
    // Option flags
    short string_params;
    short track_ids;
    short use_depths;
    
    // Result flags
    short is_simple;
    short use_partial;
};

/**
 * @brief Construct a compiler context object.
 * 
 * @return the compiler context pointer
 */
struct handlebars_compiler * handlebars_compiler_ctor(void);

/**
 * @brief Free a compiler context and it's resouces.
 * 
 * @param[in] compiler
 * @return void
 */
void handlebars_compiler_dtor(struct handlebars_compiler * compiler);

/**
 * @brief Get the compiler flags.
 * 
 * @param[in] compiler
 * @return the compiler flags
 */
int handlebars_compiler_get_flags(struct handlebars_compiler * compiler);

/**
 * @brief Set the compiler flags, with handlebars_compiler_flag_all as a 
 *        mask.
 * 
 * @param[in] compiler
 * @param[in] flags
 * @return void
 */
void handlebars_compiler_set_flags(struct handlebars_compiler * compiler, int flags);

/**
 * @brief Get a string for the integral opcode type. Should match the 
 *        handlebars format (camel case)
 * 
 * @param[in] type The integral opcode type
 * @return The string name of the opcode
 */
const char * handlebars_opcode_readable_type(enum handlebars_opcode_type type);

#ifdef	__cplusplus
}
#endif

#endif
