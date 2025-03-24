# Test not passing through ANC data identified using the --strip-anc option.

set(test_name 6)
include("${TEST_SOURCE_DIR}/test_common.cmake")

run_test("x" "--strip-anc;st2020" "x")
