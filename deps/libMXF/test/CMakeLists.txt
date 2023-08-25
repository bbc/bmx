if(LIBMXF_TEST_WITH_VALGRIND)
    set(command_prefix valgrind --leak-check=full -q --error-exitcode=1)
else()
    set(command_prefix)
endif()


# Tests with no argument
set(noarg_tests
    test_datamodel
    test_mxf_memory_file
    test_mxf_page_file
    test_mxf_rw_intl_file
)

foreach(test ${noarg_tests})
    add_executable(${test} ${test}.c)
    target_link_libraries(${test} MXF)

    include("${PROJECT_SOURCE_DIR}/cmake/source_filename.cmake")
    set_source_filename(${test} "${CMAKE_CURRENT_LIST_DIR}" "libMXF")

    add_test(NAME libMXF_${test}
        COMMAND ${command_prefix} $<TARGET_FILE:${test}>
    )
endforeach()

# Tests with an output file argument
set(tests_with_output
    test_mxf_cache_file
)

foreach(test ${tests_with_output})
    add_executable(${test} ${test}.c)
    target_link_libraries(${test} MXF)

    include("${PROJECT_SOURCE_DIR}/cmake/source_filename.cmake")
    set_source_filename(${test} "${CMAKE_CURRENT_LIST_DIR}" "libMXF")

    add_test(NAME libMXF_${test}
        COMMAND ${command_prefix} $<TARGET_FILE:${test}> ${test}.mxf
    )
endforeach()

# Tests with an output file argument (first arg after --) and a checksum
set(test_checksums
    test_essencecontainer
    test_headermetadata
    test_indextable
    test_partition
    test_primer
)

foreach(test ${test_checksums})
    add_executable(${test} ${test}.c)
    target_link_libraries(${test} MXF)

    include("${PROJECT_SOURCE_DIR}/cmake/source_filename.cmake")
    set_source_filename(${test} "${CMAKE_CURRENT_LIST_DIR}" "libMXF")

    set(test_name libMXF_${test})
    set(args
        -D SAMPLE_OUTPUT_FILE=${test}.mxf
        -D TEST_COMMAND=$<TARGET_FILE:${test}>
        -D OUTPUT_FILE=${test}.mxf
        -D CHECKSUM_FILE=${CMAKE_CURRENT_SOURCE_DIR}/${test}.md5
        -D LIBMXF_TEST_WITH_VALGRIND=${LIBMXF_TEST_WITH_VALGRIND}
        -D TEST_SAMPLES_DIR=${LIBMXF_TEST_SAMPLES_DIR}
        -P ${PROJECT_SOURCE_DIR}/cmake/test_with_checksum.cmake
        -- ${test}.mxf
    )
    add_test(NAME ${test_name}
        COMMAND ${CMAKE_COMMAND}
            -D TEST_MODE=check
            ${args}
    )

    # Adds target for creating sample files
    add_custom_target(${test_name}_samples
        COMMAND ${CMAKE_COMMAND}
            -D TEST_MODE=samples
            ${args}
    )
    add_dependencies(libMXF_test_samples ${test_name}_samples)

    # Adds target for creating test data (checksums)
    add_custom_target(${test_name}_data
        COMMAND ${CMAKE_COMMAND}
            -D TEST_MODE=data
            ${args}
    )
    add_dependencies(libMXF_test_data ${test_name}_data)
endforeach()

# Run test_file using a shell session with stdin and stdout
add_executable(test_file test_file.c)
target_link_libraries(test_file MXF)

include("${PROJECT_SOURCE_DIR}/cmake/source_filename.cmake")
set_source_filename(test_file "${CMAKE_CURRENT_LIST_DIR}" "libMXF")

if (WIN32)
    # Run run_test_file.bat via a C-executable to workaround cmake "COMMAND cmd /C ..." not working
    # because cmake 3.20 insists on quoting the '/C'.
    add_executable(win32_cmd_test_file
        win32_cmd_test_file.c
    )

    include("${PROJECT_SOURCE_DIR}/cmake/source_filename.cmake")
    set_source_filename(win32_cmd_test_file "${CMAKE_CURRENT_LIST_DIR}" "libMXF")

    add_test(NAME libMXF_test_file
        COMMAND $<TARGET_FILE:win32_cmd_test_file> ${CMAKE_CURRENT_SOURCE_DIR}/run_test_file.bat $<TARGET_FILE:test_file> test_file.mxf
    )
else()
    # Reduce LIBMXF_TEST_WITH_VALGRIND to ON or OFF
    if(LIBMXF_TEST_WITH_VALGRIND)
        set(enable_valgrind ON)
    else()
        set(enable_valgrind OFF)
    endif()

    add_test(NAME libMXF_test_file
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/run_test_file.sh $<TARGET_FILE:test_file> test_file.mxf ${enable_valgrind}
    )
endif()
