
/**
 * @file
 * @brief Opcodes
 */

#ifndef HANDLEBARS_OPCODES_H
#define HANDLEBARS_OPCODES_H

#ifdef	__cplusplus
extern "C" {
#endif

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
    handlebars_operand_type_boolean = 1,
    handlebars_operand_type_long = 2,
    handlebars_operand_type_string = 3,
    handlebars_operand_type_array = 4
};

union handlebars_operand_internals {
    short boolval;
    long longval;   // these might only need int
    char * stringval;
    void * arrayval; // maybe concat to string for string arrays
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

struct handlebars_opcode * handlebars_opcode_ctor(
        void * ctx, enum handlebars_opcode_type type);

struct handlebars_opcode * handlebars_opcode_ctor_long(
        void * ctx, enum handlebars_opcode_type type, long arg);

struct handlebars_opcode * handlebars_opcode_ctor_string(
        void * ctx, enum handlebars_opcode_type type, const char * arg);

void handlebars_operand_set_boolval(struct handlebars_operand * operand, short arg);

void handlebars_operand_set_longval(struct handlebars_operand * operand, long arg);

int handlebars_operand_set_stringval(void * ctx, struct handlebars_operand * operand, const char * arg);

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
