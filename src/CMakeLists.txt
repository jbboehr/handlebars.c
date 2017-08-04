
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR}/../)

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
