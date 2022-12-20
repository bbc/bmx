# Test creating and re-wrapping a Wave file.

set(test_name wave_read)
include("${TEST_SOURCE_DIR}/test_common.cmake")

set(create_command_1 ${RAW2BMX}
    --regtest
    -t wave
    -f 25
    -y 10:11:12:13
    --orig regtest
    -o test_1_${test_name}.wav
    -q 16 --pcm audio_1_${test_name}
    -q 16 --pcm audio_1_${test_name}
)

set(create_command_2 ${RAW2BMX}
    --regtest
    -t wave
    -f 25
    -y 10:11:12:13
    --orig regtest
    -o ${output_file}
    --wave test_1_${test_name}.wav
)

run_test_a(
    "${TEST_MODE}"
    "${BMX_TEST_WITH_VALGRIND}"
    "${create_test_audio_1}"
    ""
    ""
    "${create_command_1}"
    "${create_command_2}"
    ""
    ""
    "${output_file}"
    "wave_read.md5"
    ""
    ""
)
