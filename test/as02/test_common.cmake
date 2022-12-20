include("${TEST_SOURCE_DIR}/../testing.cmake")


function(process_checksum checksum_file output_dir_name output_file_prefix has_video)
    file(MD5 ${output_file_prefix}/${output_dir_name}.mxf checksum_version)
    if(has_video)
        file(MD5 ${output_file_prefix}/media/${output_dir_name}_v0.mxf checksum_v0)
    endif()
    file(MD5 ${output_file_prefix}/media/${output_dir_name}_a0.mxf checksum_a0)
    file(MD5 ${output_file_prefix}/media/${output_dir_name}_a1.mxf checksum_a1)

    if(TEST_MODE STREQUAL "check")
        file(READ ${TEST_SOURCE_DIR}/${checksum_file} expected_checksums)

        list(GET expected_checksums 0 expected_checksum_version)
        if(has_video)
            list(GET expected_checksums 1 expected_checksum_v0)
            list(GET expected_checksums 2 expected_checksum_a0)
            list(GET expected_checksums 3 expected_checksum_a1)
        else()
            list(GET expected_checksums 1 expected_checksum_a0)
            list(GET expected_checksums 2 expected_checksum_a1)
        endif()

        if(NOT checksum_version STREQUAL expected_checksum_version)
            message(FATAL_ERROR "AS02 version file checksum ${checksum_version} != expected ${expected_checksum_version}")
        endif()
        if(has_video AND NOT checksum_v0 STREQUAL expected_checksum_v0)
            message(FATAL_ERROR "AS02 v0 file checksum ${checksum_v0} != expected ${expected_checksum_v0}")
        endif()
        if(NOT checksum_a0 STREQUAL expected_checksum_a0)
            message(FATAL_ERROR "AS02 a0 file checksum ${checksum_a0} != expected ${expected_checksum_a0}")
        endif()
        if(NOT checksum_a1 STREQUAL expected_checksum_a1)
            message(FATAL_ERROR "AS02 a1 file checksum ${checksum_a1} != expected ${expected_checksum_a1}")
        endif()
    else()
        file(WRITE ${TEST_SOURCE_DIR}/${checksum_file} "${checksum_version};")
        if(has_video)
            file(APPEND ${TEST_SOURCE_DIR}/${checksum_file} "${checksum_v0};")
        endif()
        file(APPEND ${TEST_SOURCE_DIR}/${checksum_file} "${checksum_a0};")
        file(APPEND ${TEST_SOURCE_DIR}/${checksum_file} "${checksum_a1}")
    endif()
endfunction()

function(run_test test frame_rate)
    if(frame_rate STREQUAL "x")
        set(frame_rate)
        set(extra_opts)
    else()
        list(APPEND extra_opts -f ${frame_rate})
    endif()

    if(TEST_MODE STREQUAL "check")
        file(MAKE_DIRECTORY test_${test}${frame_rate})

        set(output_dir_name as02test)
        set(output_file_prefix test_${test}${frame_rate}/${output_dir_name})
    elseif(TEST_MODE STREQUAL "samples")
        file(MAKE_DIRECTORY ${BMX_TEST_SAMPLES_DIR}/test_${test}${frame_rate})

        set(output_dir_name as02test)
        set(output_file_prefix ${BMX_TEST_SAMPLES_DIR}/test_${test}${frame_rate}/${output_dir_name})
    else()
        file(MAKE_DIRECTORY test_${test}${frame_rate})

        set(output_dir_name as02test)
        set(output_file_prefix test_${test}${frame_rate}/${output_dir_name})
    endif()

    set(checksum_file ${test}${frame_rate}.md5s)

    set(create_command ${RAW2BMX}
        --regtest
        -t as02
        -y 10:11:12:13
        ${extra_opts}
        --clip test
        -o ${output_file_prefix}
        -a 16:9 --${test} video_${test}
        -q 16 --locked true --pcm audio_${test}
        -q 16 --locked true --pcm audio_${test}
    )

    if(TEST_MODE STREQUAL "check" AND BMX_TEST_WITH_VALGRIND)
        set(prefix_command valgrind --leak-check=full -q --error-exitcode=1)
    else()
        set(prefix_command)
    endif()

    # Create the test essences
    execute_process(COMMAND ${create_test_audio}
        OUTPUT_QUIET
        RESULT_VARIABLE ret
    )
    if(NOT ret EQUAL 0)
        message(FATAL_ERROR "Failed to create test audio essence: ${ret}")
    endif()

    execute_process(COMMAND ${create_test_video}
        OUTPUT_QUIET
        RESULT_VARIABLE ret
    )
    if(NOT ret EQUAL 0)
        message(FATAL_ERROR "Failed to create test video essence: ${ret}")
    endif()


    # Create the MXF file
    execute_process(COMMAND ${prefix_command} ${create_command}
        OUTPUT_QUIET
        RESULT_VARIABLE ret
    )
    if(NOT ret EQUAL 0)
        message(FATAL_ERROR "Failed to create AS02 MXF file: ${ret}")
    endif()


    # Validate the checksums
    if(TEST_MODE STREQUAL "check" OR TEST_MODE STREQUAL "data")
        process_checksum(${checksum_file} ${output_dir_name} ${output_file_prefix} TRUE)
    endif()
endfunction()

function(run_tests tests duration)
    list(LENGTH tests len_tests)
    math(EXPR max_index "(${len_tests} / 3) - 1")

    foreach(index RANGE ${max_index})
        math(EXPR test_index "${index} * 3")
        list(GET tests ${test_index} test)

        math(EXPR test_ess_type_index "${index} * 3 + 1")
        list(GET tests ${test_ess_type_index} test_ess_type)

        math(EXPR test_frame_rate_index "${index} * 3 + 2")
        list(GET tests ${test_frame_rate_index} test_frame_rate)

        set(create_test_audio ${CREATE_TEST_ESSENCE}
            -t 1
            -d ${duration}
            audio_${test}
        )

        set(create_test_video ${CREATE_TEST_ESSENCE}
            -t ${test_ess_type}
            -d ${duration}
            video_${test}
        )

        run_test(${test} ${test_frame_rate})
    endforeach()
endfunction()
