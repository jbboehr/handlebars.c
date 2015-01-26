
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
#include "handlebars_ast_list.h"
#include "handlebars_context.h"
#include "handlebars_memory.h"
#include "handlebars_utils.h"
#include "handlebars.tab.h"
#include "handlebars.lex.h"

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

%%

start : 
    program END {
      context->program = $1;
      return 1;
    }
    ;

program :
    statements {
      $$ = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_PROGRAM, context);
      $$->node.program.statements = $1;
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
  | CONTENT {
      struct handlebars_ast_node * ast_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_CONTENT, context);
      ast_node->node.content.string = $1;
      ast_node->node.content.length = strlen($1);
      $$ = ast_node;
    }
  | COMMENT {
      struct handlebars_ast_node * ast_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_COMMENT, context);
      ast_node->node.content.string = $1;
      ast_node->node.content.length = strlen($1);
      $$ = ast_node;
    }
  ;


raw_block
  : open_raw_block CONTENT END_RAW_BLOCK {
      struct handlebars_ast_node * content_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_CONTENT, context);
      content_node->node.content.string = handlebars_talloc_strdup(content_node, $2);
      content_node->node.content.length = strlen($2);
      
      struct handlebars_ast_node * ast_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_RAW_BLOCK, context);
      ast_node->node.block.mustache = $1;
      ast_node->node.block.program = content_node;
      ast_node->node.block.close = (char *) handlebars_talloc_strdup(ast_node, $3);
      $$ = ast_node;
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
  : open_block program close_block {
      struct handlebars_ast_node * ast_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_BLOCK, context);
      ast_node->node.block.mustache = $1;
      ast_node->node.block.program = $2;
      ast_node->node.block.close = $3;
      ast_node->node.block.inverted = 0;
      $$ = ast_node;
    }
  | open_block program inverse_and_program close_block {
      struct handlebars_ast_node * ast_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_BLOCK, context);
      ast_node->node.block.mustache = $1;
      ast_node->node.block.program = $2;
      ast_node->node.block.inverse = $3;
      ast_node->node.block.close = $4;
      ast_node->node.block.inverted = 0;
      $$ = ast_node;
    }
  | open_inverse program close_block {
      struct handlebars_ast_node * ast_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_BLOCK, context);
      ast_node->node.block.mustache = $1;
      ast_node->node.block.program = $2;
      ast_node->node.block.close = $3;
      ast_node->node.block.inverted = 1;
      $$ = ast_node;
    }
  | open_inverse program inverse_and_program close_block {
      struct handlebars_ast_node * ast_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_BLOCK, context);
      ast_node->node.block.mustache = $1;
      ast_node->node.block.program = $2;
      ast_node->node.block.inverse = $3;
      ast_node->node.block.close = $4;
      ast_node->node.block.inverted = 1;
      $$ = ast_node;
    }
  ;

open_block
  : OPEN_BLOCK sexpr CLOSE {
      struct handlebars_ast_node * ast_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_MUSTACHE, context);
      ast_node->node.mustache.sexpr = $2;
      // Todo: $1 $3
      $$ = ast_node;
    }
  ;

open_inverse
  : OPEN_INVERSE sexpr CLOSE {
      struct handlebars_ast_node * ast_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_MUSTACHE, context);
      ast_node->node.mustache.sexpr = $2;
      // Todo: $1 $3
      $$ = ast_node;
    }
  ;

inverse_and_program
  : INVERSE program {
      // this is kind of wrong
      $$ = $2;
      // Original:
      // -> { strip: yy.stripFlags($1, $1), program: $2 }
    }
  ;

close_block
  : OPEN_ENDBLOCK path CLOSE {
      // this is kind of wrong
      $$ = $2;
      // Original:
      //-> {path: $2, strip: yy.stripFlags($1, $3)}
    }
  ;

mustache
  : OPEN sexpr CLOSE {
      struct handlebars_ast_node * ast_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_MUSTACHE, context);
      ast_node->node.mustache.sexpr = $2;
      // Todo: $1 $3
      $$ = ast_node;
    }
  | OPEN_UNESCAPED sexpr CLOSE_UNESCAPED {
      struct handlebars_ast_node * ast_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_MUSTACHE, context);
      ast_node->node.mustache.sexpr = $2;
      ast_node->node.mustache.escaped = -1;
      // Todo: $1 $3
      $$ = ast_node;
    }
  ;

partial
  : OPEN_PARTIAL partial_name param CLOSE {
      struct handlebars_ast_node * ast_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_PARTIAL, context);
      // Todo: $1 $2 $3 $4
      $$ = ast_node;
    }
  | OPEN_PARTIAL partial_name param hash CLOSE {
      struct handlebars_ast_node * ast_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_PARTIAL, context);
      // Todo: $1 $2 $3 $4 $5
      $$ = ast_node;
    }
  | OPEN_PARTIAL partial_name CLOSE {
      struct handlebars_ast_node * ast_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_PARTIAL, context);
      // Todo: $1 $2 $3
      $$ = ast_node;
    }
  | OPEN_PARTIAL partial_name hash CLOSE {
      struct handlebars_ast_node * ast_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_PARTIAL, context);
      // Todo: $1 $2 $3 $4
      $$ = ast_node;
    }
  ;

sexpr
  : path params hash {
      struct handlebars_ast_node * ast_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_SEXPR, context);
      // Todo: $1 $2 $3
      $$ = ast_node;
    }
  | path params {
      struct handlebars_ast_node * ast_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_SEXPR, context);
      // Todo: $1 $2
      $$ = ast_node;
    }
  | data_name {
      struct handlebars_ast_node * ast_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_SEXPR, context);
      // Todo: $1
      $$ = ast_node;
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
      // {$2.isHelper = true; $$ = $2;}
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
      // -> new yy.PartialNameNode($1, @$)
    }
  | STRING {
      // -> new yy.PartialNameNode(new yy.StringNode($1, @$), @$)
    }
  | NUMBER {
      // -> new yy.PartialNameNode(new yy.NumberNode($1, @$))
    }
  ;

data_name
  : DATA path {
      // -> new yy.DataNode($2, @$)
    }
  ;

path
  : path_segments {
      // -> new yy.IdNode($1, @$)
    }
  ;

path_segments
  : path_segments SEP ID {
      // { $1.push({part: $3, separator: $2}); $$ = $1; }
    }
  | ID {
      // -> [{part: $1}]
    }
  ;