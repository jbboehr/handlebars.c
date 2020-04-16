/* A Bison parser, made by GNU Bison 3.4.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2019 Free Software Foundation,
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
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

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

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Undocumented macros, especially those whose name start with YY_,
   are private implementation details.  Do not rely on them.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "3.4.2"

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
#line 53 "handlebars.y"


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "handlebars.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"

#include "handlebars_ast.h"
#include "handlebars_ast_helpers.h"
#include "handlebars_ast_list.h"
#include "handlebars_string.h"
#include "handlebars_utils.h"
#include "handlebars_whitespace.h"
#include "handlebars.tab.h"
#include "handlebars.lex.h"

#ifdef YYDEBUG
#define YYPRINT handlebars_yy_print
int handlebars_yy_debug = 1;
#else
int handlebars_yy_debug = 0;
#endif

#undef CONTEXT
#define CONTEXT HBSCTX(parser)
#define scanner parser->scanner

#line 113 "handlebars.tab.c"

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

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 1
#endif

/* Use api.header.include to #include this header
   instead of duplicating it here.  */
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
#line 33 "handlebars.y"

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

#line 165 "handlebars.tab.c"

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    END = 0,
    BOOLEAN = 258,
    CLOSE = 259,
    CLOSE_RAW_BLOCK = 260,
    CLOSE_SEXPR = 261,
    CLOSE_UNESCAPED = 262,
    COMMENT = 263,
    CONTENT = 264,
    DATA = 265,
    END_RAW_BLOCK = 266,
    EQUALS = 267,
    ID = 268,
    INVALID = 269,
    INVERSE = 270,
    NUMBER = 271,
    OPEN = 272,
    OPEN_BLOCK = 273,
    OPEN_ENDBLOCK = 274,
    OPEN_INVERSE = 275,
    OPEN_PARTIAL = 276,
    OPEN_RAW_BLOCK = 277,
    OPEN_SEXPR = 278,
    OPEN_UNESCAPED = 279,
    SEP = 280,
    STRING = 281,
    CLOSE_BLOCK_PARAMS = 282,
    NUL = 283,
    OPEN_BLOCK_PARAMS = 284,
    OPEN_INVERSE_CHAIN = 285,
    UNDEFINED = 286,
    OPEN_PARTIAL_BLOCK = 287
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 90 "handlebars.y"

    struct handlebars_string * string;
    struct handlebars_ast_node * ast_node;
    struct handlebars_ast_list * ast_list;
  
    struct handlebars_yy_block_intermediate block_intermediate;
    struct handlebars_yy_block_params block_params;

#line 219 "handlebars.tab.c"

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



#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

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

#ifndef YY_ATTRIBUTE
# if (defined __GNUC__                                               \
      && (2 < __GNUC__ || (__GNUC__ == 2 && 96 <= __GNUC_MINOR__)))  \
     || defined __SUNPRO_C && 0x5110 <= __SUNPRO_C
#  define YY_ATTRIBUTE(Spec) __attribute__(Spec)
# else
#  define YY_ATTRIBUTE(Spec) /* empty */
# endif
#endif

#ifndef YY_ATTRIBUTE_PURE
# define YY_ATTRIBUTE_PURE   YY_ATTRIBUTE ((__pure__))
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# define YY_ATTRIBUTE_UNUSED YY_ATTRIBUTE ((__unused__))
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && ! defined __ICC && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN \
    _Pragma ("GCC diagnostic push") \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")\
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END \
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


#define YY_ASSERT(E) ((void) (0 && (E)))

#if ! defined yyoverflow || YYERROR_VERBOSE

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
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL \
             && defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
  YYLTYPE yyls_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE) + sizeof (YYLTYPE)) \
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
        YYSIZE_T yynewbytes;                                            \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / sizeof (*yyptr);                          \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, (Count) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYSIZE_T yyi;                         \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  48
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   244

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  34
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  34
/* YYNRULES -- Number of rules.  */
#define YYNRULES  77
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  122

#define YYUNDEFTOK  2
#define YYMAXUTOK   288

/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                                \
  ((unsigned) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_uint8 yytranslate[] =
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
      25,    26,    27,    28,    29,    30,    31,    32,    33
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   179,   179,   187,   190,   197,   201,   208,   211,   214,
     217,   220,   223,   226,   236,   240,   246,   252,   258,   261,
     264,   267,   273,   277,   281,   288,   296,   303,   310,   313,
     316,   319,   322,   328,   332,   341,   348,   352,   359,   363,
     367,   371,   378,   381,   387,   391,   395,   399,   406,   410,
     417,   420,   426,   432,   437,   441,   444,   447,   450,   456,
     464,   468,   475,   481,   485,   492,   495,   498,   501,   504,
     507,   510,   516,   519,   525,   531,   537,   543
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 1
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "$undefined", "BOOLEAN", "\"}}\"",
  "\"}}}}\"", "\")\"", "\"}}}\"", "COMMENT", "CONTENT", "DATA",
  "END_RAW_BLOCK", "\"=\"", "ID", "INVALID", "INVERSE", "NUMBER", "\"{{\"",
  "\"{{#\"", "OPEN_ENDBLOCK", "\"{{^\"", "\"{{>\"", "\"{{{{\"", "\"(\"",
  "\"{{{\"", "SEP", "STRING", "CLOSE_BLOCK_PARAMS", "\"NULL\"",
  "OPEN_BLOCK_PARAMS", "OPEN_INVERSE_CHAIN", "\"undefined\"", "\"{{#>\"",
  "\"\"", "$accept", "start", "program", "statements", "statement",
  "content", "raw_block", "open_raw_block", "block", "block_intermediate",
  "open_block", "open_inverse", "open_inverse_chain", "inverse_chain",
  "inverse_and_program", "close_block", "mustache", "partial",
  "partial_block", "open_partial_block", "params", "param", "sexpr",
  "intermediate4", "intermediate3", "hash", "hash_pairs", "hash_pair",
  "block_params", "helper_name", "partial_name", "data_name", "path",
  "path_segments", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288
};
# endif

