# Test creating an MXF file containing timed text only. The timed text is set
# to start 25 frames from the start. The timed text manifest file sets the
# "start" to 10:00:00:00.

set(test_num 6)
include("${TEST_SOURCE_DIR}/test_common.cmake")

set(create_command ${RAW2BMX}
    --regtest
    -t op1a
    -f 25
    -y 09:59:59:00
    --dur 100
    -o ${output_file}
    --tt "${TEST_SOURCE_DIR}/manifest_3.txt"
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
    "test_6.md5"
    "${output_info_file};info_6.xml.bin"
    ""
)
