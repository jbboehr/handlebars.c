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

/**
 * @file
 * @brief Functions for printing opcodes into a human-readable string
 */

#ifndef HANDLEBARS_OPCODE_PRINTER_H
#define HANDLEBARS_OPCODE_PRINTER_H

#include "handlebars.h"

HBS_EXTERN_C_START

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
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

struct handlebars_string * handlebars_operand_print(
    struct handlebars_context * context,
    struct handlebars_operand * operand
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

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
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

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
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

/**
 * @brief Print a program
 *
 * @param[in] context The handlebars conteext
 * @param[in] program The program to print
 * @param[in] flags
 * @return void
 */
struct handlebars_string * handlebars_program_print(
    struct handlebars_context * context,
    struct handlebars_program * program,
    int flags
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

HBS_EXTERN_C_END

#endif /* HANDLEBARS_OPCODE_PRINTER_H */
