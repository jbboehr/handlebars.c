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

# Keep these versions and the installed-consumer expectations in sync with configure.ac.
set(HANDLEBARS_VERSION_MAJOR 1)
set(HANDLEBARS_VERSION_MINOR 0)
set(HANDLEBARS_VERSION_PATCH 0)
set(HANDLEBARS_SPEC_VERSION "4.7.7")
set(MUSTACHE_SPEC_VERSION "1.1.3")

add_definitions(-DHANDLEBARS_VERSION_MAJOR=${HANDLEBARS_VERSION_MAJOR})
add_definitions(-DHANDLEBARS_VERSION_MINOR=${HANDLEBARS_VERSION_MINOR})
add_definitions(-DHANDLEBARS_VERSION_PATCH=${HANDLEBARS_VERSION_PATCH})
add_definitions(-DHANDLEBARS_VERSION_STRING="${HANDLEBARS_VERSION_MAJOR}.${HANDLEBARS_VERSION_MINOR}.${HANDLEBARS_VERSION_PATCH}")
add_definitions(-DHANDLEBARS_SPEC_VERSION_STRING="${HANDLEBARS_SPEC_VERSION}")
add_definitions(-DMUSTACHE_SPEC_VERSION_STRING="${MUSTACHE_SPEC_VERSION}")

option(HANDLEBARS_ENABLE_TESTS
        "Enable the compilation and running of unit tests" ON)
option(HANDLEBARS_ENABLE_FUZZING
        "Build the libFuzzer targets" OFF)
option(HANDLEBARS_ENABLE_MEMORY
        "Enable memory allocation failure testing functions" OFF)

set(HANDLEBARS_MEMORY ${HANDLEBARS_ENABLE_MEMORY})
set(HANDLEBARS_TESTING_EXPORTS 1)
foreach(feature JSON LMDB PCRE PTHREAD YAML)
    set(HANDLEBARS_HAVE_${feature} 0)
endforeach()

if(HANDLEBARS_ENABLE_FUZZING)
    if(NOT CMAKE_C_COMPILER_ID MATCHES "Clang")
        message(FATAL_ERROR "HANDLEBARS_ENABLE_FUZZING requires Clang")
    endif()
    add_compile_options(-fno-omit-frame-pointer -fsanitize=fuzzer-no-link,address,undefined)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address,undefined")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=address,undefined")
endif()

if(WIN32)
    add_definitions(-DYY_NO_UNISTD_H=1)
    add_definitions(-DYAML_DECLARE_STATIC=1 -DPCRE2_STATIC=1)

    find_package(Libcompat REQUIRED)
    set(LIBS ${LIBS} ${LIBCOMPAT_LIBRARIES})
ENDIF(WIN32)

if(HANDLEBARS_ENABLE_TESTS)
    find_package(Check REQUIRED)
    find_program(BATS_EXECUTABLE NAMES bats)
    include_directories(${CHECK_INCLUDE_DIRS})
    set(LIBS ${LIBS} ${CHECK_LIBRARIES})
endif()

find_package(Libjsonc)
include_directories(${LIBJSONC_INCLUDE_DIR} ${LIBJSONC_INCLUDE_DIR}/json-c ${LIBJSONC_INCLUDE_DIR}/json)
set(LIBS ${LIBS} ${LIBJSONC_LIBRARY})
if(LIBJSONC_LIBRARY)
    set(HANDLEBARS_HAVE_JSON 1)
endif()

find_package(LMDB)
include_directories(${LMDB_INCLUDE_DIR})
set(LIBS ${LIBS} ${LMDB_LIBRARIES})
if(LMDB_LIBRARIES)
    set(HANDLEBARS_HAVE_LMDB 1)
endif()

if(HANDLEBARS_ENABLE_TESTS)
    find_package(PCRE2 REQUIRED)
    include_directories(${PCRE2_INCLUDE_DIRS})
    set(LIBS ${LIBS} ${PCRE2_LIBRARIES})
    add_definitions(-DPCRE2_CODE_UNIT_WIDTH=8)
    set(HANDLEBARS_HAVE_PCRE 1)
endif()

find_package(Talloc REQUIRED)
include_directories(${TALLOC_INCLUDE_DIRS})
set(LIBS ${LIBS} ${TALLOC_LIBRARIES})

