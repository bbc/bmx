# Test creating an MXF file containing video, timed text, a font resource
# and image resource.

set(test_num 4)
include("${TEST_SOURCE_DIR}/test_common.cmake")

set(create_command ${RAW2BMX}
    --regtest
    -t op1a
    -f 25
    -y 10:00:00:00
    -o ${output_file}
    --tt "${TEST_SOURCE_DIR}/manifest_2.txt"
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
    "test_4.md5"
    "${output_info_file};info_4.xml.bin"
    "${output_essence_file_prefix}_d0.xml;text_example.xml.bin;${output_essence_file_prefix}_d0_12.raw;font.ttf;${output_essence_file_prefix}_d0_13.raw;image.png"
)
