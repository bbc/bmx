# Test creating an MXF file containing timed text only.

set(test_num 2)
include("${TEST_SOURCE_DIR}/test_common.cmake")

set(create_command ${RAW2BMX}
    --regtest
    -t op1a
    -f 25
    -y 10:00:00:00
    --dur 100
    -o ${output_file}
    --tt "${TEST_SOURCE_DIR}/manifest_1.txt"
)

run_test_a(
    "${TEST_MODE}"
    "${BMX_TEST_WITH_VALGRIND}"
    ""
    ""
    ""
    "${create_command}"
    ""
    ""
    "${read_command}"
    "${output_file}"
    "test_2.md5"
    "${output_info_file};info_2.xml.bin"
    "${output_essence_file_prefix}_d0.xml;text_example.xml.bin"
)
