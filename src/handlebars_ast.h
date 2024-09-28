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
 * @brief AST Node Tree
 */

#ifndef HANDLEBARS_AST_H
#define HANDLEBARS_AST_H

#include "handlebars.h"

HBS_EXTERN_C_START

/**
 * Declarations
 */
struct handlebars_context;
struct handlebars_ast_node;
struct handlebars_ast_list;
struct handlebars_parser;

extern const size_t HANDLEBARS_AST_NODE_SIZE;

/**
 * @brief An enumeration of AST node types
 */
enum handlebars_ast_node_type
{
    HANDLEBARS_AST_NODE_NIL = 0,
    HANDLEBARS_AST_NODE_BLOCK,
    HANDLEBARS_AST_NODE_BOOLEAN,
    HANDLEBARS_AST_NODE_COMMENT,
    HANDLEBARS_AST_NODE_CONTENT,
    HANDLEBARS_AST_NODE_HASH,
    HANDLEBARS_AST_NODE_HASH_PAIR,
    HANDLEBARS_AST_NODE_INTERMEDIATE,
    HANDLEBARS_AST_NODE_INVERSE,
    HANDLEBARS_AST_NODE_MUSTACHE,
    HANDLEBARS_AST_NODE_NUL,
    HANDLEBARS_AST_NODE_NUMBER,
    HANDLEBARS_AST_NODE_PARTIAL,
    HANDLEBARS_AST_NODE_PARTIAL_BLOCK,
    HANDLEBARS_AST_NODE_PATH,
    HANDLEBARS_AST_NODE_PATH_SEGMENT,
    HANDLEBARS_AST_NODE_PROGRAM,
    HANDLEBARS_AST_NODE_RAW_BLOCK,
    HANDLEBARS_AST_NODE_SEXPR,
    HANDLEBARS_AST_NODE_STRING,
    HANDLEBARS_AST_NODE_UNDEFINED
};

/**
 * @brief Contruct an AST node
 *
 * @param[in] context The handlebars context
 * @param[in] type The AST node type
 * @return the newly constructed AST node
 */
