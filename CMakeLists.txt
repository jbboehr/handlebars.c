cmake_minimum_required(VERSION 3.9)
project(handlebars C)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(HANDLEBARS_MAJOR 0)
set(HANDLEBARS_MINOR 6)
set(HANDLEBARS_PATCH 4)

option(HANDLEBARS_ENABLE_TESTS
        "Enable the compilation and running of unit tests" ON)

add_subdirectory(src)
add_subdirectory(bin)

# Unit tests
if (HANDLEBARS_ENABLE_TESTS)
    add_subdirectory(tests)
    enable_testing()
endif()
