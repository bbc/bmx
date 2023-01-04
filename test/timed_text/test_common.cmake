include("${TEST_SOURCE_DIR}/../testing.cmake")


if(TEST_MODE STREQUAL "check")
    set(output_file test_${test_num}.mxf)
    set(output_info_file test_${test_num}.xml)
    set(output_essence_file_prefix test_${test_num})
elseif(TEST_MODE STREQUAL "samples")
    file(MAKE_DIRECTORY ${BMX_TEST_SAMPLES_DIR})

    set(output_file ${BMX_TEST_SAMPLES_DIR}/test_${test_num}.mxf)
    set(output_info_file ${BMX_TEST_SAMPLES_DIR}/info_${test_num}.xml.bin)
    set(output_essence_file_prefix ${BMX_TEST_SAMPLES_DIR}/ess_${test_num})
else()
    set(output_file test_${test_num}.mxf)
    set(output_info_file ${TEST_SOURCE_DIR}/info_${test_num}.xml.bin)
    set(output_essence_file_prefix test_${test_num})
endif()


# for --avci100_1080i
set(create_test_video_1 ${CREATE_TEST_ESSENCE}
    -t 7
    -d 3
    video_1_${test_num}
)

# for --mpeg2lg_422p_hl_1080i
set(create_test_video_2 ${CREATE_TEST_ESSENCE}
    -t 21
    -d 12
    video_2_${test_num}
)

set(read_command ${MXF2RAW}
    --regtest
    --log-level 2
    --info
    --info-format xml
    --info-file ${output_info_file}
    --ess-out ${output_essence_file_prefix}
    --disable-audio
    --disable-video
    ${output_file}
)
