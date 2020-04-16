#
# Copyright (C) 2016 John Boehr
#
# This file is part of handlebars.c.
#
# handlebars.c is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published
# by the Free Software Foundation, either version 2.1 of the License, or
# (at your option) any later version.
#
# handlebars.c is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with handlebars.c.  If not, see <http://www.gnu.org/licenses/>.
#

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_BINARY_DIR})

set(SOURCE_FILES
    handlebars.c
    handlebars.lex.c
    handlebars.tab.c
    handlebars_ast.c
    handlebars_ast_helpers.c
    handlebars_ast_list.c
    handlebars_ast_printer.c
    handlebars_cache_lmdb.c
    handlebars_cache_mmap.c
    handlebars_cache_simple.c
    handlebars_compiler.c
    handlebars_delimiters.c
    handlebars_helpers.c
    handlebars_map.c
#    handlebars_memory.c
    handlebars_opcode_printer.c
    handlebars_opcode_serializer.c
    handlebars_opcodes.c
    handlebars_partial_loader.c
    handlebars_scanners.c
    handlebars_stack.c
    handlebars_string.c
    handlebars_token.c
    handlebars_utils.c
    handlebars_value.c
    handlebars_value_handlers.c
    handlebars_vm.c
    handlebars_whitespace.c)

set(HEADERS
    ${CMAKE_CURRENT_BINARY_DIR}/handlebars_config.h
    handlebars.h
    handlebars.lex.h
    handlebars.tab.h
    handlebars_ast.h
    handlebars_ast_helpers.h
    handlebars_ast_list.h
    handlebars_ast_printer.h
    handlebars_cache.h
    handlebars_compiler.h
    handlebars_helpers.h
    handlebars_helpers_ht.h
    handlebars_map.h
    handlebars_memory.h
    handlebars_opcode_printer.h
    handlebars_opcode_serializer.h
    handlebars_opcodes.h
    handlebars_partial_loader.h
    handlebars_private.h
    handlebars_scanners.h
    handlebars_stack.h
    handlebars_string.h
    handlebars_token.h
    handlebars_utils.h
    handlebars_value.h
    handlebars_value_handlers.h
    handlebars_vm.h
    handlebars_whitespace.h)

link_libraries(${LIBS})

add_library(handlebars SHARED ${SOURCE_FILES} ${HEADERS})
add_library(handlebars_static STATIC ${SOURCE_FILES} ${HEADERS})

install(TARGETS handlebars handlebars_static
    EXPORT handlebars
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)

install(FILES ${HEADERS} DESTINATION include)
install(EXPORT handlebars DESTINATION cmake)
