/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1


/* Substitute the variable and function names.  */
#define yyparse         handlebars_yy_parse
#define yylex           handlebars_yy_lex
#define yyerror         handlebars_yy_error
#define yydebug         handlebars_yy_debug
#define yynerrs         handlebars_yy_nerrs

/* First part of user prologue.  */
#line 51 "handlebars.y"


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

#line 120 "handlebars.tab.c"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

#include "handlebars.tab.h"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_BOOLEAN = 3,                    /* BOOLEAN  */
  YYSYMBOL_CLOSE = 4,                      /* "}}"  */
  YYSYMBOL_CLOSE_RAW_BLOCK = 5,            /* "}}}}"  */
  YYSYMBOL_CLOSE_SEXPR = 6,                /* ")"  */
  YYSYMBOL_CLOSE_UNESCAPED = 7,            /* "}}}"  */
  YYSYMBOL_COMMENT = 8,                    /* COMMENT  */
  YYSYMBOL_CONTENT = 9,                    /* CONTENT  */
  YYSYMBOL_DATA = 10,                      /* DATA  */
  YYSYMBOL_END_RAW_BLOCK = 11,             /* END_RAW_BLOCK  */
  YYSYMBOL_EQUALS = 12,                    /* "="  */
  YYSYMBOL_ID = 13,                        /* ID  */
  YYSYMBOL_INVALID = 14,                   /* INVALID  */
  YYSYMBOL_INVERSE = 15,                   /* INVERSE  */
  YYSYMBOL_NUMBER = 16,                    /* NUMBER  */
  YYSYMBOL_OPEN = 17,                      /* "{{"  */
  YYSYMBOL_OPEN_BLOCK = 18,                /* "{{#"  */
  YYSYMBOL_OPEN_ENDBLOCK = 19,             /* OPEN_ENDBLOCK  */
  YYSYMBOL_OPEN_INVERSE = 20,              /* "{{^"  */
  YYSYMBOL_OPEN_PARTIAL = 21,              /* "{{>"  */
  YYSYMBOL_OPEN_RAW_BLOCK = 22,            /* "{{{{"  */
  YYSYMBOL_OPEN_SEXPR = 23,                /* "("  */
  YYSYMBOL_OPEN_UNESCAPED = 24,            /* "{{{"  */
  YYSYMBOL_SEP = 25,                       /* SEP  */
  YYSYMBOL_STRING = 26,                    /* STRING  */
  YYSYMBOL_CLOSE_BLOCK_PARAMS = 27,        /* CLOSE_BLOCK_PARAMS  */
  YYSYMBOL_NUL = 28,                       /* "NULL"  */
  YYSYMBOL_OPEN_BLOCK_PARAMS = 29,         /* OPEN_BLOCK_PARAMS  */
  YYSYMBOL_OPEN_INVERSE_CHAIN = 30,        /* OPEN_INVERSE_CHAIN  */
  YYSYMBOL_UNDEFINED = 31,                 /* "undefined"  */
  YYSYMBOL_OPEN_PARTIAL_BLOCK = 32,        /* "{{#>"  */
  YYSYMBOL_LONG_COMMENT = 33,              /* LONG_COMMENT  */
  YYSYMBOL_SINGLE_STRING = 34,             /* SINGLE_STRING  */
  YYSYMBOL_35_ = 35,                       /* ""  */
  YYSYMBOL_YYACCEPT = 36,                  /* $accept  */
  YYSYMBOL_start = 37,                     /* start  */
  YYSYMBOL_program = 38,                   /* program  */
  YYSYMBOL_statements = 39,                /* statements  */
  YYSYMBOL_statement = 40,                 /* statement  */
  YYSYMBOL_content = 41,                   /* content  */
  YYSYMBOL_raw_block = 42,                 /* raw_block  */
  YYSYMBOL_open_raw_block = 43,            /* open_raw_block  */
  YYSYMBOL_block = 44,                     /* block  */
  YYSYMBOL_block_intermediate = 45,        /* block_intermediate  */
  YYSYMBOL_open_block = 46,                /* open_block  */
  YYSYMBOL_open_inverse = 47,              /* open_inverse  */
  YYSYMBOL_open_inverse_chain = 48,        /* open_inverse_chain  */
  YYSYMBOL_inverse_chain = 49,             /* inverse_chain  */
  YYSYMBOL_inverse_and_program = 50,       /* inverse_and_program  */
  YYSYMBOL_close_block = 51,               /* close_block  */
  YYSYMBOL_mustache = 52,                  /* mustache  */
  YYSYMBOL_partial = 53,                   /* partial  */
  YYSYMBOL_partial_block = 54,             /* partial_block  */
  YYSYMBOL_open_partial_block = 55,        /* open_partial_block  */
  YYSYMBOL_params = 56,                    /* params  */
  YYSYMBOL_param = 57,                     /* param  */
  YYSYMBOL_sexpr = 58,                     /* sexpr  */
  YYSYMBOL_intermediate4 = 59,             /* intermediate4  */
  YYSYMBOL_intermediate3 = 60,             /* intermediate3  */
  YYSYMBOL_hash = 61,                      /* hash  */
  YYSYMBOL_hash_pairs = 62,                /* hash_pairs  */
  YYSYMBOL_hash_pair = 63,                 /* hash_pair  */
  YYSYMBOL_block_params = 64,              /* block_params  */
  YYSYMBOL_helper_name = 65,               /* helper_name  */
  YYSYMBOL_partial_name = 66,              /* partial_name  */
  YYSYMBOL_data_name = 67,                 /* data_name  */
  YYSYMBOL_path = 68,                      /* path  */
  YYSYMBOL_path_segments = 69              /* path_segments  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_int8 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if 1

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* 1 */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL \
             && defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
  YYLTYPE yyls_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE) \
             + YYSIZEOF (YYLTYPE)) \
      + 2 * YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  50
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   283

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  36
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  34
/* YYNRULES -- Number of rules.  */
#define YYNRULES  80
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  125

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   290


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   188,   188,   196,   199,   206,   210,   217,   220,   223,
     226,   229,   232,   235,   242,   252,   256,   262,   265,   271,
     277,   280,   283,   286,   292,   296,   300,   307,   315,   322,
     329,   332,   335,   338,   341,   347,   351,   360,   367,   371,
     378,   382,   386,   390,   397,   400,   406,   410,   414,   418,
     425,   429,   436,   439,   445,   451,   456,   460,   463,   466,
     469,   475,   483,   487,   494,   500,   504,   511,   514,   517,
     520,   523,   526,   529,   532,   538,   541,   547,   553,   559,
     565
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if 1
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "BOOLEAN", "\"}}\"",
  "\"}}}}\"", "\")\"", "\"}}}\"", "COMMENT", "CONTENT", "DATA",
  "END_RAW_BLOCK", "\"=\"", "ID", "INVALID", "INVERSE", "NUMBER", "\"{{\"",
  "\"{{#\"", "OPEN_ENDBLOCK", "\"{{^\"", "\"{{>\"", "\"{{{{\"", "\"(\"",
  "\"{{{\"", "SEP", "STRING", "CLOSE_BLOCK_PARAMS", "\"NULL\"",
  "OPEN_BLOCK_PARAMS", "OPEN_INVERSE_CHAIN", "\"undefined\"", "\"{{#>\"",
  "LONG_COMMENT", "SINGLE_STRING", "\"\"", "$accept", "start", "program",
  "statements", "statement", "content", "raw_block", "open_raw_block",
  "block", "block_intermediate", "open_block", "open_inverse",
  "open_inverse_chain", "inverse_chain", "inverse_and_program",
  "close_block", "mustache", "partial", "partial_block",
  "open_partial_block", "params", "param", "sexpr", "intermediate4",
  "intermediate3", "hash", "hash_pairs", "hash_pair", "block_params",
  "helper_name", "partial_name", "data_name", "path", "path_segments", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-57)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-1)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     231,   -57,    13,   150,   150,   150,   124,   150,   150,   124,
     -57,   -57,    24,    27,   250,   -57,   -57,   -57,     3,   -57,
     174,   174,   -57,   -57,   -57,   212,   -57,   -57,    15,   -57,
     -57,   -57,   -57,   -57,   -57,    32,   146,   -57,   -57,     6,
      33,     9,    35,   150,   -57,   -57,    54,    36,    41,    87,
     -57,   -57,   -57,   -57,    31,   231,   150,   150,     2,    40,
     193,   -57,   -57,   -57,    40,   -57,    40,   -57,     6,   -57,
      43,   146,   -57,   -57,   -57,    47,   -57,   -57,    49,   -57,
      50,   -57,   -57,    60,   -57,    91,    64,   -57,   -57,   -57,
     120,    65,   -57,   -57,    67,    68,   -57,   -57,     2,   -57,
     -57,   -57,   124,   -57,   -57,    43,   -57,   -57,     8,   -57,
     -57,    69,   -57,   -57,    70,   -57,   -57,   -57,   -57,   -57,
      48,   -57,   -57,   -57,   -57
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       0,    13,    16,     0,     0,     0,     0,     0,     0,     0,
      14,     4,     0,     0,     3,     5,    12,     9,     0,     8,
       0,     0,     7,    10,    11,     0,    15,    72,     0,    80,
      71,    69,    74,    73,    70,     0,    60,    68,    67,    78,
       0,    56,     0,     0,    76,    75,     0,     0,     0,     0,
       1,     2,     6,    18,     0,    36,     0,     0,    26,     0,
      33,    24,    34,    21,     0,    23,     0,    45,    77,    38,
      80,    59,    50,    53,    58,    61,    63,    52,     0,    27,
       0,    55,    28,     0,    43,     0,     0,    19,    39,    49,
       0,     0,    17,    35,     0,     0,    25,    20,    32,    31,
      22,    44,     0,    51,    57,     0,    62,    79,     0,    54,
      41,     0,    42,    47,     0,    48,    37,    29,    30,    64,
       0,    66,    40,    46,    65
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -57,   -57,     1,   -57,    62,     7,   -57,   -57,   -57,    57,
     -57,   -57,   -57,   -53,   -57,   -15,   -57,   -57,   -57,   -57,
     -30,   -56,    14,    -3,    10,   -38,   -57,    11,   -57,    -6,
      72,   -57,   -57,    55
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
       0,    12,    58,    14,    15,    16,    17,    18,    19,    59,
      20,    21,    60,    61,    62,    63,    22,    23,    24,    25,
      71,    72,    73,    40,    41,    74,    75,    76,    81,    36,
      46,    37,    38,    39
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int8 yytable[] =
{
      45,    13,    42,    45,    47,    96,    65,    99,    86,    26,
      67,    91,     2,    35,    53,   103,    85,    55,    48,    90,
      44,   120,     2,    44,    50,    54,    66,    51,    29,   103,
      77,    78,    57,   104,   103,   121,    69,    79,    80,    82,
      77,    87,    92,    77,    97,   118,   119,   111,    88,   100,
      94,   101,   114,    83,    95,   102,    93,    27,    84,    56,
     105,    98,   107,   108,    28,    77,   109,    70,   112,   115,
      30,   116,   117,   122,   123,   124,    52,    43,    64,    77,
      31,    49,    32,    68,    77,    33,   106,     0,    34,     0,
      27,    89,     0,     0,    27,   110,    77,    28,     0,     0,
      70,    28,     0,    30,    70,     0,     0,    30,     0,     0,
      43,     0,     0,    31,    43,    32,     0,    31,    33,    32,
       0,    34,    33,    27,   113,    34,     0,    27,     0,     0,
      28,     0,     0,    70,    28,     0,    30,    29,     0,     0,
      30,     0,     0,    43,     0,     0,    31,    43,    32,    27,
      31,    33,    32,    27,    34,    33,    28,     0,    34,    70,
      28,     0,    30,    29,     0,     0,    30,     0,     0,    43,
       0,     0,    31,     0,    32,     0,    31,    33,    32,     0,
      34,    33,     1,     2,    34,     0,     0,     0,     0,    55,
       0,     3,     4,    56,     5,     6,     7,     0,     8,     0,
       0,     1,     2,     0,    57,     0,     9,    10,    55,    11,
       3,     4,     0,     5,     6,     7,     0,     8,     0,     0,
       1,     2,     0,    57,     0,     9,    10,     0,    11,     3,
       4,    56,     5,     6,     7,     0,     8,     0,     0,     1,
       2,     0,     0,     0,     9,    10,     0,    11,     3,     4,
       0,     5,     6,     7,     0,     8,     0,     0,     1,     2,
       0,     0,     0,     9,    10,     0,    11,     3,     4,     0,
       5,     6,     7,     0,     8,     0,     0,     0,     0,     0,
       0,     0,     9,    10
};

