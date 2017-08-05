
set(SOURCES
        handlebarsc.c)

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../src)

link_libraries(${LIBS} handlebars_static)

add_executable(handlebarsc handlebarsc.c)

install(TARGETS handlebarsc
        RUNTIME DESTINATION bin)
