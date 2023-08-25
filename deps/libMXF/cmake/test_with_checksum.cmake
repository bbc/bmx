# Run the test command to produce the output MXF file.
# Generate an MD5 checksum from the output MXF file and compare it to the expected value.

# Get the arguments after -- which will be passed to the command ${TEST_COMMAND}
set(command_args)
set(start_of_args FALSE)
foreach(index RANGE ${CMAKE_ARGC}-1)
    if(start_of_args)
        list(APPEND command_args "${CMAKE_ARGV${index}}")
    elseif(CMAKE_ARGV${index} STREQUAL "--")
        set(start_of_args TRUE)
    endif()
endforeach()


if(TEST_MODE STREQUAL "check" AND LIBMXF_TEST_WITH_VALGRIND)
    set(prefix_command valgrind --leak-check=full -q)
else()
    set(prefix_command)
endif()

execute_process(COMMAND ${prefix_command} ${TEST_COMMAND}
    ${command_args}
    RESULT_VARIABLE ret)
if(NOT ret EQUAL 0)
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
else()
    file(MAKE_DIRECTORY ${TEST_SAMPLES_DIR})
    file(RENAME ${OUTPUT_FILE} ${TEST_SAMPLES_DIR}/${SAMPLE_OUTPUT_FILE})
endif()
