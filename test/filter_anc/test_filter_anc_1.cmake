# Test not passing through any ANC data because neither --pass-anc or --strip-anc are used.

set(test_name 1)
include("${TEST_SOURCE_DIR}/test_common.cmake")

run_test("x" "x" "x")
