include("${TEST_SOURCE_DIR}/../testing.cmake")


function(run_test test frame_rate part fixed_part duration)
    if(frame_rate STREQUAL "x")
        set(frame_rate)
        set(rate_opt)
    else()
        set(rate_opt -f ${frame_rate})
    endif()

    if(fixed_part STREQUAL "true")
        set(fixed_part_opt --fixed-part)
        set(output_suffix _fixed_part)
    else()
        set(fixed_part_opt)
        set(output_suffix)
    endif()

    if(TEST_MODE STREQUAL "check")
        set(output_file test_${test}${frame_rate}${output_suffix}.mxf)
    elseif(TEST_MODE STREQUAL "samples")
        file(MAKE_DIRECTORY ${BMX_TEST_SAMPLES_DIR})

        set(output_file ${BMX_TEST_SAMPLES_DIR}/test_${test}${frame_rate}${output_suffix}.mxf)
    else()
        set(output_file test_${test}${frame_rate}${output_suffix}.mxf)
    endif()

    set(checksum_file ${test}${frame_rate}${output_suffix}.md5)

    set(create_test_audio ${CREATE_TEST_ESSENCE}
        -t 1
        -d ${duration}
        audio_${test}
    )

    set(create_command ${RAW2BMX}
        --regtest
        -t rdd9
        -y 10:11:12:13
        ${rate_opt}
        --clip test
        --part ${part}
        ${fixed_part_opt}
        -o ${output_file}
        -a 16:9 --${test} video_${test}
        -q 16 --locked true --pcm audio_${test}
        -q 16 --locked true --pcm audio_${test}
    )

    run_test_a(
        "${TEST_MODE}"
        "${BMX_TEST_WITH_VALGRIND}"
        "${create_test_video}"
        "${create_test_audio}"
        ""
        "${create_command}"
        ""
        ""
        ""
        "${output_file}"
        "${checksum_file}"
        ""
        ""
    )
endfunction()

function(run_tests tests duration)
    list(LENGTH tests len_tests)
    math(EXPR max_index "(${len_tests} / 5) - 1")

    foreach(index RANGE ${max_index})
        math(EXPR test_index "${index} * 5")
        list(GET tests ${test_index} test)
        
        math(EXPR test_ess_type_index "${index} * 5 + 1")
        list(GET tests ${test_ess_type_index} test_ess_type)

        math(EXPR test_frame_rate_index "${index} * 5 + 2")
        list(GET tests ${test_frame_rate_index} test_frame_rate)

        math(EXPR test_part_index "${index} * 5 + 3")
        list(GET tests ${test_part_index} test_part)

        math(EXPR test_fixed_part_index "${index} * 5 + 4")
        list(GET tests ${test_fixed_part_index} test_fixed_part)

        set(create_test_video ${CREATE_TEST_ESSENCE}
            -t ${test_ess_type}
            -d ${duration}
            video_${test}
        )

        run_test(${test} ${test_frame_rate} ${test_part} ${test_fixed_part} ${duration})
    endforeach()
endfunction()
