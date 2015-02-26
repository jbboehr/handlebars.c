
%defines
%name-prefix="handlebars_yy_"
%pure-parser
%error-verbose

%lex-param {
  void * scanner
}
%parse-param {
  struct handlebars_context * context
}
%locations

%code requires {
  struct handlebars_context; /* needed for bison 2.7 */
  #define YY_END_OF_BUFFER_CHAR 0
  #define YY_EXTRA_TYPE struct handlebars_context *
}

%start  start

%{

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "handlebars.h"
#include "handlebars_ast.h"
#include "handlebars_ast_helpers.h"
#include "handlebars_ast_list.h"
#include "handlebars_context.h"
#include "handlebars_memory.h"
#include "handlebars_utils.h"
#include "handlebars.tab.h"
#include "handlebars.lex.h"

#ifdef YYDEBUG
#define YYPRINT handlebars_yy_print
int handlebars_yy_debug = 1;
#else
int handlebars_yy_debug = 0;
#endif

#define __MEMCHECK(cond) \
  do { \
    if( !cond ) { \
      YYABORT; \
    } \
  } while(0)

#define scanner context->scanner
%}

%union {
  struct {
	  char * text;
	  size_t length;
	  struct handlebars_ast_node * ast_node;
    struct handlebars_ast_list * ast_list;
  };
}

%token END 0 "end of file"
%token <text> BOOLEAN
%token <text> CLOSE "}}"
%token <text> CLOSE_RAW_BLOCK "}}}}"
%token <text> CLOSE_SEXPR ")"
%token <text> CLOSE_UNESCAPED "}}}"
%token <text> COMMENT
%token <text> CONTENT
%token <text> DATA
%token <text> END_RAW_BLOCK // meh
%token <text> EQUALS "="
%token <text> ID
%token <text> INVALID
%token <text> INVERSE
%token <text> NUMBER
%token <text> OPEN "{{"
%token <text> OPEN_BLOCK "{{#"
%token <text> OPEN_ENDBLOCK
%token <text> OPEN_INVERSE "{{^"
%token <text> OPEN_PARTIAL "{{>"
%token <text> OPEN_RAW_BLOCK "{{{{"
%token <text> OPEN_SEXPR "("
%token <text> OPEN_UNESCAPED "{{{"
%token <text> SEP
%token <text> STRING

%type <ast_node> program
%type <ast_list> statements
%type <ast_node> statement
%type <ast_node> mustache
%type <ast_node> block
%type <ast_node> raw_block
%type <ast_node> partial
%type <ast_node> open_raw_block
%type <ast_node> sexpr
%type <ast_node> open_block
%type <ast_node> inverse_and_program
%type <ast_node> close_block
%type <ast_node> open_inverse
%type <ast_node> partial_name
%type <ast_node> param
%type <ast_node> hash
%type <ast_node> path
%type <ast_list> params
%type <ast_node> data_name
%type <ast_list> hash_segments
%type <ast_node> hash_segment
%type <ast_list> path_segments
%type <text> content

%left CLOSE CLOSE_UNESCAPED CONTENT END OPEN OPEN_BLOCK OPEN_ENDBLOCK
%left OPEN_INVERSE OPEN_PARTIAL OPEN_UNESCAPED SEP ID BOOLEAN COMMENT DATA 
%left EQUALS INTEGER STRING INVALID

%%

start : 
    program END {
      handlebars_ast_helper_prepare_program(context, $1, 1);
      context->program = $1;
      return 1;
    }
    ;

program :
    statements {
      $$ = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_PROGRAM, context);
      $$->node.program.statements = $1;
      handlebars_ast_helper_prepare_program(context, $$, 0);
    }
  | "" {
      $$ = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_PROGRAM, context);
      $$->node.program.statements = handlebars_ast_list_ctor($$);
  }
  ;

statements
  : statement {
      $$ = handlebars_ast_list_ctor(context);
      handlebars_ast_list_append($$, $1);
    }
  | statements statement {
      handlebars_ast_list_append($1, $2);
      $$ = $1;
    }
  ;

