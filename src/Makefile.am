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

AM_CPPFLAGS = $(CODE_COVERAGE_CPPFLAGS) -I$(top_srcdir)/vendor/sort_r -I$(top_srcdir)/vendor/xxhash
AM_CFLAGS = $(WARN_CFLAGS) $(HARDENING_CFLAGS) $(CODE_COVERAGE_CFLAGS) $(JSON_CFLAGS) $(LMDB_CFLAGS) $(PTHREAD_CFLAGS) $(TALLOC_CFLAGS) $(VALGRIND_CFLAGS) $(YAML_CFLAGS)
AM_LDFLAGS = $(WARN_LDFLAGS) $(HARDENING_LDFLAGS) $(CODE_COVERAGE_LIBS)
LIBADD = $(JSON_LIBS) $(LMDB_LIBS) $(PTHREAD_LIBS) $(TALLOC_LIBS) $(VALGRIND_LIBS) $(YAML_LIBS)

EXTRA_DIST = handlebars.l handlebars.y handlebars_scanners.re handlebars_helpers_ht.gperf
GENSOURCE = handlebars_config.h
GENSOURCE2 =

if BISON
handlebars.tab.h: handlebars.tab.c
handlebars.tab.c: handlebars.lex.c handlebars.y
	${BISON} ${BISONFLAGS} -b handlebars handlebars.y
GENSOURCE += handlebars.tab.h handlebars.tab.c
endif

if FLEX
handlebars.lex.h: handlebars.lex.c
handlebars.lex.c: handlebars.l
	${FLEX} ${FLEXFLAGS} -o handlebars.lex.c handlebars.l
GENSOURCE += handlebars.lex.h handlebars.lex.c
endif

if GPERF
handlebars_helpers_ht.h: handlebars_helpers_ht.gperf
	$(GPERF) ${GPERFFLAGS} --output-file=handlebars_helpers_ht.h handlebars_helpers_ht.gperf
GENSOURCE2 += handlebars_helpers_ht.h
endif

if RE2C
handlebars_scanners.c: handlebars_scanners.re
	$(RE2C) ${RE2CFLAGS} -o handlebars_scanners.c handlebars_scanners.re
GENSOURCE2 += handlebars_scanners.c
endif

if LMDB
LMDBSOURCES = handlebars_cache_lmdb.c
else
LMDBSOURCES =
endif

if PTHREAD
PTHREADSOURCES = handlebars_cache_mmap.c
else
PTHREADSOURCES =
endif

if JSON
JSONSOURCES = $(JSON_HEADERS) handlebars_json.c
else
JSONSOURCES =
endif

if YAML
YAMLSOURCES = $(YAML_HEADERS) handlebars_yaml.c
else
YAMLSOURCES =
endif

# Main
pkginclude_HEADERS = \
	handlebars.h \
	handlebars.lex.h \
	handlebars.tab.h \
	handlebars_ast.h \
	handlebars_ast_list.h \
	handlebars_ast_printer.h \
	handlebars_cache.h \
	handlebars_closure.h \
	handlebars_compiler.h \
	handlebars_config.h \
	handlebars_delimiters.h \
	handlebars_helpers.h \
	handlebars_json.h \
	handlebars_map.h \
	handlebars_memory.h \
	handlebars_module_printer.h \
	handlebars_opcode_printer.h \
	handlebars_opcode_serializer.h \
	handlebars_opcodes.h \
	handlebars_parser.h \
	handlebars_partial_loader.h \
	handlebars_ptr.h \
	handlebars_rc.h \
	handlebars_stack.h \
	handlebars_string.h \
	handlebars_token.h \
	handlebars_types.h \
	handlebars_value.h \
	handlebars_value_handlers.h \
	handlebars_vm.h \
	handlebars_yaml.h

# these headers are not (currently) installed because they only contain private symbols:
noinst_HEADERS = \
	handlebars_ast_helpers.h \
	handlebars_cache_private.h \
	handlebars_helpers_ht.h \
	handlebars_parser_private.h \
	handlebars_private.h \
	handlebars_scanners.h \
	handlebars_whitespace.h \
	handlebars_value_private.h \
	handlebars_vm_private.h

CLEANFILES = $(GENSOURCE) $(GENSOURCE2)
BUILT_SOURCES = $(GENSOURCE) $(GENSOURCE2)

lib_LTLIBRARIES=libhandlebars.la

libhandlebars_la_SOURCES = \
	handlebars.tab.h \
	handlebars.tab.c \
	handlebars.lex.h \
	handlebars.lex.c \
	handlebars_helpers_ht.h \
	handlebars_memory.h \
	handlebars.h \
	handlebars.c \
	handlebars_ast.h \
	handlebars_ast.c \
	handlebars_ast_helpers.h \
	handlebars_ast_helpers.c \
	handlebars_ast_list.h \
	handlebars_ast_list.c \
	handlebars_ast_printer.h \
	handlebars_ast_printer.c \
	handlebars_cache.h \
	handlebars_cache.c \
	$(LMDBSOURCES) \
	$(PTHREADSOURCES) \
	handlebars_cache_simple.c \
	handlebars_closure.c \
	handlebars_closure.h \
	handlebars_compiler.h \
	handlebars_compiler.c \
	handlebars_delimiters.c \
	handlebars_delimiters.h \
	handlebars_helpers.h \
	handlebars_helpers.c \
	$(JSONSOURCES) \
	handlebars_map.h \
	handlebars_map.c \
	handlebars_module_printer.h \
	handlebars_module_printer.c \
	handlebars_opcode_printer.h \
	handlebars_opcode_printer.c \
	handlebars_opcode_serializer.h \
	handlebars_opcode_serializer.c \
	handlebars_opcodes.h \
	handlebars_opcodes.c \
	handlebars_parser.h \
	handlebars_parser.c \
	handlebars_parser_private.h \
	handlebars_parser_private.c \
	handlebars_partial_loader.h \
	handlebars_partial_loader.c \
	handlebars_private.h \
	handlebars_ptr.h \
	handlebars_ptr.c \
	handlebars_rc.c \
	handlebars_rc.h \
	handlebars_scanners.c \
	handlebars_scanners.h \
	handlebars_stack.h \
	handlebars_stack.c \
	handlebars_string.h \
	handlebars_string.c \
	handlebars_token.h \
	handlebars_token.c \
	handlebars_value.h \
	handlebars_value.c \
	handlebars_value_handlers.h \
	handlebars_value_handlers.c \
	handlebars_vm.h \
	handlebars_vm.c \
	handlebars_whitespace.h \
	handlebars_whitespace.c \
	$(YAMLSOURCES)

if HANDLEBARS_MEMORY
libhandlebars_la_SOURCES += handlebars_memory.c
endif

libhandlebars_la_LIBADD = $(LIBADD)

# https://www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html
libhandlebars_la_LDFLAGS = \
	-no-undefined \
	-version-info $(HANDLEBARS_LT_CURRENT):$(HANDLEBARS_LT_REVISION):$(HANDLEBARS_LT_AGE)

