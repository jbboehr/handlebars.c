
#ifndef HANDLEBARS_AST_H
#define HANDLEBARS_AST_H

#ifdef	__cplusplus
extern "C" {
#endif

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
    
};

struct handlebars_ast_node_mustache {
    
};

struct handlebars_ast_node_sexpr {
    
};

struct handlebars_ast_node_partial {
    
};

struct handlebars_ast_node_block {
    
};

struct handlebars_ast_node_raw_block {
    
};

struct handlebars_ast_node_content {
    
};

struct handlebars_ast_node_hash {
    
};

struct handlebars_ast_node_id {
    
};

struct handlebars_ast_node_partial_name {
    
};

struct handlebars_ast_node_data {
    
};

struct handlebars_ast_node_string {
    
};

struct handlebars_ast_node_number {
    
};

struct handlebars_ast_node_boolean {
    
};

struct handlebars_ast_node_comment {
    
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
