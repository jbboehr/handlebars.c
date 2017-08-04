
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/../src)
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR}/../)

set(srcdir "${CMAKE_CURRENT_SOURCE_DIR}")

if(WIN32)
    # CMake uses Unix slashes for everything, but the tests that
    # read srcdir expect platform specific slashes. There are two
    # slashes because the shell scripts will consume srcdir.
    string(REPLACE "/" "\\\\" srcdir "${srcdir}")
    set(EXEEXT ".exe")
    set(IS_MSVC "1")
endif(WIN32)

SET(COMMON_TEST_FILES
        adler32.c
        fixtures.c
        utils.c
        utils.h)

# @todo winmm.lib
link_libraries(${LIBS} handlebars)

add_executable(test_ast ${COMMON_TEST_FILES} test_ast.c)
add_executable(test_ast_helpers ${COMMON_TEST_FILES} test_ast_helpers.c)
add_executable(test_ast_list ${COMMON_TEST_FILES} test_ast_list.c)
add_executable(test_cache ${COMMON_TEST_FILES} test_cache.c)
add_executable(test_compiler ${COMMON_TEST_FILES} test_compiler.c)
add_executable(test_main ${COMMON_TEST_FILES} test_main.c)
add_executable(test_map ${COMMON_TEST_FILES} test_map.c)
add_executable(test_opcode_printer ${COMMON_TEST_FILES} test_opcode_printer.c)
add_executable(test_opcodes ${COMMON_TEST_FILES} test_opcodes.c)
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
