# Test creating an MXF OP1a file containing sound only.
# Create file using raw2bmx and then transwrap.

include("${TEST_SOURCE_DIR}/test_common.cmake")


if(TEST_MODE STREQUAL "check")
    set(output_file_1 test_sound_only_from_raw.mxf)
    set(output_file_2 test_sound_only_transwrap.mxf)
elseif(TEST_MODE STREQUAL "samples")
    file(MAKE_DIRECTORY ${BMX_TEST_SAMPLES_DIR})

    set(output_file_1 ${BMX_TEST_SAMPLES_DIR}/test_sound_only_from_raw.mxf)
    set(output_file_2 ${BMX_TEST_SAMPLES_DIR}/test_sound_only_transwrap.mxf)
else()
    set(output_file_1 test_sound_only_from_raw.mxf)
    set(output_file_2 test_sound_only_transwrap.mxf)
endif()

set(create_test_audio ${CREATE_TEST_ESSENCE}
    -t 57
    -d 5123
    audio_sound_only
)


set(create_command ${RAW2BMX}
    --regtest
    -t op1a
    -o ${output_file_1}
    --clip-wrap
    -q 16 --pcm audio_sound_only
)
run_test_a(
    "${TEST_MODE}"
    "${BMX_TEST_WITH_VALGRIND}"
    ""
    "${create_test_audio}"
    ""
    "${create_command}"
    ""
    ""
    ""
    "${output_file_1}"
    "sound_only_from_raw.md5"
    ""
    ""
)

set(create_command ${BMXTRANSWRAP}
    --regtest
    -t op1a
    -o ${output_file_2}
    --clip-wrap
    ${output_file_1}
)
run_test_a(
    "${TEST_MODE}"
    "${BMX_TEST_WITH_VALGRIND}"
    ""
    ""
    ""
    "${create_command}"
    ""
    ""
    ""
    "${output_file_2}"
    "sound_only_transwrap.md5"
    ""
    ""
)
