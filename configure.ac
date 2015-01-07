
AC_PREREQ(2.61)
AC_INIT([handlebars], [0.0.0], [jbboehr@gmail.com], [handlebars],
        [https://github.com/jbboehr/handlebars.c])

AC_DEFINE([PACKAGE_DESCRIPTION], ["Library for handlebars"], [Description])
AC_SUBST([PACKAGE_DESCRIPTION], ["Library for handlebars"])

AC_CONFIG_HEADERS([config.h])
#AC_CONFIG_SRCDIR([src])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build])

AM_INIT_AUTOMAKE([foreign -Wall -Werror subdir-objects])

# Checks for programs
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CC_STDC
AC_PROG_LEX
AC_PROG_LIBTOOL
AC_PROG_YACC
AC_PATH_PROGS(BISON, [bison], no)
AC_PATH_PROGS(FLEX, [flex], no)

AM_CONDITIONAL([GCC], [test x$GCC = xyes])

# Checks for header files
AC_HEADER_STDC
AC_FUNC_MALLOC
AC_HEADER_DIRENT
AC_CHECK_HEADERS(ctype.h, , AC_MSG_ERROR([ctype.h is required]))
AC_CHECK_HEADERS(errno.h, , AC_MSG_ERROR([errno.h is required]))
AC_CHECK_HEADERS(memory.h, , AC_MSG_ERROR([memory.h is required]))
AC_CHECK_HEADERS(stdio.h, , AC_MSG_ERROR([stdio.h is required]))
AC_CHECK_HEADERS(stdint.h, , AC_MSG_ERROR([stdint.h is required]))
AC_CHECK_HEADERS(uthash.h, , AC_MSG_ERROR([uthash.h is required]))

# Checks for functions
AC_CHECK_FUNC(calloc, , AC_MSG_ERROR([calloc is required]))
AC_CHECK_FUNC(memcpy, , AC_MSG_ERROR([memcpy is required]))

# Checks for typedefs, structures, and compiler characteristics
AC_TYPE_SIZE_T

# Checks for system
AC_EXEEXT

# json
AC_ARG_ENABLE([json],
	[AS_HELP_STRING([--disable-json], [disable support for json])], [])

AS_IF([test "x$enable_json" != "xno"], [
  AC_CHECK_HEADERS([json/json.h json/json_object.h json/json_tokener.h], [], [
    AC_MSG_FAILURE([json header missing (--disable-json to disable)])
  ])
  AC_CHECK_LIB([json], [json_tokener_new], , [
    AC_MSG_FAILURE([json test failed (--disable-json to disable)])
  ])
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

# yydebug
AC_ARG_ENABLE([yydebug], 
  [AS_HELP_STRING([--enable-yydebug], [compile with yydebug functions])], [])

AS_IF([test "x$enable_yydebug" == "xyes"], [
  AC_DEFINE([YYDEBUG], [1], [Defined if yydebug functions requested])
])
AM_CONDITIONAL([YYDEBUG], [test x$enable_yydebug = xyes])

AC_CONFIG_FILES([
  handlebars.pc
  Makefile
])
AC_OUTPUT
