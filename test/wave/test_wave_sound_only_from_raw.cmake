# Test creating a Wave file that should include all 5123 input samples.

set(test_name wave_sound_only_from_raw)
include("${TEST_SOURCE_DIR}/test_common.cmake")

set(create_command ${RAW2BMX}
    --regtest
    -t wave
    -o ${output_file}
    -q 16 --pcm audio_2_${test_name}
)

run_test_a(
    "${TEST_MODE}"
    "${BMX_TEST_WITH_VALGRIND}"
    "${create_test_audio_2}"
    ""
    ""
    "${create_command}"
    ""
    ""
    ""
    "${output_file}"
    "wave_sound_only_from_raw.md5"
    ""
    ""
)