#define YYPACT_NINF -64

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-64)))

#define YYTABLE_NINF -1

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     211,   -64,    12,   137,   137,   137,   107,   137,   137,   107,
     -64,    10,    22,    48,   -64,   -64,   -64,    12,   -64,   158,
     158,   -64,   -64,   -64,   194,   -64,   -64,    16,   -64,   -64,
     -64,   -64,   -64,    23,   133,   -64,   -64,     8,    31,    11,
      33,   137,   -64,   -64,    51,    34,    35,    81,   -64,   -64,
     -64,    49,   211,   137,   137,     2,    26,   177,   -64,   -64,
     -64,    26,   -64,    26,   -64,     8,   -64,    47,   133,   -64,
     -64,   -64,    50,   -64,   -64,    58,   -64,    60,   -64,   -64,
      69,   -64,    85,    74,   -64,   -64,   -64,   111,    79,   -64,
     -64,    82,    83,   -64,   -64,     2,   -64,   -64,   -64,   107,
     -64,   -64,    47,   -64,   -64,     7,   -64,   -64,    86,   -64,
     -64,    88,   -64,   -64,   -64,   -64,   -64,    72,   -64,   -64,
     -64,   -64
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,    13,    15,     0,     0,     0,     0,     0,     0,     0,
       4,     0,     0,     3,     5,    12,     9,     0,     8,     0,
       0,     7,    10,    11,     0,    14,    69,     0,    77,    68,
      67,    71,    70,     0,    58,    66,    65,    75,     0,    54,
       0,     0,    73,    72,     0,     0,     0,     0,     1,     2,
       6,     0,    34,     0,     0,    24,     0,    31,    22,    32,
      19,     0,    21,     0,    43,    74,    36,    77,    57,    48,
      51,    56,    59,    61,    50,     0,    25,     0,    53,    26,
       0,    41,     0,     0,    17,    37,    47,     0,     0,    16,
      33,     0,     0,    23,    18,    30,    29,    20,    42,     0,
      49,    55,     0,    60,    76,     0,    52,    39,     0,    40,
      45,     0,    46,    35,    27,    28,    62,     0,    64,    38,
      44,    63
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -64,   -64,     1,   -64,    87,    14,   -64,   -64,   -64,    76,
     -64,   -64,   -64,   -43,   -64,   -13,   -64,   -64,   -64,   -64,
     -29,   -63,    17,    -3,     5,   -38,   -64,    30,   -64,    -6,
      94,   -64,   -64,    78
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
      -1,    11,    55,    13,    14,    15,    16,    17,    18,    56,
      19,    20,    57,    58,    59,    60,    21,    22,    23,    24,
      68,    69,    70,    38,    39,    71,    72,    73,    78,    34,
      44,    35,    36,    37
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint8 yytable[] =
{
      43,    12,    40,    43,    45,   100,    83,    62,    33,    88,
      48,    64,    93,    46,    96,    82,    25,    52,    87,   100,
     117,     2,    49,    42,   100,    63,    42,    66,    74,    28,
     101,    51,    54,    75,   118,    76,   116,    79,    74,    84,
      77,    74,    85,    94,   108,    53,    80,    91,    97,   111,
      98,    92,   115,    90,    26,    81,     1,     2,    95,    99,
      89,    27,    74,   102,    67,     3,     4,    29,     5,     6,
       7,   104,     8,   105,    41,   106,    74,    30,   109,    31,
       9,    74,    32,   112,    26,    86,   113,   114,    26,   107,
     119,    27,   120,    74,    67,    27,    61,    29,    67,   121,
      50,    29,   103,    47,    41,    65,     0,    30,    41,    31,
      26,    30,    32,    31,    26,   110,    32,    27,     0,     0,
      28,    27,     0,    29,    67,     0,     0,    29,     0,     0,
      41,     0,     0,    30,    41,    31,    26,    30,    32,    31,
      26,     0,    32,    27,     0,     0,    67,    27,     0,    29,
      28,     0,     0,    29,     0,     0,    41,     0,     0,    30,
       0,    31,     0,    30,    32,    31,     1,     2,    32,     0,
       0,     0,     0,    52,     0,     3,     4,    53,     5,     6,
       7,     0,     8,     0,     0,     1,     2,     0,    54,     0,
       9,    10,    52,     0,     3,     4,     0,     5,     6,     7,
       0,     8,     1,     2,     0,     0,     0,    54,     0,     9,
      10,     3,     4,    53,     5,     6,     7,     0,     8,     1,
       2,     0,     0,     0,     0,     0,     9,    10,     3,     4,
       0,     5,     6,     7,     0,     8,     0,     0,     0,     0,
       0,     0,     0,     9,    10
};

