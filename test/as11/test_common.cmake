include("${TEST_SOURCE_DIR}/../testing.cmake")


function(run_test test_name ess_type)
    if(TEST_MODE STREQUAL "check")
        set(output_file test_${test_name}.mxf)
        set(output_info_file test_${test_name}.xml)
    elseif(TEST_MODE STREQUAL "samples")
        file(MAKE_DIRECTORY ${BMX_TEST_SAMPLES_DIR})

        set(output_file ${BMX_TEST_SAMPLES_DIR}/test_${test_name}.mxf)
        set(output_info_file ${BMX_TEST_SAMPLES_DIR}/test_${test_name}.xml.bin)
    else()
        set(output_file test_${test_name}.mxf)
        set(output_info_file ${TEST_SOURCE_DIR}/${test_name}.xml.bin)
    endif()

    set(create_test_audio ${CREATE_TEST_ESSENCE}
        -t 42
        -d 24
        audio_${test_name}
    )

    set(create_test_video ${CREATE_TEST_ESSENCE}
        -t ${ess_type}
        -d 24
        video_${test_name}
    )

    if(test_name STREQUAL "mpeg2lg_422p_hl_1080i")
        set(output_type as11rdd9)
        set(core_opts)
        set(dpp_opts)
    else()
        set(output_type as11op1a)
        set(core_opts --dm-file as11 "${TEST_SOURCE_DIR}/as11_core_framework.txt")
        set(dpp_opts --dm-file dpp "${TEST_SOURCE_DIR}/ukdpp_framework.txt")
    endif()

    set(create_command ${RAW2BMX}
        --regtest
        -t ${output_type}
        -f 25
        -y 09:58:00:00
        -o ${output_file}
        ${core_opts}
        ${dpp_opts}
        --seg "${TEST_SOURCE_DIR}/as11_segmentation_framework.txt"
        --afd 10 
        -a 16:9 --${test_name} video_${test_name}
        -q 24 --locked true --pcm audio_${test_name}
        -q 24 --locked true --pcm audio_${test_name}
        -q 24 --locked true --pcm audio_${test_name}
        -q 24 --locked true --pcm audio_${test_name}
    )

    set(read_command ${MXF2RAW}
        --regtest
        --info
        --info-format xml
        --info-file ${output_info_file}
        --as11
        ${output_file}
    )

    run_test_a(
        "${TEST_MODE}"
        "${BMX_TEST_WITH_VALGRIND}"
        "${create_test_audio}"
        "${create_test_video}"
        ""
        "${create_command}"
        ""
        ""
        "${read_command}"
        "${output_file}"
        "${test_name}.md5"
        "${output_info_file};${test_name}.xml.bin"
        ""
    )
endfunction()
