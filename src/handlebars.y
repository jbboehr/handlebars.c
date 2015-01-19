
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
  #define YY_EXTRA_TYPE handlebars_context *
}

%start  start

%{
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "handlebars.h"
#include "handlebars_context.h"
#include "handlebars_utils.h"
#include "handlebars.tab.h"
#include "handlebars.lex.h"

#define scanner context->scanner
%}

%union {
  struct {
	  char * text;
  };
}

%token END 0 "end of file"
%token CONTENT
%token WHITESPACE

%%

start : 
    END {
      return 1;
    }
    ;