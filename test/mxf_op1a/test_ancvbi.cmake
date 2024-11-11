# Test creating an MXF OP1a file containing ANC or VBI data, video and 1 PCM track

include("${TEST_SOURCE_DIR}/test_common.cmake")

function(run_ancvbi_test test input_type data_type klv_opt)
    if(TEST_MODE STREQUAL "check")
        set(output_file test_${test}.mxf)
    elseif(TEST_MODE STREQUAL "samples")
        file(MAKE_DIRECTORY ${BMX_TEST_SAMPLES_DIR})

        set(output_file ${BMX_TEST_SAMPLES_DIR}/test_${test}.mxf)
    else()
        set(output_file test_${test}.mxf)
    endif()

    if(${klv_opt} STREQUAL "TRUE")
        set(klv_size_opt "--klv;s")
    else()
        set(klv_size_opt "--${test}-const;24")
    endif()

    set(create_test_video ${CREATE_TEST_ESSENCE}
        -t 14
        -d 12
        video_${test}
    )

    set(create_test_audio ${CREATE_TEST_ESSENCE}
        -t 1
        -d 12
        audio_${test}
    )

    set(create_test_data ${CREATE_TEST_ESSENCE}
        -t ${data_type}
        -d 12
        data_${test}
    )

    set(create_command ${RAW2BMX}
        --regtest
        -t op1a
        -o ${output_file}
        --mpeg2lg_422p_hl_1080i video_${test}
        -q 16 --pcm audio_${test}
        ${klv_size_opt} --${input_type} data_${test}
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
    anc anc 43 FALSE
    vbi vbi 44 FALSE
    anc_klv anc 61 TRUE
    vbi_klv vbi 62 TRUE
)

list(LENGTH tests len_tests)
math(EXPR max_index "(${len_tests} / 4) - 1")

foreach(index RANGE ${max_index})
    math(EXPR test_index "${index} * 4")
    list(GET tests ${test_index} test)

    math(EXPR test_input_type_index "${index} * 4 + 1")
    list(GET tests ${test_input_type_index} test_input_type)

    math(EXPR test_ess_type_index "${index} * 4 + 2")
    list(GET tests ${test_ess_type_index} test_ess_type)

    math(EXPR test_klv_opt_index "${index} * 4 + 3")
    list(GET tests ${test_klv_opt_index} test_klv_opt)

    run_ancvbi_test(${test} ${test_input_type} ${test_ess_type} ${test_klv_opt})
endforeach()
