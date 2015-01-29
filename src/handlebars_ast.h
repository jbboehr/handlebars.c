
#ifndef HANDLEBARS_AST_H
#define HANDLEBARS_AST_H

#include <stdlib.h>

#ifdef	__cplusplus
extern "C" {
#endif

/**
 * Pre-declarations
 */
struct YYLTYPE;
struct handlebars_context;
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
  HANDLEBARS_AST_NODE_HASH_SEGMENT,
  HANDLEBARS_AST_NODE_ID,
  HANDLEBARS_AST_NODE_PARTIAL_NAME,
  HANDLEBARS_AST_NODE_DATA,
  HANDLEBARS_AST_NODE_STRING,
  HANDLEBARS_AST_NODE_NUMBER,
  HANDLEBARS_AST_NODE_BOOLEAN,
  HANDLEBARS_AST_NODE_COMMENT,
  HANDLEBARS_AST_NODE_PATH_SEGMENT
};

struct handlebars_ast_node_program {
  struct handlebars_ast_list * statements;
  // Todo: strip
};

struct handlebars_ast_node_mustache {
  struct handlebars_ast_node * sexpr;
  int escaped;
  // Deprecated: id params hash eligibleHelper isHelper
};

struct handlebars_ast_node_sexpr {
  struct handlebars_ast_node * hash;
  struct handlebars_ast_node * id;
  struct handlebars_ast_list * params;
  // Todo: isHelper eligibleHelper 
};

struct handlebars_ast_node_partial {
  struct handlebars_ast_node * partial_name;
  struct handlebars_ast_node * context;
  struct handlebars_ast_node * hash;
  // Todo: strip
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
  // Todo: original
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
  size_t id_name_length;
  char * id_name;
  size_t string_length;
  char * string;
  size_t original_length;
  char * original;
  // Todo: stringModeValue
};

struct handlebars_ast_node_partial_name {
  struct handlebars_ast_node * name;
};

struct handlebars_ast_node_data {
  struct handlebars_ast_node * id;
  // Todo: stringModeValue idName
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

struct handlebars_ast_node * handlebars_ast_node_ctor(enum handlebars_node_type type, void * ctx);
void handlebars_ast_node_dtor(struct handlebars_ast_node * ast_node);

int handlebars_ast_node_id_init(struct handlebars_ast_node * ast_node, void * ctx);
int handlebars_check_open_close(struct handlebars_ast_node * ast_node, struct handlebars_context * context, struct YYLTYPE * yylloc);
int handlebars_check_raw_open_close(struct handlebars_ast_node * ast_node, struct handlebars_context * context, struct YYLTYPE * yylloc);

#ifdef	__cplusplus
}
#endif

#endif
