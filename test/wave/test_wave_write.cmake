# Test creating a Wave file.

set(test_name wave_write)
include("${TEST_SOURCE_DIR}/test_common.cmake")

set(create_command ${RAW2BMX}
    --regtest
    -t wave
    -f 25
    -y 10:11:12:13
    --orig regtest
    -o ${output_file}
    -q 16 --pcm audio_1_${test_name}
    -q 16 --pcm audio_1_${test_name}
)

run_test_a(
    "${TEST_MODE}"
    "${BMX_TEST_WITH_VALGRIND}"
    "${create_test_audio_1}"
    ""
    ""
    "${create_command}"
    ""
    ""
    ""
    "${output_file}"
    "wave_write.md5"
    ""
    ""
)
