
# Define the package version numbers
m4_define([HANDLEBARS_MAJOR], 0)
m4_define([HANDLEBARS_MINOR], 5)
m4_define([HANDLEBARS_PATCH], 1)

# Define the libtool version numbers; check the Autobook, Section 11.4.
m4_define([HANDLEBARS_CURRENT], 4)
m4_define([HANDLEBARS_REVISION], 0)
m4_define([HANDLEBARS_AGE], 0)

# Define the version of the specs
m4_define([HANDLEBARS_SPEC_VERSION], [4.0.5])
m4_define([MUSTACHE_SPEC_VERSION], [1.1.3])

# Initialize autoconf & automake.
AC_PREREQ(2.61)
AC_INIT([handlebars-c], [HANDLEBARS_MAJOR.HANDLEBARS_MINOR.HANDLEBARS_PATCH],
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
HANDLEBARS_LT_CURRENT=HANDLEBARS_CURRENT
HANDLEBARS_LT_REVISION=HANDLEBARS_REVISION
HANDLEBARS_LT_AGE=HANDLEBARS_AGE
AC_SUBST(HANDLEBARS_LT_CURRENT)
AC_SUBST(HANDLEBARS_LT_REVISION)
AC_SUBST(HANDLEBARS_LT_AGE)

# Define variables for bison and flex flags
AC_ARG_VAR([BISONFLAGS], [bison flags])
AS_IF([test "x$BISONFLAGS" == "x"], [BISONFLAGS="-Wall"])
AC_ARG_VAR([FLEXFLAGS], [flex flags])
AS_IF([test "x$FLEXFLAGS" == "x"], [FLEXFLAGS="--noreject --fast --align"])
AC_ARG_VAR([GPERFFLAGS], [gperf flags])
AS_IF([test "x$GPERFFLAGS" == "x"], [GPERFFLAGS="--struct-type --readonly-tables --pic"])
AC_ARG_VAR([RE2CFLAGS], [re2c flags])
AS_IF([test "x$RE2CFLAGS" == "x"], [RE2CFLAGS="--no-generation-date -s8"])

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
AX_PROG_FLEX([
	AX_PROG_FLEX_VERSION([2.5.35], , AC_MSG_ERROR([flex >= 2.5.35 is required]))
])
AX_PROG_BISON([
	AX_PROG_BISON_VERSION([3.0.2], , AC_MSG_ERROR([bison >= 3.0.2 is required]))
])

AM_CONDITIONAL([BISON], [test x$BISON != xno])
AM_CONDITIONAL([DEBUG], [test x$enable_debug = xyes])
AM_CONDITIONAL([FLEX], [test x$FLEX != xno])
AM_CONDITIONAL([GPERF], [test x$GPERF != xno])
AM_CONDITIONAL([GCC], [test x$GCC = xyes])
AM_CONDITIONAL([LCOV], [test x$LCOV != xno])
AM_CONDITIONAL([RE2C], [test x$RE2C != xno])
AM_CONDITIONAL(HAVE_DOXYGEN, [test "x$DOXYGEN" != "xno"])
AS_IF([test "x$DOXYGEN" = "xno"], [AC_MSG_WARN([Doxygen not found - continuing without Doxygen support])])

# Checks for header files
AC_HEADER_STDC

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
HBS_COMPUTED_GOTOS

# Checks for libraries
AX_PTHREAD
AC_CHECK_LIB([lmdb], [mdb_version])
AC_CHECK_LIB([m], [cos])
AC_CHECK_LIB([pcre], [pcre_version])
AC_CHECK_LIB([rt], [timer_create])
AC_CHECK_LIB([subunit], [subunit_test_start])
AC_CHECK_LIB([talloc], [talloc_version_major], [], [AC_MSG_FAILURE([talloc is required])])
PKG_CHECK_MODULES(CHECK, [check >= 0.9.12], [], [AC_MSG_FAILURE([check >= 0.9.12 required])])

LIBS="$PTHREAD_LIBS $LIBS"
CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
LDFLAGS="$LDFLAGS $PTHREAD_CFLAGS"

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
