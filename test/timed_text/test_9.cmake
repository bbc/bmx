# Test creating and transwrapping an MXF file containing video and timed text.

set(test_num 9)
include("${TEST_SOURCE_DIR}/test_common.cmake")

set(create_command_1 ${RAW2BMX}
    --regtest
    -t op1a
    -f 25
    -y 10:00:00:00
    --dur 100
    -o test_input_${test_num}.mxf
    --tt "${TEST_SOURCE_DIR}/manifest_1.txt"
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
    ""
    ""
    ""
    "${create_command_1}"
    "${create_command_2}"
    ""
    "${read_command}"
    "${output_file}"
    "test_9.md5"
    "${output_info_file};info_9.xml.bin"
    "${output_essence_file_prefix}_d0.xml;text_example.xml.bin"
)
