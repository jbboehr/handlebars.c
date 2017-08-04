
set(SOURCES
        handlebarsc.c)

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../src)

add_executable(handlebarsc handlebarsc.c)

target_link_libraries(handlebarsc ${LIBS} handlebars)

install(TARGETS handlebarsc
        RUNTIME DESTINATION bin)
