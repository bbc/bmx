add_executable(test_timecode_index test_timecode_index.c)
target_link_libraries(test_timecode_index
    writearchivemxf
    MXF
)

include("${PROJECT_SOURCE_DIR}/cmake/source_filename.cmake")
set_source_filename(test_timecode_index "${CMAKE_CURRENT_LIST_DIR}" "libMXF")

add_executable(test_write_archive_mxf test_write_archive_mxf.c)
target_link_libraries(test_write_archive_mxf
    writearchivemxf
    MXF
)
if(NOT MSVC)
    # math library link not required for MSVC
    target_link_libraries(test_write_archive_mxf
        m
    )
endif()

set_source_filename(test_write_archive_mxf "${CMAKE_CURRENT_LIST_DIR}" "libMXF")


# Add custom targets for creating samples and test data
add_custom_target(libMXF_test_archive_samples)
add_custom_target(libMXF_test_archive_data)
add_dependencies(libMXF_test_samples libMXF_test_archive_samples)
add_dependencies(libMXF_test_data libMXF_test_archive_data)


if(LIBMXF_TEST_WITH_VALGRIND)
    set(command_prefix valgrind --leak-check=full -q --error-exitcode=1)
else()
    set(command_prefix)
endif()

add_test(NAME libMXF_archive_test_timecode_index
    COMMAND ${command_prefix} $<TARGET_FILE:test_timecode_index>
)

set(write_archive_tests
    test_1080_25i_10b 1080i25 10
    test_1080_25i_8b 1080i25 8
    test_1080_25p_8b 1080p25 8
    test_1080_29i_8b 1080i29 8
    test_1080_29p_8b 1080p29 8
    test_1080_50p_8b 1080p50 8
    test_1080_59p_8b 1080p59 8
    test_720_25p_8b 720p25 8
    test_720_29p_8b 720p29 8
    test_720_50p_8b 720p50 8
    test_720_59p_8b 720p59 8
    test_sd_25i_10b 625i25 10
    test_sd_25i_8b 625i25 8
    test_sd_29i_8b 525i29 8
)

list(LENGTH write_archive_tests num_tests)
math(EXPR max_index "(${num_tests} - 1) / 3")

foreach(index RANGE ${max_index})
    math(EXPR test_index "${index} * 3")
    list(GET write_archive_tests ${test_index} test)

    math(EXPR format_index "${index} * 3 + 1")
    list(GET write_archive_tests ${format_index} format)

    math(EXPR bits_index "${index} * 3 + 2")
    list(GET write_archive_tests ${bits_index} bits)

    set(test_name libMXF_archive_${test})
    set(args
        -D TEST_NAME=${test}
        -D TEST_COMMAND=$<TARGET_FILE:test_write_archive_mxf>
        -D CHECKSUM_FILE=${CMAKE_CURRENT_SOURCE_DIR}/${test}.md5
        -D FORMAT=${format}
        -D BITS=${bits}
        -D LIBMXF_TEST_WITH_VALGRIND=${LIBMXF_TEST_WITH_VALGRIND}
        -D LIBMXF_TEST_SAMPLES_DIR=${LIBMXF_TEST_SAMPLES_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/test.cmake
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
    add_dependencies(libMXF_test_archive_samples ${test_name}_samples)

    # Adds target for creating test data (checksums)
    add_custom_target(${test_name}_data
        COMMAND ${CMAKE_COMMAND}
            -D TEST_MODE=data
            ${args}
    )
    add_dependencies(libMXF_test_archive_data ${test_name}_data)
endforeach()
