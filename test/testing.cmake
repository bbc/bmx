# The run_test_a function
#   * creates an MXF file
#   * creates or validates the MXF file checksum
#   * creates or validates the mxf2raw output for the MXF file
#
# The TEST_MODE parameter determines whether it validates, creates samples or creates test data.
#
# Parameters:
#   * TEST_MODE: set to one of the following:
#       * "check": to validate
#       * "samples": to generate test sample files
#       * "data": to update test data (checksums etc.)
#   * BMX_TEST_WITH_VALGRIND: run commands with valgrind to check memory access and leaks
#   * CREATE_TEST_ESSENCE_COMMAND_1: first command to execute to create test essence
#   * CREATE_TEST_ESSENCE_COMMAND_2: second command to execute to create test essence
#   * CREATE_TEST_ESSENCE_COMMAND_3: third command to execute to create test essence
#   * CREATE_COMMAND_1: first command to create the output MXF file
#   * CREATE_COMMAND_2: second command to create the output MXF file
#   * CREATE_COMMAND_3: third command to create the output MXF file
#   * READ_COMMAND: command to extract info and raw essence using mxf2raw from the output MXF file
#   * OUTPUT_FILE: output MXF file
#   * CHECKSUM_FILE: file containing expected MXF file checksum
#   * INFO_FILE_LIST: list containing a output mxf2raw info file and expected file pair
#   * ESSENCE_FILE_LIST: list containing pairs of the output mxf2raw essence file and expected file
function(run_test_a
    TEST_MODE
    BMX_TEST_WITH_VALGRIND
    CREATE_TEST_ESSENCE_COMMAND_1
    CREATE_TEST_ESSENCE_COMMAND_2
    CREATE_TEST_ESSENCE_COMMAND_3
    CREATE_COMMAND_1
    CREATE_COMMAND_2
    CREATE_COMMAND_3
    READ_COMMAND
    OUTPUT_FILE
    CHECKSUM_FILE
    INFO_FILE_LIST
    ESSENCE_FILE_LIST
)
    if(TEST_MODE STREQUAL "check" AND BMX_TEST_WITH_VALGRIND)
        set(prefix_command valgrind --leak-check=full -q --error-exitcode=1)
    else()
        set(prefix_command)
    endif()

    # Create the test essences
    if(NOT "${CREATE_TEST_ESSENCE_COMMAND_1}" STREQUAL "")
        execute_process(COMMAND ${CREATE_TEST_ESSENCE_COMMAND_1}
            OUTPUT_QUIET
            RESULT_VARIABLE ret
        )
        if(NOT ret EQUAL 0)
            message(FATAL_ERROR "Failed to create test essence 1: ${ret}")
        endif()
    endif()

    if(NOT "${CREATE_TEST_ESSENCE_COMMAND_2}" STREQUAL "")
        execute_process(COMMAND ${CREATE_TEST_ESSENCE_COMMAND_2}
            OUTPUT_QUIET
            RESULT_VARIABLE ret
        )
        if(NOT ret EQUAL 0)
            message(FATAL_ERROR "Failed to create test essence 2: ${ret}")
        endif()
    endif()

    if(NOT "${CREATE_TEST_ESSENCE_COMMAND_3}" STREQUAL "")
        execute_process(COMMAND ${CREATE_TEST_ESSENCE_COMMAND_3}
            OUTPUT_QUIET
            RESULT_VARIABLE ret
        )
        if(NOT ret EQUAL 0)
            message(FATAL_ERROR "Failed to create test essence 3: ${ret}")
        endif()
    endif()


    # Create the MXF file and validate the checksum
    if(NOT "${CREATE_COMMAND_1}" STREQUAL "")
        execute_process(COMMAND ${prefix_command} ${CREATE_COMMAND_1}
            OUTPUT_QUIET
            RESULT_VARIABLE ret
        )
        if(NOT ret EQUAL 0)
            message(FATAL_ERROR "MXF file creation command 1 failed: ${ret}")
        endif()
    endif()

    if(NOT "${CREATE_COMMAND_2}" STREQUAL "")
        execute_process(COMMAND ${prefix_command} ${CREATE_COMMAND_2}
            OUTPUT_QUIET
            RESULT_VARIABLE ret
        )
        if(NOT ret EQUAL 0)
            message(FATAL_ERROR "MXF file creation command 2 failed: ${ret}")
        endif()
    endif()

    if(NOT "${CREATE_COMMAND_3}" STREQUAL "")
        execute_process(COMMAND ${prefix_command} ${CREATE_COMMAND_3}
            OUTPUT_QUIET
            RESULT_VARIABLE ret
        )
        if(NOT ret EQUAL 0)
            message(FATAL_ERROR "MXF file creation command 3 failed: ${ret}")
        endif()
    endif()

    if(TEST_MODE STREQUAL "check" OR TEST_MODE STREQUAL "data")
        file(MD5 ${OUTPUT_FILE} checksum)

        if(TEST_MODE STREQUAL "check")
            file(READ "${TEST_SOURCE_DIR}/${CHECKSUM_FILE}" expected_checksum)

            if(NOT checksum STREQUAL expected_checksum)
                message(FATAL_ERROR "MXF file checksum ${checksum} != expected ${expected_checksum}")
            endif()
        else()
            file(WRITE "${TEST_SOURCE_DIR}/${CHECKSUM_FILE}" "${checksum}")
        endif()
    endif()

    list(LENGTH INFO_FILE_LIST num_info_files)
    if(${num_info_files} GREATER 0)
        if(NOT ${num_info_files} EQUAL 2)
            message(FATAL "Expecting pair of info files, not ${num_info_files}")
        endif()

        # Read the MXF file using mxf2raw
        execute_process(COMMAND ${READ_COMMAND}
            OUTPUT_QUIET
            RESULT_VARIABLE ret
        )
        if(NOT ret EQUAL 0)
            message(FATAL_ERROR "Read MXF file failed: ${ret}")
        endif()

        if(TEST_MODE STREQUAL "check")
            # Validate the info file has the expected contents
            list(GET INFO_FILE_LIST 0 output_info_file)
            list(GET INFO_FILE_LIST 1 expected_info_file)
            execute_process(COMMAND ${CMAKE_COMMAND}
                -E compare_files ${output_info_file} "${TEST_SOURCE_DIR}/${expected_info_file}"
                RESULT_VARIABLE compare_result
            )

            if(NOT compare_result EQUAL 0)
                message(FATAL_ERROR "Read info file '${output_info_file}' differs from '${expected_info_file}'")
            endif()

            # Validate the extracted essence data files have the expected contents
            list(LENGTH ESSENCE_FILE_LIST num_data_files)
            if(${num_data_files} GREATER 0)
                math(EXPR max_index "${num_data_files} - 1")
                foreach(index RANGE ${max_index})
                    math(EXPR column "${index} % 2")
                    if(column EQUAL 0)
                        list(GET ESSENCE_FILE_LIST ${index} output_essence_file)
                    else()
                        list(GET ESSENCE_FILE_LIST ${index} expected_essence_file)

                        execute_process(COMMAND ${CMAKE_COMMAND}
                            -E compare_files ${output_essence_file} "${TEST_SOURCE_DIR}/${expected_essence_file}"
                            RESULT_VARIABLE compare_result
                        )

                        if(NOT compare_result EQUAL 0)
                            message(FATAL_ERROR "Read essence data '${output_essence_file}' differs from '${expected_essence_file}'")
                        endif()
                    endif()
                endforeach()
            endif()
        endif()
    endif()
