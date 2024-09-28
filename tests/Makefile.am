# Copyright (c) anno Domini nostri Jesu Christi MMXVI-MMXXIV John Boehr & contributors
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

EXTRA_DIST = utils.h fixture1.hbs fixture1.json fixture1.yaml fixture2.hbs fixture3.hbs test_executable.bats
AM_CPPFLAGS = $(CODE_COVERAGE_CPPFLAGS) -I$(top_builddir)/src -I$(top_srcdir)/src
AM_CFLAGS = $(WARN_CFLAGS) $(HARDENING_BIN_CFLAGS) $(CODE_COVERAGE_CFLAGS) $(JSON_CFLAGS) $(LMDB_CFLAGS) $(PTHREAD_CFLAGS) $(TALLOC_CFLAGS) $(YAML_CFLAGS) $(CHECK_CFLAGS) $(PCRE_CFLAGS) $(SUBUNIT_CFLAGS)
AM_LDFLAGS = $(WARN_LDFLAGS) $(HARDENING_BIN_LDFLAGS) $(CODE_COVERAGE_LIBS) -static # for valgrind
LDADD = $(top_builddir)/src/libhandlebars.la $(JSON_LIBS) $(LMDB_LIBS) $(PTHREAD_LIBS) $(TALLOC_LIBS) $(YAML_LIBS) $(CHECK_LIBS) $(PCRE_LIBS) $(SUBUNIT_LIBS)

#if CHECK_HAS_TAP
LOG_DRIVER = env AM_TAP_AWK='$(AWK)' $(SHELL) \
                  $(top_srcdir)/build/tap-driver.sh
#else
#LOG_DRIVER = $(TEST_LOG_DRIVER)
#endif

check_PROGRAMS = \
	test_main \
	test_ast \
	test_ast_list \
	test_compiler \
	test_map \
	test_opcode_printer \
	test_opcodes \
	test_stack \
	test_string \
	test_token \
	test_value

COMMONFILES = utils.h utils.c fixtures.c adler32.c

test_main_SOURCES = $(COMMONFILES) test_main.c
test_ast_SOURCES = $(COMMONFILES) test_ast.c
test_ast_list_SOURCES = $(COMMONFILES) test_ast_list.c
test_compiler_SOURCES = $(COMMONFILES) test_compiler.c
test_map_SOURCES = $(COMMONFILES) test_map.c
test_opcode_printer_SOURCES = $(COMMONFILES) test_opcode_printer.c
test_opcodes_SOURCES = $(COMMONFILES) test_opcodes.c
test_stack_SOURCES = $(COMMONFILES) test_stack.c
test_string_SOURCES = $(COMMONFILES) test_string.c
test_token_SOURCES = $(COMMONFILES) test_token.c
test_value_SOURCES = $(COMMONFILES) test_value.c

if TESTING_EXPORTS
test_ast_helpers_SOURCES = $(COMMONFILES) test_ast_helpers.c
test_scanners_SOURCES = $(COMMONFILES) test_scanners.c
test_utils_SOURCES = $(COMMONFILES) test_utils.c
check_PROGRAMS += \
	test_ast_helpers \
	test_scanners \
	test_utils
endif

if JSON
test_cache_SOURCES = $(COMMONFILES) test_cache.c
test_json_SOURCES = $(COMMONFILES) test_json.c
test_partial_loader_SOURCES = $(COMMONFILES) test_partial_loader.c
test_spec_handlebars_parser_SOURCES = $(COMMONFILES) test_spec_handlebars_parser.c
test_spec_handlebars_tokenizer_SOURCES = $(COMMONFILES) test_spec_handlebars_tokenizer.c
test_spec_handlebars_compiler_SOURCES = $(COMMONFILES) test_spec_handlebars_compiler.c
test_spec_handlebars_SOURCES = $(COMMONFILES) test_spec_handlebars.c

check_PROGRAMS += \
	test_cache \
	test_json \
	test_partial_loader \
	test_spec_handlebars_parser \
	test_spec_handlebars_tokenizer \
	test_spec_handlebars_compiler \
	test_spec_handlebars
endif

if YAML
test_spec_mustache_SOURCES = $(COMMONFILES) test_spec_mustache.c
test_yaml_SOURCES = $(COMMONFILES) test_yaml.c

check_PROGRAMS += \
	test_spec_mustache \
	test_yaml
endif

if HANDLEBARS_MEMORY
test_random_alloc_fail_SOURCES = $(COMMONFILES) test_random_alloc_fail.c
check_PROGRAMS += test_random_alloc_fail
endif

TESTS = $(check_PROGRAMS)
AM_TESTS_ENVIRONMENT = \
	top_srcdir=$(top_srcdir) \
	top_builddir=$(top_builddir) \
	handlebars_export_dir=$(HANDLEBARS_SPEC_DIR)/export \
	handlebars_spec_dir=$(HANDLEBARS_SPEC_DIR)/spec \
	handlebars_tokenizer_spec=$(HANDLEBARS_SPEC_DIR)/spec/tokenizer.json \
	handlebars_parser_spec=$(HANDLEBARS_SPEC_DIR)/spec/parser.json \
	mustache_spec_dir=$(MUSTACHE_SPEC_DIR)/specs \
	CK_VERBOSITY=normal \
	HANDLEBARSC=$(top_builddir)/bin/handlebarsc \
	TEST_DIR=$(top_srcdir)/tests \
	BENCH_DIR=$(top_srcdir)/bench

if BATS
TESTS += test_executable.bats
endif

#if CHECK_HAS_TAP
AM_TESTS_ENVIRONMENT += CK_TAP_LOG_FILE_NAME=-
#endif

if VALGRIND_ENABLED
@VALGRIND_CHECK_RULES@
endif
