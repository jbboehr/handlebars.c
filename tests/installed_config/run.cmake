# Install and compile a separate consumer so build-tree definitions cannot hide
# discrepancies in the public configuration header. Keep all writes in builddir.
if(NOT IS_ABSOLUTE "${BUILD_DIR}" OR NOT EXISTS "${BUILD_DIR}/cmake_install.cmake")
    message(FATAL_ERROR "BUILD_DIR must name a configured CMake build")
endif()
set(test_dir "${BUILD_DIR}/installed-config-test")
file(REMOVE_RECURSE "${test_dir}")
set(stage "${test_dir}/stage")

function(run_checked)
    execute_process(COMMAND ${ARGV} RESULT_VARIABLE result)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "Installed configuration test failed: ${ARGV}")
    endif()
endfunction()

run_checked("${CMAKE_COMMAND}" -E env "DESTDIR=${stage}" "${CMAKE_COMMAND}"
    "-DCMAKE_INSTALL_PREFIX=/handlebars-test"
    "-DCMAKE_INSTALL_CONFIG_NAME=${CONFIG}"
    -P "${BUILD_DIR}/cmake_install.cmake")

set(consumer_args)
if(IS_ABSOLUTE "${INCLUDEDIR}" OR IS_ABSOLUTE "${LIBDIR}")
    # Exported absolute paths cannot be relocated into the staging directory.
    message(STATUS "Absolute install paths: checking staged headers; imported-target checks require relative include and library destinations")
else()
    list(APPEND consumer_args
        "-DHANDLEBARS_EXPORT=${stage}/handlebars-test/${INCLUDEDIR}/cmake/handlebars.cmake")
endif()

foreach(dir INCLUDEDIR LIBDIR)
    if(IS_ABSOLUTE "${${dir}}")
        set(${dir} "${stage}${${dir}}")
    else()
        set(${dir} "${stage}/handlebars-test/${${dir}}")
    endif()
endforeach()
find_library(installed_library NAMES handlebars
    PATHS "${LIBDIR}" NO_DEFAULT_PATH)
if(NOT installed_library)
    message(FATAL_ERROR "The installed handlebars library is missing")
endif()

foreach(feature JSON LMDB PCRE PTHREAD YAML MEMORY TESTING_EXPORTS)
    list(APPEND consumer_args "-DEXPECT_${feature}=${EXPECT_${feature}}")
endforeach()
file(MAKE_DIRECTORY "${test_dir}/build")
execute_process(COMMAND "${CMAKE_COMMAND}"
    -G "${GENERATOR}"
    "-DCMAKE_C_COMPILER=${CC}"
    "-DCMAKE_C_FLAGS=${CFLAGS}"
    "-DCMAKE_EXE_LINKER_FLAGS=${LDFLAGS}"
    "-DCMAKE_BUILD_TYPE=${CONFIG}"
    "-DHEADER_DIR=${INCLUDEDIR}/handlebars"
    "-DHANDLEBARS_LIBRARY=${installed_library}"
    ${consumer_args}
    "${SOURCE_DIR}/tests/installed_config"
    WORKING_DIRECTORY "${test_dir}/build"
    RESULT_VARIABLE result)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Could not configure the installed-header consumer")
endif()
set(build_config)
set(test_config)
if(CONFIG)
    set(build_config --config "${CONFIG}")
    set(test_config -C "${CONFIG}")
endif()
run_checked("${CMAKE_COMMAND}" --build "${test_dir}/build" ${build_config})
execute_process(COMMAND "${CTEST}" ${test_config} --output-on-failure
    WORKING_DIRECTORY "${test_dir}/build"
    RESULT_VARIABLE result)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "The installed-header consumer failed")
endif()
