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

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_BINARY_DIR})

set(SOURCE_FILES
    handlebars.c
    handlebars.lex.c
    handlebars.tab.c
    handlebars_ast.c
    handlebars_ast_helpers.c
    handlebars_ast_list.c
    handlebars_ast_printer.c
    handlebars_cache.c
    handlebars_cache_lmdb.c
    handlebars_cache_mmap.c
    handlebars_cache_simple.c
    handlebars_closure.c
    handlebars_compiler.c
    handlebars_delimiters.c
    handlebars_helpers.c
    handlebars_json.c
    handlebars_map.c
    # handlebars_memory.c
    handlebars_module_printer.c
    handlebars_opcode_printer.c
    handlebars_opcode_serializer.c
    handlebars_opcodes.c
    handlebars_parser.c
    handlebars_parser_private.c
    handlebars_partial_loader.c
    handlebars_ptr.c
    handlebars_rc.c
    handlebars_scanners.c
    handlebars_stack.c
    handlebars_string.c
    handlebars_token.c
    handlebars_value.c
    handlebars_value_handlers.c
    handlebars_vm.c
    handlebars_whitespace.c
    handlebars_yaml.c)

set(HEADERS
    ${CMAKE_CURRENT_BINARY_DIR}/handlebars_config.h
    handlebars.h
    handlebars.lex.h
    handlebars.tab.h
    handlebars_ast.h
    handlebars_ast_list.h
    handlebars_ast_printer.h
    handlebars_cache.h
    handlebars_closure.h
    handlebars_compiler.h
    handlebars_delimiters.h
    handlebars_helpers.h
    handlebars_json.h
    handlebars_map.h
    handlebars_memory.h
    handlebars_module_printer.h
    handlebars_opcode_printer.h
    handlebars_opcode_serializer.h
    handlebars_opcodes.h
    handlebars_parser.h
    handlebars_partial_loader.h
    handlebars_ptr.h
    handlebars_rc.h
    handlebars_stack.h
    handlebars_string.h
    handlebars_types.h
    handlebars_token.h
    handlebars_value.h
    handlebars_value_handlers.h
    handlebars_vm.h
    handlebars_yaml.h)

# these headers are not (currently) installed because they only contain private symbols:
# handlebars_ast_helpers.h
# handlebars_helpers_ht.h
# handlebars_parser_private.h
# handlebars_private.h
# handlebars_scanners.h
# handlebars_whitespace.h
# handlebars_value_private.h

link_libraries(${LIBS})

add_library(handlebars SHARED ${SOURCE_FILES} ${HEADERS})
add_library(handlebars_static STATIC ${SOURCE_FILES} ${HEADERS})

install(TARGETS handlebars handlebars_static
    EXPORT handlebars
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(FILES ${HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/handlebars)
install(EXPORT handlebars DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/cmake)
