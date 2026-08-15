# Copyright (c) anno Domini nostri Jesu Christi MMXVI-MMXXIV John Boehr & contributors
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

include_directories(${CMAKE_CURRENT_BINARY_DIR}/../src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src)

add_executable(fuzz_template fuzz_template.c)
target_link_libraries(fuzz_template handlebars_static ${LIBS})
set_target_properties(fuzz_template PROPERTIES
    LINK_FLAGS "-fsanitize=fuzzer,address,undefined")

if(LIBJSONC_LIBRARY)
    add_executable(fuzz_json fuzz_json.c)
    target_link_libraries(fuzz_json handlebars_static ${LIBS})
    set_target_properties(fuzz_json PROPERTIES
        LINK_FLAGS "-fsanitize=fuzzer,address,undefined")
endif()
