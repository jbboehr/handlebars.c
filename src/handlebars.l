
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
 
#define handlebars_yy_unput_all \
	do { \
		int i; \
 		char * yycopy = strdup( yytext ); \
 		for ( i = yyleng - 1; i >= 0; --i ) { \
 			unput( yycopy[i] ); \
 		} \
        free( yycopy ); \
    } while(0)
 		
 
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

%x mu
%x emu
%x com
%x raw

LEFT_STRIP    "~"
RIGHT_STRIP   "~"

LOOKAHEAD           [=~}\s\/.)]
LITERAL_LOOKAHEAD   [~}\s)]

MU \{\{
CMU \}\}
EMU \\\}\{
CONTENT [^\x00\{\}\\]

ID    [^\s!\"#%-,\.\/;->@\[-\^\`\{-~]+/{LOOKAHEAD}


%%


{CONTENT}+                         	{
										// v2.0.0 handlebars.l line 31
										yylval->text = yytext;
										return CONTENT;
									}

\\{EMU} 							{
										// v2.0.0 handlebars.l line 32
										yy_push_state(mu, yyscanner);
										yylval->text = yytext;
										return OPEN;
									}
									
{EMU} 								{			
										// v2.0.0 handlebars.l line 35
										handlebars_yy_unput_all;
										yy_push_state(emu, yyscanner);
										//return CONTENT;
									}
									
{MU}  								{
										// v2.0.0 handlebars.l line 38
										handlebars_yy_unput_all;
										yy_push_state(mu, yyscanner);
										//return OPEN;
									}

<emu>{CONTENT}+ 					{
										// v2.0.0 handlebars.l line 47
                                   		yy_pop_state(yyg);
										yylval->text = yytext;
                                   		return CONTENT;
                                 	}

<raw>"{{{{/"[^\s!"#%-,\.\/;->@\[-\^`\{-~]+/[=}\s\/.]"}}}}" {
										// v2.0.0 handlebars.l line 52
                                  		yy_pop_state(yyg);
										yylval->text = yytext;
                                  		return END_RAW_BLOCK;
                                 	}

<raw>[^\x00]*?/("{{{{/")         	{
										// v2.0.0 handlebars.l line 57
										return CONTENT;
									}

<com>[^\x00\}\-]* 					{
										// v2.0.0 handlebars.l line 59
									}

<com>"--}}"							{
										// v2.0.0 handlebars.l line 59
  										yy_pop_state(yyg);
										yylval->text = yytext;
  										return COMMENT;
									}
									
<com>[\}\-]+						{
										// v2.0.0 handlebars.l line 59
										yylval->text = yytext;
									}

<mu>"("                          	{
										// v2.0.0 handlebars.l line 61
										yylval->text = yytext;
										return OPEN_SEXPR;
									}

<mu>")"                          	{
										// v2.0.0 handlebars.l line 62
										yylval->text = yytext;
										return CLOSE_SEXPR;
									}

<mu>"{{{{"                       	{
										// v2.0.0 handlebars.l line 64
										yylval->text = yytext;
										return OPEN_RAW_BLOCK;
									}

<mu>"}}}}"                       	{
										// v2.0.0 handlebars.l line 65
										yy_pop_state(yyg);
                                  		yy_push_state(raw, yyscanner);
										yylval->text = yytext;
                                  		return CLOSE_RAW_BLOCK;
                             		}
                             		
<mu>"{{"{LEFT_STRIP}?">"         	{
										// v2.0.0 handlebars.l line 70
										return OPEN_PARTIAL;
                             		}

<mu>"{{"{LEFT_STRIP}?"#"         	{
										// v2.0.0 handlebars.l line 71
										yylval->text = yytext;
										return OPEN_BLOCK;
                             		}

<mu>"{{"{LEFT_STRIP}?"/"         	{
										// v2.0.0 handlebars.l line 72
										yylval->text = yytext;
										return OPEN_ENDBLOCK;
                             		}

<mu>"{{"{LEFT_STRIP}?"^"\s*{RIGHT_STRIP}?"}}"        {
										// v2.0.0 handlebars.l line 73
										yy_pop_state(yyg);
										yylval->text = yytext;
										return INVERSE;
                             		}

<mu>"{{"{LEFT_STRIP}?\s*"else"\s*{RIGHT_STRIP}?"}}"  {
										// v2.0.0 handlebars.l line 74
										yy_pop_state(yyg);
										yylval->text = yytext;
										return INVERSE;
                             		}

