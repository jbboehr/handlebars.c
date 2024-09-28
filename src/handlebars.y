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

%defines
%name-prefix "handlebars_yy_"
%pure-parser
%error-verbose

%lex-param {
    void * scanner
}
%parse-param {
    struct handlebars_parser * parser
}
%locations

%code requires {
    struct handlebars_parser; /* needed for bison 2.7 */
    #define YY_END_OF_BUFFER_CHAR 0
    #define YY_EXTRA_TYPE struct handlebars_parser *
    typedef struct handlebars_locinfo YYLTYPE;
    #define YYLTYPE_IS_DECLARED 1
    #define YYLTYPE_IS_TRIVIAL 1

    struct handlebars_yy_block_params {
        struct handlebars_string * block_param1;
        struct handlebars_string * block_param2;
    };
    struct handlebars_yy_block_intermediate {
        struct handlebars_ast_node * program;
        struct handlebars_ast_node * inverse_chain;
    };
}

%start  start

%{

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define HANDLEBARS_AST_PRIVATE

#include "handlebars.h"
#include "handlebars_ast.h"
#include "handlebars_ast_helpers.h"
#include "handlebars_ast_list.h"
#include "handlebars_memory.h"
#include "handlebars_parser.h"
#include "handlebars_private.h"
#include "handlebars_string.h"
#include "handlebars_whitespace.h"

#pragma GCC diagnostic ignored "-Wswitch-default"
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic warning "-Wredundant-decls"

#include "handlebars_parser_private.h"
#include "handlebars.tab.h"
#include "handlebars.lex.h"

#if defined(YYDEBUG) && YYDEBUG
#define YYPRINT handlebars_yy_print
int handlebars_yy_debug = 1;
#else
int handlebars_yy_debug = 0;
#endif

#undef CONTEXT
#define CONTEXT HBSCTX(parser)
#define scanner parser->scanner
%}

%union {
    struct handlebars_string * string;
    struct handlebars_ast_node * ast_node;
    struct handlebars_ast_list * ast_list;

    struct handlebars_yy_block_intermediate block_intermediate;
    struct handlebars_yy_block_params block_params;
}

%token END 0 "end of file"
%token <string> BOOLEAN
%token <string> CLOSE "}}"
%token <string> CLOSE_RAW_BLOCK "}}}}"
%token <string> CLOSE_SEXPR ")"
%token <string> CLOSE_UNESCAPED "}}}"
%token <string> COMMENT
%token <string> CONTENT
%token <string> DATA
%token <string> END_RAW_BLOCK // meh
%token <string> EQUALS "="
%token <string> ID
%token <string> INVALID
%token <string> INVERSE
%token <string> NUMBER
%token <string> OPEN "{{"
%token <string> OPEN_BLOCK "{{#"
%token <string> OPEN_ENDBLOCK
%token <string> OPEN_INVERSE "{{^"
%token <string> OPEN_PARTIAL "{{>"
%token <string> OPEN_RAW_BLOCK "{{{{"
%token <string> OPEN_SEXPR "("
%token <string> OPEN_UNESCAPED "{{{"
%token <string> SEP
%token <string> STRING

// Added in v3
%token <string> CLOSE_BLOCK_PARAMS
%token <string> NUL "NULL"
%token <string> OPEN_BLOCK_PARAMS
%token <string> OPEN_INVERSE_CHAIN
%token <string> UNDEFINED "undefined"

// Added in v4
%token <string> OPEN_PARTIAL_BLOCK "{{#>"

// Added later (cutom)
%token <string> LONG_COMMENT
%token <string> SINGLE_STRING

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
%type <ast_list> hash_pairs
%type <ast_node> hash_pair
%type <ast_list> path_segments
%type <string> content

// Added in v3
%type <block_params> block_params
%type <ast_node> helper_name
%type <ast_node> inverse_chain
%type <ast_node> open_inverse_chain

// Added in v3 - intermediates
%type <ast_node> intermediate3
%type <ast_node> intermediate4
%type <block_intermediate> block_intermediate;

// Added in v4
%type <ast_node> partial_block
%type <ast_node> open_partial_block

%right CONTENT

%%

start :
    program END {
      parser->program = $1;
      handlebars_whitespace_accept(parser, parser->program);
      return 1;
    }
  ;

