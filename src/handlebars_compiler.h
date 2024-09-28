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
 * @brief Compiler
 */

#ifndef HANDLEBARS_COMPILER_H
#define HANDLEBARS_COMPILER_H

#include "handlebars.h"

HBS_EXTERN_C_START

#define HANDLEBARS_COMPILER_STACK_SIZE 64

struct handlebars_ast_node;
struct handlebars_compiler;
struct handlebars_context;
struct handlebars_opcode;
struct handlebars_parser;
struct handlebars_program;

/**
 * @brief Flags to control compiler behaviour
 */
enum handlebars_compiler_flag {
    /**
     * @brief No flags
     */
    handlebars_compiler_flag_none = 0,

    // Option flags

    /**
     * @brief Track depths
     */
    handlebars_compiler_flag_use_depths = (1 << 0),

    /**
     * @brief Stringify parameters
     */
    handlebars_compiler_flag_string_params = (1 << 1),

    /**
     * @brief Track IDs
     */
    handlebars_compiler_flag_track_ids = (1 << 2),

    /**
     * @brief Disable all escaping
     */
    handlebars_compiler_flag_no_escape = (1 << 3),

    /**
     * @brief Only allow known helpers
     */
    handlebars_compiler_flag_known_helpers_only = (1 << 4),

    /**
     * @brief Prevent partial indent
     */
    handlebars_compiler_flag_prevent_indent = (1 << 5),

    handlebars_compiler_flag_use_data = (1 << 6),

    handlebars_compiler_flag_explicit_partial_context = (1 << 7),

    handlebars_compiler_flag_ignore_standalone = (1 << 8),

    handlebars_compiler_flag_alternate_decorators = (1 << 9),

    handlebars_compiler_flag_strict = (1 << 10),

    handlebars_compiler_flag_assume_objects = (1 << 11),

    handlebars_compiler_flag_mustache_style_lambdas = (1 << 12),

    // Composite option flags

    /**
     * @brief Mustache compatibility
     */
    handlebars_compiler_flag_compat = (1 << 0),

    /**
     * @brief All flags
     */
    handlebars_compiler_flag_all = ((1 << 13) - 1)
};

enum handlebars_compiler_result_flag {
    handlebars_compiler_result_flag_use_depths = (1 << 0),
    handlebars_compiler_result_flag_use_partial = (1 << 1),
    handlebars_compiler_result_flag_is_simple = (1 << 2),
    handlebars_compiler_result_flag_use_decorators = (1 << 3),
    /**
     * @brief All flags
     */
    handlebars_compiler_result_flag_all = ((1 << 4) - 1)
};

extern const size_t HANDLEBARS_COMPILER_SIZE;
extern const size_t HANDLEBARS_PROGRAM_SIZE;

struct handlebars_program * handlebars_compiler_compile_ex(
    struct handlebars_compiler * compiler,
    struct handlebars_ast_node * node
) HBS_ATTR_NONNULL_ALL HBS_ATTR_WARN_UNUSED_RESULT;

/**
 * @brief Main compile function. Compiles an AST
 *
 * @param[in] compiler The compiler context
 * @param[in] node The AST node to compile
 */
void handlebars_compiler_compile(
    struct handlebars_compiler * compiler,
    struct handlebars_ast_node * node
) HBS_ATTR_NONNULL_ALL;

// {{{ Constructors and Destructors

/**
 * @brief Construct a compiler context object.
 *
 * @param[in] context The handlebars context
 * @return the compiler context pointer
 */
struct handlebars_compiler * handlebars_compiler_ctor(
    struct handlebars_context * context
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

/**
 * @brief Free a compiler context and it's resources.
 *
 * @param[in] compiler
 * @return void
 */
void handlebars_compiler_dtor(
    struct handlebars_compiler * compiler
) HBS_ATTR_NONNULL_ALL;

// }}} Constructors and Destructors

// {{{ Getters

/**
 * @brief Get the compiler flags.
 *
 * @param[in] compiler
 * @return the compiler flags
 */
unsigned long handlebars_compiler_get_flags(
    struct handlebars_compiler * compiler
) HBS_ATTR_NONNULL_ALL;

const char ** handlebars_compiler_get_known_helpers(
    struct handlebars_compiler * compiler
);

struct handlebars_program * handlebars_compiler_get_program(
    struct handlebars_compiler * compiler
) HBS_ATTR_NONNULL_ALL;

// }}} Getters

// {{{ Mutators

/**
 * @brief Set the compiler flags, with handlebars_compiler_flag_all as a mask.
 *
 * @param[in] compiler
 * @param[in] flags
 * @return void
 */
void handlebars_compiler_set_flags(
    struct handlebars_compiler * compiler,
    unsigned long flags
) HBS_ATTR_NONNULL_ALL;

void handlebars_compiler_set_known_helpers(
    struct handlebars_compiler * compiler,
    const char ** known_helpers
) HBS_ATTR_NONNULL_ALL;

// }}} Mutators

// {{{ Extern for test suite only

/**
 * @brief Get an array of parts of an ID AST node.
 *
 * @param[in] compiler The handlebars compiler
 * @param[in] ast_node The AST node
 * @return The string array
 */
struct handlebars_string ** handlebars_ast_node_get_id_parts(
    struct handlebars_compiler * compiler,
    struct handlebars_ast_node * ast_node
) HBS_TEST_PUBLIC HBS_ATTR_NONNULL_ALL;

bool handlebars_compiler_is_known_helper(
    struct handlebars_compiler * compiler,
    struct handlebars_ast_node * path
) HBS_TEST_PUBLIC HBS_ATTR_NONNULL_ALL;

void handlebars_compiler_opcode(
    struct handlebars_compiler * compiler,
    struct handlebars_opcode * opcode
) HBS_TEST_PUBLIC HBS_ATTR_NONNULL_ALL;

// }}} Extern for test suite only

#ifdef HANDLEBARS_COMPILER_PRIVATE

struct handlebars_program {
    struct handlebars_program * main;

    long guid;

    /**
     * @brief Compiler flags
     */
    unsigned long flags;

    /**
     * @brief Result flags
     */
    int result_flags;

    /**
     * @brief Number of block params used
     */
    int block_params;

    struct handlebars_opcode ** opcodes;
    size_t opcodes_length;
    size_t opcodes_size;

    struct handlebars_program ** children;
    size_t children_length;
    size_t children_size;

    struct handlebars_program ** decorators;
    size_t decorators_length;
    size_t decorators_size;

    struct handlebars_program ** programs;
    size_t programs_index;
};

#endif /* HANDLEBARS_COMPILER_PRIVATE */

HBS_EXTERN_C_END

#endif /* HANDLEBARS_COMPILER_H */