<mu>"{{"{LEFT_STRIP}?"^"         	{
										// v2.0.0 handlebars.l line 75
										return OPEN_INVERSE;
                             		}

<mu>"{{"{LEFT_STRIP}?\s*"else"   	{
										// v2.0.0 handlebars.l line 76
										yylval->text = yytext;
										return OPEN_INVERSE;
                             		}

<mu>"{{"{LEFT_STRIP}?"{"         	{
										// v2.0.0 handlebars.l line 77
										yylval->text = yytext;
										return OPEN_UNESCAPED;
                             		}

<mu>"{{"{LEFT_STRIP}?"&"         	{
										// v2.0.0 handlebars.l line 78
										yylval->text = yytext;
										return OPEN;
                             		}
                             		
<mu>"{{!--" 						{
										// v2.0.0 handlebars.l line 79
  										// this.unput(yytext);
  										yy_pop_state(yyg);
  										yy_push_state(com, yyscanner);
										// ? yylval->text = yytext;
									}

<mu>"{{!"[\s\S]*?"}}" 				{
										// v2.0.0 handlebars.l line 80
  										yy_pop_state(yyg);
										yylval->text = yytext;
  										return COMMENT;
									}

<mu>"{{"{LEFT_STRIP}?            	{
										// v2.0.0 handlebars.l line 81
										yylval->text = yytext;
										return OPEN;
									}

<mu>"="                          	{
										// v2.0.0 handlebars.l line 83
										yylval->text = yytext;
										return EQUALS;
									}

<mu>".."                         	{
										// v2.0.0 handlebars.l line 84
										yylval->text = yytext;
										return ID;
									}

<mu>"."/{LOOKAHEAD}              	{
										// v2.0.0 handlebars.l line 85
										yylval->text = yytext;
										return ID;
									}

<mu>[\/.]                        	{
										// v2.0.0 handlebars.l line 86
										yylval->text = yytext;
										return SEP;
									}

<mu>\s+                          	{
										// v2.0.0 handlebars.l line 87
										// ignore whitespace
										// ? yylval->text = yytext;
                                 	}

<mu>"}"{RIGHT_STRIP}?"}}"        	{
										// v2.0.0 handlebars.l line 88
										yy_pop_state(yyg);
										yylval->text = yytext;
										return CLOSE_UNESCAPED;
									}

<mu>{RIGHT_STRIP}?"}}"           	{
										// v2.0.0 handlebars.l line 89
										yy_pop_state(yyg);
										yylval->text = yytext;
										return CLOSE;
									}

<mu>"\""("\\"["]|[^"])*"\""      	{
										// v2.0.0 handlebars.l line 90
										yylval->text = yytext;
                                   		return STRING;
                                 	}

<mu>"'"("\\"[']|[^'])*"'"        	{
										// v2.0.0 handlebars.l line 91
										yylval->text = yytext;
										return STRING;
                                	}
                                 
<mu>"@"                          	{
										// v2.0.0 handlebars.l line 92
										yylval->text = yytext;
										return DATA;
									}

<mu>"true"/{LITERAL_LOOKAHEAD}   	{
										// v2.0.0 handlebars.l line 93
										yylval->text = yytext;
										return BOOLEAN;
									}

<mu>"false"/{LITERAL_LOOKAHEAD}  	{
										// v2.0.0 handlebars.l line 94
										yylval->text = yytext;
										return BOOLEAN;
									}

<mu>\-?[0-9]+(?:\.[0-9]+)?/{LITERAL_LOOKAHEAD} {
										// v2.0.0 handlebars.l line 95
										yylval->text = yytext;
										return NUMBER;
									}

<mu>{ID}                			{
										// v2.0.0 handlebars.l line 97
										yylval->text = yytext;
										return ID;
									}

<mu>'['[^\]]*']'                 	{
										// v2.0.0 handlebars.l line 99
										// yytext = strip(1,2);
										yylval->text = yytext;
										return ID;
									}

<mu>.                            	{
										// v2.0.0 handlebars.l line 100
										yylval->text = yytext;
										return INVALID;
									}

<INITIAL,mu><<END>>              	{
										// v2.0.0 handlebars.l line 102
										yylval->text = yytext;
										return END;
									}

%%
