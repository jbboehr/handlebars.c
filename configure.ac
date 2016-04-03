
# Define the package version numbers
m4_define([HANDLEBARS_MAJOR], 0)
m4_define([HANDLEBARS_MINOR], 5)
m4_define([HANDLEBARS_PATCH], 0)

# Define the libtool version numbers; check the Autobook, Section 11.4.
# m4_define([HANDLEBARS_RELEASE], 0)
m4_define([HANDLEBARS_CURRENT], 4)
m4_define([HANDLEBARS_REVISION], 0)
m4_define([HANDLEBARS_AGE], 0)

# Define the version of the specs
m4_define([HANDLEBARS_SPEC_VERSION], [4.0.5])
m4_define([MUSTACHE_SPEC_VERSION], [1.1.3])

# Initialize autoconf & automake.
AC_PREREQ(2.61)
AC_INIT([handlebars-c], [HANDLEBARS_MAJOR.HANDLEBARS_MINOR.HANDLEBARS_PATCH-dev], 
        [jbboehr@gmail.com], [handlebars-c], 
        [https://github.com/jbboehr/handlebars.c])

AC_DEFINE([PACKAGE_DESCRIPTION], ["Library for handlebars"], [Description])
AC_SUBST([PACKAGE_DESCRIPTION], ["Library for handlebars"])

AC_CONFIG_AUX_DIR([build])
AC_CONFIG_HEADERS([config.h src/handlebars_config.h])
AC_CONFIG_SRCDIR([src])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([foreign subdir-objects -Wall])

AX_CHECK_ENABLE_DEBUG([no])
AX_CFLAGS_WARN_ALL

# Define macro variables for the package version numbers.
AC_DEFINE(HANDLEBARS_VERSION_MAJOR, HANDLEBARS_MAJOR, [Define the major version number.])
AC_DEFINE(HANDLEBARS_VERSION_MINOR, HANDLEBARS_MINOR, [Define the minor version number.])
AC_DEFINE(HANDLEBARS_VERSION_PATCH, HANDLEBARS_PATCH, [Define the patch version number.])
AC_DEFINE(HANDLEBARS_VERSION_STRING, "HANDLEBARS_MAJOR.HANDLEBARS_MINOR.HANDLEBARS_PATCH", [Define the version string.])
AC_DEFINE(HANDLEBARS_SPEC_VERSION_STRING, "HANDLEBARS_SPEC_VERSION", [Define the handlebars spec version])
AC_DEFINE(MUSTACHE_SPEC_VERSION_STRING, "MUSTACHE_SPEC_VERSION", [Define the mustache spec version])

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
AC_PROG_CC_C99
AC_PROG_INSTALL
AC_PROG_LEX
AC_PROG_LIBTOOL
AC_PROG_LN_S
AC_PROG_YACC
AC_PATH_PROGS(BISON, [bison], no)
AC_PATH_PROGS(FLEX, [flex], no)
AC_PATH_PROGS(GPERF, [gperf], no)
AC_PATH_PROGS(DOXYGEN, [doxygen], no)
AC_PATH_PROGS(LCOV, [lcov], no)
AC_PATH_PROGS(RE2C, [re2c], no)

AM_CONDITIONAL([BISON], [test x$BISON != xno])
AM_CONDITIONAL([DEBUG], [test x$enable_debug = xyes])
AM_CONDITIONAL([FLEX], [test x$FLEX != xno])
AM_CONDITIONAL([GPERF], [test x$GPERF != xno])
AM_CONDITIONAL([GCC], [test x$GCC = xyes])
AM_CONDITIONAL([LCOV], [test x$LCOV != xno])
AM_CONDITIONAL([RE2C], [test x$RE2C != xno])
AM_CONDITIONAL(HAVE_DOXYGEN, [test "x$DOXYGEN" != "xno"])
AS_IF([test "x$DOXYGEN" = "xno"], [
  AC_MSG_WARN([Doxygen not found - continuing without Doxygen support])
])

# Checks for header files
AC_HEADER_STDC
AC_FUNC_MALLOC
AC_HEADER_DIRENT
AC_HEADER_STDBOOL
AC_CHECK_HEADERS(ctype.h, , AC_MSG_ERROR([ctype.h is required]))
AC_CHECK_HEADERS(errno.h, , AC_MSG_ERROR([errno.h is required]))
AC_CHECK_HEADERS(stdint.h, , AC_MSG_ERROR([stdint.h is required]))
AC_CHECK_HEADERS(stdarg.h, , AC_MSG_ERROR([stdarg.h is required]))
AC_CHECK_HEADERS(stdbool.h, , AC_MSG_ERROR([stdbool.h is required]))

# Checks for functions
AC_CHECK_FUNC(snprintf, , AC_MSG_ERROR([snprintf is required]))

# Checks for typedefs, structures, and compiler characteristics
AC_C_CONST
AC_C_INLINE
AC_C_STRINGIZE
AC_TYPE_SIZE_T
AX_GCC_BUILTIN([__builtin_expect])
AX_GCC_VAR_ATTRIBUTE([unused])
AX_GCC_FUNC_ATTRIBUTE(returns_nonnull)
AX_COMPUTED_GOTOS

# Checks for system
AC_EXEEXT

# Checks for libraries
AX_PTHREAD
AC_CHECK_LIB([lmdb], [mdb_version])
AC_CHECK_LIB([pcre], [pcre_version])
AC_CHECK_LIB([talloc], [talloc_version_major], , [
    AC_MSG_FAILURE([talloc test failed, talloc is required])
])

LIBS="$PTHREAD_LIBS $LIBS"
CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
LDFLAGS="$LDFLAGS $PTHREAD_CFLAGS"

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
        AC_MSG_WARN([check required librt (--disable-check to disable)])
    ])
])
AM_CONDITIONAL([CHECK], [test x$enable_check = xyes])
AM_CONDITIONAL([CHECK_HAS_TAP], [test "x$CHECK_HAS_TAP" == "xyes"])

