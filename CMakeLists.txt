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

cmake_minimum_required(VERSION 3.5)
project(handlebars C)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" "${CMAKE_MODULE_PATH}")
include(GNUInstallDirs)
include(CheckSymbolExists)
include(Utils)

set(HANDLEBARS_VERSION_MAJOR 0)
set(HANDLEBARS_VERSION_MINOR 7)
set(HANDLEBARS_VERSION_PATCH 2)

add_definitions(-DHANDLEBARS_VERSION_MAJOR=${HANDLEBARS_VERSION_MAJOR})
add_definitions(-DHANDLEBARS_VERSION_MINOR=${HANDLEBARS_VERSION_MINOR})
add_definitions(-DHANDLEBARS_VERSION_PATCH=${HANDLEBARS_VERSION_PATCH})
add_definitions(-DHANDLEBARS_VERSION_STRING="${HANDLEBARS_VERSION_MAJOR}.${HANDLEBARS_VERSION_MINOR}.${HANDLEBARS_VERSION_PATCH}")
add_definitions(-DHANDLEBARS_SPEC_VERSION_STRING="4.7.6")
add_definitions(-DMUSTACHE_SPEC_VERSION_STRING="1.1.3")
add_definitions(-DHANDLEBARS_TESTING_EXPORTS="1")

option(HANDLEBARS_ENABLE_TESTS
        "Enable the compilation and running of unit tests" ON)

if(WIN32)
    add_definitions(-DYY_NO_UNISTD_H=1)
    add_definitions(-DYAML_DECLARE_STATIC=1 -DPCRE_STATIC=1)

    find_package(Libcompat REQUIRED)
    set(LIBS ${LIBS} ${LIBCOMPAT_LIBRARIES})
ENDIF(WIN32)

find_package(Check REQUIRED)
include_directories(${CHECK_INCLUDE_DIRS})
set(LIBS ${LIBS} ${CHECK_LIBRARIES})

find_package(Libjsonc)
include_directories(${LIBJSONC_INCLUDE_DIR} ${LIBJSONC_INCLUDE_DIR}/json-c ${LIBJSONC_INCLUDE_DIR}/json)
set(LIBS ${LIBS} ${LIBJSONC_LIBRARY})
if(LIBJSONC_LIBRARY)
    add_definitions(-DHANDLEBARS_HAVE_JSON)
endif()

find_package(LMDB)
include_directories(${LMDB_INCLUDE_DIR})
set(LIBS ${LIBS} ${LMDB_LIBRARIES})
if(LMDB_LIBRARIES)
    add_definitions(-DHANDLEBARS_HAVE_LMDB)
endif()

find_package(PCRE REQUIRED)
include_directories(${PCRE_INCLUDE_DIRS})
set(LIBS ${LIBS} ${PCRE_LIBRARIES})

find_package(Talloc REQUIRED)
include_directories(${TALLOC_INCLUDE_DIRS})
set(LIBS ${LIBS} ${TALLOC_LIBRARIES})

find_package(Threads)
set(LIBS ${LIBS} ${CMAKE_THREAD_LIBS_INIT})
if(CMAKE_USE_PTHREADS_INIT)
    add_definitions(-DHANDLEBARS_HAVE_PTHREAD)
endif()

find_package(LibYaml)
include_directories(${LIBYAML_INCLUDE_DIRS})
set(LIBS ${LIBS} ${LIBYAML_LIBRARIES})
if(LIBYAML_LIBRARIES)
    add_definitions(-DHANDLEBARS_HAVE_YAML)
endif()

if(NOT MSVC)
    check_symbol_exists(pow math.h HAVE_POW)
    if(NOT HAVE_POW)
        set(LIBS ${LIBS} m)
        check_symbol_exists(pow math.h HAVE_POW)
    endif()
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/vendor/sort_r ${CMAKE_CURRENT_SOURCE_DIR}/vendor/xxhash)

# generate config.h
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/cmake/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/src/config.h)
add_definitions(-DHAVE_CONFIG_H)

# generate handlebars_config.h
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/cmake/handlebars_config.h.in ${CMAKE_CURRENT_BINARY_DIR}/src/handlebars_config.h)

add_subdirectory(src)
add_subdirectory(bin)

# Unit tests
if (HANDLEBARS_ENABLE_TESTS)
    set(ENV{handlebars_export_dir} "${CMAKE_SOURCE_DIR}/spec/handlebars/export")
    set(ENV{handlebars_spec_dir} "${CMAKE_SOURCE_DIR}/spec/handlebars/spec")
    set(ENV{handlebars_tokenizer_spec} "${CMAKE_SOURCE_DIR}/spec/handlebars/spec/tokenizer.json")
    set(ENV{handlebars_parser_spec} "${CMAKE_SOURCE_DIR}/spec/handlebars/spec/parser.json")
    set(ENV{mustache_spec_dir} "${CMAKE_SOURCE_DIR}/spec/mustache/specs")
    add_subdirectory(tests)
    enable_testing()
    add_test(NAME test_ast COMMAND tests/test_ast)
    add_test(NAME test_ast_helpers COMMAND tests/test_ast_helpers)
    add_test(NAME test_ast_list COMMAND tests/test_ast_list)
    add_test(NAME test_cache COMMAND tests/test_cache)
    add_test(NAME test_compiler COMMAND tests/test_compiler)
    add_test(NAME test_json COMMAND tests/test_json)
    add_test(NAME test_main COMMAND tests/test_main)
    add_test(NAME test_map COMMAND tests/test_map)
    add_test(NAME test_opcode_printer COMMAND tests/test_opcode_printer)
    add_test(NAME test_opcodes COMMAND tests/test_opcodes)
    # @TODO FIXME broken because test files are in the wrong path
    #add_test(NAME test_partial_loader COMMAND tests/test_partial_loader)
    add_test(NAME test_scanners COMMAND tests/test_scanners)
    add_test(NAME test_spec_handlebars COMMAND tests/test_spec_handlebars $ENV{handlebars_spec_dir})
    add_test(NAME test_spec_handlebars_compiler COMMAND tests/test_spec_handlebars_compiler $ENV{handlebars_export_dir})
    add_test(NAME test_spec_handlebars_parser COMMAND tests/test_spec_handlebars_parser $ENV{handlebars_parser_spec})
    add_test(NAME test_spec_handlebars_tokenizer COMMAND tests/test_spec_handlebars_tokenizer $ENV{handlebars_tokenizer_spec})
    add_test(NAME test_spec_mustache COMMAND tests/test_spec_mustache $ENV{mustache_spec_dir})
    add_test(NAME test_string COMMAND tests/test_string)
    add_test(NAME test_token COMMAND tests/test_token)
    add_test(NAME test_utils COMMAND tests/test_utils)
    add_test(NAME test_value COMMAND tests/test_value)
    add_test(NAME test_yaml COMMAND tests/test_yaml)
endif()
