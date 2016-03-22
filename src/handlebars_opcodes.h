
/**
 * @file
 * @brief Opcodes
 */

#ifndef HANDLEBARS_OPCODES_H
#define HANDLEBARS_OPCODES_H

#include "handlebars.h"

#ifdef	__cplusplus
extern "C" {
#endif

struct handlebars_compiler;
struct handlebars_string;

/**
 * @brief Opcode types
 */
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
    handlebars_opcode_type_lookup_data = 24,
    
    // Added in v3
    handlebars_opcode_type_lookup_block_param = 25,

    // Added in v4
    handlebars_opcode_type_register_decorator = 26
};

/**
 * @brief Operand types
 */
enum handlebars_operand_type {
    handlebars_operand_type_null = 0,
    handlebars_operand_type_boolean = 1,
    handlebars_operand_type_long = 2,
    handlebars_operand_type_string = 3,
    handlebars_operand_type_array = 4
};

union handlebars_operand_internals {
    bool boolval;
    long longval;
    struct handlebars_string * string;
    struct handlebars_string ** array;
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
    struct handlebars_operand op4;
    struct handlebars_locinfo loc;
};

/*
struct handlebars_opcode_get_context {
    struct handlebars_opcode o;
    union {
        struct {
            long depth;
        } v;
        char padding[8];
    } u;
};
*/

/**
 * @brief Construct an opcode
 *
 * @param[in] compiler The handlebars compiler
 * @param[in] type The opcode type
 * @return The new opcode
 */
struct handlebars_opcode * handlebars_opcode_ctor(
        struct handlebars_compiler * compiler, enum handlebars_opcode_type type) HBS_ATTR_RETURNS_NONNULL;

/**
 * @brief Construct an opcode with a boolean operand
 *
 * @param[in] compiler The handlebars compiler
 * @param[in] type The opcode type
 * @param[in] arg The boolean value
 * @return The new opcode
 */
struct handlebars_opcode * handlebars_opcode_ctor_boolean(
        struct handlebars_compiler * compiler, enum handlebars_opcode_type type, bool arg) HBS_ATTR_RETURNS_NONNULL;

/**
 * @brief Construct an opcode with a long operand
 *
 * @param[in] compiler The handlebars compiler
 * @param[in] type The opcode type
 * @param[in] arg The long value
 * @return The new opcode
 */
struct handlebars_opcode * handlebars_opcode_ctor_long(
        struct handlebars_compiler * compiler, enum handlebars_opcode_type type, long arg) HBS_ATTR_RETURNS_NONNULL;

/**
 * @brief Construct an opcode with a long and a string operand
 *
 * @param[in] compiler The handlebars compiler
 * @param[in] type The opcode type
 * @param[in] arg1 The long value
 * @param[in] arg2 The string value
 * @return The new opcode
 */
struct handlebars_opcode * handlebars_opcode_ctor_long_string(
        struct handlebars_compiler * compiler, enum handlebars_opcode_type type, long arg1, struct handlebars_string * arg2) HBS_ATTR_RETURNS_NONNULL;

/**
 * @brief Construct an opcode with a string operand
 *
 * @param[in] compiler The handlebars compiler
 * @param[in] type The opcode type
 * @param[in] arg The string value
 * @return The new opcode
 */
struct handlebars_opcode * handlebars_opcode_ctor_string(
        struct handlebars_compiler * compiler, enum handlebars_opcode_type type, struct handlebars_string * arg) HBS_ATTR_RETURNS_NONNULL;

/**
 * @brief Construct an opcode with two string operands
 *
 * @param[in] compiler The handlebars compiler
 * @param[in] type The opcode type
 * @param[in] arg1 The first string value
 * @param[in] arg2 The second string value
 * @return The new opcode
 */
struct handlebars_opcode * handlebars_opcode_ctor_string2(
        struct handlebars_compiler * compiler, enum handlebars_opcode_type type, struct handlebars_string * arg1, struct handlebars_string * arg2) HBS_ATTR_RETURNS_NONNULL;

/**
 * @brief Construct an opcode with a string and a long operand
 *
 * @param[in] compiler The handlebars compiler
 * @param[in] type The opcode type
 * @param[in] arg1 The string value
 * @param[in] arg2 The long value
 * @return The new opcode
 */
struct handlebars_opcode * handlebars_opcode_ctor_string_long(
        struct handlebars_compiler * compiler, enum handlebars_opcode_type type, struct handlebars_string * arg1, long arg2) HBS_ATTR_RETURNS_NONNULL;

/**
 * @brief Set the value of an operand to null
 *
 * @param[in] operand The operand of which to change the value
 * @return void
 */
void handlebars_operand_set_null(struct handlebars_operand * operand);

/**
 * @brief Set the value of an operand to a boolean
 *
 * @param[in] operand The operand of which to change the value
 * @param[in] arg The boolean value
 * @return void
 */
void handlebars_operand_set_boolval(struct handlebars_operand * operand, bool arg);

/**
 * @brief Set the value of an operand to a long
 *
 * @param[in] operand The operand of which to change the value
 * @param[in] arg The long value
 * @return void
 */
void handlebars_operand_set_longval(struct handlebars_operand * operand, long arg);

/**
 * @brief Set the value of an operand to a string
 *
 * @param[in] compiler The handlebars compiler
 * @param[in] operand The operand of which to change the value
 * @param[in] arg The string value
 * @return void
 */
int handlebars_operand_set_stringval(struct handlebars_compiler * compiler, struct handlebars_operand * operand, struct handlebars_string * string);

int handlebars_operand_set_strval(struct handlebars_compiler * compiler, struct handlebars_operand * operand, const char * arg);


/**
 * @brief Set the value of an operand to an array
 *
 * @param[in] compiler The handlebars compiler
 * @param[in] operand The operand of which to change the value
 * @param[in] arg The array value
 * @return void
 */
int handlebars_operand_set_arrayval(struct handlebars_compiler * compiler, struct handlebars_operand * operand, const char ** arg);

/**
 * @brief Get a string for the integral opcode type. Should match the 
 *        handlebars format (camel case)
 * 
 * @param[in] type The integral opcode type
 * @return The string name of the opcode
 */
const char * handlebars_opcode_readable_type(enum handlebars_opcode_type type);

/**
 * @brief Get the integral type of an opcode given a string of its type name
 *
 * @param[in] type The string type name
 * @return The integral type
 */
enum handlebars_opcode_type handlebars_opcode_reverse_readable_type(const char * type);

/**
 * @brief Get the number of operands a particular opcode type should have
 *
 * @param[in] type The opcode type
 * @return The number of operands
 */
short handlebars_opcode_num_operands(enum handlebars_opcode_type type);

#ifdef	__cplusplus
}
#endif

#endif
