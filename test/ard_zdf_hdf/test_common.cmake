include("${TEST_SOURCE_DIR}/../testing.cmake")


function(run_test test_name output_type video_opt_name ess_type duration)
    if(TEST_MODE STREQUAL "check")
        set(output_file test_${test_name}.mxf)
        set(output_info_file test_${test_name}.xml)
    elseif(TEST_MODE STREQUAL "samples")
        file(MAKE_DIRECTORY ${BMX_TEST_SAMPLES_DIR})

        set(output_file ${BMX_TEST_SAMPLES_DIR}/test_${test_name}.mxf)
        set(output_info_file ${BMX_TEST_SAMPLES_DIR}/test_${test_name}.xml)
    else()
        set(output_file test_${test_name}.mxf)
        set(output_info_file ${TEST_SOURCE_DIR}/${test_name}.xml)
    endif()

    set(create_test_audio ${CREATE_TEST_ESSENCE}
        -t 42
        -d ${duration}
        audio_${test_name}
    )

    set(create_test_video ${CREATE_TEST_ESSENCE}
        -t ${ess_type}
        -d ${duration}
        video_${test_name}
    )

    if(output_type STREQUAL "rdd9")
        set(part_opt --part 12)
    else()
        set(part_opt)
    endif()

    set(create_command ${RAW2BMX}
        --regtest
        -t ${output_type}
        --ard-zdf-hdf
        -f 25
        -y 10:00:00:00
        ${part_opt}
        -o ${output_file}
        --afd 10
        --${video_opt_name} video_${test_name}
        -q 24 --pcm audio_${test_name}
        -q 24 --pcm audio_${test_name}
        -q 24 --pcm audio_${test_name}
        -q 24 --pcm audio_${test_name}
        -q 24 --pcm audio_${test_name}
        -q 24 --pcm audio_${test_name}
        -q 24 --pcm audio_${test_name}
        -q 24 --pcm audio_${test_name}
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
        ""
        "${output_file}"
        "${test_name}.md5"
        ""
        ""
    )
endfunction()
