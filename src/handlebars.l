
%{

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdint.h>

#include "handlebars.h"
#include "handlebars_context.h"
#include "handlebars_memory.h"
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
  #define YY_USER_DEBUG_ACTION fprintf(stderr, "Action: Line %d, yytext: %s\n", __LINE__, yytext)
  #define YY_BREAK fprintf(stderr, "Action: Line %d (end)\n", __LINE__); break; 
#else
  #define YY_USER_DEBUG_INIT
  #define YY_USER_DEBUG_ACTION 
#endif

#define YY_USER_INIT \
    YY_USER_DEBUG_INIT \
    memset(yylval, 0, sizeof(YYSTYPE)); \
    if( yy_flex_debug ) { \
     	fprintf( stderr, "--initializing yylval\n" ); \
	}
 
#define handlebars_yy_unput_all \
	handlebars_yy_unput_n(yyleng)
 		
#define handlebars_yy_unput_n(n) \
	do { \
		int i; \
 		char * yycopy = strdup( yytext ); \
 		for ( i = yyleng - 1; i >= yyleng - n; --i ) { \
 			unput( yycopy[i] ); \
 		} \
        free( yycopy ); \
    } while(0)

#define handlebars_yy_copy_lval() \
	yylval->text = handlebars_talloc_strndup(yyextra, yytext, yyleng); \
	yylval->length = yyleng

#define YY_USER_ACTION \
    yylloc->first_line = yylloc->last_line = yylineno; \
    yylloc->first_column = yycolumn; \
    yylloc->last_column = yycolumn + yyleng - 1; \
    yycolumn += yyleng; \
   	YY_USER_DEBUG_ACTION;
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
%option noyyalloc
%option noyyrealloc
%option noyyfree

%x mu
%x emu
%x com
%x com1
%x raw

LEFT_STRIP    "~"
RIGHT_STRIP   "~"

LOOKAHEAD           [=~} \r\n\t\/.)]
LITERAL_LOOKAHEAD   [~} \r\n\t)]