find_package(Threads)
set(LIBS ${LIBS} ${CMAKE_THREAD_LIBS_INIT})
if(CMAKE_USE_PTHREADS_INIT)
    set(HANDLEBARS_HAVE_PTHREAD 1)
endif()

find_package(LibYaml)
include_directories(${LIBYAML_INCLUDE_DIRS})
set(LIBS ${LIBS} ${LIBYAML_LIBRARIES})
if(LIBYAML_LIBRARIES)
    set(HANDLEBARS_HAVE_YAML 1)
endif()

# These definitions are needed before some source files include handlebars.h.
# Use the same resolved values when configuring the installed header below.
foreach(feature HANDLEBARS_MEMORY HANDLEBARS_TESTING_EXPORTS
        HANDLEBARS_HAVE_JSON HANDLEBARS_HAVE_LMDB HANDLEBARS_HAVE_PCRE
        HANDLEBARS_HAVE_PTHREAD HANDLEBARS_HAVE_YAML)
    if(${feature})
        add_definitions(-D${feature}=1)
    endif()
endforeach()

if(NOT MSVC)
    check_symbol_exists(pow math.h HAVE_POW)
    if(NOT HAVE_POW)
        set(LIBS ${LIBS} m)
        check_symbol_exists(pow math.h HAVE_POW)
    endif()
endif()

if(NOT WIN32)
    check_symbol_exists(posix_spawn "spawn.h" HANDLEBARSC_HAVE_POSIX_SPAWN)
    check_symbol_exists(posix_spawnp "spawn.h" HANDLEBARSC_HAVE_POSIX_SPAWNP)
    check_symbol_exists(posix_spawnattr_setpgroup "spawn.h" HANDLEBARSC_HAVE_POSIX_SPAWNATTR_SETPGROUP)
    check_symbol_exists(POSIX_SPAWN_SETPGROUP "spawn.h" HANDLEBARSC_HAVE_POSIX_SPAWN_SETPGROUP)
    check_symbol_exists(poll "poll.h" HANDLEBARSC_HAVE_POLL)
    check_symbol_exists(clock_gettime "time.h" HANDLEBARSC_HAVE_CLOCK_GETTIME)
    if(NOT HANDLEBARSC_HAVE_CLOCK_GETTIME)
        set(HANDLEBARSC_SAVED_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES})
        list(APPEND CMAKE_REQUIRED_LIBRARIES rt)
        check_symbol_exists(clock_gettime "time.h" HANDLEBARSC_HAVE_CLOCK_GETTIME_IN_RT)
        set(CMAKE_REQUIRED_LIBRARIES ${HANDLEBARSC_SAVED_REQUIRED_LIBRARIES})
        unset(HANDLEBARSC_SAVED_REQUIRED_LIBRARIES)
        if(HANDLEBARSC_HAVE_CLOCK_GETTIME_IN_RT)
            set(HANDLEBARSC_HAVE_CLOCK_GETTIME 1)
            list(APPEND LIBS rt)
        endif()
    endif()
    check_symbol_exists(waitpid "sys/wait.h" HANDLEBARSC_HAVE_WAITPID)
    if(HANDLEBARSC_HAVE_POSIX_SPAWN
            AND HANDLEBARSC_HAVE_POSIX_SPAWNP
            AND HANDLEBARSC_HAVE_POSIX_SPAWNATTR_SETPGROUP
            AND HANDLEBARSC_HAVE_POSIX_SPAWN_SETPGROUP
            AND HANDLEBARSC_HAVE_POLL
            AND HANDLEBARSC_HAVE_CLOCK_GETTIME
            AND HANDLEBARSC_HAVE_WAITPID)
        add_definitions(-DHANDLEBARSC_HAVE_EXEC_HELPERS=1)
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
if(HANDLEBARS_ENABLE_FUZZING)
    add_subdirectory(fuzz)
endif()

