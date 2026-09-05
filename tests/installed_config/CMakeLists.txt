cmake_minimum_required(VERSION 3.5)
project(handlebars_installed_config C)

add_executable(test_installed_config main.c)
target_include_directories(test_installed_config PRIVATE "${HEADER_DIR}")
target_link_libraries(test_installed_config PRIVATE "${HANDLEBARS_LIBRARY}")
foreach(feature JSON LMDB PCRE PTHREAD YAML MEMORY TESTING_EXPORTS)
    target_compile_definitions(test_installed_config PRIVATE
        "EXPECT_${feature}=${EXPECT_${feature}}")
endforeach()

enable_testing()
add_test(NAME installed_config COMMAND test_installed_config)
