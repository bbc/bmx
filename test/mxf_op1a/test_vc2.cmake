# Test creating an MXF OP1a file containing VC2 video and 2 PCM tracks

include("${TEST_SOURCE_DIR}/test_common.cmake")

set(tests
    vc2 54 "x"
)

run_tests("${tests}" 3)
