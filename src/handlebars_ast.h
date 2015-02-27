
/**
 * @file
 * @brief AST Node Tree 
 */

#ifndef HANDLEBARS_AST_H
#define HANDLEBARS_AST_H

#include <stddef.h>

#ifdef	__cplusplus
extern "C" {
#endif

/**
 * Declarations
 */
struct YYLTYPE;
struct handlebars_context;
struct handlebars_ast_node;
struct handlebars_ast_list;

/**
 * @brief An enumeration of AST node types
 */
enum handlebars_ast_node_type
{
  HANDLEBARS_AST_NODE_NIL = 0,
  HANDLEBARS_AST_NODE_PROGRAM,
  HANDLEBARS_AST_NODE_MUSTACHE,
  HANDLEBARS_AST_NODE_SEXPR,
  HANDLEBARS_AST_NODE_PARTIAL,
  HANDLEBARS_AST_NODE_BLOCK,
  HANDLEBARS_AST_NODE_RAW_BLOCK,
  HANDLEBARS_AST_NODE_CONTENT,
  HANDLEBARS_AST_NODE_HASH,
  HANDLEBARS_AST_NODE_HASH_SEGMENT,
  HANDLEBARS_AST_NODE_ID,
  HANDLEBARS_AST_NODE_PARTIAL_NAME,
  HANDLEBARS_AST_NODE_DATA,
  HANDLEBARS_AST_NODE_STRING,
  HANDLEBARS_AST_NODE_NUMBER,
  HANDLEBARS_AST_NODE_BOOLEAN,
  HANDLEBARS_AST_NODE_COMMENT,
  HANDLEBARS_AST_NODE_PATH_SEGMENT,
  HANDLEBARS_AST_NODE_INVERSE_AND_PROGRAM
};

struct handlebars_ast_node_program {
  struct handlebars_ast_list * statements;
  // Todo: strip
};

struct handlebars_ast_node_mustache {
  struct handlebars_ast_node * sexpr;
  short unescaped;
  // Deprecated: id params hash eligibleHelper isHelper
};

struct handlebars_ast_node_sexpr {
  struct handlebars_ast_node * hash;
  struct handlebars_ast_node * id;
  struct handlebars_ast_list * params;
  short is_helper;
  short eligible_helper;
};

struct handlebars_ast_node_partial {
  struct handlebars_ast_node * partial_name;
  struct handlebars_ast_node * context;
  struct handlebars_ast_node * hash;
  char * indent;
};

struct handlebars_ast_node_block {
  struct handlebars_ast_node * mustache;
  struct handlebars_ast_node * program;
  struct handlebars_ast_node * inverse;
  struct handlebars_ast_node * close;
  int inverted;
  // Todo: strip isInverse
};

struct handlebars_ast_node_raw_block {
  struct handlebars_ast_node * mustache;
  struct handlebars_ast_node * program;
  char * close;
  // Note: program is just content
};

struct handlebars_ast_node_content {
  char * string;
  size_t length;
  char * original;
};

struct handlebars_ast_node_hash {
  struct handlebars_ast_list * segments;
};

struct handlebars_ast_node_hash_segment {
  char * key;
  size_t key_length;
  struct handlebars_ast_node * value;
};

struct handlebars_ast_node_id {
  struct handlebars_ast_list * parts;
  int depth;
  int is_simple;
  int is_scoped;
  short is_falsy; /* used in the compiler */
  size_t id_name_length;
  char * id_name;
  size_t string_length;
  char * string;
  size_t original_length;
  char * original;
};

struct handlebars_ast_node_partial_name {
  struct handlebars_ast_node * name;
};

struct handlebars_ast_node_data {
  struct handlebars_ast_node * id;
  size_t id_name_length;
  char * id_name;
};

struct handlebars_ast_node_string {
  char * string;
  size_t length;
};

struct handlebars_ast_node_number {
  char * string;
  size_t length;
};

struct handlebars_ast_node_boolean {
  char * string;
  size_t length;
};

struct handlebars_ast_node_comment {
  char * comment;
  size_t length;
  // Todo: strip
};

struct handlebars_ast_node_path_segment {
  char * part;
  size_t part_length;
  char * separator;
  size_t separator_length;
};

struct handlebars_ast_node_inverse_and_program {
  struct handlebars_ast_node * program;
};

union handlebars_ast_internals {
    struct handlebars_ast_node_block block;
    struct handlebars_ast_node_boolean boolean;
    struct handlebars_ast_node_comment comment;
    struct handlebars_ast_node_content content;
    struct handlebars_ast_node_data data;
    struct handlebars_ast_node_hash hash;
    struct handlebars_ast_node_hash_segment hash_segment;
    struct handlebars_ast_node_id id;
    struct handlebars_ast_node_mustache mustache;
    struct handlebars_ast_node_number number;
    struct handlebars_ast_node_partial partial;
    struct handlebars_ast_node_partial_name partial_name;
    struct handlebars_ast_node_path_segment path_segment;
    struct handlebars_ast_node_program program;
    struct handlebars_ast_node_raw_block raw_block;
    struct handlebars_ast_node_sexpr sexpr;
    struct handlebars_ast_node_string string;
    struct handlebars_ast_node_inverse_and_program inverse_and_program;
};

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
  unsigned strip;
  
  /**
   * @brief A union with structs of the different node types
   */
  union handlebars_ast_internals node;
};

/**
 * @brief Contruct an AST node
 * 
 * @param[in] type The AST node type
 * @param[in] ctx The talloc context on which to allocate
 * @return the newly constructed AST node
 */
struct handlebars_ast_node * handlebars_ast_node_ctor(enum handlebars_ast_node_type type, void * ctx);

/**
 * @brief Destruct an AST node
 * 
 * @param[in] ast_node The AST node to destruct
 * @return void
 */
void handlebars_ast_node_dtor(struct handlebars_ast_node * ast_node);

/**
 * @brief Get the ID name of an AST node. Returns NULL if not 
 * applicable. Returns a pointer to the current buffer.
 * 
 * @param[in] ast_node The AST node
 * @return The string
 */
const char * handlebars_ast_node_get_id_name(struct handlebars_ast_node * ast_node);

/**
 * @brief Get the first part of an ID name of an AST node. Returns NULL if not 
 * applicable. Returns a pointer to the current buffer.
 * 
 * @param[in] ast_node The AST node
 * @return The string
 */
const char * handlebars_ast_node_get_id_part(struct handlebars_ast_node * ast_node);

/**
 * @brief Get an array of parts of an ID AST node.
 * 
 * @param[in] ast_node The AST node
 * @return The string array
 */
char ** handlebars_ast_node_get_id_parts(void * ctx, struct handlebars_ast_node * ast_node);

/**
 * @brief Get the string mode value of an AST node. Returns NULL if not 
 * applicable. Returns a pointer to the current buffer.
 * 
 * @param[in] ast_node The AST node
 * @return The string
 */
const char * handlebars_ast_node_get_string_mode_value(struct handlebars_ast_node * ast_node);

/**
 * @brief Get a string for the integral AST node type
 * 
 * @param[in] type The integral AST node type
 * @return The string name of the type
 */
const char * handlebars_ast_node_readable_type(int type);

#ifdef	__cplusplus
}
#endif

#endif