static const yytype_int8 yycheck[] =
{
       6,     0,     5,     9,     7,    68,    44,    20,     3,    47,
       0,    24,    55,     8,    57,    44,     2,    15,    47,    82,
      13,     9,     0,     6,    87,    24,     9,     4,    34,    13,
      68,    17,    30,    25,    27,     4,    99,     4,    44,     5,
      29,    47,     7,    56,    82,    19,    41,    53,    61,    87,
      63,    54,    95,    52,     3,     4,     8,     9,    57,    12,
      11,    10,    68,    13,    13,    17,    18,    16,    20,    21,
      22,    13,    24,    13,    23,     6,    82,    26,     4,    28,
      32,    87,    31,     4,     3,     4,     4,     4,     3,     4,
       4,    10,     4,    99,    13,    10,    20,    16,    13,    27,
      13,    16,    72,     9,    23,    27,    -1,    26,    23,    28,
       3,    26,    31,    28,     3,     4,    31,    10,    -1,    -1,
      13,    10,    -1,    16,    13,    -1,    -1,    16,    -1,    -1,
      23,    -1,    -1,    26,    23,    28,     3,    26,    31,    28,
       3,    -1,    31,    10,    -1,    -1,    13,    10,    -1,    16,
      13,    -1,    -1,    16,    -1,    -1,    23,    -1,    -1,    26,
      -1,    28,    -1,    26,    31,    28,     8,     9,    31,    -1,
      -1,    -1,    -1,    15,    -1,    17,    18,    19,    20,    21,
      22,    -1,    24,    -1,    -1,     8,     9,    -1,    30,    -1,
      32,    33,    15,    -1,    17,    18,    -1,    20,    21,    22,
      -1,    24,     8,     9,    -1,    -1,    -1,    30,    -1,    32,
      33,    17,    18,    19,    20,    21,    22,    -1,    24,     8,
       9,    -1,    -1,    -1,    -1,    -1,    32,    33,    17,    18,
      -1,    20,    21,    22,    -1,    24,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    32,    33
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     8,     9,    17,    18,    20,    21,    22,    24,    32,
      33,    35,    36,    37,    38,    39,    40,    41,    42,    44,
      45,    50,    51,    52,    53,    39,     3,    10,    13,    16,
      26,    28,    31,    58,    63,    65,    66,    67,    57,    58,
      57,    23,    56,    63,    64,    57,    58,    64,     0,     0,
      38,    39,    15,    19,    30,    36,    43,    46,    47,    48,
      49,    43,    49,    36,    49,    67,     4,    13,    54,    55,
      56,    59,    60,    61,    63,    25,     4,    29,    62,     4,
      58,     4,    54,    59,     5,     7,     4,    54,    59,    11,
      36,    63,    57,    47,    49,    36,    47,    49,    49,    12,
      55,    59,    13,    61,    13,    13,     6,     4,    59,     4,
       4,    59,     4,     4,     4,    47,    55,    13,    27,     4,
       4,    27
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    34,    35,    36,    36,    37,    37,    38,    38,    38,
      38,    38,    38,    38,    39,    39,    40,    41,    42,    42,
      42,    42,    43,    43,    43,    44,    45,    46,    47,    47,
      47,    47,    47,    48,    48,    49,    50,    50,    51,    51,
      51,    51,    52,    52,    53,    53,    53,    53,    54,    54,
      55,    55,    56,    57,    57,    58,    58,    58,    58,    59,
      60,    60,    61,    62,    62,    63,    63,    63,    63,    63,
      63,    63,    64,    64,    65,    66,    67,    67
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     2,     1,     1,     1,     2,     1,     1,     1,
       1,     1,     1,     1,     2,     1,     3,     3,     3,     2,
       3,     2,     1,     2,     1,     3,     3,     3,     3,     2,
       2,     1,     1,     2,     1,     3,     3,     3,     5,     4,
       4,     3,     3,     2,     5,     4,     4,     3,     1,     2,
       1,     1,     3,     2,     1,     3,     2,     2,     1,     1,
       2,     1,     3,     4,     3,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     2,     1,     3,     1
};


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


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

/* Error token number */
#define YYTERROR        1
#define YYERRCODE       256


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


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL

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

#  define YY_LOCATION_PRINT(File, Loc)          \
  yy_location_print_ (File, &(Loc))

# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Type, Value, Location, parser); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, struct handlebars_parser * parser)
{
  FILE *yyoutput = yyo;
  YYUSE (yyoutput);
  YYUSE (yylocationp);
  YYUSE (parser);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyo, yytoknum[yytype], *yyvaluep);
