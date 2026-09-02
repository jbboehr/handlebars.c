#!/bin/sh

set -eu

: "${HEADER_SOURCE_DIR:?HEADER_SOURCE_DIR is required}"
: "${HEADER_BUILD_DIR:?HEADER_BUILD_DIR is required}"
: "${HEADER_PROBE:?HEADER_PROBE is required}"

header_cc=${HEADER_CC:-${CC:-cc}}
header_cxx=${HEADER_CXX:-${CXX:-c++}}
header_cppflags=${HEADER_CPPFLAGS:-}
header_cflags=${HEADER_CFLAGS:-}
header_cxxflags=${HEADER_CXXFLAGS:-}
header_c_test_flags="-std=c99 -Wall -Wextra -Werror -pedantic-errors"
header_cxx_test_flags="-std=c++11 -Wall -Wextra -Werror -pedantic-errors"
checked=0
seen=" "

compiler_available() (
    set -f
    set -- $1
    test "$#" -gt 0 && command -v "$1" >/dev/null 2>&1
)

compile_header() (
    compiler_command=$1
    compiler_flags=$2
    test_flags=$3
    language=$4
    header_path=$5

    set -f
    set -- $compiler_command
    "$@" $header_cppflags $compiler_flags $test_flags \
        -I"$HEADER_BUILD_DIR" \
        -I"$HEADER_SOURCE_DIR" \
        -include "$header_path" \
        -x "$language" \
        -fsyntax-only \
        "$HEADER_PROBE"
)

if compiler_available "$header_cxx"; then
    have_cxx=1
else
    have_cxx=0
    echo "C++ compiler '$header_cxx' not found, skipping C++ header checks"
fi

for header_path in \
    "$HEADER_BUILD_DIR/handlebars_config.h" \
    "$HEADER_SOURCE_DIR"/handlebars*.h
do
    if test ! -f "$header_path"; then
        continue
    fi

    header=${header_path##*/}
    case "$header" in
        handlebars.lex.h | \
        handlebars.tab.h | \
        handlebars_ast_helpers.h | \
        handlebars_cache_private.h | \
        handlebars_helpers_ht.h | \
        handlebars_parser_private.h | \
        handlebars_private.h | \
        handlebars_scanners.h | \
        handlebars_value_private.h | \
        handlebars_vm_private.h | \
        handlebars_whitespace.h)
            continue
            ;;
    esac

    case "$seen" in
        *" $header "*)
            continue
            ;;
    esac
    seen="$seen$header "
    checked=$((checked + 1))

    compile_header \
        "$header_cc" \
        "$header_cflags" \
        "$header_c_test_flags" \
        c \
        "$header_path"

    if test "$have_cxx" -eq 1; then
        compile_header \
            "$header_cxx" \
            "$header_cxxflags" \
            "$header_cxx_test_flags" \
            c++ \
            "$header_path"
    fi
done

if test "$checked" -eq 0; then
    echo "No public headers were found" >&2
    exit 1
fi
