add_executable(writeaviddv50
    writeaviddv50.c
)

target_link_libraries(writeaviddv50
    MXF
)

include("${PROJECT_SOURCE_DIR}/cmake/source_filename.cmake")
set_source_filename(writeaviddv50 "${CMAKE_CURRENT_LIST_DIR}" "libMXF")


# Add custom targets for creating samples and test data
add_custom_target(libMXF_test_writeaviddv50_samples)
add_custom_target(libMXF_test_writeaviddv50_data)
add_dependencies(libMXF_test_samples libMXF_test_writeaviddv50_samples)
add_dependencies(libMXF_test_data libMXF_test_writeaviddv50_data)


if(LIBMXF_BUILD_TESTING AND (NOT DEFINED BUILD_TESTING OR BUILD_TESTING))
    set(test_name libMXF_writeaviddv50)
    set(args
        -D SAMPLE_OUTPUT_FILE=writeaviddv50.mxf
        -D TEST_COMMAND=$<TARGET_FILE:writeaviddv50>
        -D OUTPUT_FILE=writeaviddv50.mxf
        -D CHECKSUM_FILE=${CMAKE_CURRENT_SOURCE_DIR}/writeaviddv50.md5
        -D LIBMXF_TEST_WITH_VALGRIND=${LIBMXF_TEST_WITH_VALGRIND}
        -D TEST_SAMPLES_DIR=${LIBMXF_TEST_SAMPLES_DIR}/writeaviddv50
        -P ${PROJECT_SOURCE_DIR}/cmake/test_with_checksum.cmake
        -- --test writeaviddv50.mxf
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
    add_dependencies(libMXF_test_writeaviddv50_samples ${test_name}_samples)

    # Adds target for creating test data (checksums)
    add_custom_target(${test_name}_data
        COMMAND ${CMAKE_COMMAND}
            -D TEST_MODE=data
            ${args}
    )
    add_dependencies(libMXF_test_writeaviddv50_data ${test_name}_data)
endif()
