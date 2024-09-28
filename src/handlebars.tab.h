/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY_HANDLEBARS_YY_HANDLEBARS_TAB_H_INCLUDED
# define YY_HANDLEBARS_YY_HANDLEBARS_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int handlebars_yy_debug;
#endif
/* "%code requires" blocks.  */
#line 31 "handlebars.y"

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

#line 67 "handlebars.tab.h"

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    END = 0,                       /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    BOOLEAN = 258,                 /* BOOLEAN  */
    CLOSE = 259,                   /* "}}"  */
    CLOSE_RAW_BLOCK = 260,         /* "}}}}"  */
    CLOSE_SEXPR = 261,             /* ")"  */
    CLOSE_UNESCAPED = 262,         /* "}}}"  */
    COMMENT = 263,                 /* COMMENT  */
    CONTENT = 264,                 /* CONTENT  */
    DATA = 265,                    /* DATA  */
    END_RAW_BLOCK = 266,           /* END_RAW_BLOCK  */
    EQUALS = 267,                  /* "="  */
    ID = 268,                      /* ID  */
    INVALID = 269,                 /* INVALID  */
    INVERSE = 270,                 /* INVERSE  */
    NUMBER = 271,                  /* NUMBER  */
    OPEN = 272,                    /* "{{"  */
    OPEN_BLOCK = 273,              /* "{{#"  */
    OPEN_ENDBLOCK = 274,           /* OPEN_ENDBLOCK  */
    OPEN_INVERSE = 275,            /* "{{^"  */
    OPEN_PARTIAL = 276,            /* "{{>"  */
    OPEN_RAW_BLOCK = 277,          /* "{{{{"  */
    OPEN_SEXPR = 278,              /* "("  */
    OPEN_UNESCAPED = 279,          /* "{{{"  */
    SEP = 280,                     /* SEP  */
    STRING = 281,                  /* STRING  */
    CLOSE_BLOCK_PARAMS = 282,      /* CLOSE_BLOCK_PARAMS  */
    NUL = 283,                     /* "NULL"  */
    OPEN_BLOCK_PARAMS = 284,       /* OPEN_BLOCK_PARAMS  */
    OPEN_INVERSE_CHAIN = 285,      /* OPEN_INVERSE_CHAIN  */
    UNDEFINED = 286,               /* "undefined"  */
    OPEN_PARTIAL_BLOCK = 287,      /* "{{#>"  */
    LONG_COMMENT = 288,            /* LONG_COMMENT  */
    SINGLE_STRING = 289            /* SINGLE_STRING  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 95 "handlebars.y"

    struct handlebars_string * string;
    struct handlebars_ast_node * ast_node;
    struct handlebars_ast_list * ast_list;

    struct handlebars_yy_block_intermediate block_intermediate;
    struct handlebars_yy_block_params block_params;

#line 127 "handlebars.tab.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif

/* Location type.  */
#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE YYLTYPE;
struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
};
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif




int handlebars_yy_parse (struct handlebars_parser * parser);


#endif /* !YY_HANDLEBARS_YY_HANDLEBARS_TAB_H_INCLUDED  */