endfunction()

# The run_test_b function
#   * creates an MXF file
#   * creates or validates the checksum of the mxf2raw info file output for the MXF file
#
# The TEST_MODE parameter determines whether it validates, creates samples or creates test data.
#
# Parameters:
#   * TEST_MODE: set to one of the following:
#       * "check": to validate
#       * "samples": to generate test sample files
#       * "data": to update test data (checksums etc.)
#   * BMX_TEST_WITH_VALGRIND: run commands with valgrind to check memory access and leaks
#   * CREATE_TEST_ESSENCE_COMMAND_1: first command to execute to create test essence
#   * CREATE_TEST_ESSENCE_COMMAND_2: second command to execute to create test essence
#   * CREATE_TEST_ESSENCE_COMMAND_3: third command to execute to create test essence
#   * CREATE_COMMAND_1: first command to create the output MXF file
#   * CREATE_COMMAND_2: second command to create the output MXF file
#   * CREATE_COMMAND_3: third command to create the output MXF file
#   * READ_COMMAND: command to extract info and raw essence using mxf2raw from the output MXF file
#   * OUTPUT_INFO_FILE: output mxf2raw info file
#   * CHECKSUM_FILE: file containing expected checksum for the info file
function(run_test_b
    TEST_MODE
    BMX_TEST_WITH_VALGRIND
    CREATE_TEST_ESSENCE_COMMAND_1
    CREATE_TEST_ESSENCE_COMMAND_2
    CREATE_TEST_ESSENCE_COMMAND_3
    CREATE_COMMAND_1
    CREATE_COMMAND_2
    CREATE_COMMAND_3
    READ_COMMAND
    OUTPUT_INFO_FILE
    CHECKSUM_FILE
)
    if(TEST_MODE STREQUAL "check" AND BMX_TEST_WITH_VALGRIND)
        set(prefix_command valgrind --leak-check=full -q --error-exitcode=1)
    else()
        set(prefix_command)
    endif()

    # Create the test essences
    if(NOT "${CREATE_TEST_ESSENCE_COMMAND_1}" STREQUAL "")
        execute_process(COMMAND ${CREATE_TEST_ESSENCE_COMMAND_1}
            OUTPUT_QUIET
            RESULT_VARIABLE ret
        )
        if(NOT ret EQUAL 0)
            message(FATAL_ERROR "Failed to create test essence 1: ${ret}")
        endif()
    endif()

    if(NOT "${CREATE_TEST_ESSENCE_COMMAND_2}" STREQUAL "")
        execute_process(COMMAND ${CREATE_TEST_ESSENCE_COMMAND_2}
            OUTPUT_QUIET
            RESULT_VARIABLE ret
        )
        if(NOT ret EQUAL 0)
            message(FATAL_ERROR "Failed to create test essence 2: ${ret}")
        endif()
    endif()

    if(NOT "${CREATE_TEST_ESSENCE_COMMAND_3}" STREQUAL "")
        execute_process(COMMAND ${CREATE_TEST_ESSENCE_COMMAND_3}
            OUTPUT_QUIET
            RESULT_VARIABLE ret
        )
        if(NOT ret EQUAL 0)
            message(FATAL_ERROR "Failed to create test essence 3: ${ret}")
        endif()
    endif()


    # Create the MXF file and validate the checksum
    if(NOT "${CREATE_COMMAND_1}" STREQUAL "")
        execute_process(COMMAND ${prefix_command} ${CREATE_COMMAND_1}
            OUTPUT_QUIET
            RESULT_VARIABLE ret
        )
        if(NOT ret EQUAL 0)
            message(FATAL_ERROR "MXF file creation command 1 failed: ${ret}")
        endif()
    endif()

    if(NOT "${CREATE_COMMAND_2}" STREQUAL "")
        execute_process(COMMAND ${prefix_command} ${CREATE_COMMAND_2}
            OUTPUT_QUIET
            RESULT_VARIABLE ret
        )
        if(NOT ret EQUAL 0)
            message(FATAL_ERROR "MXF file creation command 2 failed: ${ret}")
        endif()
    endif()

    if(NOT "${CREATE_COMMAND_3}" STREQUAL "")
        execute_process(COMMAND ${prefix_command} ${CREATE_COMMAND_3}
            OUTPUT_QUIET
            RESULT_VARIABLE ret
        )
        if(NOT ret EQUAL 0)
            message(FATAL_ERROR "MXF file creation command 3 failed: ${ret}")
        endif()
    endif()

    # Read the MXF file using mxf2raw
    execute_process(COMMAND ${READ_COMMAND}
        OUTPUT_QUIET
        RESULT_VARIABLE ret
    )
    if(NOT ret EQUAL 0)
        message(FATAL_ERROR "Read MXF file failed: ${ret}")
    endif()

    if(TEST_MODE STREQUAL "check" OR TEST_MODE STREQUAL "data")
        file(MD5 ${OUTPUT_INFO_FILE} checksum)

        if(TEST_MODE STREQUAL "check")
            file(READ "${TEST_SOURCE_DIR}/${CHECKSUM_FILE}" expected_checksum)

            if(NOT checksum STREQUAL expected_checksum)
                message(FATAL_ERROR "Info file checksum ${checksum} != expected ${expected_checksum}")
            endif()
        else()
            file(WRITE "${TEST_SOURCE_DIR}/${CHECKSUM_FILE}" "${checksum}")
        endif()
    endif()
