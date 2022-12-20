include("${TEST_SOURCE_DIR}/../testing.cmake")


function(run_test test frame_rate duration)
    if(frame_rate STREQUAL "x")
        set(frame_rate)
        set(rate_opt -f 25)
    else()
        set(rate_opt -f ${frame_rate})
    endif()

    if(TEST_MODE STREQUAL "check")
        set(output_dir test_${test}${frame_rate})
        set(output_version_file test_${test}${frame_rate}/test_${test}${frame_rate}.mxf)
        set(output_info_file info_${test}${frame_rate}.xml)
    elseif(TEST_MODE STREQUAL "samples")
        file(MAKE_DIRECTORY ${BMX_TEST_SAMPLES_DIR})

        set(output_dir ${BMX_TEST_SAMPLES_DIR}/test_${test}${frame_rate})
        set(output_version_file ${BMX_TEST_SAMPLES_DIR}/test_${test}${frame_rate}/test_${test}${frame_rate}.mxf)
        set(output_info_file ${BMX_TEST_SAMPLES_DIR}/info_${test}${frame_rate}.xml)
    else()
        set(output_dir test_${test}${frame_rate})
        set(output_version_file test_${test}${frame_rate}/test_${test}${frame_rate}.mxf)
        set(output_info_file info_${test}${frame_rate}.xml)
    endif()

    set(checksum_file ${test}${frame_rate}.md5)

    set(create_test_audio ${CREATE_TEST_ESSENCE}
        -t 1
        -d ${duration}
        audio_${test}
    )

    set(create_command ${RAW2BMX}
        --regtest
        -t as02
        --mic-type md5 
        ${rate_opt}
        -y 10:11:12:13
        --clip test
        -o ${output_dir}
        -a 16:9 --${test} video_${test}
        -q 16 --locked true --pcm audio_${test}
        -q 16 --locked true --pcm audio_${test}
    )

    set(read_command ${MXF2RAW}
        --regtest
        --info
        --info-format xml
        --info-file ${output_info_file}
        --track-chksum md5
        ${output_version_file}
    )

    run_test_b(
        "${TEST_MODE}"
        "${BMX_TEST_WITH_VALGRIND}"
        "${create_test_video}"
        "${create_test_audio}"
        ""
        "${create_command}"
        ""
        ""
        "${read_command}"
        "${output_info_file}"
        "${checksum_file}"
    )
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

        set(create_test_video ${CREATE_TEST_ESSENCE}
            -t ${test_ess_type}
            -d ${duration}
            video_${test}
        )

        run_test(${test} ${test_frame_rate} ${duration})
    endforeach()
endfunction()
