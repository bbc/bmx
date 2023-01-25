# Test creating and then re-wrapping an MXF file containing video, timed text,
# a font resource and image resource.

set(test_num 8)
include("${TEST_SOURCE_DIR}/test_common.cmake")

set(create_command_1 ${RAW2BMX}
    --regtest
    -t op1a
    -f 25
    -y 10:00:00:00
    -o test_input_${test_num}.mxf
    --tt "${TEST_SOURCE_DIR}/manifest_2.txt"
    --avci100_1080i video_1_${test_num}
)

set(create_command_2 ${BMXTRANSWRAP}
    --regtest
    -t op1a
    -o ${output_file}
    test_input_${test_num}.mxf
)

run_test_a(
    "${TEST_MODE}"
    "${BMX_TEST_WITH_VALGRIND}"
    "${create_test_video_1}"
    ""
    ""
    "${create_command_1}"
    "${create_command_2}"
    ""
    "${read_command}"
    "${output_file}"
    "test_4.md5"
    "${output_info_file};info_4.xml.bin"
    "${output_essence_file_prefix}_d0.xml;text_example.xml.bin;${output_essence_file_prefix}_d0_12.raw;font.ttf;${output_essence_file_prefix}_d0_13.raw;image.png"
)