# endif
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, struct handlebars_parser * parser)
{
  YYFPRINTF (yyo, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  YY_LOCATION_PRINT (yyo, *yylocationp);
  YYFPRINTF (yyo, ": ");
  yy_symbol_value_print (yyo, yytype, yyvaluep, yylocationp, parser);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
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
yy_reduce_print (yytype_int16 *yyssp, YYSTYPE *yyvsp, YYLTYPE *yylsp, int yyrule, struct handlebars_parser * parser)
{
  unsigned long yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       yystos[yyssp[yyi + 1 - yynrhs]],
                       &yyvsp[(yyi + 1) - (yynrhs)]
                       , &(yylsp[(yyi + 1) - (yynrhs)])                       , parser);
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
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
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


#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
yystrlen (const char *yystr)
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
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
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
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

  if (! yyres)
    return yystrlen (yystr);

  return (YYSIZE_T) (yystpcpy (yyres, yystr) - yyres);
}
# endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (YY_NULLPTR, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
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
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
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
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULLPTR, yytname[yyx]);
                  if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
                    yysize = yysize1;
                  else
                    return 2;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
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
# undef YYCASE_
    }

  {
    YYSIZE_T yysize1 = yysize + yystrlen (yyformat);
    if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
      yysize = yysize1;
    else
      return 2;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
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
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, YYLTYPE *yylocationp, struct handlebars_parser * parser)
{
  YYUSE (yyvaluep);
  YYUSE (yylocationp);
  YYUSE (parser);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/*----------.
| yyparse.  |
`----------*/

int
yyparse (struct handlebars_parser * parser)
{
/* The lookahead symbol.  */
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
    int yynerrs;

    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.
       'yyls': related to locations.

       Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    /* The location stack.  */
    YYLTYPE yylsa[YYINITDEPTH];
    YYLTYPE *yyls;
    YYLTYPE *yylsp;

    /* The locations where the error started and ended.  */
    YYLTYPE yyerror_range[3];

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
  YYLTYPE yyloc;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N), yylsp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yylsp = yyls = yylsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
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
| yynewstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  *yyssp = (yytype_int16) yystate;

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    goto yyexhaustedlab;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = (YYSIZE_T) (yyssp - yyss + 1);

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        YYSTYPE *yyvs1 = yyvs;
        yytype_int16 *yyss1 = yyss;
        YYLTYPE *yyls1 = yyls;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * sizeof (*yyssp),
                    &yyvs1, yysize * sizeof (*yyvsp),
                    &yyls1, yysize * sizeof (*yylsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
        yyls = yyls1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yytype_int16 *yyss1 = yyss;
        union yyalloc *yyptr =
          (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
        if (! yyptr)
          goto yyexhaustedlab;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
        YYSTACK_RELOCATE (yyls_alloc, yyls);
# undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
      yylsp = yyls + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
                  (unsigned long) yystacksize));

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

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = yylex (&yylval, &yylloc, scanner);
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
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

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END
  *++yylsp = yylloc;
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
  case 2:
#line 179 "handlebars.y"
    {
      parser->program = (yyvsp[-1].ast_node);
      handlebars_whitespace_accept(parser, parser->program);
      return 1;
    }
#line 1553 "handlebars.tab.c"
    break;

  case 3:
#line 187 "handlebars.y"
    {
      (yyval.ast_node) = handlebars_ast_node_ctor_program(parser, (yyvsp[0].ast_list), NULL, NULL, 0, 0, &(yyloc));
    }
#line 1561 "handlebars.tab.c"
    break;

  case 4:
#line 190 "handlebars.y"
    {
      struct handlebars_ast_list * list = handlebars_ast_list_ctor(CONTEXT);
      (yyval.ast_node) = handlebars_ast_node_ctor_program(parser, list, NULL, NULL, 0, 0, &(yyloc));
    }
#line 1570 "handlebars.tab.c"
    break;

  case 5:
#line 197 "handlebars.y"
    {
      (yyval.ast_list) = handlebars_ast_list_ctor(CONTEXT);
      handlebars_ast_list_append((yyval.ast_list), (yyvsp[0].ast_node));
    }
#line 1579 "handlebars.tab.c"
    break;

  case 6:
#line 201 "handlebars.y"
    {
      handlebars_ast_list_append((yyvsp[-1].ast_list), (yyvsp[0].ast_node));
      (yyval.ast_list) = (yyvsp[-1].ast_list);
    }
#line 1588 "handlebars.tab.c"
    break;

  case 7:
#line 208 "handlebars.y"
    {
      (yyval.ast_node) = (yyvsp[0].ast_node);
    }
#line 1596 "handlebars.tab.c"
    break;

  case 8:
#line 211 "handlebars.y"
    {
      (yyval.ast_node) = (yyvsp[0].ast_node);
    }
#line 1604 "handlebars.tab.c"
    break;

  case 9:
#line 214 "handlebars.y"
    {
      (yyval.ast_node) = (yyvsp[0].ast_node);
    }
#line 1612 "handlebars.tab.c"
    break;

  case 10:
#line 217 "handlebars.y"
    {
      (yyval.ast_node) = (yyvsp[0].ast_node);
    }
#line 1620 "handlebars.tab.c"
    break;

  case 11:
#line 220 "handlebars.y"
    {
      (yyval.ast_node) = (yyvsp[0].ast_node);
    }
#line 1628 "handlebars.tab.c"
    break;

  case 12:
#line 223 "handlebars.y"
    {
      (yyval.ast_node) = handlebars_ast_node_ctor_content(parser, (yyvsp[0].string), &(yyloc));
    }
#line 1636 "handlebars.tab.c"
    break;

  case 13:
#line 226 "handlebars.y"
    {
      // Strip comment strips in place
      unsigned strip = handlebars_ast_helper_strip_flags((yyvsp[0].string), (yyvsp[0].string));
      (yyval.ast_node) = handlebars_ast_node_ctor_comment(parser, 
      			handlebars_ast_helper_strip_comment((yyvsp[0].string)), &(yyloc));
      (yyval.ast_node)->strip = strip;
    }
#line 1648 "handlebars.tab.c"
    break;

  case 14:
#line 236 "handlebars.y"
    {
      (yyval.string) = handlebars_string_append(CONTEXT, (yyvsp[-1].string), (yyvsp[0].string)->val, (yyvsp[0].string)->len);
      (yyval.string) = talloc_steal(parser, (yyval.string));
    }
#line 1657 "handlebars.tab.c"
    break;

  case 15:
#line 240 "handlebars.y"
    {
      (yyval.string) = (yyvsp[0].string);
    }
#line 1665 "handlebars.tab.c"
    break;

  case 16:
#line 246 "handlebars.y"
    {
      (yyval.ast_node) = handlebars_ast_helper_prepare_raw_block(parser, (yyvsp[-2].ast_node), (yyvsp[-1].string), (yyvsp[0].string), &(yyloc));
    }
#line 1673 "handlebars.tab.c"
    break;

  case 17:
#line 252 "handlebars.y"
    {
      (yyval.ast_node) = (yyvsp[-1].ast_node);
    }
#line 1681 "handlebars.tab.c"
    break;

  case 18:
#line 258 "handlebars.y"
    {
      (yyval.ast_node) = handlebars_ast_helper_prepare_block(parser, (yyvsp[-2].ast_node), (yyvsp[-1].block_intermediate).program, (yyvsp[-1].block_intermediate).inverse_chain, (yyvsp[0].ast_node), 0, &(yyloc));
    }
#line 1689 "handlebars.tab.c"
    break;

  case 19:
#line 261 "handlebars.y"
    {
      (yyval.ast_node) = handlebars_ast_helper_prepare_block(parser, (yyvsp[-1].ast_node), NULL, NULL, (yyvsp[0].ast_node), 0, &(yyloc));
    }
#line 1697 "handlebars.tab.c"
    break;

  case 20:
#line 264 "handlebars.y"
    {
      (yyval.ast_node) = handlebars_ast_helper_prepare_block(parser, (yyvsp[-2].ast_node), (yyvsp[-1].block_intermediate).program, (yyvsp[-1].block_intermediate).inverse_chain, (yyvsp[0].ast_node), 1, &(yyloc));
    }
#line 1705 "handlebars.tab.c"
    break;

  case 21:
#line 267 "handlebars.y"
    {
      (yyval.ast_node) = handlebars_ast_helper_prepare_block(parser, (yyvsp[-1].ast_node), NULL, NULL, (yyvsp[0].ast_node), 1, &(yyloc));
    }
#line 1713 "handlebars.tab.c"
    break;

  case 22:
#line 273 "handlebars.y"
    {
      (yyval.block_intermediate).program = NULL;
      (yyval.block_intermediate).inverse_chain = (yyvsp[0].ast_node);
    }
#line 1722 "handlebars.tab.c"
    break;

  case 23:
#line 277 "handlebars.y"
    {
      (yyval.block_intermediate).program = (yyvsp[-1].ast_node);
      (yyval.block_intermediate).inverse_chain = (yyvsp[0].ast_node);
    }
#line 1731 "handlebars.tab.c"
    break;

  case 24:
#line 281 "handlebars.y"
    {
      (yyval.block_intermediate).program = (yyvsp[0].ast_node);
      (yyval.block_intermediate).inverse_chain = NULL;
    }
#line 1740 "handlebars.tab.c"
    break;

  case 25:
#line 288 "handlebars.y"
    {
      (yyval.ast_node) = (yyvsp[-1].ast_node);
      (yyval.ast_node)->strip = handlebars_ast_helper_strip_flags((yyvsp[-2].string), (yyvsp[0].string));
      (yyval.ast_node)->node.intermediate.open = talloc_steal((yyval.ast_node), handlebars_string_copy_ctor(CONTEXT, (yyvsp[-2].string)));
    }
#line 1750 "handlebars.tab.c"
    break;

  case 26:
#line 296 "handlebars.y"
    {
      (yyval.ast_node) = (yyvsp[-1].ast_node);
      (yyval.ast_node)->strip = handlebars_ast_helper_strip_flags((yyvsp[-2].string), (yyvsp[0].string));
    }
#line 1759 "handlebars.tab.c"
    break;

  case 27:
#line 303 "handlebars.y"
    {
      (yyval.ast_node) = (yyvsp[-1].ast_node);
      (yyval.ast_node)->strip = handlebars_ast_helper_strip_flags((yyvsp[-2].string), (yyvsp[0].string));
    }
#line 1768 "handlebars.tab.c"
    break;

  case 28:
#line 310 "handlebars.y"
    {
      (yyval.ast_node) = handlebars_ast_helper_prepare_inverse_chain(parser, (yyvsp[-2].ast_node), (yyvsp[-1].ast_node), (yyvsp[0].ast_node), &(yyloc));
  	}
#line 1776 "handlebars.tab.c"
    break;

  case 29:
#line 313 "handlebars.y"
    {
      (yyval.ast_node) = handlebars_ast_helper_prepare_inverse_chain(parser, (yyvsp[-1].ast_node), NULL, (yyvsp[0].ast_node), &(yyloc));
  	}
#line 1784 "handlebars.tab.c"
    break;

  case 30:
#line 316 "handlebars.y"
    {
      (yyval.ast_node) = handlebars_ast_helper_prepare_inverse_chain(parser, (yyvsp[-1].ast_node), (yyvsp[0].ast_node), NULL, &(yyloc));
    }
#line 1792 "handlebars.tab.c"
    break;

  case 31:
#line 319 "handlebars.y"
    {
      (yyval.ast_node) = handlebars_ast_helper_prepare_inverse_chain(parser, (yyvsp[0].ast_node), NULL, NULL, &(yyloc));
    }
#line 1800 "handlebars.tab.c"
    break;

  case 32:
#line 322 "handlebars.y"
    {
      (yyval.ast_node) = (yyvsp[0].ast_node);
    }
#line 1808 "handlebars.tab.c"
    break;

  case 33:
#line 328 "handlebars.y"
    {
      (yyval.ast_node) = handlebars_ast_node_ctor_inverse(parser, (yyvsp[0].ast_node), 0, 
              handlebars_ast_helper_strip_flags((yyvsp[-1].string), (yyvsp[-1].string)), &(yyloc));
    }
#line 1817 "handlebars.tab.c"
    break;

  case 34:
#line 332 "handlebars.y"
    {
      struct handlebars_ast_node * program_node;
      program_node = handlebars_ast_node_ctor(CONTEXT, HANDLEBARS_AST_NODE_PROGRAM);
      (yyval.ast_node) = handlebars_ast_node_ctor_inverse(parser, program_node, 0, 
              handlebars_ast_helper_strip_flags((yyvsp[0].string), (yyvsp[0].string)), &(yyloc));
    }
#line 1828 "handlebars.tab.c"
    break;

  case 35:
#line 341 "handlebars.y"
    {
      (yyval.ast_node) = handlebars_ast_node_ctor_intermediate(parser, (yyvsp[-1].ast_node), NULL, NULL, 
              handlebars_ast_helper_strip_flags((yyvsp[-2].string), (yyvsp[0].string)), &(yyloc));
    }
#line 1837 "handlebars.tab.c"
    break;

  case 36:
#line 348 "handlebars.y"
    {
      (yyval.ast_node) = handlebars_ast_helper_prepare_mustache(parser, (yyvsp[-1].ast_node), (yyvsp[-2].string),
        			handlebars_ast_helper_strip_flags((yyvsp[-2].string), (yyvsp[0].string)), &(yyloc));
    }
#line 1846 "handlebars.tab.c"
    break;

  case 37:
#line 352 "handlebars.y"
    {
      (yyval.ast_node) = handlebars_ast_helper_prepare_mustache(parser, (yyvsp[-1].ast_node), (yyvsp[-2].string),
        			handlebars_ast_helper_strip_flags((yyvsp[-2].string), (yyvsp[0].string)), &(yyloc));
    }
#line 1855 "handlebars.tab.c"
    break;

  case 38:
#line 359 "handlebars.y"
    {
      (yyval.ast_node) = handlebars_ast_node_ctor_partial(parser, (yyvsp[-3].ast_node), (yyvsp[-2].ast_list), (yyvsp[-1].ast_node),
              handlebars_ast_helper_strip_flags((yyvsp[-4].string), (yyvsp[0].string)), &(yyloc));
    }
#line 1864 "handlebars.tab.c"
    break;

  case 39:
#line 363 "handlebars.y"
    {
      (yyval.ast_node) = handlebars_ast_node_ctor_partial(parser, (yyvsp[-2].ast_node), (yyvsp[-1].ast_list), NULL,
              handlebars_ast_helper_strip_flags((yyvsp[-3].string), (yyvsp[0].string)), &(yyloc));
    }
#line 1873 "handlebars.tab.c"
    break;

  case 40:
#line 367 "handlebars.y"
    {
      (yyval.ast_node) = handlebars_ast_node_ctor_partial(parser, (yyvsp[-2].ast_node), NULL, (yyvsp[-1].ast_node),
              handlebars_ast_helper_strip_flags((yyvsp[-3].string), (yyvsp[0].string)), &(yyloc));
    }
#line 1882 "handlebars.tab.c"
    break;

  case 41:
#line 371 "handlebars.y"
    {
      (yyval.ast_node) = handlebars_ast_node_ctor_partial(parser, (yyvsp[-1].ast_node), NULL, NULL,
              handlebars_ast_helper_strip_flags((yyvsp[-2].string), (yyvsp[0].string)), &(yyloc));
    }
#line 1891 "handlebars.tab.c"
    break;

  case 42:
#line 378 "handlebars.y"
    {
      (yyval.ast_node) = handlebars_ast_helper_prepare_partial_block(parser, (yyvsp[-2].ast_node), (yyvsp[-1].ast_node), (yyvsp[0].ast_node), &(yyloc));
  }
#line 1899 "handlebars.tab.c"
    break;

  case 43:
#line 381 "handlebars.y"
    {
      struct handlebars_ast_node * program = handlebars_ast_node_ctor(CONTEXT, HANDLEBARS_AST_NODE_PROGRAM);
      (yyval.ast_node) = handlebars_ast_helper_prepare_partial_block(parser, (yyvsp[-1].ast_node), program, (yyvsp[0].ast_node), &(yyloc));
  }
#line 1908 "handlebars.tab.c"
    break;

  case 44:
#line 387 "handlebars.y"
    {
      (yyval.ast_node) = handlebars_ast_node_ctor_intermediate(parser, (yyvsp[-3].ast_node), (yyvsp[-2].ast_list), (yyvsp[-1].ast_node),
      			handlebars_ast_helper_strip_flags((yyvsp[-4].string), (yyvsp[0].string)), &(yyloc));
    }
#line 1917 "handlebars.tab.c"
    break;

  case 45:
#line 391 "handlebars.y"
    {
      (yyval.ast_node) = handlebars_ast_node_ctor_intermediate(parser, (yyvsp[-2].ast_node), (yyvsp[-1].ast_list), NULL,
      			handlebars_ast_helper_strip_flags((yyvsp[-3].string), (yyvsp[0].string)), &(yyloc));
    }
#line 1926 "handlebars.tab.c"
    break;

  case 46:
#line 395 "handlebars.y"
    {
      (yyval.ast_node) = handlebars_ast_node_ctor_intermediate(parser, (yyvsp[-2].ast_node), NULL, (yyvsp[-1].ast_node),
              handlebars_ast_helper_strip_flags((yyvsp[-3].string), (yyvsp[0].string)), &(yyloc));
    }
#line 1935 "handlebars.tab.c"
    break;

  case 47:
#line 399 "handlebars.y"
    {
      (yyval.ast_node) = handlebars_ast_node_ctor_intermediate(parser, (yyvsp[-1].ast_node), NULL, NULL,
              handlebars_ast_helper_strip_flags((yyvsp[-2].string), (yyvsp[0].string)), &(yyloc));
    }
#line 1944 "handlebars.tab.c"
    break;

  case 48:
#line 406 "handlebars.y"
    {
      (yyval.ast_list) = handlebars_ast_list_ctor(CONTEXT);
      handlebars_ast_list_append((yyval.ast_list), (yyvsp[0].ast_node));
    }
#line 1953 "handlebars.tab.c"
    break;

  case 49:
#line 410 "handlebars.y"
    {
      handlebars_ast_list_append((yyvsp[-1].ast_list), (yyvsp[0].ast_node));
      (yyval.ast_list) = (yyvsp[-1].ast_list);
    }
#line 1962 "handlebars.tab.c"
    break;

  case 50:
#line 417 "handlebars.y"
    {
      (yyval.ast_node) = (yyvsp[0].ast_node);
    }
#line 1970 "handlebars.tab.c"
    break;

  case 51:
#line 420 "handlebars.y"
    {
      (yyval.ast_node) = (yyvsp[0].ast_node);
    }
#line 1978 "handlebars.tab.c"
    break;

  case 52:
#line 426 "handlebars.y"
    {
      (yyval.ast_node) = handlebars_ast_node_ctor_sexpr(parser, (yyvsp[-1].ast_node), &(yyloc));
    }
#line 1986 "handlebars.tab.c"
    break;

  case 53:
#line 432 "handlebars.y"
    {
      (yyval.ast_node) = (yyvsp[-1].ast_node);
      (yyval.ast_node)->node.intermediate.block_param1 = (yyvsp[0].block_params).block_param1;
      (yyval.ast_node)->node.intermediate.block_param2 = (yyvsp[0].block_params).block_param2;
    }
#line 1996 "handlebars.tab.c"
    break;

  case 55:
#line 441 "handlebars.y"
    {
      (yyval.ast_node) = handlebars_ast_node_ctor_intermediate(parser, (yyvsp[-2].ast_node), (yyvsp[-1].ast_list), (yyvsp[0].ast_node), 0, &(yyloc));
    }
#line 2004 "handlebars.tab.c"
    break;

  case 56:
#line 444 "handlebars.y"
    {
      (yyval.ast_node) = handlebars_ast_node_ctor_intermediate(parser, (yyvsp[-1].ast_node), NULL, (yyvsp[0].ast_node), 0, &(yyloc));
    }
#line 2012 "handlebars.tab.c"
    break;

  case 57:
#line 447 "handlebars.y"
    {
      (yyval.ast_node) = handlebars_ast_node_ctor_intermediate(parser, (yyvsp[-1].ast_node), (yyvsp[0].ast_list), NULL, 0, &(yyloc));
    }
#line 2020 "handlebars.tab.c"
    break;

  case 58:
#line 450 "handlebars.y"
    {
      (yyval.ast_node) = handlebars_ast_node_ctor_intermediate(parser, (yyvsp[0].ast_node), NULL, NULL, 0, &(yyloc));
    }
#line 2028 "handlebars.tab.c"
    break;

  case 59:
#line 456 "handlebars.y"
    {
      struct handlebars_ast_node * ast_node = handlebars_ast_node_ctor(CONTEXT, HANDLEBARS_AST_NODE_HASH);
      ast_node->node.hash.pairs = (yyvsp[0].ast_list);
      (yyval.ast_node) = ast_node;
    }
#line 2038 "handlebars.tab.c"
    break;

  case 60:
#line 464 "handlebars.y"
    {
      handlebars_ast_list_append((yyvsp[-1].ast_list), (yyvsp[0].ast_node));
      (yyval.ast_list) = (yyvsp[-1].ast_list);
    }
#line 2047 "handlebars.tab.c"
    break;

  case 61:
#line 468 "handlebars.y"
    {
      (yyval.ast_list) = handlebars_ast_list_ctor(CONTEXT);
      handlebars_ast_list_append((yyval.ast_list), (yyvsp[0].ast_node));
    }
#line 2056 "handlebars.tab.c"
    break;

  case 62:
#line 475 "handlebars.y"
    {
      (yyval.ast_node) = handlebars_ast_node_ctor_hash_pair(parser, (yyvsp[-2].string), (yyvsp[0].ast_node), &(yyloc));
    }
#line 2064 "handlebars.tab.c"
    break;

  case 63:
#line 481 "handlebars.y"
    {
      (yyval.block_params).block_param1 = handlebars_string_copy_ctor(CONTEXT, (yyvsp[-2].string));
      (yyval.block_params).block_param2 = handlebars_string_copy_ctor(CONTEXT, (yyvsp[-1].string));
    }
#line 2073 "handlebars.tab.c"
    break;

  case 64:
#line 485 "handlebars.y"
    {
      (yyval.block_params).block_param1 = handlebars_string_copy_ctor(CONTEXT, (yyvsp[-1].string));
      (yyval.block_params).block_param2 = NULL;
    }
#line 2082 "handlebars.tab.c"
    break;

  case 65:
#line 492 "handlebars.y"
    {
      (yyval.ast_node) = (yyvsp[0].ast_node);
    }
#line 2090 "handlebars.tab.c"
    break;

  case 66:
#line 495 "handlebars.y"
    {
      (yyval.ast_node) = (yyvsp[0].ast_node);
    }
#line 2098 "handlebars.tab.c"
    break;

  case 67:
#line 498 "handlebars.y"
    {
      (yyval.ast_node) = handlebars_ast_node_ctor_string(parser, (yyvsp[0].string), &(yyloc));
    }
#line 2106 "handlebars.tab.c"
    break;

  case 68:
#line 501 "handlebars.y"
    {
      (yyval.ast_node) = handlebars_ast_node_ctor_number(parser, (yyvsp[0].string), &(yyloc));
    }
#line 2114 "handlebars.tab.c"
    break;

  case 69:
#line 504 "handlebars.y"
    {
      (yyval.ast_node) = handlebars_ast_node_ctor_boolean(parser, (yyvsp[0].string), &(yyloc));
    }
#line 2122 "handlebars.tab.c"
    break;

  case 70:
#line 507 "handlebars.y"
    {
      (yyval.ast_node) = handlebars_ast_node_ctor_undefined(parser, (yyvsp[0].string), &(yyloc));
    }
#line 2130 "handlebars.tab.c"
    break;

  case 71:
#line 510 "handlebars.y"
    {
      (yyval.ast_node) = handlebars_ast_node_ctor_null(parser, (yyvsp[0].string), &(yyloc));
    }
#line 2138 "handlebars.tab.c"
    break;

  case 72:
#line 516 "handlebars.y"
    {
      (yyval.ast_node) = (yyvsp[0].ast_node);
    }
#line 2146 "handlebars.tab.c"
    break;

  case 73:
#line 519 "handlebars.y"
    {
      (yyval.ast_node) = (yyvsp[0].ast_node);
    }
#line 2154 "handlebars.tab.c"
    break;

  case 74:
#line 525 "handlebars.y"
    {
      (yyval.ast_node) = handlebars_ast_helper_prepare_path(parser, (yyvsp[0].ast_list), 1, &(yyloc));
    }
#line 2162 "handlebars.tab.c"
    break;

  case 75:
#line 531 "handlebars.y"
    {
      (yyval.ast_node) = handlebars_ast_helper_prepare_path(parser, (yyvsp[0].ast_list), 0, &(yyloc));
    }
#line 2170 "handlebars.tab.c"
    break;

  case 76:
#line 537 "handlebars.y"
    {
      struct handlebars_ast_node * ast_node = handlebars_ast_node_ctor_path_segment(parser, (yyvsp[0].string), (yyvsp[-1].string), &(yyloc));
      
      handlebars_ast_list_append((yyvsp[-2].ast_list), ast_node);
      (yyval.ast_list) = (yyvsp[-2].ast_list);
    }
#line 2181 "handlebars.tab.c"
    break;

  case 77:
#line 543 "handlebars.y"
    {
      struct handlebars_ast_node * ast_node;
      MEMCHK((yyvsp[0].string)); // this is weird
  	  
      ast_node = handlebars_ast_node_ctor_path_segment(parser, (yyvsp[0].string), NULL, &(yyloc));
      MEMCHK(ast_node); // this is weird
      
      (yyval.ast_list) = handlebars_ast_list_ctor(CONTEXT);
      handlebars_ast_list_append((yyval.ast_list), ast_node);
    }
#line 2196 "handlebars.tab.c"
    break;


#line 2200 "handlebars.tab.c"

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
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

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
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (&yylloc, parser, YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (&yylloc, parser, yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }

  yyerror_range[1] = yylloc;

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
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

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYTERROR;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
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
                  yystos[yystate], yyvsp, yylsp, parser);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  yyerror_range[2] = yylloc;
  /* Using YYLLOC is tempting, but would change the location of
     the lookahead.  YYLOC is available though.  */
  YYLLOC_DEFAULT (yyloc, yyerror_range, 2);
  *++yylsp = yyloc;

  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;


#if !defined yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (&yylloc, parser, YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif


/*-----------------------------------------------------.
| yyreturn -- parsing is finished, return the result.  |
`-----------------------------------------------------*/
yyreturn:
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
                  yystos[*yyssp], yyvsp, yylsp, parser);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  return yyresult;
}