static const yytype_int8 yycheck[] =
{
       6,     0,     5,     9,     7,    58,    21,    60,    46,     2,
      25,    49,     9,     3,    11,    71,    46,    15,     8,    49,
       6,    13,     9,     9,     0,    18,    25,     0,    13,    85,
      36,    25,    30,    71,    90,    27,     4,     4,    29,     4,
      46,     5,    11,    49,    59,    98,   102,    85,     7,    64,
      56,    66,    90,    43,    57,    12,    55,     3,     4,    19,
      13,    60,    13,    13,    10,    71,     6,    13,     4,     4,
      16,     4,     4,     4,     4,    27,    14,    23,    21,    85,
      26,     9,    28,    28,    90,    31,    75,    -1,    34,    -1,
       3,     4,    -1,    -1,     3,     4,   102,    10,    -1,    -1,
      13,    10,    -1,    16,    13,    -1,    -1,    16,    -1,    -1,
      23,    -1,    -1,    26,    23,    28,    -1,    26,    31,    28,
      -1,    34,    31,     3,     4,    34,    -1,     3,    -1,    -1,
      10,    -1,    -1,    13,    10,    -1,    16,    13,    -1,    -1,
      16,    -1,    -1,    23,    -1,    -1,    26,    23,    28,     3,
      26,    31,    28,     3,    34,    31,    10,    -1,    34,    13,
      10,    -1,    16,    13,    -1,    -1,    16,    -1,    -1,    23,
      -1,    -1,    26,    -1,    28,    -1,    26,    31,    28,    -1,
      34,    31,     8,     9,    34,    -1,    -1,    -1,    -1,    15,
      -1,    17,    18,    19,    20,    21,    22,    -1,    24,    -1,
      -1,     8,     9,    -1,    30,    -1,    32,    33,    15,    35,
      17,    18,    -1,    20,    21,    22,    -1,    24,    -1,    -1,
       8,     9,    -1,    30,    -1,    32,    33,    -1,    35,    17,
      18,    19,    20,    21,    22,    -1,    24,    -1,    -1,     8,
       9,    -1,    -1,    -1,    32,    33,    -1,    35,    17,    18,
      -1,    20,    21,    22,    -1,    24,    -1,    -1,     8,     9,
      -1,    -1,    -1,    32,    33,    -1,    35,    17,    18,    -1,
      20,    21,    22,    -1,    24,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    32,    33
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,     8,     9,    17,    18,    20,    21,    22,    24,    32,
      33,    35,    37,    38,    39,    40,    41,    42,    43,    44,
      46,    47,    52,    53,    54,    55,    41,     3,    10,    13,
      16,    26,    28,    31,    34,    60,    65,    67,    68,    69,
      59,    60,    59,    23,    58,    65,    66,    59,    60,    66,
       0,     0,    40,    11,    41,    15,    19,    30,    38,    45,
      48,    49,    50,    51,    45,    51,    38,    51,    69,     4,
      13,    56,    57,    58,    61,    62,    63,    65,    25,     4,
      29,    64,     4,    60,     4,    56,    61,     5,     7,     4,
      56,    61,    11,    38,    65,    59,    49,    51,    38,    49,
      51,    51,    12,    57,    61,    13,    63,    13,    13,     6,
       4,    61,     4,     4,    61,     4,     4,     4,    49,    57,
      13,    27,     4,     4,    27
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    36,    37,    38,    38,    39,    39,    40,    40,    40,
      40,    40,    40,    40,    40,    41,    41,    42,    42,    43,
      44,    44,    44,    44,    45,    45,    45,    46,    47,    48,
      49,    49,    49,    49,    49,    50,    50,    51,    52,    52,
      53,    53,    53,    53,    54,    54,    55,    55,    55,    55,
      56,    56,    57,    57,    58,    59,    59,    60,    60,    60,
      60,    61,    62,    62,    63,    64,    64,    65,    65,    65,
      65,    65,    65,    65,    65,    66,    66,    67,    68,    69,
      69
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     2,     1,     1,     1,     2,     1,     1,     1,
       1,     1,     1,     1,     1,     2,     1,     3,     2,     3,
       3,     2,     3,     2,     1,     2,     1,     3,     3,     3,
       3,     2,     2,     1,     1,     2,     1,     3,     3,     3,
       5,     4,     4,     3,     3,     2,     5,     4,     4,     3,
       1,     2,     1,     1,     3,     2,     1,     3,     2,     2,
       1,     1,     2,     1,     3,     4,     3,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     2,     1,     3,
       1
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (&yylloc, parser, YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF

/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)                                \
    do                                                                  \
      if (N)                                                            \
        {                                                               \
          (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;        \
          (Current).first_column = YYRHSLOC (Rhs, 1).first_column;      \
          (Current).last_line    = YYRHSLOC (Rhs, N).last_line;         \
          (Current).last_column  = YYRHSLOC (Rhs, N).last_column;       \
        }                                                               \
      else                                                              \
        {                                                               \
          (Current).first_line   = (Current).last_line   =              \
            YYRHSLOC (Rhs, 0).last_line;                                \
          (Current).first_column = (Current).last_column =              \
            YYRHSLOC (Rhs, 0).last_column;                              \
        }                                                               \
    while (0)
#endif

#define YYRHSLOC(Rhs, K) ((Rhs)[K])


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)


/* YYLOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

# ifndef YYLOCATION_PRINT

#  if defined YY_LOCATION_PRINT

   /* Temporary convenience wrapper in case some people defined the
      undocumented and private YY_LOCATION_PRINT macros.  */
#   define YYLOCATION_PRINT(File, Loc)  YY_LOCATION_PRINT(File, *(Loc))

#  elif defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL

/* Print *YYLOCP on YYO.  Private, do not rely on its existence. */

YY_ATTRIBUTE_UNUSED
static int
yy_location_print_ (FILE *yyo, YYLTYPE const * const yylocp)
{
  int res = 0;
  int end_col = 0 != yylocp->last_column ? yylocp->last_column - 1 : 0;
  if (0 <= yylocp->first_line)
    {
      res += YYFPRINTF (yyo, "%d", yylocp->first_line);
      if (0 <= yylocp->first_column)
        res += YYFPRINTF (yyo, ".%d", yylocp->first_column);
    }
  if (0 <= yylocp->last_line)
    {
      if (yylocp->first_line < yylocp->last_line)
        {
          res += YYFPRINTF (yyo, "-%d", yylocp->last_line);
          if (0 <= end_col)
            res += YYFPRINTF (yyo, ".%d", end_col);
        }
      else if (0 <= end_col && yylocp->first_column < end_col)
        res += YYFPRINTF (yyo, "-%d", end_col);
    }
  return res;
}

#   define YYLOCATION_PRINT  yy_location_print_

    /* Temporary convenience wrapper in case some people defined the
       undocumented and private YY_LOCATION_PRINT macros.  */
#   define YY_LOCATION_PRINT(File, Loc)  YYLOCATION_PRINT(File, &(Loc))

#  else

#   define YYLOCATION_PRINT(File, Loc) ((void) 0)
    /* Temporary convenience wrapper in case some people defined the
       undocumented and private YY_LOCATION_PRINT macros.  */
#   define YY_LOCATION_PRINT  YYLOCATION_PRINT

#  endif
# endif /* !defined YYLOCATION_PRINT */


# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value, Location, parser); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, struct handlebars_parser * parser)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  YY_USE (yylocationp);
  YY_USE (parser);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, struct handlebars_parser * parser)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  YYLOCATION_PRINT (yyo, yylocationp);
  YYFPRINTF (yyo, ": ");
  yy_symbol_value_print (yyo, yykind, yyvaluep, yylocationp, parser);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp, YYLTYPE *yylsp,
                 int yyrule, struct handlebars_parser * parser)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)],
                       &(yylsp[(yyi + 1) - (yynrhs)]), parser);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, yylsp, Rule, parser); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


