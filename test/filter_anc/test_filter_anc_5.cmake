# Test passing through ANC data because other ANC data is stripped.

set(test_name 5)
include("${TEST_SOURCE_DIR}/test_common.cmake")

run_test("x" "--strip-anc;sdp" "${output_rdd6_file};test.xml.bin")
