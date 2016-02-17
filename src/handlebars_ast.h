
/**
 * @file
 * @brief AST Node Tree 
 */

#ifndef HANDLEBARS_AST_H
#define HANDLEBARS_AST_H

#include "handlebars.h"

#ifdef	__cplusplus
extern "C" {
#endif

/**
 * Declarations
 */
struct handlebars_context;
struct handlebars_ast_node;
struct handlebars_ast_list;

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

struct handlebars_ast_node_block {
    struct handlebars_ast_node * path;
    struct handlebars_ast_list * params;
    struct handlebars_ast_node * hash;
    struct handlebars_ast_node * program;
    struct handlebars_ast_node * inverse;
    unsigned open_strip;
    unsigned inverse_strip;
    unsigned close_strip;
    bool is_decorator;
};

struct handlebars_ast_node_comment {
    char * value;
};

struct handlebars_ast_node_hash {
    struct handlebars_ast_list * pairs;
};

struct handlebars_ast_node_hash_pair {
    char * key;
    struct handlebars_ast_node * value;
};

struct handlebars_ast_node_intermediate {
    struct handlebars_ast_node * path;
    struct handlebars_ast_list * params;
    struct handlebars_ast_node * hash;
    char * block_param1;
    char * block_param2;
    char * open;
};

struct handlebars_ast_node_inverse {
    struct handlebars_ast_node * program;
	bool chained;
};

struct handlebars_ast_node_literal {
    char * value;
    char * original;
};

struct handlebars_ast_node_mustache {
    struct handlebars_ast_node * path;
    struct handlebars_ast_list * params;
    struct handlebars_ast_node * hash;
	bool unescaped;
	bool is_decorator;
};

struct handlebars_ast_node_partial {
    struct handlebars_ast_node * name;
    struct handlebars_ast_list * params;
    struct handlebars_ast_node * hash;
    char * indent;
};

struct handlebars_ast_node_path {
    char * original;
    struct handlebars_ast_list * parts;
    int depth;
	bool data;
	bool falsy;
	bool strict;
};

struct handlebars_ast_node_path_segment {
    char * part;
    char * separator;
    char * original;
};

struct handlebars_ast_node_program {
    struct handlebars_ast_list * statements;
    char * block_param1;
    char * block_param2;
	bool chained;
};

struct handlebars_ast_node_sexpr {
    struct handlebars_ast_node * path;
    struct handlebars_ast_list * params;
    struct handlebars_ast_node * hash;
    //bool is_helper;
    //bool eligible_helper;
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
  unsigned strip;
  
  /**
   * @brief Stores info about location
   */
  struct YYLTYPE loc;
  
  /**
   * @brief A union with structs of the different node types
   */
  union handlebars_ast_internals node;
};

/**
 * @brief Contruct an AST node
 *
 * @param[in] ctx The handlebars context
 * @param[in] type The AST node type
 * @return the newly constructed AST node
 */
struct handlebars_ast_node * handlebars_ast_node_ctor(
		struct handlebars_parser * parser, enum handlebars_ast_node_type type) HBSARN;

/**
 * @brief Destruct an AST node
 * 
 * @param[in] ast_node The AST node to destruct
 * @return void
 */
void handlebars_ast_node_dtor(struct handlebars_ast_node * ast_node);

/**
 * @brief Get the first part of an ID name of an AST node. Returns NULL if not 
 * applicable. Returns a pointer to the current buffer.
 * 
 * @param[in] ast_node The AST node
 * @return The string
 */
const char * handlebars_ast_node_get_id_part(struct handlebars_ast_node * ast_node);

/**
 * @brief Get the string mode value of an AST node. Returns NULL if not 
 * applicable. Returns a pointer to the current buffer.
 * 
 * @param[in] ast_node The AST node
 * @return The string
 */
const char * handlebars_ast_node_get_string_mode_value(struct handlebars_ast_node * ast_node) HBSARN;

struct handlebars_ast_node * handlebars_ast_node_get_path(struct handlebars_ast_node * node);

struct handlebars_ast_list * handlebars_ast_node_get_params(struct handlebars_ast_node * node);

struct handlebars_ast_node * handlebars_ast_node_get_hash(struct handlebars_ast_node * node);

/**
 * @brief Get a string for the integral AST node type
 * 
 * @param[in] type The integral AST node type
 * @return The string name of the type
 */
const char * handlebars_ast_node_readable_type(int type);

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
) HBSARN;
    
