# Test creating an MXF file from video with pre-charge and filler required for the audio.

include("${TEST_SOURCE_DIR}/../testing.cmake")

if(TEST_MODE STREQUAL "check")
    set(output_file test_2.wav)
elseif(TEST_MODE STREQUAL "samples")
    file(MAKE_DIRECTORY ${BMX_TEST_SAMPLES_DIR})

    set(output_file ${BMX_TEST_SAMPLES_DIR}/test_2.wav)
else()
    set(output_file test_2.wav)
endif()


# 16-bit PCM, with duration in samples
set(create_test_audio ${CREATE_TEST_ESSENCE}
    -t 57
    -d 960
    audio_2
)

# Clip wrap in MXF, using -f 50 to ensure 960 samples are transferred
set(create_command_1 ${RAW2BMX}
    -t op1a
    -f 50
    --clip-wrap
    -o test_pass1_2.mxf
    --audio-chan 1 -q 16 --pcm audio_2
)

# Transwrap to wave. This should accept the partial "50Hz frame" containing 960 instead of 1920 samples
set(create_command_2 ${BMXTRANSWRAP}
    --regtest
    -t wave
    -o ${output_file}
    test_pass1_2.mxf
)

run_test_a(
    "${TEST_MODE}"
    "${BMX_TEST_WITH_VALGRIND}"
    "${create_test_audio}"
    ""
    ""
    "${create_command_1}"
    "${create_command_2}"
    ""
    ""
    "${output_file}"
    "test_2.md5"
    ""
    ""
)
