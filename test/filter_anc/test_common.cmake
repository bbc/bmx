include("${TEST_SOURCE_DIR}/../testing.cmake")


if(TEST_MODE STREQUAL "check")
    set(output_file test_${test_name}.mxf)
    set(output_rdd6_file test_${test_name}.xml)
elseif(TEST_MODE STREQUAL "samples")
    file(MAKE_DIRECTORY ${BMX_TEST_SAMPLES_DIR})

    set(output_file ${BMX_TEST_SAMPLES_DIR}/test_${test_name}.mxf)
    set(output_rdd6_file ${BMX_TEST_SAMPLES_DIR}/test_${test_name}.xml)
else()
    set(output_file test_${test_name}.mxf)
    set(output_rdd6_file test_${test_name}.xml)
endif()

# 24-bit PCM
set(create_test_audio ${CREATE_TEST_ESSENCE}
    -t 42
    -d 3
    audio_${test_name}
)

# AVCI
set(create_test_video ${CREATE_TEST_ESSENCE}
    -t 7
    -d 3
    video_${test_name}
)


function(run_test pass_anc_option strip_anc_option expected_outputs)
    if(pass_anc_option STREQUAL "x")
        set(pass_anc_option "")
    endif()

    if(strip_anc_option STREQUAL "x")
        set(strip_anc_option "")
    endif()

    if(expected_outputs STREQUAL "x")
        set(expected_outputs "")
    endif()

    set(create_command_1 ${RAW2BMX}
        --regtest
        -t op1a
        -f 25
        -o test_intermediate_1_${test_name}.mxf
        --avci100_1080i video_${test_name}
        -q 24 --pcm audio_${test_name}
        -q 24 --pcm audio_${test_name}
        -q 24 --pcm audio_${test_name}
        -q 24 --pcm audio_${test_name}
        -q 24 --pcm audio_${test_name}
        -q 24 --pcm audio_${test_name}
        -q 24 --pcm audio_${test_name}
        -q 24 --pcm audio_${test_name}
    )

    set(create_command_2 ${BMXTRANSWRAP}
        --regtest
        -t op1a
        -o test_intermediate_2_${test_name}.mxf
        --rdd6 ${TEST_SOURCE_DIR}/test.xml.bin
        --rdd6-lines 9,572
        --rdd6-sdid 4
        test_intermediate_1_${test_name}.mxf
    )

    set(create_command_3 ${BMXTRANSWRAP}
        --regtest
        -t op1a
        -o ${output_file}
        ${pass_anc_option}
        ${strip_anc_option}
        test_intermediate_2_${test_name}.mxf
    )

    set(read_command ${MXF2RAW}
        --regtest
        --rdd6 0-2 ${output_rdd6_file}
        ${output_file}
    )

    run_test_a(
        "${TEST_MODE}"
        "${BMX_TEST_WITH_VALGRIND}"
        "${create_test_audio}"
        "${create_test_video}"
        ""
        "${create_command_1}"
        "${create_command_2}"
        "${create_command_3}"
        "${read_command}"
        "${output_file}"
        "test${test_name}.md5"
        "${expected_outputs}"
        ""
    )
endfunction()
