
AC_DEFUN([HBS_STATEMENT_EXPRESSIONS],[dnl
AC_MSG_CHECKING([if ${CC-gcc} supports statement expressions])
AC_COMPILE_IFELSE(
  [AC_LANG_PROGRAM(
    [],
    [[
// from https://gcc.gnu.org/onlinedocs/gcc/Statement-Exprs.html
#define maxint(a,b) ({int _a = (a), _b = (b); _a > _b ? _a : _b; });
		int _a = 1, _b = 2, c;
		c = maxint (_a, _b);
      	return c == 2 ? 0 : 1;
    ]])],
  [AC_MSG_RESULT(yes)
   AC_DEFINE(HAVE_STATEMENT_EXPRESSIONS, 1,
     [Define to 1 if the compiler supports statement expressions])],
  [AC_MSG_RESULT(no)])
])
