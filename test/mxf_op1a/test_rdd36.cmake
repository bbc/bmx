# Test creating an MXF OP1a file containing RDD 36 (ProRes) video and 2 PCM tracks

include("${TEST_SOURCE_DIR}/test_common.cmake")

set(tests
    rdd36_422 55 "x"
    rdd36_4444 56 "x"
)

run_tests("${tests}" 3)


# Test with ITU 2020 colorimetry

include("${TEST_SOURCE_DIR}/../testing.cmake")

set(create_test_video ${CREATE_TEST_ESSENCE}
    -t 58
    -d 3
    video_rdd36_422_itu2020
)

if(TEST_MODE STREQUAL "check")
    set(output_file test_rdd36_422_itu2020.mxf)
elseif(TEST_MODE STREQUAL "samples")
    file(MAKE_DIRECTORY ${BMX_TEST_SAMPLES_DIR})

    set(output_file ${BMX_TEST_SAMPLES_DIR}/test_rdd36_422_itu2020.mxf)
else()
    set(output_file test_rdd36_422_itu2020.mxf)
endif()

set(checksum_file rdd36_422_itu2020.md5)

set(create_test_audio ${CREATE_TEST_ESSENCE}
    -t 1
    -d 3
    audio_rdd36_422_itu2020
)

set(create_command ${RAW2BMX}
    --regtest
    -t op1a
    -y 10:11:12:13
    --clip test
    -o ${output_file}
    -a 16:9 --rdd36_422 video_rdd36_422_itu2020
    -q 16 --locked true --pcm audio_rdd36_422_itu2020
    -q 16 --locked true --pcm audio_rdd36_422_itu2020
)

run_test_a(
    "${TEST_MODE}"
    "${BMX_TEST_WITH_VALGRIND}"
    "${create_test_video}"
    "${create_test_audio}"
    ""
    "${create_command}"
    ""
    ""
    ""
    "${output_file}"
    "${checksum_file}"
    ""
    ""
)
