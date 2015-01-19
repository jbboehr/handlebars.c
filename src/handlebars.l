
%{

#include <stdint.h>
#include "handlebars.h"
#include "handlebars_context.h"
#include "handlebars_utils.h"
#include "handlebars.tab.h"

// Make sure nothing goes to stdout
#define ECHO

#ifndef YY_FATAL_ERROR
#define YY_FATAL_ERROR(msg) handlebars_yy_fatal_error(msg, yyscanner)
#endif

#undef YY_INPUT
#define YY_INPUT(b,r,s) handlebars_yy_input(b, &r, s, handlebars_yy_get_extra(yyscanner))

#if defined(YYDEBUG) && YYDEBUG
  #define YY_USER_DEBUG_INIT handlebars_yy_set_debug(1, yyg);
#else
  #define YY_USER_DEBUG_INIT
#endif

#define YY_USER_INIT \
     YY_USER_DEBUG_INIT \
     memset(yylval, 0, sizeof(YYSTYPE)); \
     if( yy_flex_debug ) fprintf( stderr, "--initializing yylval\n" )
 
#define YY_USER_ACTION \
    yylloc->first_line = yylloc->last_line = yylineno; \
    yylloc->first_column = yycolumn; \
    yylloc->last_column = yycolumn + yyleng - 1; \
    yycolumn += yyleng;
%}

%option prefix="handlebars_yy_"
%option header-file="handlebars.lex.h"
%option noyywrap
%option stack
%option reentrant
%option bison-bridge
%option bison-locations
%option yylineno
%option warn

%option 8bit

%%

[^\x00\n]+ {
                    //printf( "Content: %s\n", yytext );
                    yylval->text = yytext;
                    return CONTENT;
}

\n {
                    //printf( "Whitespace: %s\n", yytext );
                    yylval->text = yytext;
                    return WHITESPACE;
}

%%
