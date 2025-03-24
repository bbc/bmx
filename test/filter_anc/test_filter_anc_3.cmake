# Test passing through one type of ANC data identified using --pass-anc.

set(test_name 3)
include("${TEST_SOURCE_DIR}/test_common.cmake")

run_test("--pass-anc;st2020" "x" "${output_rdd6_file};test.xml.bin")
