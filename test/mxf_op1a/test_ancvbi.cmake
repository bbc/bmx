# Test creating an MXF OP1a file containing ANC or VBI data, video and 1 PCM track

include("${TEST_SOURCE_DIR}/test_common.cmake")

function(run_ancvbi_test test data_type)
    if(TEST_MODE STREQUAL "check")
        set(output_file test_${test}.mxf)
    elseif(TEST_MODE STREQUAL "samples")
        file(MAKE_DIRECTORY ${BMX_TEST_SAMPLES_DIR})

        set(output_file ${BMX_TEST_SAMPLES_DIR}/test_${test}.mxf)
    else()
        set(output_file test_${test}.mxf)
    endif()

    set(create_test_video ${CREATE_TEST_ESSENCE}
        -t 7
        -d 3
        video_${test}
    )

    set(create_test_audio ${CREATE_TEST_ESSENCE}
        -t 1
        -d 3
        audio_${test}
    )

    set(create_test_data ${CREATE_TEST_ESSENCE}
        -t ${data_type}
        -d 3
        data_${test}
    )

    set(create_command ${RAW2BMX}
        --regtest
        -t op1a
        -o ${output_file}
        --avci100_1080i video_${test}
        -q 16 --pcm audio_${test}
        --${test}-const 24 --${test} data_${test}
    )

    run_test_a(
        "${TEST_MODE}"
        "${BMX_TEST_WITH_VALGRIND}"
        "${create_test_video}"
        "${create_test_audio}"
        "${create_test_data}"
        "${create_command}"
        ""
        ""
        ""
        "${output_file}"
        "${test}.md5"
        ""
        ""
    )
endfunction()

set(tests
    anc 43
    vbi 44
)

list(LENGTH tests len_tests)
math(EXPR max_index "(${len_tests} / 2) - 1")

foreach(index RANGE ${max_index})
    math(EXPR test_index "${index} * 2")
    list(GET tests ${test_index} test)
    
    math(EXPR test_ess_type_index "${index} * 2 + 1")
    list(GET tests ${test_ess_type_index} test_ess_type)

    run_ancvbi_test(${test} ${test_ess_type})
endforeach()