statement
  : mustache {
      $$ = $1;
    }
  | block {
      $$ = $1;
    }
  | raw_block {
      $$ = $1;
    }
  | partial {
      $$ = $1;
    }
  | content {
      struct handlebars_ast_node * ast_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_CONTENT, context);
      ast_node->node.content.string = $1;
      ast_node->node.content.length = strlen($1);
      ast_node->node.content.original = handlebars_talloc_strdup(ast_node, $1);
      $$ = ast_node;
    }
  | COMMENT {
      struct handlebars_ast_node * ast_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_COMMENT, context);
      ast_node->node.comment.comment = $1;
      ast_node->node.comment.length = strlen($1);
      $$ = ast_node;
    }
  ;


raw_block
  : open_raw_block content END_RAW_BLOCK {
      $$ = handlebars_ast_helper_prepare_raw_block(context, $1, $2, $3, &yylloc);
      __MEMCHECK($$);
    }
  ;

open_raw_block
  : OPEN_RAW_BLOCK sexpr CLOSE_RAW_BLOCK {
      struct handlebars_ast_node * ast_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_MUSTACHE, context);
      ast_node->node.mustache.sexpr = $2;
      $$ = ast_node;
    }
  ;

block
  : open_block program inverse_and_program close_block {
      $$ = handlebars_ast_helper_prepare_block(context, $1, $2, $3, $4, 0, &yylloc);
      __MEMCHECK($$);
    }
  | open_block inverse_and_program close_block {
      struct handlebars_ast_node * program = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_PROGRAM, context);
      $$ = handlebars_ast_helper_prepare_block(context, $1, program, $2, $3, 0, &yylloc);
      __MEMCHECK($$);
    }
  | open_block program close_block {
      $$ = handlebars_ast_helper_prepare_block(context, $1, $2, NULL, $3, 0, &yylloc);
      __MEMCHECK($$);
    }
  | open_block close_block {
      struct handlebars_ast_node * program = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_PROGRAM, context);
      $$ = handlebars_ast_helper_prepare_block(context, $1, program, NULL, $2, 0, &yylloc);
      __MEMCHECK($$);
    }
  | open_inverse program inverse_and_program close_block {
      $$ = handlebars_ast_helper_prepare_block(context, $1, $3, $2, $4, 1, &yylloc);
      __MEMCHECK($$);
    }
  | open_inverse inverse_and_program close_block {
      $$ = handlebars_ast_helper_prepare_block(context, $1, $2, NULL, $3, 1, &yylloc);
      __MEMCHECK($$);
    }
  | open_inverse program close_block {
      $$ = handlebars_ast_helper_prepare_block(context, $1, NULL, $2, $3, 1, &yylloc);
      __MEMCHECK($$);
    }
  | open_inverse close_block {
      $$ = handlebars_ast_helper_prepare_block(context, $1, NULL, NULL, $2, 1, &yylloc);
      __MEMCHECK($$);
    }
  ;

open_block
  : OPEN_BLOCK sexpr CLOSE {
      struct handlebars_ast_node * ast_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_MUSTACHE, context);
      ast_node->node.mustache.sexpr = $2;
      handlebars_ast_helper_set_strip_flags(ast_node, $1, $3);
      $$ = ast_node;
    }
  ;

open_inverse
  : OPEN_INVERSE sexpr CLOSE {
      struct handlebars_ast_node * ast_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_MUSTACHE, context);
      ast_node->node.mustache.sexpr = $2;
      handlebars_ast_helper_set_strip_flags(ast_node, $1, $3);
      $$ = ast_node;
    }
  ;

inverse_and_program
  : INVERSE program {
      // @todo might not be right
      $$ = $2;
      handlebars_ast_helper_set_strip_flags($$, $1, $1);
      // Original:
      // -> { strip: yy.stripFlags($1, $1), program: $2 }
    }
  | INVERSE {
      $$ = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_PROGRAM, context);
    }
  ;

