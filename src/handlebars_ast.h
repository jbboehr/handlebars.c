
#ifndef HANDLEBARS_AST_H
#define HANDLEBARS_AST_H

#include <stdlib.h>

#ifdef	__cplusplus
extern "C" {
#endif

/**
 * Pre-declarations
 */
struct handlebars_ast_node;
struct handlebars_ast_list;

enum handlebars_node_type {
  HANDLEBARS_AST_NODE_NULL = 0,
  HANDLEBARS_AST_NODE_PROGRAM,
  HANDLEBARS_AST_NODE_MUSTACHE,
  HANDLEBARS_AST_NODE_SEXPR,
  HANDLEBARS_AST_NODE_PARTIAL,
  HANDLEBARS_AST_NODE_BLOCK,
  HANDLEBARS_AST_NODE_RAW_BLOCK,
  HANDLEBARS_AST_NODE_CONTENT,
  HANDLEBARS_AST_NODE_HASH,
  HANDLEBARS_AST_NODE_ID,
  HANDLEBARS_AST_NODE_PARTIAL_NAME,
  HANDLEBARS_AST_NODE_DATA,
  HANDLEBARS_AST_NODE_STRING,
  HANDLEBARS_AST_NODE_NUMBER,
  HANDLEBARS_AST_NODE_BOOLEAN,
  HANDLEBARS_AST_NODE_COMMENT
};

struct handlebars_ast_node_program {
  struct handlebars_ast_list * statements;
  // Todo: strip
};

struct handlebars_ast_node_mustache {
  struct handlebars_node * sexpr;
  int escaped;
  // Deprecated: id params hash eligibleHelper isHelper
};

struct handlebars_ast_node_sexpr {
  // Todo: hash id params isHelper eligibleHelper 
};

struct handlebars_ast_node_partial {
  // Todo: partialName context hash strip
};

struct handlebars_ast_node_block {
  struct handlebars_node * mustache;
  struct handlebars_node * program;
  struct handlebars_node * inverse;
  // Todo: strip isInverse
};

struct handlebars_ast_node_raw_block {
  struct handlebars_node * mustache;
  struct handlebars_node * program;
  // Note: program is just content
};

struct handlebars_ast_node_content {
  char * string;
  size_t length;
  // Todo: original
};

struct handlebars_ast_node_hash {
  // Todo: pairs
};

struct handlebars_ast_node_hash_pair {
  char * key;
  size_t key_length;
  struct handlebars_node * value;
};

struct handlebars_ast_node_id {
  // Todo: original parts string depth idName isSimple isScoped stringModeValue
};

struct handlebars_ast_node_partial_name {
  // Todo: name
};

struct handlebars_ast_node_data {
  // Todo: id stringModeValue idName
};

struct handlebars_ast_node_string {
  char * string;
  size_t length;
  // Todo: stringModeValue
};

struct handlebars_ast_node_number {
  char * string;
  size_t length;
  // Todo: stringModeValue
};

struct handlebars_ast_node_boolean {
  char * string;
  size_t length;
  // Todo: stringModeValue
};

struct handlebars_ast_node_comment {
  // Todo: comment strip
};

union handlebars_ast_internals {
    struct handlebars_ast_node_program program;
    struct handlebars_ast_node_mustache mustache;
    struct handlebars_ast_node_sexpr sexpr;
    struct handlebars_ast_node_partial partial;
    struct handlebars_ast_node_block block;
    struct handlebars_ast_node_raw_block raw_block;
    struct handlebars_ast_node_content content;
    struct handlebars_ast_node_hash hash;
    struct handlebars_ast_node_hash_pair hash_pair;
    struct handlebars_ast_node_id id;
    struct handlebars_ast_node_partial_name partial_name;
    struct handlebars_ast_node_data data;
    struct handlebars_ast_node_string string;
    struct handlebars_ast_node_number number;
    struct handlebars_ast_node_boolean boolean;
    struct handlebars_ast_node_comment comment;
};

struct handlebars_ast_node {
  /**
   * Enum describing the type of node
   */
  enum handlebars_node_type type;
  
  /**
   * The guts
   */
  union handlebars_ast_internals node;
};

#ifdef	__cplusplus
}
#endif

#endif