/* Context of a parse error.  */
typedef struct
{
  yy_state_t *yyssp;
  yysymbol_kind_t yytoken;
  YYLTYPE *yylloc;
} yypcontext_t;

/* Put in YYARG at most YYARGN of the expected tokens given the
   current YYCTX, and return the number of tokens stored in YYARG.  If
   YYARG is null, return the number of expected tokens (guaranteed to
   be less than YYNTOKENS).  Return YYENOMEM on memory exhaustion.
   Return 0 if there are more than YYARGN expected tokens, yet fill
   YYARG up to YYARGN. */
static int
yypcontext_expected_tokens (const yypcontext_t *yyctx,
                            yysymbol_kind_t yyarg[], int yyargn)
{
  /* Actual size of YYARG. */
  int yycount = 0;
  int yyn = yypact[+*yyctx->yyssp];
  if (!yypact_value_is_default (yyn))
    {
      /* Start YYX at -YYN if negative to avoid negative indexes in
         YYCHECK.  In other words, skip the first -YYN actions for
         this state because they are default actions.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;
      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yyx;
      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
        if (yycheck[yyx + yyn] == yyx && yyx != YYSYMBOL_YYerror
            && !yytable_value_is_error (yytable[yyx + yyn]))
          {
            if (!yyarg)
              ++yycount;
            else if (yycount == yyargn)
              return 0;
            else
              yyarg[yycount++] = YY_CAST (yysymbol_kind_t, yyx);
          }
    }
  if (yyarg && yycount == 0 && 0 < yyargn)
    yyarg[0] = YYSYMBOL_YYEMPTY;
  return yycount;
}




#ifndef yystrlen
# if defined __GLIBC__ && defined _STRING_H
#  define yystrlen(S) (YY_CAST (YYPTRDIFF_T, strlen (S)))
# else
/* Return the length of YYSTR.  */
static YYPTRDIFF_T
yystrlen (const char *yystr)
{
  YYPTRDIFF_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
# endif
#endif

#ifndef yystpcpy
# if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#  define yystpcpy stpcpy
# else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
yystpcpy (char *yydest, const char *yysrc)
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
# endif
#endif

#ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYPTRDIFF_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYPTRDIFF_T yyn = 0;
      char const *yyp = yystr;
      for (;;)
        switch (*++yyp)
          {
          case '\'':
          case ',':
            goto do_not_strip_quotes;

          case '\\':
            if (*++yyp != '\\')
              goto do_not_strip_quotes;
            else
              goto append;

          append:
          default:
            if (yyres)
              yyres[yyn] = *yyp;
            yyn++;
            break;

          case '"':
            if (yyres)
              yyres[yyn] = '\0';
            return yyn;
          }
    do_not_strip_quotes: ;
    }

  if (yyres)
    return yystpcpy (yyres, yystr) - yyres;
  else
    return yystrlen (yystr);
}
#endif


static int
yy_syntax_error_arguments (const yypcontext_t *yyctx,
                           yysymbol_kind_t yyarg[], int yyargn)
{
  /* Actual size of YYARG. */
  int yycount = 0;
  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yyctx->yytoken != YYSYMBOL_YYEMPTY)
    {
      int yyn;
      if (yyarg)
        yyarg[yycount] = yyctx->yytoken;
      ++yycount;
      yyn = yypcontext_expected_tokens (yyctx,
                                        yyarg ? yyarg + 1 : yyarg, yyargn - 1);
      if (yyn == YYENOMEM)
        return YYENOMEM;
      else
        yycount += yyn;
    }
  return yycount;
}

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return -1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return YYENOMEM if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYPTRDIFF_T *yymsg_alloc, char **yymsg,
                const yypcontext_t *yyctx)
{
  enum { YYARGS_MAX = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat: reported tokens (one for the "unexpected",
     one per "expected"). */
  yysymbol_kind_t yyarg[YYARGS_MAX];
  /* Cumulated lengths of YYARG.  */
  YYPTRDIFF_T yysize = 0;

  /* Actual size of YYARG. */
  int yycount = yy_syntax_error_arguments (yyctx, yyarg, YYARGS_MAX);
  if (yycount == YYENOMEM)
    return YYENOMEM;

  switch (yycount)
    {
#define YYCASE_(N, S)                       \
      case N:                               \
        yyformat = S;                       \
        break
    default: /* Avoid compiler warnings. */
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
#undef YYCASE_
    }

  /* Compute error message size.  Don't count the "%s"s, but reserve
     room for the terminator.  */
  yysize = yystrlen (yyformat) - 2 * yycount + 1;
  {
    int yyi;
    for (yyi = 0; yyi < yycount; ++yyi)
      {
        YYPTRDIFF_T yysize1
          = yysize + yytnamerr (YY_NULLPTR, yytname[yyarg[yyi]]);
        if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
          yysize = yysize1;
        else
          return YYENOMEM;
      }
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return -1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yytname[yyarg[yyi++]]);
          yyformat += 2;
        }
      else
        {
          ++yyp;
          ++yyformat;
        }
  }
  return 0;
}


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep, YYLTYPE *yylocationp, struct handlebars_parser * parser)
{
  YY_USE (yyvaluep);
  YY_USE (yylocationp);
  YY_USE (parser);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}