struct handlebars_ast_node * handlebars_ast_node_ctor_boolean(
    struct handlebars_parser * parser,
    const char * boolean,
    struct handlebars_locinfo * locinfo
) HBSARN;

struct handlebars_ast_node * handlebars_ast_node_ctor_comment(
    struct handlebars_parser * parser,
    const char * comment,
    struct handlebars_locinfo * locinfo
) HBSARN;

struct handlebars_ast_node * handlebars_ast_node_ctor_content(
    struct handlebars_parser * parser,
    const char * content,
    struct handlebars_locinfo * locinfo
) HBSARN;

struct handlebars_ast_node * handlebars_ast_node_ctor_hash_pair(
    struct handlebars_parser * parser,
    const char * key,
    struct handlebars_ast_node * value,
    struct handlebars_locinfo * locinfo
) HBSARN;

struct handlebars_ast_node * handlebars_ast_node_ctor_intermediate(
	    struct handlebars_parser * parser,
        struct handlebars_ast_node * path,
	    struct handlebars_ast_list * params,
        struct handlebars_ast_node * hash,
	    unsigned strip,
        struct handlebars_locinfo * locinfo
) HBSARN;

struct handlebars_ast_node * handlebars_ast_node_ctor_inverse(
	    struct handlebars_parser * parser,
        struct handlebars_ast_node * program,
		bool chained,
        unsigned strip,
        struct handlebars_locinfo * locinfo
) HBSARN;

struct handlebars_ast_node * handlebars_ast_node_ctor_null(
    struct handlebars_parser * parser,
    struct handlebars_locinfo * locinfo
) HBSARN;

struct handlebars_ast_node * handlebars_ast_node_ctor_number(
    struct handlebars_parser * parser,
    const char * number,
    struct handlebars_locinfo * locinfo
) HBSARN;

struct handlebars_ast_node * handlebars_ast_node_ctor_partial(
    struct handlebars_parser * parser,
    struct handlebars_ast_node * partial_name,
    struct handlebars_ast_list * params,
    struct handlebars_ast_node * hash,
    unsigned strip,
    struct handlebars_locinfo * locinfo
) HBSARN;

struct handlebars_ast_node * handlebars_ast_node_ctor_partial_block(
    struct handlebars_parser * parser,
    struct handlebars_ast_node * open,
    struct handlebars_ast_node * program,
    struct handlebars_ast_node * close,
    struct handlebars_locinfo * locinfo
) HBSARN;

struct handlebars_ast_node * handlebars_ast_node_ctor_program(
    struct handlebars_parser * parser,
    struct handlebars_ast_list * statements,
    char * block_param1,
    char * block_param2,
    unsigned strip,
	bool chained,
    struct handlebars_locinfo * locinfo
) HBSARN;

struct handlebars_ast_node * handlebars_ast_node_ctor_path(
    struct handlebars_parser * parser,
    struct handlebars_ast_list * parts,
    char * original,
    int depth,
    bool data,
    struct handlebars_locinfo * locinfo
) HBSARN;
    
struct handlebars_ast_node * handlebars_ast_node_ctor_path_segment(
    struct handlebars_parser * parser,
    const char * part,
    const char * separator,
    struct handlebars_locinfo * locinfo
) HBSARN;

struct handlebars_ast_node * handlebars_ast_node_ctor_raw_block(
    struct handlebars_parser * parser,
    struct handlebars_ast_node * intermediate,
    struct handlebars_ast_node * content,
    struct handlebars_locinfo * locinfo
) HBSARN;

struct handlebars_ast_node * handlebars_ast_node_ctor_sexpr(
    struct handlebars_parser * parser,
    struct handlebars_ast_node * intermediate,
    struct handlebars_locinfo * locinfo
) HBSARN;

struct handlebars_ast_node * handlebars_ast_node_ctor_string(
    struct handlebars_parser * parser,
    const char * string,
    struct handlebars_locinfo * locinfo
) HBSARN;

struct handlebars_ast_node * handlebars_ast_node_ctor_undefined(
    struct handlebars_parser * parser,
    struct handlebars_locinfo * locinfo
) HBSARN;

#ifdef	__cplusplus
}
#endif

#endif
