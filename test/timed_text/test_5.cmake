# Test creating an MXF file containing video and timed text. The timed text
# is set to start 1 frame from the start of the video. The timed text manifest
# sets "start" to 10:00:00:00.

set(test_num 5)
include("${TEST_SOURCE_DIR}/test_common.cmake")

set(create_command ${RAW2BMX}
    --regtest
    -t op1a
    -f 25
    -y 09:59:59:24
    -o ${output_file}
    --tt "${TEST_SOURCE_DIR}/manifest_3.txt"
    --avci100_1080i video_1_${test_num}
)

run_test_a(
    "${TEST_MODE}"
    "${BMX_TEST_WITH_VALGRIND}"
    "${create_test_video_1}"
    ""
    ""
    "${create_command}"
    ""
    ""
    "${read_command}"
    "${output_file}"
    "test_5.md5"
    "${output_info_file};info_5.xml.bin"
    ""
)
