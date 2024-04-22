# Test creating an MXF OP1a file containing MPEG2LG 576i video and 2 PCM tracks

include("${TEST_SOURCE_DIR}/../testing.cmake")


function(run_test test aspect_ratio duration)
    if(TEST_MODE STREQUAL "check")
        set(output_file test_${test}_${aspect_ratio}.mxf)
    elseif(TEST_MODE STREQUAL "samples")
        file(MAKE_DIRECTORY ${BMX_TEST_SAMPLES_DIR})

        set(output_file ${BMX_TEST_SAMPLES_DIR}/test_${test}_${aspect_ratio}.mxf)
    else()
        set(output_file test_${test}${aspect_ratio}.mxf)
    endif()

    set(checksum_file ${test}_${aspect_ratio}.md5)

    set(create_test_audio ${CREATE_TEST_ESSENCE}
        -t 1
        -d ${duration}
        audio_${test}
    )

    set(create_command ${RAW2BMX}
        --regtest
        -t op1a
        -y 10:11:12:13
        --clip test
        -o ${output_file}
        --${test} video_${test}
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
    math(EXPR max_index "(${len_tests} / 3) - 1")

    foreach(index RANGE ${max_index})
        math(EXPR test_index "${index} * 3")
        list(GET tests ${test_index} test)

        math(EXPR test_ess_type_index "${index} * 3 + 1")
        list(GET tests ${test_ess_type_index} test_ess_type)

        math(EXPR test_aspect_ratio_index "${index} * 3 + 2")
        list(GET tests ${test_aspect_ratio_index} test_aspect_ratio)

        set(create_test_video ${CREATE_TEST_ESSENCE}
            -t ${test_ess_type}
            -d ${duration}
            video_${test}
        )

        run_test(${test} ${test_aspect_ratio} ${duration})
    endforeach()
endfunction()


set(tests
    mpeg2lg_mp_ml_576i 59 16by9
    mpeg2lg_mp_ml_576i 60 4by3
)


run_tests("${tests}" 24)
