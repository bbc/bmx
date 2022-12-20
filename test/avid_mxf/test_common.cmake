include("${TEST_SOURCE_DIR}/../testing.cmake")


function(run_test test test_suffix frame_rate duration extra_opts extra_video_opts)
    if(frame_rate STREQUAL "x")
        set(frame_rate)
    else()
        list(APPEND extra_opts -f ${frame_rate})
    endif()

    if(TEST_MODE STREQUAL "check")
        set(output_file_prefix test_${test}${test_suffix}${frame_rate})
    elseif(TEST_MODE STREQUAL "samples")
        file(MAKE_DIRECTORY ${BMX_TEST_SAMPLES_DIR})

        set(output_file_prefix ${BMX_TEST_SAMPLES_DIR}/test_${test}${test_suffix}${frame_rate})
    else()
        set(output_file_prefix test_${test}${test_suffix}${frame_rate})
    endif()

    set(checksum_file ${test}${test_suffix}${frame_rate}.md5s)

    set(create_test_audio ${CREATE_TEST_ESSENCE}
        -t 1
        -d ${duration}
        audio_${test}
    )

    set(create_command ${RAW2BMX}
        --regtest
        -t avid
        -y 10:11:12:13
        ${extra_opts}
        --clip test
        --tape testtape
        -o ${output_file_prefix}
        ${extra_video_opts} -a 16:9 --${test} video_${test}
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
        message(FATAL_ERROR "Failed to create Avid MXF file: ${ret}")
    endif()


    # Validate the checksums
    if(TEST_MODE STREQUAL "check" OR TEST_MODE STREQUAL "data")
        file(MD5 ${output_file_prefix}_v1.mxf checksum_v1)
        file(MD5 ${output_file_prefix}_a1.mxf checksum_a1)
        file(MD5 ${output_file_prefix}_a2.mxf checksum_a2)

        if(TEST_MODE STREQUAL "check")
            file(READ ${TEST_SOURCE_DIR}/${checksum_file} expected_checksums)

            list(GET expected_checksums 0 expected_checksum_v1)
            list(GET expected_checksums 1 expected_checksum_a1)
            list(GET expected_checksums 2 expected_checksum_a2)

            if(NOT checksum_v1 STREQUAL expected_checksum_v1)
                message(FATAL_ERROR "MXF file v1 checksum ${checksum_v1} != expected ${expected_checksum_v1}")
            endif()
            if(NOT checksum_a1 STREQUAL expected_checksum_a1)
                message(FATAL_ERROR "MXF file a1 checksum ${checksum_a1} != expected ${expected_checksum_a1}")
            endif()
            if(NOT checksum_a2 STREQUAL expected_checksum_a2)
                message(FATAL_ERROR "MXF file a2 checksum ${checksum_a2} != expected ${expected_checksum_a2}")
            endif()
        else()
            file(WRITE ${TEST_SOURCE_DIR}/${checksum_file} "${checksum_v1};")
            file(APPEND ${TEST_SOURCE_DIR}/${checksum_file} "${checksum_a1};")
            file(APPEND ${TEST_SOURCE_DIR}/${checksum_file} "${checksum_a2}")
        endif()
    endif()
endfunction()

function(run_tests tests duration)
    list(LENGTH tests len_tests)
    math(EXPR max_index "(${len_tests} / 3) - 1")

    foreach(index RANGE ${max_index})
        set(extra_opts)
        set(extra_video_opts)
        set(test_suffix)

        math(EXPR test_index "${index} * 3")
        list(GET tests ${test_index} test_in)

        if(test_in STREQUAL "unc")
            set(test ${test_in})
            list(APPEND extra_video_opts --height 576)
        else()
            # Separate the _gf and _gfp suffixes used for the growing file tests
            # and add extra options
            string(REGEX REPLACE "_gf$" "" test ${test_in})
            if(NOT test STREQUAL ${test_in})
                set(test_suffix "_gf")
                list(APPEND extra_opts --avid-gf)
            else()
                string(REGEX REPLACE "_gfp$" "" test ${test_in})
                if(NOT test STREQUAL ${test_in})
                    set(test_suffix "_gfp")
                    math(EXPR duration_minus_1 "${duration} - 1")
                    list(APPEND extra_opts --regtest-end ${duration_minus_1})
                endif()
            endif()
        endif()
        
        math(EXPR test_ess_type_index "${index} * 3 + 1")
        list(GET tests ${test_ess_type_index} test_ess_type)

        math(EXPR test_frame_rate_index "${index} * 3 + 2")
        list(GET tests ${test_frame_rate_index} test_frame_rate)

        set(create_test_video ${CREATE_TEST_ESSENCE}
            -t ${test_ess_type}
            -d ${duration}
            video_${test}
        )

        run_test(${test} "${test_suffix}" ${test_frame_rate} ${duration} "${extra_opts}" "${extra_video_opts}")
    endforeach()
endfunction()
