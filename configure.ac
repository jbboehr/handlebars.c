
# Define the package version numbers
m4_define([HANDLEBARS_MAJOR], 0)
m4_define([HANDLEBARS_MINOR], 1)
m4_define([HANDLEBARS_PATCH], 0)

# Define the libtool version numbers; check the Autobook, Section 11.4.
# m4_define([HANDLEBARS_RELEASE], 0)
m4_define([HANDLEBARS_CURRENT], 1)
m4_define([HANDLEBARS_REVISION], 0)
m4_define([HANDLEBARS_AGE], 0)

# Initialize autoconf & automake.
AC_PREREQ(2.61)
AC_INIT([handlebars], [HANDLEBARS_MAJOR.HANDLEBARS_MINOR.HANDLEBARS_PATCH], 
        [jbboehr@gmail.com], [handlebars], 
        [https://github.com/jbboehr/handlebars.c])

AC_DEFINE([PACKAGE_DESCRIPTION], ["Library for handlebars"], [Description])
AC_SUBST([PACKAGE_DESCRIPTION], ["Library for handlebars"])

AC_CONFIG_AUX_DIR([build])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([src])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([foreign -Wall -Werror subdir-objects])

# Define macro variables for the package version numbers.
AC_DEFINE(HANDLEBARS_VERSION_MAJOR, HANDLEBARS_MAJOR, [Define the major version number.])
AC_DEFINE(HANDLEBARS_VERSION_MINOR, HANDLEBARS_MINOR, [Define the minor version number.])
AC_DEFINE(HANDLEBARS_VERSION_PATCH, HANDLEBARS_PATCH, [Define the patch version number.])
AC_DEFINE(HANDLEBARS_VERSION_STRING, "HANDLEBARS_MAJOR.HANDLEBARS_MINOR.HANDLEBARS_PATCH", [Define the version string.])

# Define substitutions for the libtool version numbers.
# HANDLEBARS_LT_RELEASE=HANDLEBARS_RELEASE
HANDLEBARS_LT_CURRENT=HANDLEBARS_CURRENT
HANDLEBARS_LT_REVISION=HANDLEBARS_REVISION
HANDLEBARS_LT_AGE=HANDLEBARS_AGE
# AC_SUBST(HANDLEBARS_LT_RELEASE)
AC_SUBST(HANDLEBARS_LT_CURRENT)
AC_SUBST(HANDLEBARS_LT_REVISION)
AC_SUBST(HANDLEBARS_LT_AGE)

# Checks for scripts
AC_REQUIRE_AUX_FILE([tap-driver.sh])

# Checks for programs
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CC_STDC
AC_PROG_INSTALL
AC_PROG_LEX
AC_PROG_LIBTOOL
AC_PROG_LN_S
AC_PROG_YACC
AC_PATH_PROGS(BISON, [bison], no)
AC_PATH_PROGS(FLEX, [flex], no)
AC_PATH_PROGS(DOXYGEN, [doxygen], no)
AC_PATH_PROGS(LCOV, [lcov], no)
AM_CONDITIONAL([GCC], [test x$GCC = xyes])
AM_CONDITIONAL([LCOV], [test x$LCOV != xno])
AM_CONDITIONAL(HAVE_DOXYGEN, [test "x$DOXYGEN" != "xno"])
AS_IF([test "x$DOXYGEN" = "xno"], [
  AC_MSG_WARN([Doxygen not found - continuing without Doxygen support])
])

# Checks for header files
AC_HEADER_STDC
AC_FUNC_MALLOC
AC_HEADER_DIRENT
AC_CHECK_HEADERS(ctype.h, , AC_MSG_ERROR([ctype.h is required]))
AC_CHECK_HEADERS(errno.h, , AC_MSG_ERROR([errno.h is required]))
AC_CHECK_HEADERS(stdint.h, , AC_MSG_ERROR([stdint.h is required]))
AC_CHECK_HEADERS(uthash.h, , AC_MSG_ERROR([uthash.h is required]))

# Checks for functions
AC_CHECK_FUNC(snprintf, , AC_MSG_ERROR([snprintf is required]))

# Checks for typedefs, structures, and compiler characteristics
AC_C_CONST
AC_TYPE_SIZE_T

# Checks for system
AC_EXEEXT

# Checks for libraries

# check
AC_ARG_ENABLE([check],
  [AS_HELP_STRING([--disable-check], [disable support for check])])

AS_IF([test "x$enable_check" != "xno"], [
  PKG_CHECK_MODULES(CHECK, [check >= 0.9.12], [
    CHECK_HAS_TAP=yes
  ], [
    CHECK_HAS_TAP=no
    PKG_CHECK_MODULES(CHECK, [check >= 0.9.4], [
    ], [
      AC_MSG_FAILURE([check version too low (--disable-check to disable)])
    ])
  ])
  AC_CHECK_HEADERS(check.h, [], [
    AC_MSG_FAILURE([check.h missing (--disable-check to disable)])
  ])
#  AC_CHECK_LIB([check], [check_major_version], [
    AC_DEFINE([HAVE_LIBCHECK], [1], [Use check])
    AC_SUBST([LIBCHECK_LIBS], ["-lcheck"])
#  ], [
#    AC_MSG_FAILURE([check test failed (--disable-check to disable)])
#  ])
	AC_CHECK_LIB([m], [cos], [], [
		AC_MSG_FAILURE([check required libm (--disable-check to disable)])
	])
	AC_CHECK_LIB([rt], [timer_create], [], [
		AC_MSG_FAILURE([check required librt (--disable-check to disable)])
	])
])
AM_CONDITIONAL([CHECK], [test x$enable_check = xyes])
AM_CONDITIONAL([CHECK_HAS_TAP], [test "x$CHECK_HAS_TAP" == "xyes"])

# json
AC_ARG_ENABLE([json],
	[AS_HELP_STRING([--disable-json], [disable support for json])], [])

AS_IF([test "x$enable_json" != "xno"], [
  # it was renamed in trusty, detect if old or new name is available
  AC_CHECK_LIB([json], [json_tokener_new], , [
    AC_CHECK_LIB([json-c], [json_tokener_new], , [
      AC_MSG_FAILURE([json test failed (--disable-json to disable)])
    ])
  ])
  AC_CHECK_HEADERS([json/json.h json/json_object.h json/json_tokener.h], , [
    AC_CHECK_HEADERS([json-c/json.h json-c/json_object.h json-c/json_tokener.h], , [
      AC_MSG_FAILURE([json header missing (--disable-json to disable)])
    ])
  ])
])

# talloc
AC_CHECK_LIB([talloc], [talloc_version_major], , [
  AC_MSG_FAILURE([talloc test failed, talloc is required])
])

# yaml
AC_ARG_ENABLE([yaml], 
	[AS_HELP_STRING([--disable-yaml], [disable support for yaml])], [])

AS_IF([test "x$enable_yaml" != "xno"], [
  AC_CHECK_HEADERS(yaml.h, [], [
    AC_MSG_FAILURE([yaml.h missing (--disable-yaml to disable)])
  ])
  AC_CHECK_LIB([yaml], [yaml_get_version_string], , [
    AC_MSG_FAILURE([yaml test failed (--disable-yaml to disable)])
  ])
])

# Options
AC_DEFINE([YY_NO_UNISTD_H], [1], [Disable usage of unistd.h])

# gcov
AC_ARG_ENABLE([gcov], AS_HELP_STRING([--enable-gcov], [compile with gcov]))

AS_IF([test "x$enable_gcov" == "xyes"], [
  AC_CHECK_LIB(gcov, __gcov_open, , [
    AC_MSG_FAILURE([gcov test failed (--disable-gcov to disable)])
  ])

  AC_SUBST([LIBGCOV_LIBS], ["-lgcov -coverage"])
  LIBGCOV_CFLAGS="-g -O0 -coverage" # -disable-shared
  CFLAGS="$CFLAGS $LIBGCOV_CFLAGS"
])

AM_CONDITIONAL([GCOV], [test x$enable_gcov = xyes])

# memdebug
#AC_ARG_ENABLE([memdebug], 
#  [AS_HELP_STRING([--enable-memdebug], [compile with memory debug functions])], [])
#
#AS_IF([test "x$enable_memdebug" != "xno"], [
#  AC_DEFINE([HANDLEBARS_MEMDEBUG], [1], [Defined if memory debugging is requested])
#])
#AM_CONDITIONAL([HANDLEBARS_MEMDEBUG], [test x$enable_memdebug = xyes])

# yydebug
AC_ARG_ENABLE([yydebug], 
  [AS_HELP_STRING([--enable-yydebug], [compile with yydebug functions])], [])

AS_IF([test "x$enable_yydebug" == "xyes"], [
  AC_DEFINE([YYDEBUG], [1], [Defined if yydebug functions requested])
])
AM_CONDITIONAL([YYDEBUG], [test x$enable_yydebug = xyes])

# Define Makefiles.
AC_CONFIG_FILES([
  handlebars.pc
  src/Makefile
  Makefile
  doc/Doxyfile
  doc/Makefile
  bin/Makefile
  tests/Makefile
])

# Generate the "configure" script.
AC_OUTPUT