struct handlebars_ast_node * handlebars_ast_node_ctor(
    struct handlebars_context * context,
    enum handlebars_ast_node_type type
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

/**
 * @brief Destruct an AST node
 *
 * @param[in] ast_node The AST node to destruct
 * @return void
 */
void handlebars_ast_node_dtor(
    struct handlebars_ast_node * ast_node
) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Get the first part of an ID name of an AST node. Returns NULL if not
 * applicable. Returns a pointer to the current buffer.
 *
 * @param[in] ast_node The AST node
 * @return The string
 */
struct handlebars_string * handlebars_ast_node_get_id_part(
    struct handlebars_ast_node * ast_node
) HBS_TEST_PUBLIC HBS_ATTR_NONNULL_ALL;

/**
 * @brief Get the string mode value of an AST node. Returns NULL if not
 * applicable. Returns a pointer to the current buffer.
 *
 * @param[in] context The handlebars context
 * @param[in] ast_node The AST node
 * @return The string
 */
struct handlebars_string * handlebars_ast_node_get_string_mode_value(
	struct handlebars_context * context,
	struct handlebars_ast_node * ast_node
) HBS_TEST_PUBLIC HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL;

/**
 * @brief Get the path child of an AST node
 *
 * @param[in] node The parent node
 * @return The path AST node, or NULL
 */
struct handlebars_ast_node * handlebars_ast_node_get_path(
    struct handlebars_ast_node * node
) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Get the params child of an AST node
 *
 * @param[in] node The parent node
 * @return The params node list, or NULL
 */
struct handlebars_ast_list * handlebars_ast_node_get_params(
    struct handlebars_ast_node * node
) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Get the hash child of an AST node
 *
 * @param[in] node The parent node
 * @return The hash AST node, or NULL
 */
struct handlebars_ast_node * handlebars_ast_node_get_hash(
    struct handlebars_ast_node * node
) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Get a string for the integral AST node type
 * @param[in] type The integral AST node type
 * @return The string name of the type
 */
const char * handlebars_ast_node_readable_type(
    int type
) HBS_ATTR_RETURNS_NONNULL;

// Specialized constructors

struct handlebars_ast_node * handlebars_ast_node_ctor_block(
    struct handlebars_parser * parser,
    struct handlebars_ast_node * intermediate,
    struct handlebars_ast_node * program,
    struct handlebars_ast_node * inverse,
    unsigned open_strip,
    unsigned inverse_strip,
    unsigned close_strip,
    struct handlebars_locinfo * locinfo
) HBS_TEST_PUBLIC HBS_ATTR_NONNULL(1, 8) HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

struct handlebars_ast_node * handlebars_ast_node_ctor_boolean(
    struct handlebars_parser * parser,
    struct handlebars_string * boolean,
    struct handlebars_locinfo * locinfo
) HBS_TEST_PUBLIC HBS_ATTR_NONNULL(1, 3) HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

struct handlebars_ast_node * handlebars_ast_node_ctor_comment(
    struct handlebars_parser * parser,
	struct handlebars_string * comment,
    bool is_long,
    struct handlebars_locinfo * locinfo
) HBS_TEST_PUBLIC HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

struct handlebars_ast_node * handlebars_ast_node_ctor_content(
    struct handlebars_parser * parser,
	struct handlebars_string * content,
    struct handlebars_locinfo * locinfo
) HBS_TEST_PUBLIC HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

struct handlebars_ast_node * handlebars_ast_node_ctor_hash_pair(
    struct handlebars_parser * parser,
	struct handlebars_string * key,
    struct handlebars_ast_node * value,
    struct handlebars_locinfo * locinfo
) HBS_TEST_PUBLIC HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

struct handlebars_ast_node * handlebars_ast_node_ctor_intermediate(
    struct handlebars_parser * parser,
    struct handlebars_ast_node * path,
    struct handlebars_ast_list * params,
    struct handlebars_ast_node * hash,
    unsigned strip,
    struct handlebars_locinfo * locinfo
) HBS_TEST_PUBLIC HBS_ATTR_NONNULL(1, 6) HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

struct handlebars_ast_node * handlebars_ast_node_ctor_inverse(
    struct handlebars_parser * parser,
    struct handlebars_ast_node * program,
    bool chained,
    unsigned strip,
    struct handlebars_locinfo * locinfo
) HBS_TEST_PUBLIC HBS_ATTR_NONNULL(1, 5) HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

struct handlebars_ast_node * handlebars_ast_node_ctor_null(
    struct handlebars_parser * parser,
    struct handlebars_string * string,
    struct handlebars_locinfo * locinfo
) HBS_TEST_PUBLIC HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

struct handlebars_ast_node * handlebars_ast_node_ctor_number(
    struct handlebars_parser * parser,
	struct handlebars_string * number,
    struct handlebars_locinfo * locinfo
) HBS_TEST_PUBLIC HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

struct handlebars_ast_node * handlebars_ast_node_ctor_partial(
    struct handlebars_parser * parser,
    struct handlebars_ast_node * partial_name,
    struct handlebars_ast_list * params,
    struct handlebars_ast_node * hash,
    unsigned strip,
    struct handlebars_locinfo * locinfo
) HBS_TEST_PUBLIC HBS_ATTR_NONNULL(1, 6) HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

struct handlebars_ast_node * handlebars_ast_node_ctor_partial_block(
    struct handlebars_parser * parser,
    struct handlebars_ast_node * open,
    struct handlebars_ast_node * program,
    struct handlebars_ast_node * close,
    struct handlebars_locinfo * locinfo
) HBS_TEST_PUBLIC HBS_ATTR_NONNULL(1, 5) HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

struct handlebars_ast_node * handlebars_ast_node_ctor_program(
    struct handlebars_parser * parser,
    struct handlebars_ast_list * statements,
    struct handlebars_string * block_param1,
	struct handlebars_string * block_param2,
    unsigned strip,
	bool chained,
    struct handlebars_locinfo * locinfo
) HBS_TEST_PUBLIC HBS_ATTR_NONNULL(1, 7) HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

struct handlebars_ast_node * handlebars_ast_node_ctor_path(
    struct handlebars_parser * parser,
    struct handlebars_ast_list * parts,
	struct handlebars_string * original,
    unsigned int depth,
    bool data,
    struct handlebars_locinfo * locinfo
) HBS_TEST_PUBLIC HBS_ATTR_NONNULL(1, 3, 6) HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

struct handlebars_ast_node * handlebars_ast_node_ctor_path_segment(
    struct handlebars_parser * parser,
	struct handlebars_string * part,
	struct handlebars_string * separator,
    struct handlebars_locinfo * locinfo
) HBS_TEST_PUBLIC HBS_ATTR_NONNULL(1, 2, 4) HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

struct handlebars_ast_node * handlebars_ast_node_ctor_raw_block(
    struct handlebars_parser * parser,
    struct handlebars_ast_node * intermediate,
    struct handlebars_ast_node * content,
    struct handlebars_locinfo * locinfo
) HBS_TEST_PUBLIC HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

struct handlebars_ast_node * handlebars_ast_node_ctor_sexpr(
    struct handlebars_parser * parser,
    struct handlebars_ast_node * intermediate,
    struct handlebars_locinfo * locinfo
) HBS_TEST_PUBLIC HBS_ATTR_NONNULL(1, 3) HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

struct handlebars_ast_node * handlebars_ast_node_ctor_string(
    struct handlebars_parser * parser,
    struct handlebars_string * string,
    bool is_single_quoted,
    struct handlebars_locinfo * locinfo
) HBS_TEST_PUBLIC HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

struct handlebars_ast_node * handlebars_ast_node_ctor_undefined(
    struct handlebars_parser * parser,
    struct handlebars_string * string,
    struct handlebars_locinfo * locinfo
) HBS_TEST_PUBLIC HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

void handlebars_ast_node_set_strip(
    struct handlebars_ast_node * node,
    unsigned flag
) HBS_TEST_PUBLIC HBS_ATTR_NONNULL_ALL;

#ifdef HANDLEBARS_AST_PRIVATE

struct handlebars_ast_node_block {
    struct handlebars_ast_node * path;
    struct handlebars_ast_list * params;
    struct handlebars_ast_node * hash;
    struct handlebars_ast_node * program;
    struct handlebars_ast_node * inverse;
    unsigned int open_strip;
    unsigned int inverse_strip;
    unsigned int close_strip;
    bool is_decorator;
};

struct handlebars_ast_node_comment {
    struct handlebars_string * value;
    bool is_long;
};

struct handlebars_ast_node_hash {
    struct handlebars_ast_list * pairs;
};

struct handlebars_ast_node_hash_pair {
	struct handlebars_string * key;
    struct handlebars_ast_node * value;
};

struct handlebars_ast_node_intermediate {
    struct handlebars_ast_node * path;
    struct handlebars_ast_list * params;
    struct handlebars_ast_node * hash;
    struct handlebars_string * block_param1;
	struct handlebars_string * block_param2;
	struct handlebars_string * open;
};

struct handlebars_ast_node_inverse {
    struct handlebars_ast_node * program;
	bool chained;
};

struct handlebars_ast_node_literal {
    struct handlebars_string * value;
	struct handlebars_string * original;
    bool is_single_quoted;
};

struct handlebars_ast_node_mustache {
    struct handlebars_ast_node * path;
    struct handlebars_ast_list * params;
    struct handlebars_ast_node * hash;
    //! 0 for default, 1 for '{', 3 for '&'
    unsigned char unescaped;
    bool is_decorator;
};

struct handlebars_ast_node_partial {
    struct handlebars_ast_node * name;
    struct handlebars_ast_list * params;
    struct handlebars_ast_node * hash;
	struct handlebars_string * indent;
};

struct handlebars_ast_node_path {
    struct handlebars_string * original;
    struct handlebars_ast_list * parts;
    unsigned int depth;
	bool data;
	bool falsy;
	bool strict;
};

struct handlebars_ast_node_path_segment {
    struct handlebars_string * part;
    struct handlebars_string * separator;
    struct handlebars_string * original;
};

struct handlebars_ast_node_program {
    struct handlebars_ast_list * statements;
	struct handlebars_string * block_param1;
	struct handlebars_string * block_param2;
	bool chained;
};

struct handlebars_ast_node_sexpr {
    struct handlebars_ast_node * path;
    struct handlebars_ast_list * params;
    struct handlebars_ast_node * hash;
};

union handlebars_ast_internals {
    struct handlebars_ast_node_block block;
    struct handlebars_ast_node_literal boolean;
    struct handlebars_ast_node_comment comment;
    struct handlebars_ast_node_literal content;
    struct handlebars_ast_node_hash hash;
    struct handlebars_ast_node_hash_pair hash_pair;
    struct handlebars_ast_node_intermediate intermediate;
    struct handlebars_ast_node_inverse inverse;
    struct handlebars_ast_node_mustache mustache;
    struct handlebars_ast_node_literal number;
    struct handlebars_ast_node_literal nul;
    struct handlebars_ast_node_partial partial;
    struct handlebars_ast_node_block partial_block;
    struct handlebars_ast_node_path path;
    struct handlebars_ast_node_path_segment path_segment;
    struct handlebars_ast_node_program program;
    struct handlebars_ast_node_block raw_block;
    struct handlebars_ast_node_sexpr sexpr;
    struct handlebars_ast_node_literal string;
    struct handlebars_ast_node_literal undefined;
};

/**
 * @brief Flags to control and about whitespace control
 */
enum handlebars_ast_strip_flag {
  handlebars_ast_strip_flag_none = 0,
  handlebars_ast_strip_flag_set = (1 << 0),
  handlebars_ast_strip_flag_left = (1 << 1),
  handlebars_ast_strip_flag_right = (1 << 2),
  handlebars_ast_strip_flag_open_standalone = (1 << 3),
  handlebars_ast_strip_flag_close_standalone = (1 << 4),
  handlebars_ast_strip_flag_inline_standalone = (1 << 5),
  handlebars_ast_strip_flag_left_stripped = (1 << 6),
  handlebars_ast_strip_flag_right_stripped = (1 << 7)
};

/**
 * @brief The main AST node structure
 */
struct handlebars_ast_node {
  /**
   * @brief Enum describing the type of node
   */
  enum handlebars_ast_node_type type;

  /**
   * @brief Stores info about whitespace stripping
   */
  unsigned int strip;

  /**
   * @brief Stores info about location
   */
  struct handlebars_locinfo loc;

  /**
   * @brief A union with structs of the different node types
   */
  union handlebars_ast_internals node;
};

#endif /* HANDLEBARS_AST_PRIVATE */

HBS_EXTERN_C_END

#endif /* HANDLEBARS_AST_H */
