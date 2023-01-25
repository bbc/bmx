# Test reading an MXF OP1a file that is growing.

include("${TEST_SOURCE_DIR}/../testing.cmake")


if(TEST_MODE STREQUAL "check")
    set(output_file test.mxf)
    set(output_text test.txt)
elseif(TEST_MODE STREQUAL "samples")
    file(MAKE_DIRECTORY ${BMX_TEST_SAMPLES_DIR})

    set(output_file ${BMX_TEST_SAMPLES_DIR}/test_growing_file.mxf)
    set(output_text ${BMX_TEST_SAMPLES_DIR}/test_growing_file.txt)
else()
    set(output_file test.mxf)
    set(output_text test.txt)
endif()

execute_process(COMMAND ${CREATE_TEST_ESSENCE}
    -t 1
    -d 24
    audio
    OUTPUT_QUIET
    RESULT_VARIABLE ret
)
if(NOT ret EQUAL 0)
    message(FATAL_ERROR "Failed to create test audio: ${ret}")
endif()

execute_process(COMMAND ${CREATE_TEST_ESSENCE}
    -t 14
    -d 24
    video
    OUTPUT_QUIET
    RESULT_VARIABLE ret
)
if(NOT ret EQUAL 0)
    message(FATAL_ERROR "Failed to create test video: ${ret}")
endif()

execute_process(COMMAND ${RAW2BMX}
    --regtest
    -t op1a
    -o ${output_file}
    --single-pass
    --part 10
    --mpeg2lg_422p_hl_1080i video
    -q 16 --pcm audio
    OUTPUT_QUIET
    RESULT_VARIABLE ret
)
if(NOT ret EQUAL 0)
    message(FATAL_ERROR "Failed to create MXF file: ${ret}")
endif()


# Truncate lengths that results in a partial file to simulate a growing file in reverse.
# First length is the full file and the last one should result in an error.
#
# * full size,
# * RIP, footer, index body partition,
# * content package, audio element, content packages,
# * partial index segment, index segment,
# * no essence,
# * incomplete header
set(truncate_lengths
    5941560
    5941464 5927832 5927350
    5683470 5679610 2970790
    2970328 2970308
    13592
    4866
)

file(REMOVE ${output_text})
file(TOUCH ${output_text})

foreach(truncate_len ${truncate_lengths})
    if(TEST_MODE STREQUAL "samples")
        file(APPEND ${output_text} "${truncate_len}\n")
    else()
        file(APPEND ${output_text} "${truncate_len}\\n")
    endif()

    execute_process(COMMAND ${FILE_TRUNCATE}
        ${truncate_len}
        ${output_file}
        OUTPUT_QUIET
        RESULT_VARIABLE ret
    )
    if(NOT ret EQUAL 0)
        message(FATAL_ERROR "Failed to truncate test file: ${ret}")
    endif()

    execute_process(COMMAND ${MXF2RAW}
        --regtest
        --track-chksum md5
        ${output_file}
        OUTPUT_VARIABLE read_output
        ERROR_VARIABLE read_error
    )

    # Remove text that has file paths that can be different
    string(REGEX REPLACE "at [^\n]*" "" read_output "${read_output}")
    string(REGEX REPLACE "in [^\n]*" "" read_output "${read_output}")
    string(REGEX REPLACE "near [^\n]*" "" read_output "${read_output}")

    if(NOT TEST_MODE STREQUAL "samples")
        # Replace newlines
        string(REGEX REPLACE "\n" "\\\\n" read_output "${read_output}")
    endif()

    file(APPEND ${output_text} "${read_output}")

    if(read_error)
        # Remove text that has file paths that can be different
        string(REGEX REPLACE "at [^\n]*" "" read_error "${read_error}")
        string(REGEX REPLACE "in [^\n]*" "" read_error "${read_error}")
        string(REGEX REPLACE "Failed to open MXF file.*" "Failed to open MXF file" read_error "${read_error}")

        if(NOT TEST_MODE STREQUAL "samples")
            # Replace newlines
            string(REGEX REPLACE "\n" "\\\\n" read_error "${read_error}")
        endif()

        file(APPEND ${output_text} "${read_error}")
    endif()
endforeach()

if(TEST_MODE STREQUAL "check" OR TEST_MODE STREQUAL "data")
    file(MD5 ${output_text} checksum)

    if(TEST_MODE STREQUAL "check")
        file(READ "${TEST_SOURCE_DIR}/growing_file.md5" expected_checksum)

        if(NOT checksum STREQUAL expected_checksum)
            message(FATAL_ERROR "Text file checksum ${checksum} != expected ${expected_checksum}")
        endif()
    else()
        file(WRITE "${TEST_SOURCE_DIR}/growing_file.md5" "${checksum}")
    endif()
endif()
