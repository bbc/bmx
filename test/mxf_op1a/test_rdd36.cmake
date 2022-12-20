# Test creating an MXF OP1a file containing RDD 36 (ProRes) video and 2 PCM tracks

include("${TEST_SOURCE_DIR}/test_common.cmake")

set(tests
    rdd36_422 55 "x"
    rdd36_4444 56 "x"
)

run_tests("${tests}" 3)
