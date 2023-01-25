# Test transwrap from a clip-wrapped MXF to a Wave.

set(test_name wave_sound_only_transwrap)
include("${TEST_SOURCE_DIR}/test_common.cmake")

set(create_command_1 ${RAW2BMX}
    --regtest
    -t op1a
    --clip-wrap
    -o test_${test_name}.mxf
    -q 16 --pcm audio_2_${test_name}
)

set(create_command_2 ${BMXTRANSWRAP}
    --regtest
    -t wave
    -o ${output_file}
    test_${test_name}.mxf
)

run_test_a(
    "${TEST_MODE}"
    "${BMX_TEST_WITH_VALGRIND}"
    "${create_test_audio_2}"
    ""
    ""
    "${create_command_1}"
    "${create_command_2}"
    ""
    ""
    "${output_file}"
    "wave_sound_only_transwrap.md5"
    ""
    ""
)
