# Test creating an RDD9 MXF file with the index repeated in the footer.

include("${TEST_SOURCE_DIR}/../testing.cmake")


if(TEST_MODE STREQUAL "check")
    set(output_file test_repeatindex.mxf)
elseif(TEST_MODE STREQUAL "samples")
    file(MAKE_DIRECTORY ${BMX_TEST_SAMPLES_DIR})

    set(output_file ${BMX_TEST_SAMPLES_DIR}/test_repeatindex.mxf)
else()
    set(output_file test_repeatindex.mxf)
endif()

set(checksum_file repeatindex.md5)

set(create_test_audio ${CREATE_TEST_ESSENCE}
    -t 1
    -d 24
    audio
)

set(create_test_video ${CREATE_TEST_ESSENCE}
    -t 14
    -d 24
    video
)

set(create_command ${RAW2BMX}
    --regtest
    -t rdd9
    -y 10:11:12:13
    --clip test
    --part 12
    --repeat-index
    -o ${output_file}
    -a 16:9 --mpeg2lg_422p_hl_1080i video
    -q 16 --locked true --pcm audio
    -q 16 --locked true --pcm audio
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