/*----------.
| yyparse.  |
`----------*/

int
yyparse (struct handlebars_parser * parser)
{
/* Lookahead token kind.  */
int yychar;


/* The semantic value of the lookahead symbol.  */
/* Default value used for initialization, for pacifying older GCCs
   or non-GCC compilers.  */
YY_INITIAL_VALUE (static YYSTYPE yyval_default;)
YYSTYPE yylval YY_INITIAL_VALUE (= yyval_default);

/* Location data for the lookahead symbol.  */
static YYLTYPE yyloc_default
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
  = { 1, 1, 1, 1 }
# endif
;
YYLTYPE yylloc = yyloc_default;

    /* Number of syntax errors so far.  */
    int yynerrs = 0;

    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

    /* The location stack: array, bottom, top.  */
    YYLTYPE yylsa[YYINITDEPTH];
    YYLTYPE *yyls = yylsa;
    YYLTYPE *yylsp = yyls;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
  YYLTYPE yyloc;

  /* The locations where the error started and ended.  */
  YYLTYPE yyerror_range[3];

  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYPTRDIFF_T yymsg_alloc = sizeof yymsgbuf;

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N), yylsp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */

  yylsp[0] = yylloc;
  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;
        YYLTYPE *yyls1 = yyls;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yyls1, yysize * YYSIZEOF (*yylsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
        yyls = yyls1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
        YYSTACK_RELOCATE (yyls_alloc, yyls);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
      yylsp = yyls + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */


  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex (&yylval, &yylloc, scanner);
    }

  if (yychar <= END)
    {
      yychar = END;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      yyerror_range[1] = yylloc;
      goto yyerrlab1;
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END
  *++yylsp = yylloc;

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];

  /* Default location. */
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
  yyerror_range[1] = yyloc;
  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 2: /* start: program "end of file"  */
#line 188 "handlebars.y"
                {
      parser->program = (yyvsp[-1].ast_node);
      handlebars_whitespace_accept(parser, parser->program);
      return 1;
    }
#line 1701 "handlebars.tab.c"
    break;

  case 3: /* program: statements  */
#line 196 "handlebars.y"
               {
      (yyval.ast_node) = handlebars_ast_node_ctor_program(parser, (yyvsp[0].ast_list), NULL, NULL, 0, 0, &(yyloc));
    }
#line 1709 "handlebars.tab.c"
    break;

  case 4: /* program: ""  */
#line 199 "handlebars.y"
       {
      struct handlebars_ast_list * list = handlebars_ast_list_ctor(CONTEXT);
      (yyval.ast_node) = handlebars_ast_node_ctor_program(parser, list, NULL, NULL, 0, 0, &(yyloc));
    }
#line 1718 "handlebars.tab.c"
    break;

  case 5: /* statements: statement  */
