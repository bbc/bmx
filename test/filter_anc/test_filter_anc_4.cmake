# Test not passing through any ANC data because --pass-anc only lists ANC types that are not present.

set(test_name 4)
include("${TEST_SOURCE_DIR}/test_common.cmake")

run_test("--pass-anc;sdp" "x" "x")
