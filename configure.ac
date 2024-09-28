# Copyright (c) anno Domini nostri Jesu Christi MMXVI-MMXXIV John Boehr & contributors
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, see <http://www.gnu.org/licenses/>.

# Define the package version numbers
m4_define([HANDLEBARS_MAJOR], 1)
m4_define([HANDLEBARS_MINOR], 0)
m4_define([HANDLEBARS_PATCH], 0)

# Define the libtool version numbers; check the Autobook, Section 11.4.
m4_define([HANDLEBARS_CURRENT], 9)
m4_define([HANDLEBARS_REVISION], 0)
m4_define([HANDLEBARS_AGE], 0)

# Define the version of the specs
m4_define([HANDLEBARS_SPEC_VERSION], [4.7.7])
m4_define([MUSTACHE_SPEC_VERSION], [1.1.3])

# Initialize autoconf & automake.
AC_PREREQ(2.61)
AC_INIT([handlebars], [HANDLEBARS_MAJOR.HANDLEBARS_MINOR.HANDLEBARS_PATCH],
        [jbboehr@gmail.com], [handlebars],
        [https://github.com/jbboehr/handlebars.c])

AC_DEFINE([PACKAGE_DESCRIPTION], ["Library for handlebars"], [Description])
AC_SUBST([PACKAGE_DESCRIPTION], ["Library for handlebars"])

AC_CONFIG_AUX_DIR([build])
AC_CONFIG_HEADERS([config.h src/handlebars_config.h])
AC_CONFIG_SRCDIR([src])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([foreign subdir-objects -Wall -Werror])

AX_IS_RELEASE([git-directory])
AX_CHECK_ENABLE_DEBUG
AX_CFLAGS_WARN_ALL([WARN_CFLAGS])
AX_COMPILER_FLAGS(,,,,[-Wno-declaration-after-statement -Wno-error=declaration-after-statement -Wno-switch-enum -Wno-error=switch-enum -Wdeprecated-declarations -Wclobbered -Wmaybe-uninitialized])

# my custom hacked up ax_am_macros_static
AC_DEFUN([AMINCLUDE_STATIC],[build/aminclude_static.am])
AC_DEFUN([AX_AM_MACROS_STATIC],
[
AX_AC_PRINT_TO_FILE(AMINCLUDE_STATIC,[
# ]AMINCLUDE_STATIC[ generated automatically by Autoconf
# from AX_AM_MACROS_STATIC
])
])

# Define macro variables for the package version numbers.
AC_DEFINE(HANDLEBARS_VERSION_MAJOR, HANDLEBARS_MAJOR, [Define the major version number.])
AC_DEFINE(HANDLEBARS_VERSION_MINOR, HANDLEBARS_MINOR, [Define the minor version number.])
AC_DEFINE(HANDLEBARS_VERSION_PATCH, HANDLEBARS_PATCH, [Define the patch version number.])
AC_DEFINE(HANDLEBARS_VERSION_STRING, "HANDLEBARS_MAJOR.HANDLEBARS_MINOR.HANDLEBARS_PATCH", [Define the version string.])
AC_DEFINE(HANDLEBARS_VERSION_INT, (HANDLEBARS_PATCH + HANDLEBARS_MINOR * 100 + HANDLEBARS_MAJOR * 10000), [Define the version integer.])
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
AC_ARG_VAR([RE2CFLAGS], [re2c flags])
AS_IF([test "x$RE2CFLAGS" == "x"], [RE2CFLAGS="--no-generation-date -s8"])

# Checks for scripts
AC_REQUIRE_AUX_FILE([tap-driver.sh])

# Checks for compilers
AC_PROG_CC
AC_PROG_CC_STDC
AC_PROG_CC_C99

AC_MSG_CHECKING([if compiling with gcc])
AC_COMPILE_IFELSE(
[AC_LANG_PROGRAM([], [[
#ifndef __GNUC__
       not gcc
#endif
]])],
[GCC=yes], [GCC=no])
AC_MSG_RESULT([$GCC])
AM_CONDITIONAL([GCC], [test "x$GCC" = "xyes"])

AC_MSG_CHECKING([if compiling with clang])
AC_COMPILE_IFELSE(
[AC_LANG_PROGRAM([], [[
#ifndef __clang__
       not clang
#endif
]])],
[CLANG=yes], [CLANG=no])
AC_MSG_RESULT([$CLANG])

# This has to come before AC_PROG_LIBTOOL I think
AC_ARG_ENABLE([lto],
	[AS_HELP_STRING([--enable-lto], [enable support for Link Time Optimization])], [])

AS_IF([test "x$enable_lto" == "xyes"], [
	AS_IF([test "x$enable_debug" != "xyes"], [
		AX_APPEND_COMPILE_FLAGS([-O3])
		AX_APPEND_LINK_FLAGS([-O3])
	])
	AS_IF([test "x$enable_shared" != "xno"], [
		AC_MSG_WARN([--enable-shared and --enable-lto do not currently work together])
	])
	AS_IF([test "x$GCC" = "xyes" && test "x$CLANG" != "xyes"], [ # clang pretends to be gcc
		AC_CHECK_TOOL([LD], [gcc], [$LD])
		AC_CHECK_TOOL([AR], [gcc-ar], [$AR])
		AC_CHECK_TOOL([RANLIB], [gcc-ranlib], [$RANLIB])
		AC_CHECK_TOOL([NM], [gcc-nm], [$NM])
		AX_APPEND_COMPILE_FLAGS([-flto=auto])
		AX_APPEND_LINK_FLAGS([-flto=auto])
	])
	AS_IF([test "x$CLANG" = "xyes"], [
		AC_CHECK_TOOL([AR], [llvm-ar], [$AR])
		AC_CHECK_TOOL([RANLIB], [llvm-ranlib], [$RANLIB])
		AX_CHECK_COMPILE_FLAG([-flto=thin], [
			AX_APPEND_COMPILE_FLAGS([-flto=thin])
			AX_APPEND_LINK_FLAGS([-flto=thin])
		], [
			AX_APPEND_COMPILE_FLAGS([-flto])
			AX_APPEND_LINK_FLAGS([-flto])
		])
	])
	# not sure why, but these errors only show up with LTO enabled
	AX_APPEND_LINK_FLAGS([-Wno-inline -Wno-error=inline -Wno-null-dereference -Wno-error=null-dereference])
])

# Checks for programs
AM_PROG_AR
AC_PROG_AWK
AC_PROG_INSTALL
AC_PROG_LEX
AC_PROG_LIBTOOL
AC_PROG_LN_S
AC_PROG_YACC
AC_PATH_PROGS(LCOV, [lcov], no)
AC_PATH_PROGS(RE2C, [re2c], no)

AM_CONDITIONAL([LCOV], [test "x$LCOV" != "xno"])
AM_CONDITIONAL([RE2C], [test "x$RE2C" != "xno"])
AM_CONDITIONAL([DEBUG], [test x$enable_debug = xyes])
AS_IF([test "x$enable_debug" = "xyes"], [
	AC_DEFINE([HANDLEBARS_ENABLE_DEBUG], [1], [Enable handlebars debugging])
])

# Checks for header files
AC_HEADER_STDC

# Checks for functions
AC_FUNC_ALLOCA
AC_CHECK_FUNC(snprintf, , AC_MSG_ERROR([snprintf is required]))

# Checks for typedefs, structures, and compiler characteristics
AC_C_CONST
AC_C_INLINE
AC_C_STRINGIZE
AC_TYPE_SIZE_T
AX_GCC_BUILTIN([__builtin_expect])
AX_GCC_BUILTIN([__builtin_clz])
AX_GCC_BUILTIN([__builtin_clzl])
AX_GCC_BUILTIN([__builtin_clzll])
AX_GCC_VAR_ATTRIBUTE([unused])
AX_GCC_FUNC_ATTRIBUTE(returns_nonnull)
AX_GCC_FUNC_ATTRIBUTE(visibility)
HBS_COMPUTED_GOTOS
HBS_STATEMENT_EXPRESSIONS
AX_COMPILE_CHECK_SIZEOF(size_t)
AX_COMPILE_CHECK_SIZEOF(unsigned)
AX_COMPILE_CHECK_SIZEOF(unsigned long)
AX_COMPILE_CHECK_SIZEOF(unsigned long long)
AX_TLS([with_tls=yes], [with_tls=no])

# Checks for libraries
AC_CHECK_LIB([m], [cos])
AC_CHECK_LIB([rt], [timer_create])

# bats
AC_ARG_ENABLE([bats], [AS_HELP_STRING([--disable-bats], [disable support for bats])], [])
AS_IF([test "x$enable_bats" != "xno"], [
	AC_PATH_PROGS(BATS, [bats])
])
AM_CONDITIONAL([BATS], [test "x$BATS" != "x"])

# benchmark
AC_ARG_ENABLE([benchmark], [AS_HELP_STRING([--enable-benchmark], [enable benchmark])], [])
AM_CONDITIONAL([BENCHMARK], [test "x$enable_benchmark" == "xyes"])

# bison
AC_ARG_VAR([BISONFLAGS], [bison flags])
AS_IF([test "x$BISONFLAGS" == "x"], [BISONFLAGS="-Wall"])

AC_ARG_ENABLE([bison], [AS_HELP_STRING([--disable-bison], [disable support for bison])], [])
AS_IF([test "x$enable_bison" != "xno"], [
	AC_PATH_PROGS(BISON, [bison])
	AX_PROG_BISON([
		AX_PROG_BISON_VERSION([3.0.2], , AC_MSG_ERROR([bison >= 3.0.2 is required]))
		enable_bison=yes
	])
])
AM_CONDITIONAL([BISON], [test "x$BISON" != "x" && test "x$enable_bison" == "xyes"])

# check
AC_ARG_ENABLE([check], [AS_HELP_STRING([--disable-check], [disable support for check])], [])
AS_IF([test "x$enable_check" != "xno"], [
	PKG_CHECK_MODULES(CHECK, [check], [enable_check=yes], [AC_MSG_WARN([check not found])])
])
AS_IF([test "x$enable_check" == "xyes"], [
	AC_DEFINE([HANDLEBARS_HAVE_CHECK], [1], [Use check])
])
AM_CONDITIONAL([CHECK], [test "x$enable_check" == "xyes"])

# code coverage
HBS_CODE_COVERAGE()

# doxygen
AC_ARG_ENABLE([doxygen], [AS_HELP_STRING([--disable-doxygen], [disable support for doxygen])], [])
AS_IF([test "x$enable_doxygen" != "xno"], [
	AC_PATH_PROGS(DOXYGEN, [doxygen])
	AS_IF([test "x$DOXYGEN" != "x"], [
		enable_doxygen=yes
	])
])
AM_CONDITIONAL(DOXYGEN, [test "x$DOXYGEN" != "x"])

# flex
AC_ARG_VAR([FLEXFLAGS], [flex flags])
AS_IF([test "x$FLEXFLAGS" == "x"], [FLEXFLAGS="--noreject --fast --align"])

AC_ARG_ENABLE([flex], [AS_HELP_STRING([--disable-flex], [disable support for flex])], [])
AS_IF([test "x$enable_flex" != "xno"], [
	AC_PATH_PROGS(FLEX, [flex])
	AX_PROG_FLEX([
		AX_PROG_FLEX_VERSION([2.5.35], , AC_MSG_ERROR([flex >= 2.5.35 is required]))
		enable_flex=yes
	])
])
AM_CONDITIONAL([FLEX], [test "x$FLEX" != "x" && test "x$enable_flex" == "xyes"])

# gperf
AC_ARG_VAR([GPERFFLAGS], [gperf flags])
AS_IF([test "x$GPERFFLAGS" == "x"], [GPERFFLAGS="--struct-type --readonly-tables --compare-strncmp --compare-lengths --global-table"])

AC_ARG_ENABLE([gperf], [AS_HELP_STRING([--disable-gperf], [disable support for gperf])], [])
AS_IF([test "x$enable_gperf" != "xno"], [
	AC_PATH_PROGS(GPERF, [gperf])
])
AM_CONDITIONAL([GPERF], [test "x$GPERF" != "x"])

# testing-exports
AC_ARG_ENABLE([testing-exports], [AS_HELP_STRING([--disable-testing-exports], [disable some exports used for testing])], [])
AS_IF([test "x$enable_testing_exports" != "xno"], [
	AC_DEFINE([HANDLEBARS_TESTING_EXPORTS], [1], [Export some symbols used for testing])
	enable_testing_exports=yes
])
AM_CONDITIONAL([TESTING_EXPORTS], [test "x$enable_testing_exports" == "xyes"])

# handlebars-spec
AC_ARG_WITH([handlebars-spec],
	[AS_HELP_STRING([--with-handlebars-spec=SPEC_LOCATION], [location to find handlebars specification @<:@default=./spec/handlebars@>:@])])

AS_IF([test "x$with_handlebars_spec" != "xno"], [
	AS_IF([test "x$with_handlebars_spec" != "x"], [
		AC_CHECK_FILE([$with_handlebars_spec/spec/basic.json], [
			AC_SUBST([HANDLEBARS_SPEC_DIR], [$with_handlebars_spec])
		], [
			AC_MSG_FAILURE([handlebars spec missing (--without-handlebars-spec to disable)])
		])
	], [
		AC_SUBST([HANDLEBARS_SPEC_DIR], ['$(top_srcdir)/spec/handlebars'])
	])
])

# hardening
AC_ARG_ENABLE([hardening], [AS_HELP_STRING([--disable-hardening], [disable hardening])], [])
AS_IF([test "x$enable_hardening" != "xno"], [
	AX_ADD_FORTIFY_SOURCE()
	AX_APPEND_COMPILE_FLAGS([-fstack-protector-strong -fstack-clash-protection], [HARDENING_CFLAGS])
	AX_APPEND_LINK_FLAGS([-fstack-protector-strong -fstack-clash-protection], [HARDENING_LDFLAGS])
	# See: https://github.com/gdnsd/gdnsd/blob/f23273c688ae267d2b6049f0608f7b42193211fe/configure.ac#L84
	AS_IF([test "x$CLANG" != "xyes"], [
		AX_APPEND_COMPILE_FLAGS([-fcf-protection], [HARDENING_CFLAGS])
		AX_APPEND_LINK_FLAGS([-fcf-protection], [HARDENING_LDFLAGS])
	])
	# fpic
	AX_APPEND_COMPILE_FLAGS([-fpic], [HARDENING_CFLAGS])
	AX_APPEND_LINK_FLAGS([-fpic], [HARDENING_LDFLAGS])
	# fpie
	AX_CHECK_COMPILE_FLAG([-fpie], [HARDENING_BIN_CFLAGS="-fpie"])
	AS_IF([test "x$CLANG" != "xyes"], [
		AX_CHECK_LINK_FLAG([-pie], [HARDENING_BIN_LDFLAGS="-pie"])
	], [
		# clang requires -Wl,-pie instead of -pie
		AX_CHECK_LINK_FLAG([-Wl,-pie], [HARDENING_BIN_LDFLAGS="-Wl,-pie"])
	])
	HARDENING_BIN_CFLAGS="$HARDENING_CFLAGS $HARDENING_BIN_CFLAGS"
	HARDENING_BIN_LDFLAGS="$HARDENING_LDFLAGS $HARDENING_BIN_LDFLAGS"
	AC_SUBST([HARDENING_CFLAGS])
	AC_SUBST([HARDENING_LDFLAGS])
	AC_SUBST([HARDENING_BIN_CFLAGS])
	AC_SUBST([HARDENING_BIN_LDFLAGS])
])

# Work around buggy handling in Nix
# https://github.com/NixOS/nixpkgs/issues/18995
AS_IF([test "x$enable_debug" = "xyes"], [AX_APPEND_COMPILE_FLAGS([-U_FORTIFY_SOURCE])])

# json
AC_ARG_ENABLE([json], [AS_HELP_STRING([--disable-json], [disable support for json])], [])
AS_IF([test "x$enable_json" != "xno"], [
	PKG_CHECK_MODULES(JSON, [json-c], [enable_json=yes], [AC_MSG_WARN([json-c not found])])
])
AS_IF([test "x$enable_json" == "xyes"], [
	AC_DEFINE([HANDLEBARS_HAVE_JSON], [1], [Use JSON])
])
AM_CONDITIONAL([JSON], [test "x$enable_json" == "xyes"])

# lmdb
AC_ARG_ENABLE([lmdb], [AS_HELP_STRING([--disable-lmdb], [disable support for LMDB])], [])
AS_IF([test "x$enable_lmdb" != "xno"], [
	PKG_CHECK_MODULES(LMDB, [lmdb], [enable_lmdb=yes], [
		# xenial doesn't have a pkgconfig for lmdb
		AC_CHECK_LIB([lmdb], [mdb_version], [
			enable_lmdb=yes
			AC_SUBST([LMDB_CFLAGS], [])
			AC_SUBST([LMDB_LIBS], [-llmdb])
		], [AC_MSG_WARN([lmdb not found])])
	])
])
AS_IF([test "x$enable_lmdb" == "xyes"], [
	AC_DEFINE([HANDLEBARS_HAVE_LMDB], [1], [Use LMDB])
])
AM_CONDITIONAL([LMDB], [test "x$enable_lmdb" == "xyes"])

# memory
AC_ARG_ENABLE([handlebars-memory],
	[AS_HELP_STRING([--enable-handlebars-memory], [enable handlebars memory testing functions])], [])

AS_IF([test "x$enable_handlebars_memory" == "xyes"], [
    AC_DEFINE([HANDLEBARS_MEMORY], [1], [Enable handlebars memory testing functions])
])
AM_CONDITIONAL([HANDLEBARS_MEMORY], [test x$enable_handlebars_memory = xyes])

# mustache-spec
AC_ARG_WITH([mustache-spec],
	[AS_HELP_STRING([--with-mustache-spec=SPEC_LOCATION], [location to find mustache specification @<:@default=./spec/mustache@>:@])])

AS_IF([test "x$with_mustache_spec" != "xno"], [
	AS_IF([test "x$with_mustache_spec" != "x"], [
		AC_CHECK_FILE([$with_mustache_spec/specs/comments.yml], [
			AC_SUBST([MUSTACHE_SPEC_DIR], [$with_mustache_spec])
		], [
			AC_MSG_FAILURE([mustache spec missing (--without-mustache-spec to disable)])
		])
	], [
		AC_SUBST([MUSTACHE_SPEC_DIR], ['$(top_srcdir)/spec/mustache'])
	])
])

# pcre
AC_ARG_ENABLE([pcre], [AS_HELP_STRING([--disable-pcre], [disable support for pcre])], [])
AS_IF([test "x$enable_pcre" != "xno"], [
	PKG_CHECK_MODULES(PCRE, [libpcre], [enable_pcre=yes], [AC_MSG_WARN([libpcre not found])])
])
AS_IF([test "x$enable_pcre" == "xyes"], [
	AC_DEFINE([HANDLEBARS_HAVE_PCRE], [1], [Use PCRE])
])
AM_CONDITIONAL([PCRE], [test "x$enable_pcre" == "xyes"])

# pthread
AC_ARG_ENABLE([pthread], [AS_HELP_STRING([--disable-pthread], [disable support for pthread])], [])
AS_IF([test "x$enable_pthread" != "xno"], [
	AX_PTHREAD([enable_pthread=yes], [AC_MSG_WARN([pthread not found])])
])
AS_IF([test "x$enable_pthread" == "xyes"], [
	AC_DEFINE([HANDLEBARS_HAVE_PTHREAD], [1], [Use pthread])
])
AM_CONDITIONAL([PTHREAD], [test "x$enable_pthread" == "xyes"])

# refcounting
AC_ARG_ENABLE([refcounting],
	[AS_HELP_STRING([--disable-refcounting], [disable refcounting of handlebars values])], [])

AS_IF([test "x$enable_refcounting" == "xno"], [
    AC_DEFINE([HANDLEBARS_NO_REFCOUNT], [1], [Disable refcounting of handlebars values])
])

# subunit
AC_ARG_ENABLE([subunit], [AS_HELP_STRING([--disable-subunit], [disable support for subunit])], [])
AS_IF([test "x$enable_subunit" != "xno"], [
	PKG_CHECK_MODULES(SUBUNIT, [libsubunit], [enable_subunit=yes], [AC_MSG_WARN([libsubunit not found])])
])
AS_IF([test "x$enable_subunit" == "xyes"], [
	AC_DEFINE([HANDLEBARS_HAVE_SUBUNIT], [1], [Use subunit])
])
AM_CONDITIONAL([SUBUNIT], [test "x$enable_subunit" == "xyes"])

# talloc
PKG_CHECK_MODULES(TALLOC, [talloc], [enable_talloc=yes])

# valgrind
AX_VALGRIND_CHECK
AS_IF([test "x$enable_valgrind" == "xyes"], [
	AC_DEFINE([HANDLEBARS_HAVE_VALGRIND], [1], [Use valgrind])
])

# yaml
AC_ARG_ENABLE([yaml], [AS_HELP_STRING([--disable-yaml], [disable support for yaml])], [])
AS_IF([test "x$enable_yaml" != "xno"], [
	PKG_CHECK_MODULES(YAML, [yaml-0.1], [enable_yaml=yes], [AC_MSG_WARN([libyaml not found])])
])
AS_IF([test "x$enable_yaml" == "xyes"], [
	AC_DEFINE([HANDLEBARS_HAVE_YAML], [1], [Use YAML])
])
AM_CONDITIONAL([YAML], [test "x$enable_yaml" == "xyes"])

# Define Makefiles.
AC_CONFIG_FILES([
  handlebars.pc
  src/Makefile
  Makefile
  bin/Makefile
  doc/Doxyfile
  doc/Makefile
  tests/Makefile
  bench/Makefile
])

# Generate the "configure" script.
AC_OUTPUT