endfunction()

# The setup_test function
#   * adds a test
#   * adds a custom target for creating sample files
#   * adds a custom target for creating test data
#
# Parameters:
#   * dir_name: the test directory name
#   * test_name: the test name
#   * args: arguments to pass to the test .cmake file
function(setup_test dir_name test_name args)
    # Add the test
    add_test(NAME ${test_name}
        COMMAND ${CMAKE_COMMAND}
            -D TEST_MODE=check
            ${args}
    )
    set_tests_properties(${test_name} PROPERTIES FIXTURES_REQUIRED create_test_essence)
    set_tests_properties(${test_name} PROPERTIES FIXTURES_REQUIRED file_truncate)

    # Adds target for creating sample files
    add_custom_target(${test_name}_samples
        COMMAND ${CMAKE_COMMAND}
            -D TEST_MODE=samples
            ${args}
    )
    add_dependencies(${test_name}_samples create_test_essence file_truncate)
    add_dependencies(bmx_test_${dir_name}_samples ${test_name}_samples)

    # Adds target for creating test data (checksums)
    add_custom_target(${test_name}_data
        COMMAND ${CMAKE_COMMAND}
            -D TEST_MODE=data
            ${args}
    )
    add_dependencies(${test_name}_data create_test_essence file_truncate)
    add_dependencies(bmx_test_${dir_name}_data ${test_name}_data)
