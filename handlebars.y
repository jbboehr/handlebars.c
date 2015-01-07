
%{
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "handlebars.h"
#include "handlebars_context.h"
#include "handlebars_utils.h"
#include "handlebars.tab.h"

#define scanner context->scanner
%}

%code requires {
  struct handlebars_context; /* needed for bison 2.7 */
  #define YY_END_OF_BUFFER_CHAR 0
  #define YY_EXTRA_TYPE handlebars_context *
}


%start  start

%defines
%name-prefix="handlebars_yy_"
%pure-parser
%error-verbose
%lex-param {
  void * scanner
}
%locations

%parse-param {
  struct handlebars_context * context
}

/*
%union {
  struct {
	char * text;
        struct handlebars_string * string;
	struct handlebars_node * token;
        struct handlebars_node_list * token_list;
  };
}
*/

%token END 0 "end of file"

%%

start : 
    END {
      return 1;
    }
    ;