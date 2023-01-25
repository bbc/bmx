# Test creating an MXF file from video with pre-charge and filler required for the audio.

include("${TEST_SOURCE_DIR}/../testing.cmake")

if(TEST_MODE STREQUAL "check")
    set(output_file test_1.mxf)
elseif(TEST_MODE STREQUAL "samples")
    file(MAKE_DIRECTORY ${BMX_TEST_SAMPLES_DIR})

    set(output_file ${BMX_TEST_SAMPLES_DIR}/test_1.mxf)
else()
    set(output_file test_1.mxf)
endif()


# 16-bit PCM
set(create_test_audio ${CREATE_TEST_ESSENCE}
    -t 1
    -d 12
    audio_1
)

# MPEG2 LG
set(create_test_video ${CREATE_TEST_ESSENCE}
    -t 24
    -d 12
    video_1
)

# Wrap in MXF
set(create_command_1 ${RAW2BMX}
    --regtest
    -t op1a
    -f 25
    -o test_pass1_1.mxf
    --audio-chan 1 -q 16 --pcm audio_1
    --mpeg2lg_422p_hl_1080i video_1
)

# Transwrap to Avid with 1 frame start offset, resulting in video with 1 frame pre-charge
set(create_command_2 ${BMXTRANSWRAP}
    --regtest
    -t avid
    -o test_pass2_1
    --start 1
    test_pass1_1.mxf
)

# Transwrap both Avid files to OP1a, which should add 1 frame of PCM padding for the pre-charge
set(create_command_3 ${BMXTRANSWRAP}
    --regtest
    -t op1a
    -o ${output_file}
    --clip test
    -y 23:58:30:00
    test_pass2_1_a1.mxf
    test_pass2_1_v1.mxf
)

run_test_a(
    "${TEST_MODE}"
    "${BMX_TEST_WITH_VALGRIND}"
    "${create_test_audio}"
    "${create_test_video}"
    ""
    "${create_command_1}"
    "${create_command_2}"
    "${create_command_3}"
    ""
    "${output_file}"
    "test_1.md5"
    ""
    ""
)
