# Run the test command and compare the checksum with the expected value
function(test TEST_MODE OUTPUT_FILE CHECKSUM_FILE TEST_COMMAND LIBMXF_TEST_WITH_VALGRIND ARGS)
    if(TEST_MODE STREQUAL "check" AND LIBMXF_TEST_WITH_VALGRIND)
        set(prefix_command valgrind --leak-check=full -q --error-exitcode=1)
    else()
        set(prefix_command)
    endif()

    execute_process(COMMAND ${prefix_command} ${TEST_COMMAND}
        ${ARGS}
        OUTPUT_QUIET
        ERROR_QUIET
        RESULT_VARIABLE ret
    )
    if(NOT ret EQUAL 0)
        message("COMMAND: ${prefix_command} ${TEST_COMMAND} ${ARGS}")
        message(FATAL_ERROR "Command failed: ${ret}")
    endif()

    if(TEST_MODE STREQUAL "check" OR TEST_MODE STREQUAL "data")
        file(MD5 ${OUTPUT_FILE} checksum)

        if(TEST_MODE STREQUAL "check")
            file(READ ${CHECKSUM_FILE} expected_checksum)

            if(NOT checksum STREQUAL expected_checksum)
                message(FATAL_ERROR "File checksum ${checksum} != expected ${expected_checksum}")
            endif()
        else()
            file(WRITE ${CHECKSUM_FILE} "${checksum}")
        endif()
    endif()
endfunction()

# Construct the test command options and run the 10-bit test
function(test_8bit TEST_MODE FORMAT OUTPUT_FILE CHECKSUM_FILE TEST_COMMAND LIBMXF_TEST_WITH_VALGRIND ARGS)
    set(command_args
        --regtest
        --num-audio 6
        --format "${FORMAT}"
        --16by9
        --crc32
        3
        "${OUTPUT_FILE}"
    )

    test(
        "${TEST_MODE}"
        "${OUTPUT_FILE}"
        "${CHECKSUM_FILE}"
        "${TEST_COMMAND}"
        "${LIBMXF_TEST_WITH_VALGRIND}"
        "${command_args}"
    )
endfunction()

# Construct the test command options and run the 10-bit test
function(test_10bit TEST_MODE FORMAT OUTPUT_FILE CHECKSUM_FILE TEST_COMMAND LIBMXF_TEST_WITH_VALGRIND ARGS)
    set(command_args
        --regtest
        --num-audio 6
        --format "${FORMAT}"
        --10bit
        --16by9
        --crc32
        3
        "${OUTPUT_FILE}"
    )

    test(
        "${TEST_MODE}"
        "${OUTPUT_FILE}"
        "${CHECKSUM_FILE}"
        "${TEST_COMMAND}"
        "${LIBMXF_TEST_WITH_VALGRIND}"
        "${command_args}"
    )
endfunction()

if(TEST_MODE STREQUAL "check")
    set(output_file ${TEST_NAME}.mxf)
elseif(TEST_MODE STREQUAL "samples")
    file(MAKE_DIRECTORY ${LIBMXF_TEST_SAMPLES_DIR}/archive)

    set(output_file ${LIBMXF_TEST_SAMPLES_DIR}/archive/${TEST_NAME}.mxf)
else()
    set(output_file ${TEST_NAME}.mxf)
endif()

# Run the 10-bit or 8-bit test depending on the BITS define
if(${BITS} STREQUAL "10")
    test_10bit(
        "${TEST_MODE}"
        "${FORMAT}"
        "${output_file}"
        "${CHECKSUM_FILE}"
        "${TEST_COMMAND}"
        "${LIBMXF_TEST_WITH_VALGRIND}"
        "${ARGS}"
    )
else()
    test_8bit(
        "${TEST_MODE}"
        "${FORMAT}"
        "${output_file}"
        "${CHECKSUM_FILE}"
        "${TEST_COMMAND}"
        "${LIBMXF_TEST_WITH_VALGRIND}"
        "${ARGS}"
    )
endif()