# Unit tests
if (HANDLEBARS_ENABLE_TESTS)
    include(CheckLanguage)
    check_language(CXX)
    if(CMAKE_CXX_COMPILER)
        enable_language(CXX)
    endif()
    function(handlebars_resolve_test_path output value fallback)
        set(path "${value}")
        if(path)
            get_filename_component(path "${path}" ABSOLUTE BASE_DIR "${CMAKE_SOURCE_DIR}")
        endif()
        if(NOT EXISTS "${path}")
            set(path "${fallback}")
        endif()
        set(${output} "${path}" PARENT_SCOPE)
    endfunction()
    handlebars_resolve_test_path(HANDLEBARS_EXPORT_DIR
        "$ENV{handlebars_export_dir}" "${CMAKE_SOURCE_DIR}/spec/handlebars/export")
    handlebars_resolve_test_path(HANDLEBARS_SPEC_DIR
        "$ENV{handlebars_spec_dir}" "${CMAKE_SOURCE_DIR}/spec/handlebars/spec")
    handlebars_resolve_test_path(HANDLEBARS_TOKENIZER_SPEC
        "$ENV{handlebars_tokenizer_spec}" "${CMAKE_SOURCE_DIR}/spec/handlebars/spec/tokenizer.json")
    handlebars_resolve_test_path(HANDLEBARS_PARSER_SPEC
        "$ENV{handlebars_parser_spec}" "${CMAKE_SOURCE_DIR}/spec/handlebars/spec/parser.json")
    handlebars_resolve_test_path(MUSTACHE_SPEC_DIR
        "$ENV{mustache_spec_dir}" "${CMAKE_SOURCE_DIR}/spec/mustache/specs")
    add_subdirectory(tests)
    enable_testing()
    add_test(NAME test_ast COMMAND test_ast)
    add_test(NAME test_ast_helpers COMMAND test_ast_helpers)
    add_test(NAME test_ast_list COMMAND test_ast_list)
    add_test(NAME test_cache COMMAND test_cache)
    add_test(NAME test_compiler COMMAND test_compiler)
    add_test(NAME test_json COMMAND test_json)
    add_test(NAME test_main COMMAND test_main)
    add_test(NAME test_map COMMAND test_map)
    add_test(NAME test_opcode_printer COMMAND test_opcode_printer)
    add_test(NAME test_opcodes COMMAND test_opcodes)
    add_test(NAME test_partial_loader COMMAND test_partial_loader)
    set_tests_properties(test_partial_loader PROPERTIES
        ENVIRONMENT "top_srcdir=${CMAKE_SOURCE_DIR}")
    if(HANDLEBARS_ENABLE_MEMORY AND LIBJSONC_LIBRARY)
        add_test(NAME test_random_alloc_fail COMMAND test_random_alloc_fail)
    elseif(HANDLEBARS_ENABLE_MEMORY)
        message(STATUS "json-c not found; the random allocation failure test will not run")
    endif()
    add_test(NAME test_scanners COMMAND test_scanners)
    add_test(NAME test_spec_handlebars COMMAND test_spec_handlebars "${HANDLEBARS_SPEC_DIR}")
    add_test(NAME test_spec_handlebars_compiler COMMAND test_spec_handlebars_compiler "${HANDLEBARS_EXPORT_DIR}")
    add_test(NAME test_spec_handlebars_parser COMMAND test_spec_handlebars_parser "${HANDLEBARS_PARSER_SPEC}")
    add_test(NAME test_spec_handlebars_tokenizer COMMAND test_spec_handlebars_tokenizer "${HANDLEBARS_TOKENIZER_SPEC}")
    add_test(NAME test_spec_mustache COMMAND test_spec_mustache "${MUSTACHE_SPEC_DIR}")
    set_tests_properties(test_spec_handlebars PROPERTIES
        ENVIRONMENT "handlebars_spec_dir=${HANDLEBARS_SPEC_DIR}")
    set_tests_properties(test_spec_handlebars_compiler PROPERTIES
        ENVIRONMENT "handlebars_export_dir=${HANDLEBARS_EXPORT_DIR}")
    set_tests_properties(test_spec_handlebars_parser PROPERTIES
        ENVIRONMENT "handlebars_parser_spec=${HANDLEBARS_PARSER_SPEC}")
    set_tests_properties(test_spec_handlebars_tokenizer PROPERTIES
        ENVIRONMENT "handlebars_tokenizer_spec=${HANDLEBARS_TOKENIZER_SPEC}")
    set_tests_properties(test_spec_mustache PROPERTIES
        ENVIRONMENT "mustache_spec_dir=${MUSTACHE_SPEC_DIR}")
    add_test(NAME test_stack COMMAND test_stack)
    add_test(NAME test_string COMMAND test_string)
    add_test(NAME test_token COMMAND test_token)
    add_test(NAME test_utils COMMAND test_utils)
    add_test(NAME test_value COMMAND test_value)
    add_test(NAME test_value_api COMMAND test_value_api)
    if(UNIX)
        add_test(NAME test_installed_config
            COMMAND ${CMAKE_COMMAND}
                "-DSOURCE_DIR=${CMAKE_SOURCE_DIR}"
                "-DBUILD_DIR=${CMAKE_BINARY_DIR}"
                "-DINCLUDEDIR=${CMAKE_INSTALL_INCLUDEDIR}"
                "-DLIBDIR=${CMAKE_INSTALL_LIBDIR}"
                "-DGENERATOR=${CMAKE_GENERATOR}"
                "-DCC=${CMAKE_C_COMPILER}"
                "-DCFLAGS=${CMAKE_C_FLAGS}"
                "-DLDFLAGS=${CMAKE_EXE_LINKER_FLAGS}"
                "-DCTEST=${CMAKE_CTEST_COMMAND}"
                "-DCONFIG=$<CONFIG>"
                "-DEXPECT_JSON=$<BOOL:${LIBJSONC_LIBRARY}>"
                "-DEXPECT_LMDB=$<BOOL:${LMDB_LIBRARIES}>"
                "-DEXPECT_PCRE=$<BOOL:${PCRE2_FOUND}>"
                "-DEXPECT_PTHREAD=$<BOOL:${CMAKE_USE_PTHREADS_INIT}>"
                "-DEXPECT_YAML=$<BOOL:${LIBYAML_LIBRARIES}>"
                "-DEXPECT_MEMORY=$<BOOL:${HANDLEBARS_ENABLE_MEMORY}>"
                "-DEXPECT_TESTING_EXPORTS=1"
                -P "${CMAKE_SOURCE_DIR}/tests/installed_config/run.cmake")
    endif()
    if(UNIX AND CMAKE_CXX_COMPILER)
        set(HANDLEBARS_HEADER_CPPFLAGS "")
        foreach(HANDLEBARS_HEADER_DEFINITION ${TALLOC_DEFINITIONS})
            set(HANDLEBARS_HEADER_CPPFLAGS
                "${HANDLEBARS_HEADER_CPPFLAGS} ${HANDLEBARS_HEADER_DEFINITION}")
        endforeach()
        foreach(HANDLEBARS_HEADER_INCLUDE_DIR ${TALLOC_INCLUDE_DIRS})
            set(HANDLEBARS_HEADER_CPPFLAGS
                "${HANDLEBARS_HEADER_CPPFLAGS} -I${HANDLEBARS_HEADER_INCLUDE_DIR}")
        endforeach()
        add_test(NAME test_headers
            COMMAND ${CMAKE_COMMAND} -E env
                "HEADER_SOURCE_DIR=${CMAKE_SOURCE_DIR}/src"
                "HEADER_BUILD_DIR=${CMAKE_BINARY_DIR}/src"
                "HEADER_PROBE=${CMAKE_SOURCE_DIR}/tests/header_compile_probe.c"
                "HEADER_CC=${CMAKE_C_COMPILER}"
                "HEADER_CXX=${CMAKE_CXX_COMPILER}"
                "HEADER_CPPFLAGS=${HANDLEBARS_HEADER_CPPFLAGS}"
                "HEADER_CFLAGS=${CMAKE_C_FLAGS}"
                "HEADER_CXXFLAGS=${CMAKE_CXX_FLAGS}"
                sh "${CMAKE_SOURCE_DIR}/tests/test_headers.sh")
    endif()
    add_test(NAME test_yaml COMMAND test_yaml)
    if(BATS_EXECUTABLE)
        add_test(NAME test_executable
            COMMAND ${CMAKE_COMMAND} -E env
                "HANDLEBARSC=$<TARGET_FILE:handlebarsc>"
                "TEST_DIR=${CMAKE_SOURCE_DIR}/tests"
                "BENCH_DIR=${CMAKE_SOURCE_DIR}/bench"
                "${BATS_EXECUTABLE}"
                "${CMAKE_SOURCE_DIR}/tests/test_executable.bats")
    else()
        message(STATUS "bats not found; the handlebarsc CLI test suite will not run")
    endif()
endif()
