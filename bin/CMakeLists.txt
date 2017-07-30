
set(SOURCES
        handlebarsc.c)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(handlebarsc handlebarsc.c)

target_link_libraries(handlebarsc json-c talloc yaml handlebars)

install(TARGETS handlebarsc
        RUNTIME DESTINATION bin)