program :
    statements {
      $$ = handlebars_ast_node_ctor_program(parser, $1, NULL, NULL, 0, 0, &@$);
    }
  | "" {
      struct handlebars_ast_list * list = handlebars_ast_list_ctor(CONTEXT);
      $$ = handlebars_ast_node_ctor_program(parser, list, NULL, NULL, 0, 0, &@$);
    }
  ;

statements
  : statement {
      $$ = handlebars_ast_list_ctor(CONTEXT);
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
  | partial_block {
      $$ = $1;
    }
  | content {
      $$ = handlebars_ast_node_ctor_content(parser, $1, &@$);
    }
  | COMMENT {
      // Strip comment strips in place
      unsigned strip = handlebars_ast_helper_strip_flags($1, $1);
      $$ = handlebars_ast_node_ctor_comment(parser,
      			handlebars_ast_helper_strip_comment($1), false, &@$);
      handlebars_ast_node_set_strip($$, strip);
    }
  | LONG_COMMENT {
      // Strip comment strips in place
      unsigned strip = handlebars_ast_helper_strip_flags($1, $1);
      $$ = handlebars_ast_node_ctor_comment(parser,
      			handlebars_ast_helper_strip_comment($1), true, &@$);
      handlebars_ast_node_set_strip($$, strip);
  }
  ;

content
  : CONTENT content {
      $$ = handlebars_string_append_str(CONTEXT, $1, $2);
      $$ = talloc_steal(parser, $$);
    }
  | CONTENT {
      $$ = $1;
    }
  ;

raw_block
  : open_raw_block content END_RAW_BLOCK {
      $$ = handlebars_ast_helper_prepare_raw_block(parser, $1, $2, $3, &@$);
    }
    | open_raw_block END_RAW_BLOCK {
      $$ = handlebars_ast_helper_prepare_raw_block(parser, $1, handlebars_string_ctor(HBSCTX(parser), HBS_STRL("")), $2, &@$);
    }
  ;

open_raw_block
  : OPEN_RAW_BLOCK intermediate4 CLOSE_RAW_BLOCK {
      $$ = $2;
    }
  ;

block
  : open_block block_intermediate close_block {
      $$ = handlebars_ast_helper_prepare_block(parser, $1, $2.program, $2.inverse_chain, $3, 0, &@$);
    }
  | open_block close_block {
      $$ = handlebars_ast_helper_prepare_block(parser, $1, NULL, NULL, $2, 0, &@$);
    }
  | open_inverse block_intermediate close_block {
      $$ = handlebars_ast_helper_prepare_block(parser, $1, $2.program, $2.inverse_chain, $3, 1, &@$);
    }
  | open_inverse close_block {
      $$ = handlebars_ast_helper_prepare_block(parser, $1, NULL, NULL, $2, 1, &@$);
    }
  ;

block_intermediate
  : inverse_chain {
      $$.program = NULL;
      $$.inverse_chain = $1;
    }
  | program inverse_chain {
      $$.program = $1;
      $$.inverse_chain = $2;
    }
  | program {
      $$.program = $1;
      $$.inverse_chain = NULL;
    }
  ;

open_block
  : OPEN_BLOCK intermediate4 CLOSE {
      $$ = $2;
      handlebars_ast_node_set_strip($$, handlebars_ast_helper_strip_flags($1, $3));
      $$->node.intermediate.open = talloc_steal($$, handlebars_string_copy_ctor(CONTEXT, $1));
    }
  ;

open_inverse
  : OPEN_INVERSE intermediate4 CLOSE {
      $$ = $2;
      handlebars_ast_node_set_strip($$, handlebars_ast_helper_strip_flags($1, $3));
    }
  ;

open_inverse_chain
  : OPEN_INVERSE_CHAIN intermediate4 CLOSE {
      $$ = $2;
      handlebars_ast_node_set_strip($$, handlebars_ast_helper_strip_flags($1, $3));
    }
  ;

inverse_chain
  : open_inverse_chain program inverse_chain {
      $$ = handlebars_ast_helper_prepare_inverse_chain(parser, $1, $2, $3, &@$);
  	}
  | open_inverse_chain inverse_chain {
      $$ = handlebars_ast_helper_prepare_inverse_chain(parser, $1, NULL, $2, &@$);
  	}
  | open_inverse_chain program {
      $$ = handlebars_ast_helper_prepare_inverse_chain(parser, $1, $2, NULL, &@$);
    }
  | open_inverse_chain {
      $$ = handlebars_ast_helper_prepare_inverse_chain(parser, $1, NULL, NULL, &@$);
    }
  | inverse_and_program {
      $$ = $1;
    }
  ;

inverse_and_program
  : INVERSE program {
      $$ = handlebars_ast_node_ctor_inverse(parser, $2, 0,
              handlebars_ast_helper_strip_flags($1, $1), &@$);
    }
  | INVERSE {
      struct handlebars_ast_node * program_node;
      program_node = handlebars_ast_node_ctor(CONTEXT, HANDLEBARS_AST_NODE_PROGRAM);
      $$ = handlebars_ast_node_ctor_inverse(parser, program_node, 0,
              handlebars_ast_helper_strip_flags($1, $1), &@$);
    }
  ;

close_block
  : OPEN_ENDBLOCK helper_name CLOSE {
      $$ = handlebars_ast_node_ctor_intermediate(parser, $2, NULL, NULL,
              handlebars_ast_helper_strip_flags($1, $3), &@$);
    }
  ;

mustache
  : OPEN intermediate3 CLOSE {
      $$ = handlebars_ast_helper_prepare_mustache(parser, $2, $1,
        			handlebars_ast_helper_strip_flags($1, $3), &@$);
    }
  | OPEN_UNESCAPED intermediate3 CLOSE_UNESCAPED {
      $$ = handlebars_ast_helper_prepare_mustache(parser, $2, $1,
        			handlebars_ast_helper_strip_flags($1, $3), &@$);
    }
  ;

partial
  : OPEN_PARTIAL partial_name params hash CLOSE {
      $$ = handlebars_ast_node_ctor_partial(parser, $2, $3, $4,
              handlebars_ast_helper_strip_flags($1, $5), &@$);
    }
  | OPEN_PARTIAL partial_name params CLOSE {
      $$ = handlebars_ast_node_ctor_partial(parser, $2, $3, NULL,
              handlebars_ast_helper_strip_flags($1, $4), &@$);
    }
  | OPEN_PARTIAL partial_name hash CLOSE {
      $$ = handlebars_ast_node_ctor_partial(parser, $2, NULL, $3,
              handlebars_ast_helper_strip_flags($1, $4), &@$);
    }
  | OPEN_PARTIAL partial_name CLOSE {
      $$ = handlebars_ast_node_ctor_partial(parser, $2, NULL, NULL,
              handlebars_ast_helper_strip_flags($1, $3), &@$);
    }
  ;

partial_block
  : open_partial_block program close_block {
      $$ = handlebars_ast_helper_prepare_partial_block(parser, $1, $2, $3, &@$);
  }
  | open_partial_block close_block {
      struct handlebars_ast_node * program = handlebars_ast_node_ctor(CONTEXT, HANDLEBARS_AST_NODE_PROGRAM);
      $$ = handlebars_ast_helper_prepare_partial_block(parser, $1, program, $2, &@$);
  }

open_partial_block
  : OPEN_PARTIAL_BLOCK partial_name params hash CLOSE {
      $$ = handlebars_ast_node_ctor_intermediate(parser, $2, $3, $4,
      			handlebars_ast_helper_strip_flags($1, $5), &@$);
    }
  | OPEN_PARTIAL_BLOCK partial_name params CLOSE {
      $$ = handlebars_ast_node_ctor_intermediate(parser, $2, $3, NULL,
      			handlebars_ast_helper_strip_flags($1, $4), &@$);
    }
  | OPEN_PARTIAL_BLOCK partial_name hash CLOSE {
      $$ = handlebars_ast_node_ctor_intermediate(parser, $2, NULL, $3,
              handlebars_ast_helper_strip_flags($1, $4), &@$);
    }
  | OPEN_PARTIAL_BLOCK partial_name CLOSE {
      $$ = handlebars_ast_node_ctor_intermediate(parser, $2, NULL, NULL,
              handlebars_ast_helper_strip_flags($1, $3), &@$);
    }
  ;

params
  : param {
      $$ = handlebars_ast_list_ctor(CONTEXT);
      handlebars_ast_list_append($$, $1);
    }
  | params param {
      handlebars_ast_list_append($1, $2);
      $$ = $1;
    }
  ;

param
  : helper_name {
      $$ = $1;
    }
  | sexpr {
      $$ = $1;
    }
  ;

sexpr
  : OPEN_SEXPR intermediate3 CLOSE_SEXPR {
      $$ = handlebars_ast_node_ctor_sexpr(parser, $2, &@$);
    }
  ;

intermediate4
  : intermediate3 block_params {
      $$ = $1;
      $$->node.intermediate.block_param1 = $2.block_param1;
      $$->node.intermediate.block_param2 = $2.block_param2;
    }
  | intermediate3
  ;

intermediate3
  : helper_name params hash {
      $$ = handlebars_ast_node_ctor_intermediate(parser, $1, $2, $3, 0, &@$);
    }
  | helper_name hash {
      $$ = handlebars_ast_node_ctor_intermediate(parser, $1, NULL, $2, 0, &@$);
    }
  | helper_name params {
      $$ = handlebars_ast_node_ctor_intermediate(parser, $1, $2, NULL, 0, &@$);
    }
  | helper_name {
      $$ = handlebars_ast_node_ctor_intermediate(parser, $1, NULL, NULL, 0, &@$);
    }
  ;

hash
  : hash_pairs {
      struct handlebars_ast_node * ast_node = handlebars_ast_node_ctor(CONTEXT, HANDLEBARS_AST_NODE_HASH);
      ast_node->node.hash.pairs = $1;
      $$ = ast_node;
    }
  ;

hash_pairs
  : hash_pairs hash_pair {
      handlebars_ast_list_append($1, $2);
      $$ = $1;
    }
  | hash_pair {
      $$ = handlebars_ast_list_ctor(CONTEXT);
      handlebars_ast_list_append($$, $1);
    }
  ;

hash_pair
  : ID EQUALS param {
      $$ = handlebars_ast_node_ctor_hash_pair(parser, $1, $3, &@$);
    }
  ;

block_params
  : OPEN_BLOCK_PARAMS ID ID CLOSE_BLOCK_PARAMS {
      $$.block_param1 = handlebars_string_copy_ctor(CONTEXT, $2);
      $$.block_param2 = handlebars_string_copy_ctor(CONTEXT, $3);
    }
  | OPEN_BLOCK_PARAMS ID CLOSE_BLOCK_PARAMS {
      $$.block_param1 = handlebars_string_copy_ctor(CONTEXT, $2);
      $$.block_param2 = NULL;
    }
  ;

helper_name
  : path {
      $$ = $1;
    }
  | data_name {
      $$ = $1;
    }
  | STRING {
      $$ = handlebars_ast_node_ctor_string(parser, $1, false, &@$);
    }
  | SINGLE_STRING {
      $$ = handlebars_ast_node_ctor_string(parser, $1, true, &@$);
  }
  | NUMBER {
      $$ = handlebars_ast_node_ctor_number(parser, $1, &@$);
    }
  | BOOLEAN {
      $$ = handlebars_ast_node_ctor_boolean(parser, $1, &@$);
    }
  | UNDEFINED {
      $$ = handlebars_ast_node_ctor_undefined(parser, $1, &@$);
    }
  | NUL {
      $$ = handlebars_ast_node_ctor_null(parser, $1, &@$);
    }
  ;

partial_name
  : helper_name {
      $$ = $1;
    }
  | sexpr {
      $$ = $1;
    }
  ;

data_name
  : DATA path_segments {
      $$ = handlebars_ast_helper_prepare_path(parser, $2, 1, &@$);
    }
  ;

path
  : path_segments {
      $$ = handlebars_ast_helper_prepare_path(parser, $1, 0, &@$);
    }
  ;

path_segments
  : path_segments SEP ID {
      struct handlebars_ast_node * ast_node = handlebars_ast_node_ctor_path_segment(parser, $3, $2, &@$);

      handlebars_ast_list_append($1, ast_node);
      $$ = $1;
    }
  | ID {
      struct handlebars_ast_node * ast_node;
      MEMCHK($1); // this is weird

      ast_node = handlebars_ast_node_ctor_path_segment(parser, $1, NULL, &@$);
      MEMCHK(ast_node); // this is weird

      $$ = handlebars_ast_list_ctor(CONTEXT);
      handlebars_ast_list_append($$, ast_node);
    }
  ;