# handlebars-spec
AC_ARG_WITH([handlebars-spec],
	[AS_HELP_STRING([--with-handlebars-spec=SPEC_LOCATION], [location to find handlebars specification @<:@default=./spec/handlebars@>:@])],
	[], [with_handlebars_spec=`pwd`/spec/handlebars])

AS_IF([test "x$with_handlebars_spec" != "xno"], [
  AC_CHECK_FILE([$with_handlebars_spec/spec/basic.json], [
    AC_SUBST([HANDLEBARS_SPEC_DIR], [$with_handlebars_spec])
  ], [
    AC_MSG_FAILURE([handlebars spec missing (--without-handlebars-spec to disable)])
  ])
])

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

# mustache-spec
AC_ARG_WITH([mustache-spec],
	[AS_HELP_STRING([--with-mustache-spec=SPEC_LOCATION], [location to find mustache specification @<:@default=./spec/mustache@>:@])],
	[], [with_mustache_spec=`pwd`/spec/mustache])

AS_IF([test "x$with_mustache_spec" != "xno"], [
  AC_CHECK_FILE([$with_mustache_spec/specs/comments.yml], [
    AC_SUBST([MUSTACHE_SPEC_DIR], [$with_mustache_spec])
  ], [
    AC_MSG_FAILURE([mustache spec missing (--without-mustache-spec to disable)])
  ])
])

# refcounting
AC_ARG_ENABLE([refcounting],
	[AS_HELP_STRING([--disable-refcounting], [disable refcounting of handlebars values])], [])

AS_IF([test "x$enable_refcounting" == "xno"], [
    AC_DEFINE([HANDLEBARS_NO_REFCOUNT], [1], [Disable refcounting of handlebars values])
])

# memory
AC_ARG_ENABLE([handlebars-memory],
	[AS_HELP_STRING([--enable-handlebars-memory], [enable handlebars memory testing functions])], [])

AS_IF([test "x$enable_handlebars_memory" == "xyes"], [
    AC_DEFINE([HANDLEBARS_MEMORY], [1], [Enable handlebars memory testing functions])
])
AM_CONDITIONAL([HANDLEBARS_MEMORY], [test x$enable_handlebars_memory = xyes])

# Options
AC_DEFINE([YY_NO_UNISTD_H], [1], [Disable usage of unistd.h])

# gcov
AC_ARG_ENABLE([gcov], AS_HELP_STRING([--enable-gcov], [compile with gcov]))

AS_IF([test "x$enable_gcov" == "xyes"], [
  AC_CHECK_LIB(gcov, __gcov_open, , [
    AC_MSG_FAILURE([gcov test failed (--disable-gcov to disable)])
  ])

  AC_SUBST([LIBGCOV_LIBS], ["-lgcov -coverage"])
  LIBGCOV_CFLAGS="-g -O0 --coverage" # -disable-shared
  LIBGCOV_LDFLAGS="--coverage"
  CFLAGS+=" $LIBGCOV_CFLAGS"
  LDFLAGS+=" $LIBGCOV_LDFLAGS"
])

AM_CONDITIONAL([GCOV], [test x$enable_gcov = xyes])

# yaml
AC_ARG_ENABLE([yaml],
	[AS_HELP_STRING([--disable-yaml], [disable support for yaml])], [])

AS_IF([test "x$enable_yaml" != "xno"], [
  AC_CHECK_HEADERS(yaml.h)
  AC_CHECK_LIB([yaml], [yaml_get_version_string])
])

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
