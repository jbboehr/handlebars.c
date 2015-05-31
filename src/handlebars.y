
%defines
%name-prefix "handlebars_yy_"
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
  typedef struct handlebars_locinfo YYLTYPE;
  #define YYLTYPE_IS_DECLARED 1
  #define YYLTYPE_IS_TRIVIAL 1
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
#include "handlebars_private.h"
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
    if( unlikely(!cond) ) { \
      context->errnum = HANDLEBARS_NOMEM; \
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
      handlebars_ast_helper_prepare_program(context, $1, 1, NULL);
      context->program = $1;
      return 1;
    }
    ;

program :
    statements {
      $$ = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_PROGRAM, context);
      __MEMCHECK($$);
      $$->node.program.statements = $1;
      handlebars_ast_helper_prepare_program(context, $$, 0, &yylloc);
    }
  | "" {
      $$ = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_PROGRAM, context);
      __MEMCHECK($$);
      $$->node.program.statements = handlebars_ast_list_ctor($$);
      handlebars_ast_helper_prepare_program(context, $$, 0, &yylloc);
  }
  ;

statements
  : statement {
      $$ = handlebars_ast_list_ctor(context);
      __MEMCHECK($$);
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
      $$ = handlebars_ast_node_ctor_content(context, $1, &yylloc);
      __MEMCHECK($$);
    }
  | COMMENT {
      $$ = handlebars_ast_node_ctor_comment(context, $1, &yylloc);
      __MEMCHECK($$);
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
      $$ = handlebars_ast_helper_prepare_mustache(context, $2, $1, $3, -1, &yylloc);
      __MEMCHECK($$);
    }
  ;

block
  : open_block program inverse_and_program close_block {
      $$ = handlebars_ast_helper_prepare_block(context, $1, $2, $3, $4, 0, &yylloc);
      __MEMCHECK($$);
    }
  | open_block inverse_and_program close_block {
      struct handlebars_ast_node * program = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_PROGRAM, context);
      __MEMCHECK(program);
      $$ = handlebars_ast_helper_prepare_block(context, $1, program, $2, $3, 0, &yylloc);
      __MEMCHECK($$);
    }
  | open_block program close_block {
      $$ = handlebars_ast_helper_prepare_block(context, $1, $2, NULL, $3, 0, &yylloc);
      __MEMCHECK($$);
    }
  | open_block close_block {
      struct handlebars_ast_node * program = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_PROGRAM, context);
      __MEMCHECK(program);
      $$ = handlebars_ast_helper_prepare_block(context, $1, program, NULL, $2, 0, &yylloc);
      __MEMCHECK($$);
    }
  | open_inverse program inverse_and_program close_block {
      $$ = handlebars_ast_helper_prepare_block(context, $1, $2, $3, $4, 1, &yylloc);
      __MEMCHECK($$);
    }
  | open_inverse inverse_and_program close_block {
      struct handlebars_ast_node * program = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_PROGRAM, context);
      __MEMCHECK(program);
      $$ = handlebars_ast_helper_prepare_block(context, $1, program, $2, $3, 1, &yylloc);
      __MEMCHECK($$);
    }
  | open_inverse program close_block {
      $$ = handlebars_ast_helper_prepare_block(context, $1, $2, NULL, $3, 1, &yylloc);
      __MEMCHECK($$);
    }
  | open_inverse close_block {
      $$ = handlebars_ast_helper_prepare_block(context, $1, NULL, NULL, $2, 1, &yylloc);
      __MEMCHECK($$);
    }
  ;

open_block
  : OPEN_BLOCK sexpr CLOSE {
      $$ = handlebars_ast_helper_prepare_mustache(context, $2, $1, $3, -1, &yylloc);
      __MEMCHECK($$);
    }
  ;

open_inverse
  : OPEN_INVERSE sexpr CLOSE {
      $$ = handlebars_ast_helper_prepare_mustache(context, $2, $1, $3, -1, &yylloc);
      __MEMCHECK($$);
    }
  ;

inverse_and_program
  : INVERSE program {
      $$ = handlebars_ast_node_ctor_inverse_and_program(context, $2,
              handlebars_ast_helper_strip_flags($1, $1), &yylloc);
      __MEMCHECK($$);
    }
  | INVERSE {
      $$ = handlebars_ast_node_ctor_inverse_and_program(context, NULL,
              handlebars_ast_helper_strip_flags($1, $1), &yylloc);
      __MEMCHECK($$);
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
      $$ = handlebars_ast_helper_prepare_mustache(context, $2, $1, $3, 0, &yylloc);
      __MEMCHECK($$);
    }
  | OPEN_UNESCAPED sexpr CLOSE_UNESCAPED {
      $$ = handlebars_ast_helper_prepare_mustache(context, $2, $1, $3, 1, &yylloc);
      __MEMCHECK($$);
    }
  ;

