/* C code produced by gperf version 3.0.4 */
/* Command-line: /usr/bin/gperf --struct-type --readonly-tables --pic --output-file=handlebars_helpers_ht.h handlebars_helpers_ht.gperf  */
/* Computed positions: -k'1' */

#if !((' ' == 32) && ('!' == 33) && ('"' == 34) && ('#' == 35) \
      && ('%' == 37) && ('&' == 38) && ('\'' == 39) && ('(' == 40) \
      && (')' == 41) && ('*' == 42) && ('+' == 43) && (',' == 44) \
      && ('-' == 45) && ('.' == 46) && ('/' == 47) && ('0' == 48) \
      && ('1' == 49) && ('2' == 50) && ('3' == 51) && ('4' == 52) \
      && ('5' == 53) && ('6' == 54) && ('7' == 55) && ('8' == 56) \
      && ('9' == 57) && (':' == 58) && (';' == 59) && ('<' == 60) \
      && ('=' == 61) && ('>' == 62) && ('?' == 63) && ('A' == 65) \
      && ('B' == 66) && ('C' == 67) && ('D' == 68) && ('E' == 69) \
      && ('F' == 70) && ('G' == 71) && ('H' == 72) && ('I' == 73) \
      && ('J' == 74) && ('K' == 75) && ('L' == 76) && ('M' == 77) \
      && ('N' == 78) && ('O' == 79) && ('P' == 80) && ('Q' == 81) \
      && ('R' == 82) && ('S' == 83) && ('T' == 84) && ('U' == 85) \
      && ('V' == 86) && ('W' == 87) && ('X' == 88) && ('Y' == 89) \
      && ('Z' == 90) && ('[' == 91) && ('\\' == 92) && (']' == 93) \
      && ('^' == 94) && ('_' == 95) && ('a' == 97) && ('b' == 98) \
      && ('c' == 99) && ('d' == 100) && ('e' == 101) && ('f' == 102) \
      && ('g' == 103) && ('h' == 104) && ('i' == 105) && ('j' == 106) \
      && ('k' == 107) && ('l' == 108) && ('m' == 109) && ('n' == 110) \
      && ('o' == 111) && ('p' == 112) && ('q' == 113) && ('r' == 114) \
      && ('s' == 115) && ('t' == 116) && ('u' == 117) && ('v' == 118) \
      && ('w' == 119) && ('x' == 120) && ('y' == 121) && ('z' == 122) \
      && ('{' == 123) && ('|' == 124) && ('}' == 125) && ('~' == 126))
/* The character set is not based on ISO-646.  */
error "gperf generated tables don't work with this execution character set. Please report a bug to <bug-gnu-gperf@gnu.org>."
#endif

#line 1 "handlebars_helpers_ht.gperf"
struct handlebars_builtin_pair { const char * name; int pos };

#define TOTAL_KEYWORDS 8
#define MIN_WORD_LENGTH 2
#define MAX_WORD_LENGTH 18
#define MIN_HASH_VALUE 2
#define MAX_HASH_VALUE 18
/* maximum key range = 17, duplicates = 0 */

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
static unsigned int
hash (str, len)
     register const char *str;
     register unsigned int len;
{
  static const unsigned char asso_values[] =
    {
      19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
      19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
      19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
      19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
      19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
      19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
      19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
      19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
      19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
      19, 19, 19, 19, 19, 19, 19, 19,  0, 19,
      19,  5, 19, 19,  0,  0, 19, 19,  0, 19,
      19, 19, 19, 19, 19, 19, 19,  5, 19,  0,
      19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
      19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
      19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
      19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
      19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
      19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
      19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
      19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
      19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
      19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
      19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
      19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
      19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
      19, 19, 19, 19, 19, 19
    };
  return len + asso_values[(unsigned char)str[0]];
}

struct stringpool_t
  {
    char stringpool_str2[sizeof("if")];
    char stringpool_str3[sizeof("log")];
    char stringpool_str4[sizeof("with")];
    char stringpool_str6[sizeof("lookup")];
    char stringpool_str9[sizeof("each")];
    char stringpool_str11[sizeof("unless")];
    char stringpool_str13[sizeof("helperMissing")];
    char stringpool_str18[sizeof("blockHelperMissing")];
  };
static const struct stringpool_t stringpool_contents =
  {
    "if",
    "log",
    "with",
    "lookup",
    "each",
    "unless",
    "helperMissing",
    "blockHelperMissing"
  };
#define stringpool ((const char *) &stringpool_contents)
#ifdef __GNUC__
__inline
#if defined __GNUC_STDC_INLINE__ || defined __GNUC_GNU_INLINE__
__attribute__ ((__gnu_inline__))
#endif
#endif
const struct handlebars_builtin_pair *
in_word_set (str, len)
     register const char *str;
     register unsigned int len;
{
  static const struct handlebars_builtin_pair wordlist[] =
    {
      {-1}, {-1},
#line 6 "handlebars_helpers_ht.gperf"
      {(int)(long)&((struct stringpool_t *)0)->stringpool_str2,3},
#line 9 "handlebars_helpers_ht.gperf"
      {(int)(long)&((struct stringpool_t *)0)->stringpool_str3,6},
#line 8 "handlebars_helpers_ht.gperf"
      {(int)(long)&((struct stringpool_t *)0)->stringpool_str4,5},
      {-1},
#line 10 "handlebars_helpers_ht.gperf"
      {(int)(long)&((struct stringpool_t *)0)->stringpool_str6,7},
      {-1}, {-1},
#line 5 "handlebars_helpers_ht.gperf"
      {(int)(long)&((struct stringpool_t *)0)->stringpool_str9,2},
      {-1},
#line 7 "handlebars_helpers_ht.gperf"
      {(int)(long)&((struct stringpool_t *)0)->stringpool_str11,4},
      {-1},
#line 4 "handlebars_helpers_ht.gperf"
      {(int)(long)&((struct stringpool_t *)0)->stringpool_str13,1},
      {-1}, {-1}, {-1}, {-1},
#line 3 "handlebars_helpers_ht.gperf"
      {(int)(long)&((struct stringpool_t *)0)->stringpool_str18,0}
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = hash (str, len);

      if (key <= MAX_HASH_VALUE && key >= 0)
        {
          register int o = wordlist[key].name;
          if (o >= 0)
            {
              register const char *s = o + stringpool;

              if (*str == *s && !strcmp (str + 1, s + 1))
                return &wordlist[key];
            }
        }
    }
  return 0;
}