close_block
  : OPEN_ENDBLOCK path CLOSE {
      // @todo might not be right
      $$ = $2;
      handlebars_ast_helper_set_strip_flags($$, $1, $3);
      // Original:
      //-> {path: $2, strip: yy.stripFlags($1, $3)}
    }
  ;

mustache
  : OPEN sexpr CLOSE {
      struct handlebars_ast_node * ast_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_MUSTACHE, context);
      ast_node->node.mustache.sexpr = $2;
      // @todo this won't work w/ whitespace
      if( $1 && strlen($1) >= 3 && *($1 + 2) == '&' ) {
        ast_node->node.mustache.unescaped = 1;
      }
      handlebars_ast_helper_set_strip_flags(ast_node, $1, $3);
      $$ = ast_node;
    }
  | OPEN_UNESCAPED sexpr CLOSE_UNESCAPED {
      struct handlebars_ast_node * ast_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_MUSTACHE, context);
      ast_node->node.mustache.sexpr = $2;
      ast_node->node.mustache.unescaped = 1;
      handlebars_ast_helper_set_strip_flags(ast_node, $1, $3);
      $$ = ast_node;
    }
  ;

partial
  : OPEN_PARTIAL partial_name param CLOSE {
      struct handlebars_ast_node * ast_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_PARTIAL, context);
      ast_node->node.partial.partial_name = $2;
      ast_node->node.partial.context = $3;
      handlebars_ast_helper_set_strip_flags(ast_node, $1, $4);
      $$ = ast_node;
    }
  | OPEN_PARTIAL partial_name param hash CLOSE {
      struct handlebars_ast_node * ast_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_PARTIAL, context);
      ast_node->node.partial.partial_name = $2;
      ast_node->node.partial.context = $3;
      ast_node->node.partial.hash = $4;
      handlebars_ast_helper_set_strip_flags(ast_node, $1, $5);
      $$ = ast_node;
    }
  | OPEN_PARTIAL partial_name CLOSE {
      struct handlebars_ast_node * ast_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_PARTIAL, context);
      ast_node->node.partial.partial_name = $2;
      handlebars_ast_helper_set_strip_flags(ast_node, $1, $3);
      $$ = ast_node;
    }
  | OPEN_PARTIAL partial_name hash CLOSE {
      struct handlebars_ast_node * ast_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_PARTIAL, context);
      ast_node->node.partial.partial_name = $2;
      ast_node->node.partial.hash = $3;
      handlebars_ast_helper_set_strip_flags(ast_node, $1, $4);
      $$ = ast_node;
    }
  ;

sexpr
  : path {
      $$ = handlebars_ast_helper_prepare_sexpr(context, $1, NULL, NULL);
      __MEMCHECK($$);
    }
  | path hash {
      $$ = handlebars_ast_helper_prepare_sexpr(context, $1, NULL, $2);
      __MEMCHECK($$);
    }
  | path params hash {
      $$ = handlebars_ast_helper_prepare_sexpr(context, $1, $2, $3);
      __MEMCHECK($$);
    }
  | path params {
      $$ = handlebars_ast_helper_prepare_sexpr(context, $1, $2, NULL);
      __MEMCHECK($$);
    }
  | data_name {
      $$ = handlebars_ast_helper_prepare_sexpr(context, $1, NULL, NULL);
      __MEMCHECK($$);
    }
  ;

params
  : params param {
      handlebars_ast_list_append($1, $2);
      $$ = $1;
    }
  | param {
      $$ = handlebars_ast_list_ctor(context);
      handlebars_ast_list_append($$, $1);
    }
  ;