endfunction()

# The setup_test_dir function does setting up for all the tests in the test directory.
#
# Parameters:
#   * dir_name: the test directory name
function(setup_test_dir dir_name)
    # Add custom targets for creating samples and test data
    add_custom_target(bmx_test_${dir_name}_samples)
    add_custom_target(bmx_test_${dir_name}_data)
    add_dependencies(bmx_test_samples bmx_test_${dir_name}_samples)
    add_dependencies(bmx_test_data bmx_test_${dir_name}_data)

    # Set common arguments (in parent scope) to pass to the test .cmake file
    set(common_args
        -D BMX_TEST_WITH_VALGRIND=${BMX_TEST_WITH_VALGRIND}
        -D BMXTRANSWRAP=$<TARGET_FILE:bmxtranswrap>
        -D MXF2RAW=$<TARGET_FILE:mxf2raw>
        -D RAW2BMX=$<TARGET_FILE:raw2bmx>
        -D CREATE_TEST_ESSENCE=$<TARGET_FILE:create_test_essence>
        -D FILE_TRUNCATE=$<TARGET_FILE:file_truncate>
        -D TEST_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
        -D BMX_TEST_SAMPLES_DIR=${BMX_TEST_SAMPLES_DIR}/${dir_name}
        PARENT_SCOPE
    )
endfunction()
