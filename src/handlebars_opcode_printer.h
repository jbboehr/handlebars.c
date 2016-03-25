
/**
 * @file
 * @brief Functions for printing opcodes into a human-readable string
 */

#ifndef HANDLEBARS_OPCODE_PRINTER_H
#define HANDLEBARS_OPCODE_PRINTER_H

#include "handlebars.h"

#ifdef	__cplusplus
extern "C" {
#endif

// Declarations
struct handlebars_opcode;
struct handlebars_operand;
struct handlebars_compiler;

/**
 * @brief Flags to control opcode printer behaviour
 */
enum handlebars_opcode_printer_flag {
    /**
     * @brief Default printer behaviour
     */
    handlebars_opcode_printer_flag_none = 0,

    /**
     * @brief Join with spaces instead of newlines
     */
    handlebars_opcode_printer_flag_no_newlines = (1 << 0),

    /**
     * @brief Dump all operands (not implemented)
     */
    handlebars_opcode_printer_flag_dump_all_operands = (1 << 1),
    
    /**
     * @brief Print locations
     */
    handlebars_opcode_printer_flag_locations = (1 << 2),
    
    handlebars_opcode_printer_flag_all = (1 << 3) - 1
};

/**
 * @brief Print an operand and append it to the specified buffer
 *
 * @param[in] context The handlebars context
 * @param[in] string The string to which to append
 * @param[in] operand The operand to print
 * @return The original pointer, or a new pointer if reallocated
 */
struct handlebars_string * handlebars_operand_print_append(
    struct handlebars_context * context,
    struct handlebars_string * string,
    struct handlebars_operand * operand
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL;

struct handlebars_string * handlebars_operand_print(
    struct handlebars_context * context,
    struct handlebars_operand * operand
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL;

/**
 * @brief Print an opcode and append it to the specified buffer
 *
 * @param[in] context The handlebars context
 * @param[in] string The string to which to append
 * @param[in] opcode The opcode to print
 * @param[in] flags The print flags
 * @return The original pointer, or a new pointer if reallocated
 */
struct handlebars_string * handlebars_opcode_print_append(
    struct handlebars_context * context,
    struct handlebars_string * string,
    struct handlebars_opcode * opcode,
    int flags
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL;

/**
 * @brief Print an opcode and return the string
 *
 * @param[in] context The handlebars context
 * @param[in] opcode The opcode to print
 * @param[in] flags The print flags
 * @return The printed opcode
 */
struct handlebars_string * handlebars_opcode_print(
    struct handlebars_context * context,
    struct handlebars_opcode * opcode,
    int flags
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL;

/**
 * @brief Print a compiler context
 *
 * @param[in] compiler The compiler context
 * @param[in] flags
 * @return void
 */
struct handlebars_string * handlebars_compiler_print(
    struct handlebars_compiler * compiler,
    int flags
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL;

#ifdef	__cplusplus
}
#endif

#endif
