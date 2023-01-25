# Test transwrapping MXF files.
# Create an input file using raw2bmx and then transwrap it.

include("${TEST_SOURCE_DIR}/../testing.cmake")


function(run_test test raw2bmx_type bmxtranswrap_type video_ess_type)
    if(TEST_MODE STREQUAL "check")
        set(output_file test.mxf)
    elseif(TEST_MODE STREQUAL "samples")
        set(output_file ${BMX_TEST_SAMPLES_DIR}/test_${test}_${raw2bmx_type}_${bmxtranswrap_type}.mxf)
    else()
        set(output_file test.mxf)
    endif()

    set(create_test_audio ${CREATE_TEST_ESSENCE}
        -t 42
        -d 24
        audio
    )

    set(create_test_video ${CREATE_TEST_ESSENCE}
        -t ${video_ess_type}
        -d 24
        video
    )

    set(create_command_1 ${RAW2BMX}
        --regtest
        -t ${raw2bmx_type}
        -f 25
        -y 09:58:00:00
        -o input.mxf
        --afd 10 -a 16:9 --${test} video
        -q 24 --locked true --pcm audio
        -q 24 --locked true --pcm audio
        -q 24 --locked true --pcm audio
        -q 24 --locked true --pcm audio
    )

    set(create_command_2 ${BMXTRANSWRAP}
        --regtest
        -t ${bmxtranswrap_type}
        -o ${output_file}
        input.mxf
    )

    run_test_a(
        "${TEST_MODE}"
        "${BMX_TEST_WITH_VALGRIND}"
        "${create_test_video}"
        "${create_test_audio}"
        ""
        "${create_command_1}"
        "${create_command_2}"
        ""
        ""
        "${output_file}"
        "${test}_${raw2bmx_type}_${bmxtranswrap_type}.md5"
        ""
        ""
    )
endfunction()


if(TEST_MODE STREQUAL "samples")
    file(MAKE_DIRECTORY ${BMX_TEST_SAMPLES_DIR})
endif()


set(tests
    avci100_1080i op1a op1a 7
    d10_50 d10 op1a 11
    d10_50 op1a d10 11
)

list(LENGTH tests len_tests)
math(EXPR max_index "(${len_tests} / 4) - 1")

foreach(index RANGE ${max_index})
    math(EXPR test_index "${index} * 4")
    list(GET tests ${test_index} test)
    
    math(EXPR test_raw2bmx_type_index "${index} * 4 + 1")
    list(GET tests ${test_raw2bmx_type_index} test_raw2bmx_type)

    math(EXPR test_bmxtranswrap_type_index "${index} * 4 + 2")
    list(GET tests ${test_bmxtranswrap_type_index} test_bmxtranswrap_type)

    math(EXPR test_ess_type_index "${index} * 4 + 3")
    list(GET tests ${test_ess_type_index} test_ess_type)

    run_test(${test} ${test_raw2bmx_type} ${test_bmxtranswrap_type} ${test_ess_type})
endforeach()
