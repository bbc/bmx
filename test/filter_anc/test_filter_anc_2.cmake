# Test passing through all ANC data using --pass-anc all.

set(test_name 2)
include("${TEST_SOURCE_DIR}/test_common.cmake")

run_test("--pass-anc;all" "x" "${output_rdd6_file};test.xml.bin")