#line 206 "handlebars.y"
              {
      (yyval.ast_list) = handlebars_ast_list_ctor(CONTEXT);
      handlebars_ast_list_append((yyval.ast_list), (yyvsp[0].ast_node));
    }
#line 1727 "handlebars.tab.c"
    break;

  case 6: /* statements: statements statement  */
#line 210 "handlebars.y"
                         {
      handlebars_ast_list_append((yyvsp[-1].ast_list), (yyvsp[0].ast_node));
      (yyval.ast_list) = (yyvsp[-1].ast_list);
    }
#line 1736 "handlebars.tab.c"
    break;

  case 7: /* statement: mustache  */
#line 217 "handlebars.y"
             {
      (yyval.ast_node) = (yyvsp[0].ast_node);
    }
#line 1744 "handlebars.tab.c"
    break;

  case 8: /* statement: block  */
#line 220 "handlebars.y"
          {
      (yyval.ast_node) = (yyvsp[0].ast_node);
    }
#line 1752 "handlebars.tab.c"
    break;

  case 9: /* statement: raw_block  */
#line 223 "handlebars.y"
              {
      (yyval.ast_node) = (yyvsp[0].ast_node);
    }
#line 1760 "handlebars.tab.c"
    break;

  case 10: /* statement: partial  */
#line 226 "handlebars.y"
            {
      (yyval.ast_node) = (yyvsp[0].ast_node);
    }
#line 1768 "handlebars.tab.c"
    break;

  case 11: /* statement: partial_block  */
#line 229 "handlebars.y"
                  {
      (yyval.ast_node) = (yyvsp[0].ast_node);
    }
#line 1776 "handlebars.tab.c"
    break;

  case 12: /* statement: content  */
#line 232 "handlebars.y"
            {
      (yyval.ast_node) = handlebars_ast_node_ctor_content(parser, (yyvsp[0].string), &(yyloc));
    }
#line 1784 "handlebars.tab.c"
    break;

  case 13: /* statement: COMMENT  */
#line 235 "handlebars.y"
            {
      // Strip comment strips in place
      unsigned strip = handlebars_ast_helper_strip_flags((yyvsp[0].string), (yyvsp[0].string));
      (yyval.ast_node) = handlebars_ast_node_ctor_comment(parser,
      			handlebars_ast_helper_strip_comment((yyvsp[0].string)), false, &(yyloc));
      handlebars_ast_node_set_strip((yyval.ast_node), strip);
    }
#line 1796 "handlebars.tab.c"
    break;

  case 14: /* statement: LONG_COMMENT  */
#line 242 "handlebars.y"
                 {
      // Strip comment strips in place
      unsigned strip = handlebars_ast_helper_strip_flags((yyvsp[0].string), (yyvsp[0].string));
      (yyval.ast_node) = handlebars_ast_node_ctor_comment(parser,
      			handlebars_ast_helper_strip_comment((yyvsp[0].string)), true, &(yyloc));
      handlebars_ast_node_set_strip((yyval.ast_node), strip);
  }
#line 1808 "handlebars.tab.c"
    break;

  case 15: /* content: CONTENT content  */
#line 252 "handlebars.y"
                    {
      (yyval.string) = handlebars_string_append_str(CONTEXT, (yyvsp[-1].string), (yyvsp[0].string));
      (yyval.string) = talloc_steal(parser, (yyval.string));
    }
#line 1817 "handlebars.tab.c"
    break;

  case 16: /* content: CONTENT  */
#line 256 "handlebars.y"
            {
      (yyval.string) = (yyvsp[0].string);
    }
#line 1825 "handlebars.tab.c"
    break;

  case 17: /* raw_block: open_raw_block content END_RAW_BLOCK  */
#line 262 "handlebars.y"
                                         {
      (yyval.ast_node) = handlebars_ast_helper_prepare_raw_block(parser, (yyvsp[-2].ast_node), (yyvsp[-1].string), (yyvsp[0].string), &(yyloc));
    }
#line 1833 "handlebars.tab.c"
    break;

  case 18: /* raw_block: open_raw_block END_RAW_BLOCK  */
#line 265 "handlebars.y"
                                   {
      (yyval.ast_node) = handlebars_ast_helper_prepare_raw_block(parser, (yyvsp[-1].ast_node), handlebars_string_ctor(HBSCTX(parser), HBS_STRL("")), (yyvsp[0].string), &(yyloc));
    }
#line 1841 "handlebars.tab.c"
    break;

  case 19: /* open_raw_block: "{{{{" intermediate4 "}}}}"  */
#line 271 "handlebars.y"
                                                 {
      (yyval.ast_node) = (yyvsp[-1].ast_node);
    }
#line 1849 "handlebars.tab.c"
    break;

  case 20: /* block: open_block block_intermediate close_block  */
#line 277 "handlebars.y"
                                              {
      (yyval.ast_node) = handlebars_ast_helper_prepare_block(parser, (yyvsp[-2].ast_node), (yyvsp[-1].block_intermediate).program, (yyvsp[-1].block_intermediate).inverse_chain, (yyvsp[0].ast_node), 0, &(yyloc));
    }
#line 1857 "handlebars.tab.c"
    break;

  case 21: /* block: open_block close_block  */
#line 280 "handlebars.y"
                           {
      (yyval.ast_node) = handlebars_ast_helper_prepare_block(parser, (yyvsp[-1].ast_node), NULL, NULL, (yyvsp[0].ast_node), 0, &(yyloc));
    }
#line 1865 "handlebars.tab.c"
    break;

  case 22: /* block: open_inverse block_intermediate close_block  */
#line 283 "handlebars.y"
                                                {
      (yyval.ast_node) = handlebars_ast_helper_prepare_block(parser, (yyvsp[-2].ast_node), (yyvsp[-1].block_intermediate).program, (yyvsp[-1].block_intermediate).inverse_chain, (yyvsp[0].ast_node), 1, &(yyloc));
    }
#line 1873 "handlebars.tab.c"
    break;

  case 23: /* block: open_inverse close_block  */
#line 286 "handlebars.y"
                             {
      (yyval.ast_node) = handlebars_ast_helper_prepare_block(parser, (yyvsp[-1].ast_node), NULL, NULL, (yyvsp[0].ast_node), 1, &(yyloc));
    }
#line 1881 "handlebars.tab.c"
    break;

  case 24: /* block_intermediate: inverse_chain  */
#line 292 "handlebars.y"
                  {
      (yyval.block_intermediate).program = NULL;
      (yyval.block_intermediate).inverse_chain = (yyvsp[0].ast_node);
    }
#line 1890 "handlebars.tab.c"
    break;

  case 25: /* block_intermediate: program inverse_chain  */
#line 296 "handlebars.y"
                          {
      (yyval.block_intermediate).program = (yyvsp[-1].ast_node);
      (yyval.block_intermediate).inverse_chain = (yyvsp[0].ast_node);
    }
#line 1899 "handlebars.tab.c"
    break;

  case 26: /* block_intermediate: program  */
#line 300 "handlebars.y"
            {
      (yyval.block_intermediate).program = (yyvsp[0].ast_node);
      (yyval.block_intermediate).inverse_chain = NULL;
    }
