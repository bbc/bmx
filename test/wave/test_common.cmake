include("${TEST_SOURCE_DIR}/../testing.cmake")


if(TEST_MODE STREQUAL "check")
    set(output_file test_${test_name}.wav)
elseif(TEST_MODE STREQUAL "samples")
    file(MAKE_DIRECTORY ${BMX_TEST_SAMPLES_DIR})

    set(output_file ${BMX_TEST_SAMPLES_DIR}/test_${test_name}.wav)
else()
    set(output_file test_${test_name}.wav)
endif()


set(create_test_audio_1 ${CREATE_TEST_ESSENCE}
    -t 1
    -d 25
    audio_1_${test_name}
)

set(create_test_audio_2 ${CREATE_TEST_ESSENCE}
    -t 57
    -d 5123
    audio_2_${test_name}
)
