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

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/..)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/../src)

SET(COMMON_TEST_FILES
        adler32.c
        fixtures.c
        utils.c
        utils.h)

link_libraries(${LIBS} handlebars_static)

add_executable(test_ast ${COMMON_TEST_FILES} test_ast.c)
add_executable(test_ast_helpers ${COMMON_TEST_FILES} test_ast_helpers.c)
add_executable(test_ast_list ${COMMON_TEST_FILES} test_ast_list.c)
add_executable(test_cache ${COMMON_TEST_FILES} test_cache.c)
add_executable(test_compiler ${COMMON_TEST_FILES} test_compiler.c)
add_executable(test_main ${COMMON_TEST_FILES} test_main.c)
add_executable(test_json ${COMMON_TEST_FILES} test_json.c)
add_executable(test_map ${COMMON_TEST_FILES} test_map.c)
add_executable(test_opcode_printer ${COMMON_TEST_FILES} test_opcode_printer.c)
add_executable(test_opcodes ${COMMON_TEST_FILES} test_opcodes.c)
# @TODO FIXME broken because test files are in the wrong path
#add_executable(test_partial_loader ${COMMON_TEST_FILES} test_partial_loader.c)
#add_executable(test_random_alloc_fail ${COMMON_TEST_FILES} test_random_alloc_fail.c)
add_executable(test_scanners ${COMMON_TEST_FILES} test_scanners.c)
add_executable(test_spec_handlebars ${COMMON_TEST_FILES} test_spec_handlebars.c)
add_executable(test_spec_handlebars_compiler ${COMMON_TEST_FILES} test_spec_handlebars_compiler.c)
add_executable(test_spec_handlebars_parser ${COMMON_TEST_FILES} test_spec_handlebars_parser.c)
add_executable(test_spec_handlebars_tokenizer ${COMMON_TEST_FILES} test_spec_handlebars_tokenizer.c)
add_executable(test_spec_mustache ${COMMON_TEST_FILES} test_spec_mustache.c)
add_executable(test_string ${COMMON_TEST_FILES} test_string.c)
add_executable(test_token ${COMMON_TEST_FILES} test_token.c)
add_executable(test_utils ${COMMON_TEST_FILES} test_utils.c)
add_executable(test_value ${COMMON_TEST_FILES} test_value.c)
add_executable(test_yaml ${COMMON_TEST_FILES} test_yaml.c)