#line 1908 "handlebars.tab.c"
    break;

  case 27: /* open_block: "{{#" intermediate4 "}}"  */
#line 307 "handlebars.y"
                                   {
      (yyval.ast_node) = (yyvsp[-1].ast_node);
      handlebars_ast_node_set_strip((yyval.ast_node), handlebars_ast_helper_strip_flags((yyvsp[-2].string), (yyvsp[0].string)));
      (yyval.ast_node)->node.intermediate.open = talloc_steal((yyval.ast_node), handlebars_string_copy_ctor(CONTEXT, (yyvsp[-2].string)));
    }
#line 1918 "handlebars.tab.c"
    break;

  case 28: /* open_inverse: "{{^" intermediate4 "}}"  */
#line 315 "handlebars.y"
                                     {
      (yyval.ast_node) = (yyvsp[-1].ast_node);
      handlebars_ast_node_set_strip((yyval.ast_node), handlebars_ast_helper_strip_flags((yyvsp[-2].string), (yyvsp[0].string)));
    }
#line 1927 "handlebars.tab.c"
    break;

  case 29: /* open_inverse_chain: OPEN_INVERSE_CHAIN intermediate4 "}}"  */
#line 322 "handlebars.y"
                                           {
      (yyval.ast_node) = (yyvsp[-1].ast_node);
      handlebars_ast_node_set_strip((yyval.ast_node), handlebars_ast_helper_strip_flags((yyvsp[-2].string), (yyvsp[0].string)));
    }
#line 1936 "handlebars.tab.c"
    break;

  case 30: /* inverse_chain: open_inverse_chain program inverse_chain  */
#line 329 "handlebars.y"
                                             {
      (yyval.ast_node) = handlebars_ast_helper_prepare_inverse_chain(parser, (yyvsp[-2].ast_node), (yyvsp[-1].ast_node), (yyvsp[0].ast_node), &(yyloc));
  	}
#line 1944 "handlebars.tab.c"
    break;

  case 31: /* inverse_chain: open_inverse_chain inverse_chain  */
#line 332 "handlebars.y"
                                     {
      (yyval.ast_node) = handlebars_ast_helper_prepare_inverse_chain(parser, (yyvsp[-1].ast_node), NULL, (yyvsp[0].ast_node), &(yyloc));
  	}
#line 1952 "handlebars.tab.c"
    break;

  case 32: /* inverse_chain: open_inverse_chain program  */
#line 335 "handlebars.y"
                               {
      (yyval.ast_node) = handlebars_ast_helper_prepare_inverse_chain(parser, (yyvsp[-1].ast_node), (yyvsp[0].ast_node), NULL, &(yyloc));
    }
#line 1960 "handlebars.tab.c"
    break;

  case 33: /* inverse_chain: open_inverse_chain  */
#line 338 "handlebars.y"
                       {
      (yyval.ast_node) = handlebars_ast_helper_prepare_inverse_chain(parser, (yyvsp[0].ast_node), NULL, NULL, &(yyloc));
    }
#line 1968 "handlebars.tab.c"
    break;

  case 34: /* inverse_chain: inverse_and_program  */
#line 341 "handlebars.y"
                        {
      (yyval.ast_node) = (yyvsp[0].ast_node);
    }
#line 1976 "handlebars.tab.c"
    break;

  case 35: /* inverse_and_program: INVERSE program  */
#line 347 "handlebars.y"
                    {
      (yyval.ast_node) = handlebars_ast_node_ctor_inverse(parser, (yyvsp[0].ast_node), 0,
              handlebars_ast_helper_strip_flags((yyvsp[-1].string), (yyvsp[-1].string)), &(yyloc));
    }
#line 1985 "handlebars.tab.c"
    break;

  case 36: /* inverse_and_program: INVERSE  */
#line 351 "handlebars.y"
            {
      struct handlebars_ast_node * program_node;
      program_node = handlebars_ast_node_ctor(CONTEXT, HANDLEBARS_AST_NODE_PROGRAM);
      (yyval.ast_node) = handlebars_ast_node_ctor_inverse(parser, program_node, 0,
              handlebars_ast_helper_strip_flags((yyvsp[0].string), (yyvsp[0].string)), &(yyloc));
    }
#line 1996 "handlebars.tab.c"
    break;

  case 37: /* close_block: OPEN_ENDBLOCK helper_name "}}"  */
#line 360 "handlebars.y"
                                    {
      (yyval.ast_node) = handlebars_ast_node_ctor_intermediate(parser, (yyvsp[-1].ast_node), NULL, NULL,
              handlebars_ast_helper_strip_flags((yyvsp[-2].string), (yyvsp[0].string)), &(yyloc));
    }
#line 2005 "handlebars.tab.c"
    break;

  case 38: /* mustache: "{{" intermediate3 "}}"  */
#line 367 "handlebars.y"
                             {
      (yyval.ast_node) = handlebars_ast_helper_prepare_mustache(parser, (yyvsp[-1].ast_node), (yyvsp[-2].string),
        			handlebars_ast_helper_strip_flags((yyvsp[-2].string), (yyvsp[0].string)), &(yyloc));
    }
#line 2014 "handlebars.tab.c"
    break;

  case 39: /* mustache: "{{{" intermediate3 "}}}"  */
#line 371 "handlebars.y"
                                                 {
      (yyval.ast_node) = handlebars_ast_helper_prepare_mustache(parser, (yyvsp[-1].ast_node), (yyvsp[-2].string),
        			handlebars_ast_helper_strip_flags((yyvsp[-2].string), (yyvsp[0].string)), &(yyloc));
    }
#line 2023 "handlebars.tab.c"
    break;

  case 40: /* partial: "{{>" partial_name params hash "}}"  */
#line 378 "handlebars.y"
                                                {
      (yyval.ast_node) = handlebars_ast_node_ctor_partial(parser, (yyvsp[-3].ast_node), (yyvsp[-2].ast_list), (yyvsp[-1].ast_node),
              handlebars_ast_helper_strip_flags((yyvsp[-4].string), (yyvsp[0].string)), &(yyloc));
    }
#line 2032 "handlebars.tab.c"
    break;

  case 41: /* partial: "{{>" partial_name params "}}"  */
#line 382 "handlebars.y"
                                           {
      (yyval.ast_node) = handlebars_ast_node_ctor_partial(parser, (yyvsp[-2].ast_node), (yyvsp[-1].ast_list), NULL,
              handlebars_ast_helper_strip_flags((yyvsp[-3].string), (yyvsp[0].string)), &(yyloc));
    }
#line 2041 "handlebars.tab.c"
    break;

  case 42: /* partial: "{{>" partial_name hash "}}"  */
#line 386 "handlebars.y"
                                         {
      (yyval.ast_node) = handlebars_ast_node_ctor_partial(parser, (yyvsp[-2].ast_node), NULL, (yyvsp[-1].ast_node),
              handlebars_ast_helper_strip_flags((yyvsp[-3].string), (yyvsp[0].string)), &(yyloc));
    }
#line 2050 "handlebars.tab.c"
    break;

  case 43: /* partial: "{{>" partial_name "}}"  */
