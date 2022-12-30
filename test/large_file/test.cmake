# Test creating, checking the last frame can be read and then transwrapping a large (>4GB) MXF file.

if(TEST_MODE STREQUAL "samples")
    file(MAKE_DIRECTORY ${BMX_TEST_SAMPLES_DIR})

    set(output_file_raw2bmx ${BMX_TEST_SAMPLES_DIR}/test_raw2bmx.mxf)
    set(output_file_bmxtranswrap ${BMX_TEST_SAMPLES_DIR}/test_bmxtranswrap.mxf)
else()
    set(output_file_raw2bmx test_raw2bmx.mxf)
    set(output_file_bmxtranswrap test_bmxtranswrap.mxf)
endif()


if(TEST_MODE STREQUAL "check" AND BMX_TEST_WITH_VALGRIND)
    set(prefix_command valgrind --leak-check=full -q --error-exitcode=1)
else()
    set(prefix_command)
endif()


# 260 UHD frames == 4313088000 bytes, which is > 2^32 + 1 UHD frame
set(create_test_video ${CREATE_TEST_ESSENCE}
    -t 45
    -d 260
    video
)

execute_process(COMMAND ${create_test_video}
    OUTPUT_QUIET
    RESULT_VARIABLE ret
)
if(NOT ret EQUAL 0)
    message(FATAL_ERROR "Failed to create test essence: ${ret}")
endif()


set(create_command ${RAW2BMX}
    --regtest
    -t op1a
    -f 50
    --index-follows
    -o ${output_file_raw2bmx}
    --unc_3840 video
)

execute_process(COMMAND ${prefix_command} ${create_command}
    OUTPUT_QUIET
    RESULT_VARIABLE ret
)
if(NOT ret EQUAL 0)
    message(FATAL_ERROR "raw2bmx command failed: ${ret}")
endif()

if(TEST_MODE STREQUAL "check" OR TEST_MODE STREQUAL "data")
    file(MD5 ${output_file_raw2bmx} checksum)

    if(TEST_MODE STREQUAL "check")
        file(READ "${TEST_SOURCE_DIR}/test.md5" expected_checksum)

        if(NOT checksum STREQUAL expected_checksum)
            message(FATAL_ERROR "raw2bmx checksum ${checksum} != expected ${expected_checksum}")
        endif()
    else()
        file(WRITE "${TEST_SOURCE_DIR}/test.md5" "${checksum}")
    endif()
endif()

file(REMOVE video)


set(read_command ${MXF2RAW}
    --check-end
    --check-complete
    ${output_file_raw2bmx}
)

execute_process(COMMAND ${prefix_command} ${read_command}
    OUTPUT_QUIET
    RESULT_VARIABLE ret
)
if(NOT ret EQUAL 0)
    message(FATAL_ERROR "mxf2raw command failed: ${ret}")
endif()


set(create_command ${BMXTRANSWRAP}
    --regtest
    -t op1a
    --index-follows
    -o ${output_file_bmxtranswrap}
    ${output_file_raw2bmx}
)

execute_process(COMMAND ${prefix_command} ${create_command}
    OUTPUT_QUIET
    RESULT_VARIABLE ret
)
if(NOT ret EQUAL 0)
    message(FATAL_ERROR "Transwrap command failed: ${ret}")
endif()


# The checksum should be the same and so it is tested here for both check and data modes
if(TEST_MODE STREQUAL "check" OR TEST_MODE STREQUAL "data")
    file(MD5 ${output_file_bmxtranswrap} checksum)

    file(READ "${TEST_SOURCE_DIR}/test.md5" expected_checksum)

    if(NOT checksum STREQUAL expected_checksum)
        message(FATAL_ERROR "Transwrapped MXF file checksum ${checksum} != expected ${expected_checksum}")
    endif()
endif()
