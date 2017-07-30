cmake_minimum_required(VERSION 3.9)
project(handlebars C)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(HANDLEBARS_VERSION_MAJOR 0)
set(HANDLEBARS_VERSION_MINOR 6)
set(HANDLEBARS_VERSION_PATCH 4)

add_definitions(-DHANDLEBARS_VERSION_MAJOR=${HANDLEBARS_VERSION_MAJOR})
add_definitions(-DHANDLEBARS_VERSION_MINOR=${HANDLEBARS_VERSION_MINOR})
add_definitions(-DHANDLEBARS_VERSION_PATCH=${HANDLEBARS_VERSION_PATCH})
add_definitions(-DHANDLEBARS_VERSION_STRING="${HANDLEBARS_VERSION_MAJOR}.${HANDLEBARS_VERSION_MINOR}.${HANDLEBARS_VERSION_PATCH}")
add_definitions(-DHANDLEBARS_SPEC_VERSION_STRING="4.0.5")
add_definitions(-DMUSTACHE_SPEC_VERSION_STRING="1.1.3")
add_definitions(-DYY_NO_UNISTD_H=1)

option(HANDLEBARS_ENABLE_TESTS
        "Enable the compilation and running of unit tests" ON)

find_library(LIBCHECK
        NAME check
        HINTS ${CMAKE_CURRENT_SOURCE_DIR}/../output/lib)

find_library(LIBJSONC
        NAME json-c
        HINTS ${CMAKE_CURRENT_SOURCE_DIR}/../output/lib)

find_library(LIBPCRE
        NAME pcreposixd
        HINTS ${CMAKE_CURRENT_SOURCE_DIR}/../output/lib)

find_library(LIBTALLOC
        NAME talloc
        HINTS ${CMAKE_CURRENT_SOURCE_DIR}/../output/lib)

find_library(LIBYAML
        NAME yaml
        HINTS ${CMAKE_CURRENT_SOURCE_DIR}/../output/lib)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../output/include)

add_subdirectory(src)
add_subdirectory(bin)

# Unit tests
if (HANDLEBARS_ENABLE_TESTS)
    add_subdirectory(tests)
    enable_testing()
endif()