#line 390 "handlebars.y"
                                    {
      (yyval.ast_node) = handlebars_ast_node_ctor_partial(parser, (yyvsp[-1].ast_node), NULL, NULL,
              handlebars_ast_helper_strip_flags((yyvsp[-2].string), (yyvsp[0].string)), &(yyloc));
    }
#line 2059 "handlebars.tab.c"
    break;

  case 44: /* partial_block: open_partial_block program close_block  */
#line 397 "handlebars.y"
                                           {
      (yyval.ast_node) = handlebars_ast_helper_prepare_partial_block(parser, (yyvsp[-2].ast_node), (yyvsp[-1].ast_node), (yyvsp[0].ast_node), &(yyloc));
  }
#line 2067 "handlebars.tab.c"
    break;

  case 45: /* partial_block: open_partial_block close_block  */
#line 400 "handlebars.y"
                                   {
      struct handlebars_ast_node * program = handlebars_ast_node_ctor(CONTEXT, HANDLEBARS_AST_NODE_PROGRAM);
      (yyval.ast_node) = handlebars_ast_helper_prepare_partial_block(parser, (yyvsp[-1].ast_node), program, (yyvsp[0].ast_node), &(yyloc));
  }
#line 2076 "handlebars.tab.c"
    break;

  case 46: /* open_partial_block: "{{#>" partial_name params hash "}}"  */
#line 406 "handlebars.y"
                                                      {
      (yyval.ast_node) = handlebars_ast_node_ctor_intermediate(parser, (yyvsp[-3].ast_node), (yyvsp[-2].ast_list), (yyvsp[-1].ast_node),
      			handlebars_ast_helper_strip_flags((yyvsp[-4].string), (yyvsp[0].string)), &(yyloc));
    }
#line 2085 "handlebars.tab.c"
    break;

  case 47: /* open_partial_block: "{{#>" partial_name params "}}"  */
#line 410 "handlebars.y"
                                                 {
      (yyval.ast_node) = handlebars_ast_node_ctor_intermediate(parser, (yyvsp[-2].ast_node), (yyvsp[-1].ast_list), NULL,
      			handlebars_ast_helper_strip_flags((yyvsp[-3].string), (yyvsp[0].string)), &(yyloc));
    }
#line 2094 "handlebars.tab.c"
    break;

  case 48: /* open_partial_block: "{{#>" partial_name hash "}}"  */
#line 414 "handlebars.y"
                                               {
      (yyval.ast_node) = handlebars_ast_node_ctor_intermediate(parser, (yyvsp[-2].ast_node), NULL, (yyvsp[-1].ast_node),
              handlebars_ast_helper_strip_flags((yyvsp[-3].string), (yyvsp[0].string)), &(yyloc));
    }
#line 2103 "handlebars.tab.c"
    break;

  case 49: /* open_partial_block: "{{#>" partial_name "}}"  */
#line 418 "handlebars.y"
                                          {
      (yyval.ast_node) = handlebars_ast_node_ctor_intermediate(parser, (yyvsp[-1].ast_node), NULL, NULL,
              handlebars_ast_helper_strip_flags((yyvsp[-2].string), (yyvsp[0].string)), &(yyloc));
    }
#line 2112 "handlebars.tab.c"
    break;

  case 50: /* params: param  */
#line 425 "handlebars.y"
          {
      (yyval.ast_list) = handlebars_ast_list_ctor(CONTEXT);
      handlebars_ast_list_append((yyval.ast_list), (yyvsp[0].ast_node));
    }
#line 2121 "handlebars.tab.c"
    break;

  case 51: /* params: params param  */
#line 429 "handlebars.y"
                 {
      handlebars_ast_list_append((yyvsp[-1].ast_list), (yyvsp[0].ast_node));
      (yyval.ast_list) = (yyvsp[-1].ast_list);
    }
#line 2130 "handlebars.tab.c"
    break;

  case 52: /* param: helper_name  */
#line 436 "handlebars.y"
                {
      (yyval.ast_node) = (yyvsp[0].ast_node);
    }
#line 2138 "handlebars.tab.c"
    break;

  case 53: /* param: sexpr  */
#line 439 "handlebars.y"
          {
      (yyval.ast_node) = (yyvsp[0].ast_node);
    }
#line 2146 "handlebars.tab.c"
    break;

  case 54: /* sexpr: "(" intermediate3 ")"  */
#line 445 "handlebars.y"
                                         {
      (yyval.ast_node) = handlebars_ast_node_ctor_sexpr(parser, (yyvsp[-1].ast_node), &(yyloc));
    }
#line 2154 "handlebars.tab.c"
    break;

  case 55: /* intermediate4: intermediate3 block_params  */
#line 451 "handlebars.y"
                               {
      (yyval.ast_node) = (yyvsp[-1].ast_node);
      (yyval.ast_node)->node.intermediate.block_param1 = (yyvsp[0].block_params).block_param1;
      (yyval.ast_node)->node.intermediate.block_param2 = (yyvsp[0].block_params).block_param2;
    }
#line 2164 "handlebars.tab.c"
    break;

  case 57: /* intermediate3: helper_name params hash  */
#line 460 "handlebars.y"
                            {
      (yyval.ast_node) = handlebars_ast_node_ctor_intermediate(parser, (yyvsp[-2].ast_node), (yyvsp[-1].ast_list), (yyvsp[0].ast_node), 0, &(yyloc));
    }
#line 2172 "handlebars.tab.c"
    break;

  case 58: /* intermediate3: helper_name hash  */
#line 463 "handlebars.y"
                     {
      (yyval.ast_node) = handlebars_ast_node_ctor_intermediate(parser, (yyvsp[-1].ast_node), NULL, (yyvsp[0].ast_node), 0, &(yyloc));
    }
#line 2180 "handlebars.tab.c"
    break;

  case 59: /* intermediate3: helper_name params  */
#line 466 "handlebars.y"
                       {
      (yyval.ast_node) = handlebars_ast_node_ctor_intermediate(parser, (yyvsp[-1].ast_node), (yyvsp[0].ast_list), NULL, 0, &(yyloc));
    }
#line 2188 "handlebars.tab.c"
    break;

  case 60: /* intermediate3: helper_name  */
#line 469 "handlebars.y"
                {
      (yyval.ast_node) = handlebars_ast_node_ctor_intermediate(parser, (yyvsp[0].ast_node), NULL, NULL, 0, &(yyloc));
    }
#line 2196 "handlebars.tab.c"
    break;

  case 61: /* hash: hash_pairs  */
#line 475 "handlebars.y"
               {
      struct handlebars_ast_node * ast_node = handlebars_ast_node_ctor(CONTEXT, HANDLEBARS_AST_NODE_HASH);
      ast_node->node.hash.pairs = (yyvsp[0].ast_list);
      (yyval.ast_node) = ast_node;
    }
#line 2206 "handlebars.tab.c"
    break;

  case 62: /* hash_pairs: hash_pairs hash_pair  */
#line 483 "handlebars.y"
                         {
      handlebars_ast_list_append((yyvsp[-1].ast_list), (yyvsp[0].ast_node));
      (yyval.ast_list) = (yyvsp[-1].ast_list);
    }