partial
  : OPEN_PARTIAL partial_name param CLOSE {
      $$ = handlebars_ast_node_ctor_partial(context, $2, $3, NULL,
              handlebars_ast_helper_strip_flags($1, $4), &yylloc);
      __MEMCHECK($$);
    }
  | OPEN_PARTIAL partial_name param hash CLOSE {
      $$ = handlebars_ast_node_ctor_partial(context, $2, $3, $4,
              handlebars_ast_helper_strip_flags($1, $5), &yylloc);
      __MEMCHECK($$);
    }
  | OPEN_PARTIAL partial_name CLOSE {
      $$ = handlebars_ast_node_ctor_partial(context, $2, NULL, NULL,
              handlebars_ast_helper_strip_flags($1, $3), &yylloc);
      __MEMCHECK($$);
    }
  | OPEN_PARTIAL partial_name hash CLOSE {
      $$ = handlebars_ast_node_ctor_partial(context, $2, NULL, $3,
              handlebars_ast_helper_strip_flags($1, $4), &yylloc);
      __MEMCHECK($$);
    }
  ;

sexpr
  : path {
      $$ = handlebars_ast_helper_prepare_sexpr(context, $1, NULL, NULL, &yylloc);
      __MEMCHECK($$);
    }
  | path hash {
      $$ = handlebars_ast_helper_prepare_sexpr(context, $1, NULL, $2, &yylloc);
      __MEMCHECK($$);
    }
  | path params hash {
      $$ = handlebars_ast_helper_prepare_sexpr(context, $1, $2, $3, &yylloc);
      __MEMCHECK($$);
    }
  | path params {
      $$ = handlebars_ast_helper_prepare_sexpr(context, $1, $2, NULL, &yylloc);
      __MEMCHECK($$);
    }
  | data_name {
      $$ = handlebars_ast_helper_prepare_sexpr(context, $1, NULL, NULL, &yylloc);
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
      __MEMCHECK($$);
      handlebars_ast_list_append($$, $1);
    }
  ;

param
  : path {
      $$ = $1;
    }
  | STRING {
      $$ = handlebars_ast_node_ctor_string(context, $1, &yylloc);
      __MEMCHECK($$);
    }
  | NUMBER {
      $$ = handlebars_ast_node_ctor_number(context, $1, &yylloc);
      __MEMCHECK($$);
    }
  | BOOLEAN {
      $$ = handlebars_ast_node_ctor_boolean(context, $1, &yylloc);
      __MEMCHECK($$);
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
      __MEMCHECK(ast_node);
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
      __MEMCHECK($$);
      handlebars_ast_list_append($$, $1);
    }
  ;

hash_segment
  : ID EQUALS param {
      $$ = handlebars_ast_node_ctor_hash_segment(context, $1, $3, &yylloc);
      __MEMCHECK($$);
    }
  ;

partial_name
  : path {
      $$ = handlebars_ast_node_ctor_partial_name(context, $1, &yylloc);
      __MEMCHECK($$);
    }
  | STRING {
      struct handlebars_ast_node * string_node = handlebars_ast_node_ctor_string(context, $1, &yylloc);
      __MEMCHECK(string_node);
      $$ = handlebars_ast_node_ctor_partial_name(context, string_node, &yylloc);
      __MEMCHECK($$);
    }
  | NUMBER {
      struct handlebars_ast_node * string_node = handlebars_ast_node_ctor_number(context, $1, &yylloc);
      __MEMCHECK(string_node);
      $$ = handlebars_ast_node_ctor_partial_name(context, string_node, &yylloc);
      __MEMCHECK($$);
    }
  ;

data_name
  : DATA path {
      $$ = handlebars_ast_node_ctor_data(context, $2, &yylloc);
      __MEMCHECK($$);
    }
  ;

path
  : path_segments {
      $$ = handlebars_ast_helper_prepare_id(context, $1, &yylloc);
      __MEMCHECK($$);
    }
  ;

path_segments
  : path_segments SEP ID {
      struct handlebars_ast_node * ast_node = handlebars_ast_node_ctor_path_segment(context, $3, $2, &yylloc);
  	  __MEMCHECK(ast_node);
      
      handlebars_ast_list_append($1, ast_node);
      $$ = $1;
    }
  | ID {
      struct handlebars_ast_node * ast_node;
  	  __MEMCHECK($1); // this is weird
  	  
      ast_node = handlebars_ast_node_ctor_path_segment(context, $1, NULL, &yylloc);
  	  __MEMCHECK(ast_node); // this is weird
      
      $$ = handlebars_ast_list_ctor(context);
  	  __MEMCHECK($$);
      handlebars_ast_list_append($$, ast_node);
    }
  ;

content
  : content CONTENT {
      $$ = handlebars_talloc_strdup_append($1, $2);
      __MEMCHECK($$);
  }
  | CONTENT {
      $$ = $1;
  }
  