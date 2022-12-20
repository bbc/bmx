include("${TEST_SOURCE_DIR}/../testing.cmake")


if(TEST_MODE STREQUAL "check")
    set(output_file test_${test_name}.mxf)
elseif(TEST_MODE STREQUAL "samples")
    file(MAKE_DIRECTORY ${BMX_TEST_SAMPLES_DIR})

    set(output_file ${BMX_TEST_SAMPLES_DIR}/test_${test_name}.mxf)
else()
    set(output_file test_${test_name}.mxf)
endif()

# 24-bit PCM, with values 0x00
set(create_test_audio_1 ${CREATE_TEST_ESSENCE}
    -t 42
    -d 3
    -s 0
    audio_${test_name}_1
)

# 24-bit PCM, with values 0x01
set(create_test_audio_2 ${CREATE_TEST_ESSENCE}
    -t 42
    -d 3
    -s 1
    audio_${test_name}_2
)

# AVCI
set(create_test_video ${CREATE_TEST_ESSENCE}
    -t 8
    -d 3
    video_${test_name}
)