#line 2215 "handlebars.tab.c"
    break;

  case 63: /* hash_pairs: hash_pair  */
#line 487 "handlebars.y"
              {
      (yyval.ast_list) = handlebars_ast_list_ctor(CONTEXT);
      handlebars_ast_list_append((yyval.ast_list), (yyvsp[0].ast_node));
    }
#line 2224 "handlebars.tab.c"
    break;

  case 64: /* hash_pair: ID "=" param  */
#line 494 "handlebars.y"
                    {
      (yyval.ast_node) = handlebars_ast_node_ctor_hash_pair(parser, (yyvsp[-2].string), (yyvsp[0].ast_node), &(yyloc));
    }
#line 2232 "handlebars.tab.c"
    break;

  case 65: /* block_params: OPEN_BLOCK_PARAMS ID ID CLOSE_BLOCK_PARAMS  */
#line 500 "handlebars.y"
                                               {
      (yyval.block_params).block_param1 = handlebars_string_copy_ctor(CONTEXT, (yyvsp[-2].string));
      (yyval.block_params).block_param2 = handlebars_string_copy_ctor(CONTEXT, (yyvsp[-1].string));
    }
#line 2241 "handlebars.tab.c"
    break;

  case 66: /* block_params: OPEN_BLOCK_PARAMS ID CLOSE_BLOCK_PARAMS  */
#line 504 "handlebars.y"
                                            {
      (yyval.block_params).block_param1 = handlebars_string_copy_ctor(CONTEXT, (yyvsp[-1].string));
      (yyval.block_params).block_param2 = NULL;
    }
#line 2250 "handlebars.tab.c"
    break;

  case 67: /* helper_name: path  */
#line 511 "handlebars.y"
         {
      (yyval.ast_node) = (yyvsp[0].ast_node);
    }
#line 2258 "handlebars.tab.c"
    break;

  case 68: /* helper_name: data_name  */
#line 514 "handlebars.y"
              {
      (yyval.ast_node) = (yyvsp[0].ast_node);
    }
#line 2266 "handlebars.tab.c"
    break;

  case 69: /* helper_name: STRING  */
#line 517 "handlebars.y"
           {
      (yyval.ast_node) = handlebars_ast_node_ctor_string(parser, (yyvsp[0].string), false, &(yyloc));
    }
#line 2274 "handlebars.tab.c"
    break;

  case 70: /* helper_name: SINGLE_STRING  */
#line 520 "handlebars.y"
                  {
      (yyval.ast_node) = handlebars_ast_node_ctor_string(parser, (yyvsp[0].string), true, &(yyloc));
  }
#line 2282 "handlebars.tab.c"
    break;

  case 71: /* helper_name: NUMBER  */
#line 523 "handlebars.y"
           {
      (yyval.ast_node) = handlebars_ast_node_ctor_number(parser, (yyvsp[0].string), &(yyloc));
    }
#line 2290 "handlebars.tab.c"
    break;

  case 72: /* helper_name: BOOLEAN  */
#line 526 "handlebars.y"
            {
      (yyval.ast_node) = handlebars_ast_node_ctor_boolean(parser, (yyvsp[0].string), &(yyloc));
    }
#line 2298 "handlebars.tab.c"
    break;

  case 73: /* helper_name: "undefined"  */
#line 529 "handlebars.y"
              {
      (yyval.ast_node) = handlebars_ast_node_ctor_undefined(parser, (yyvsp[0].string), &(yyloc));
    }
#line 2306 "handlebars.tab.c"
    break;

  case 74: /* helper_name: "NULL"  */
#line 532 "handlebars.y"
        {
      (yyval.ast_node) = handlebars_ast_node_ctor_null(parser, (yyvsp[0].string), &(yyloc));
    }
#line 2314 "handlebars.tab.c"
    break;

  case 75: /* partial_name: helper_name  */
#line 538 "handlebars.y"
                {
      (yyval.ast_node) = (yyvsp[0].ast_node);
    }
#line 2322 "handlebars.tab.c"
    break;

  case 76: /* partial_name: sexpr  */
#line 541 "handlebars.y"
          {
      (yyval.ast_node) = (yyvsp[0].ast_node);
    }
#line 2330 "handlebars.tab.c"
    break;

  case 77: /* data_name: DATA path_segments  */
#line 547 "handlebars.y"
                       {
      (yyval.ast_node) = handlebars_ast_helper_prepare_path(parser, (yyvsp[0].ast_list), 1, &(yyloc));
    }
#line 2338 "handlebars.tab.c"
    break;

  case 78: /* path: path_segments  */
#line 553 "handlebars.y"
                  {
      (yyval.ast_node) = handlebars_ast_helper_prepare_path(parser, (yyvsp[0].ast_list), 0, &(yyloc));
    }
#line 2346 "handlebars.tab.c"
    break;

  case 79: /* path_segments: path_segments SEP ID  */
#line 559 "handlebars.y"
                         {
      struct handlebars_ast_node * ast_node = handlebars_ast_node_ctor_path_segment(parser, (yyvsp[0].string), (yyvsp[-1].string), &(yyloc));

      handlebars_ast_list_append((yyvsp[-2].ast_list), ast_node);
      (yyval.ast_list) = (yyvsp[-2].ast_list);
    }
#line 2357 "handlebars.tab.c"
    break;

  case 80: /* path_segments: ID  */
#line 565 "handlebars.y"
       {
      struct handlebars_ast_node * ast_node;
      MEMCHK((yyvsp[0].string)); // this is weird

      ast_node = handlebars_ast_node_ctor_path_segment(parser, (yyvsp[0].string), NULL, &(yyloc));
      MEMCHK(ast_node); // this is weird

      (yyval.ast_list) = handlebars_ast_list_ctor(CONTEXT);
      handlebars_ast_list_append((yyval.ast_list), ast_node);
    }
#line 2372 "handlebars.tab.c"
    break;


#line 2376 "handlebars.tab.c"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;
  *++yylsp = yyloc;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      {
        yypcontext_t yyctx
          = {yyssp, yytoken, &yylloc};
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = yysyntax_error (&yymsg_alloc, &yymsg, &yyctx);
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == -1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = YY_CAST (char *,
                             YYSTACK_ALLOC (YY_CAST (YYSIZE_T, yymsg_alloc)));
            if (yymsg)
              {
                yysyntax_error_status
                  = yysyntax_error (&yymsg_alloc, &yymsg, &yyctx);
                yymsgp = yymsg;
              }
            else
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = YYENOMEM;
              }
          }
        yyerror (&yylloc, parser, yymsgp);
        if (yysyntax_error_status == YYENOMEM)
          YYNOMEM;
      }
    }

  yyerror_range[1] = yylloc;
  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= END)
        {
          /* Return failure if at end of input.  */
          if (yychar == END)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval, &yylloc, parser);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;
  ++yynerrs;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;

      yyerror_range[1] = *yylsp;
      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp, yylsp, parser);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  yyerror_range[2] = yylloc;
  ++yylsp;
  YYLLOC_DEFAULT (*yylsp, yyerror_range, 2);

  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (&yylloc, parser, YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval, &yylloc, parser);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp, yylsp, parser);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
  return yyresult;
}