MU \{\{\{?
CMU \}\}\}?
EMU \\\{\{\{?
EEMU \\\\\{\{\{?
CONTENT [^\x00\{\\]

WHITESPACE [ \r\n\t]

IDC	  [^ \r\n\t!\"#%-,\.\/;->@\[-\^`\{-~]
ID    {IDC}+/{LOOKAHEAD}


%%


{CONTENT}+\\\\/{MU}             	{
										yytext[--yyleng] = 0;
										yy_push_state(mu, yyscanner);
										handlebars_yy_copy_lval();
										return CONTENT;
									}

{CONTENT}+                         	{
										// v2.0.0 handlebars.l line 31
										handlebars_yy_copy_lval();
										return CONTENT;
									}

{EEMU} 								{
										// v2.0.0 handlebars.l line 32
										handlebars_yy_unput_n(2);
										// @todo make sure this is safe
										yytext = "\\";
										yyleng = 1;
										handlebars_yy_copy_lval();
										yy_push_state(mu, yyscanner);
										return CONTENT;
									}
									
{EMU}{CONTENT}+ 					{		
										// v2.0.0 handlebars.l line 35
										yy_push_state(emu, yyscanner);
										yytext++, yyleng--;
										handlebars_yy_copy_lval();
										return CONTENT;
									}

{MU}  								{
										// v2.0.0 handlebars.l line 38
										handlebars_yy_unput_all;
										yy_push_state(mu, yyscanner);
									}

"\\"                                {
										handlebars_yy_copy_lval();
										return CONTENT;
									}

<emu>. 								{
										// v2.0.0 handlebars.l line 47
										handlebars_yy_unput_all;
                                   		yy_pop_state(yyg);
                                 	}

<raw>"{{{{/"{IDC}+"}}}}" 			{
										// v2.0.0 handlebars.l line 52
										yytext += 5, yyleng -= 9;
                                  		yy_pop_state(yyg);
										handlebars_yy_copy_lval();
                                  		return END_RAW_BLOCK;
                                 	}

<raw>[^\x00]+/("{{{{/")         	{
										// v2.0.0 handlebars.l line 57
										handlebars_yy_copy_lval();
										return CONTENT;
									}

<com>[^\x00]+"--}}"					{
										// v2.0.0 handlebars.l line 59
  										yy_pop_state(yyg);
  										yytext[yyleng -= 4] = 0;
  										yytext += 5, yyleng -= 5;
										handlebars_yy_copy_lval();
  										return COMMENT;
									}

<com>[^\x00\}\-]+ 					{
										// v2.0.0 handlebars.l line 59
									}

<com>"--}}"							{
										// v2.0.0 handlebars.l line 59
  										yy_pop_state(yyg);
										handlebars_yy_copy_lval();
  										return COMMENT;
									}
									
<com>[\}\-]+						{
										// v2.0.0 handlebars.l line 59
										handlebars_yy_copy_lval();
									}

<com1>[^\x00\}]*?"}}"				{
  										yytext[yyleng -= 2] = 0;
  										yy_pop_state(yyg);
										handlebars_yy_copy_lval();
  										return COMMENT;
									}

<mu>"("                          	{
										// v2.0.0 handlebars.l line 61
										handlebars_yy_copy_lval();
										return OPEN_SEXPR;
									}

<mu>")"                          	{
										// v2.0.0 handlebars.l line 62
										handlebars_yy_copy_lval();
										return CLOSE_SEXPR;
									}

<mu>"{{{{"                       	{
										// v2.0.0 handlebars.l line 64
										handlebars_yy_copy_lval();
										return OPEN_RAW_BLOCK;
									}

<mu>"}}}}"                       	{
										// v2.0.0 handlebars.l line 65
										yy_pop_state(yyg);
                                  		yy_push_state(raw, yyscanner);
										handlebars_yy_copy_lval();
                                  		return CLOSE_RAW_BLOCK;
                             		}
                             		
<mu>"{{"{LEFT_STRIP}?">"         	{
										// v2.0.0 handlebars.l line 70
										handlebars_yy_copy_lval();
										return OPEN_PARTIAL;
                             		}

<mu>"{{"{LEFT_STRIP}?"#"         	{
										// v2.0.0 handlebars.l line 71
										handlebars_yy_copy_lval();
										return OPEN_BLOCK;
                             		}

<mu>"{{"{LEFT_STRIP}?"/"         	{
										// v2.0.0 handlebars.l line 72
										handlebars_yy_copy_lval();
										return OPEN_ENDBLOCK;
                             		}

<mu>"{{"{LEFT_STRIP}?"^"\s*{RIGHT_STRIP}?"}}"        {
										// v2.0.0 handlebars.l line 73
										yy_pop_state(yyg);
										handlebars_yy_copy_lval();
										return INVERSE;
                             		}

<mu>"{{"{LEFT_STRIP}?{WHITESPACE}*"else"{WHITESPACE}*{RIGHT_STRIP}?"}}"  {
										// v2.0.0 handlebars.l line 74
										yy_pop_state(yyg);
										handlebars_yy_copy_lval();
										return INVERSE;
                             		}

<mu>"{{"{LEFT_STRIP}?"^"         	{
										// v2.0.0 handlebars.l line 75
										handlebars_yy_copy_lval();
										return OPEN_INVERSE;
                             		}

<mu>"{{"{LEFT_STRIP}?\s*"else"   	{
										// v2.0.0 handlebars.l line 76
										handlebars_yy_copy_lval();
										return OPEN_INVERSE;
                             		}

<mu>"{{"{LEFT_STRIP}?"{"         	{
										// v2.0.0 handlebars.l line 77
										handlebars_yy_copy_lval();
										return OPEN_UNESCAPED;
                             		}

<mu>"{{"{LEFT_STRIP}?"&"         	{
										// v2.0.0 handlebars.l line 78
										handlebars_yy_copy_lval();
										return OPEN;
                             		}
                             		
<mu>"{{!--" 						{
										// v2.0.0 handlebars.l line 79
										handlebars_yy_unput_all;
  										yy_pop_state(yyg);
  										yy_push_state(com, yyscanner);
									}

<mu>"{{!" 							{
										// v2.0.0 handlebars.l line 80
  										yy_pop_state(yyg);
  										yy_push_state(com1, yyscanner);
									}

<mu>"{{"{LEFT_STRIP}?            	{
										// v2.0.0 handlebars.l line 81
										handlebars_yy_copy_lval();
										return OPEN;
									}

<mu>"="                          	{
										// v2.0.0 handlebars.l line 83
										handlebars_yy_copy_lval();
										return EQUALS;
									}

<mu>".."                         	{
										// v2.0.0 handlebars.l line 84
										handlebars_yy_copy_lval();
										return ID;
									}

<mu>"."/{LOOKAHEAD}              	{
										// v2.0.0 handlebars.l line 85
										handlebars_yy_copy_lval();
										return ID;
									}

<mu>[\/.]                        	{
										// v2.0.0 handlebars.l line 86
										handlebars_yy_copy_lval();
										return SEP;
									}

<mu>{WHITESPACE}+                   {
										// v2.0.0 handlebars.l line 87
										// ignore whitespace
                                 	}

<mu>"}"{RIGHT_STRIP}?"}}"        	{
										// v2.0.0 handlebars.l line 88
										yy_pop_state(yyg);
										handlebars_yy_copy_lval();
										return CLOSE_UNESCAPED;
									}

<mu>{RIGHT_STRIP}?"}}"           	{
										// v2.0.0 handlebars.l line 89
										yy_pop_state(yyg);
										handlebars_yy_copy_lval();
										return CLOSE;
									}

<mu>"\""("\\"["]|[^"])*"\""      	{
										// v2.0.0 handlebars.l line 90
										yytext[--yyleng] = 0;
										yytext++, yyleng--;
										handlebars_yy_copy_lval();
										handlebars_stripcslashes(yylval->text, &yylval->length);
                                   		return STRING;
                                 	}

<mu>"'"("\\"[']|[^'])*"'"        	{
										// v2.0.0 handlebars.l line 91
										yytext[--yyleng] = 0;
										yytext++, yyleng--;
										handlebars_yy_copy_lval();
										handlebars_stripcslashes(yylval->text, &yylval->length);
										return STRING;
                                	}
                                 
<mu>"@"                          	{
										// v2.0.0 handlebars.l line 92
										handlebars_yy_copy_lval();
										return DATA;
									}

<mu>"true"/{LITERAL_LOOKAHEAD}   	{
										// v2.0.0 handlebars.l line 93
										handlebars_yy_copy_lval();
										return BOOLEAN;
									}

<mu>"false"/{LITERAL_LOOKAHEAD}  	{
										// v2.0.0 handlebars.l line 94
										handlebars_yy_copy_lval();
										return BOOLEAN;
									}

<mu>\-?[0-9]+(?:\.[0-9]+)?/{LITERAL_LOOKAHEAD} {
										// v2.0.0 handlebars.l line 95
										handlebars_yy_copy_lval();
										return NUMBER;
									}

<mu>{ID}                			{
										// v2.0.0 handlebars.l line 97
										handlebars_yy_copy_lval();
										return ID;
									}

<mu>"["[^\]]*"]"                	{
										// v2.0.0 handlebars.l line 99
										// yytext = strip(1,2);
										yytext[--yyleng] = 0;
										yytext++, yyleng--;
										handlebars_yy_copy_lval();
										return ID;
									}

<mu>.                            	{
										// v2.0.0 handlebars.l line 100
										handlebars_yy_copy_lval();
										return INVALID;
									}

<<EOF>>              	            {
										// v2.0.0 handlebars.l line 102
										return END;
									}

%%
