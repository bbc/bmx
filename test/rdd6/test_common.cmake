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


function(run_test rate video_type rdd6_lines rdd6_sdid)
    set(create_command_1 ${RAW2BMX}
        --regtest
        -t op1a
        -f ${rate}
        -o test_intermediate_${test_name}.mxf
        --${video_type} video_${test_name}
        -q 24 --pcm audio_${test_name}
        -q 24 --pcm audio_${test_name}
        -q 24 --pcm audio_${test_name}
        -q 24 --pcm audio_${test_name}
        -q 24 --pcm audio_${test_name}
        -q 24 --pcm audio_${test_name}
        -q 24 --pcm audio_${test_name}
        -q 24 --pcm audio_${test_name}
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
        -o ${output_file}
        --rdd6 ${TEST_SOURCE_DIR}/test${test_name}.xml.bin
        --rdd6-lines ${rdd6_lines}
        --rdd6-sdid ${rdd6_sdid}
        test_intermediate_${test_name}.mxf
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
        ""
        "${read_command}"
        "${output_file}"
        "test${test_name}.md5"
        "${output_rdd6_file};test${test_name}.xml.bin"
        ""
    )
endfunction()
