# Test creating an MXF file containing video with precharge and timed text.
# The timed text is set to start 1 frame after the start of the video after
# 4 frames of precharge. The timed text manifest file sets the "start"
# to 10:00:00:00.

set(test_num 7)
include("${TEST_SOURCE_DIR}/test_common.cmake")

set(create_command ${RAW2BMX}
    --regtest
    -t op1a
    -f 25
    -y 09:59:59:24
    --out-start 4
    -o ${output_file}
    --tt "${TEST_SOURCE_DIR}/manifest_3.txt"
    --mpeg2lg_422p_hl_1080i video_2_${test_num}
)

run_test_a(
    "${TEST_MODE}"
    "${BMX_TEST_WITH_VALGRIND}"
    "${create_test_video_2}"
    ""
    ""
    "${create_command}"
    ""
    ""
    "${read_command}"
    "${output_file}"
    "test_7.md5"
    "${output_info_file};info_7.xml.bin"
    ""
)