param
  : path {
      $$ = $1;
    }
  | STRING {
      struct handlebars_ast_node * ast_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_STRING, context);
      ast_node->node.string.string = handlebars_talloc_strdup(ast_node, $1);
      ast_node->node.string.length = strlen($1);
      $$ = ast_node;
    }
  | NUMBER {
      struct handlebars_ast_node * ast_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_NUMBER, context);
      ast_node->node.number.string = handlebars_talloc_strdup(ast_node, $1);
      ast_node->node.number.length = strlen($1);
      $$ = ast_node;
    }
  | BOOLEAN {
      struct handlebars_ast_node * ast_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_BOOLEAN, context);
      ast_node->node.boolean.string = handlebars_talloc_strdup(ast_node, $1);
      ast_node->node.boolean.length = strlen($1);
      $$ = ast_node;
    }
  | data_name {
      $$ = $1;
    }
  | OPEN_SEXPR sexpr CLOSE_SEXPR {
      $$ = $2;
      $$->node.sexpr.is_helper = 1;
    }
  ;

hash
  : hash_segments {
      struct handlebars_ast_node * ast_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_HASH, context);
      ast_node->node.hash.segments = $1;
      $$ = ast_node;
    }
  ;

hash_segments
  : hash_segments hash_segment {
      handlebars_ast_list_append($1, $2);
      $$ = $1;
    }
  | hash_segment {
      $$ = handlebars_ast_list_ctor(context);
      handlebars_ast_list_append($$, $1);
    }
  ;

hash_segment
  : ID EQUALS param {
      struct handlebars_ast_node * ast_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_HASH_SEGMENT, context);
      ast_node->node.hash_segment.key = handlebars_talloc_strdup(ast_node, $1);
      ast_node->node.hash_segment.key_length = strlen(ast_node->node.hash_segment.key);
      ast_node->node.hash_segment.value = $3;
      $$ = ast_node;
    }
  ;

partial_name
  : path {
      struct handlebars_ast_node * ast_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_PARTIAL_NAME, context);
      ast_node->node.partial_name.name = $1;
      $$ = ast_node;
    }
  | STRING {
      struct handlebars_ast_node * ast_node;
      struct handlebars_ast_node * string_node;
      
      string_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_STRING, context);
      string_node->node.string.string = $1;
      
      ast_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_PARTIAL_NAME, context);
      ast_node->node.partial_name.name = string_node;
      $$ = ast_node;
    }
  | NUMBER {
      struct handlebars_ast_node * ast_node;
      struct handlebars_ast_node * string_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_NUMBER, context);
      string_node->node.number.string = $1;
      
      ast_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_PARTIAL_NAME, context);
      ast_node->node.partial_name.name = string_node;
      $$ = ast_node;
    }
  ;

data_name
  : DATA path {
      struct handlebars_ast_node * ast_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_DATA, context);
      ast_node->node.data.id = $2;
      $$ = ast_node;
    }
  ;

path
  : path_segments {
      $$ = handlebars_ast_helper_prepare_id(context, $1);
      __MEMCHECK($$);
    }
  ;

path_segments
  : path_segments SEP ID {
      struct handlebars_ast_node * ast_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_PATH_SEGMENT, context);
      ast_node->node.path_segment.part = handlebars_talloc_strndup(ast_node, $3, strlen($3));
      ast_node->node.path_segment.part_length = strlen($3);
      ast_node->node.path_segment.separator = handlebars_talloc_strndup(ast_node, $2, strlen($2));
      ast_node->node.path_segment.separator_length = strlen($2);
      
      handlebars_ast_list_append($1, ast_node);
      $$ = $1;
    }
  | ID {
      struct handlebars_ast_node * ast_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_PATH_SEGMENT, context);
      ast_node->node.path_segment.part = handlebars_talloc_strdup(ast_node, $1);
      ast_node->node.path_segment.part_length = strlen($1);
      
      $$ = handlebars_ast_list_ctor(context);
      handlebars_ast_list_append($$, ast_node);
    }
  ;

content
  : content CONTENT {
    $$ = handlebars_talloc_strdup_append($1, $2);
  }
  | CONTENT {
    $$ = $1;
  }