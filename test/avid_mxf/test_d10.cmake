# Test creating an Avid MXF file containing D10 video and 2 PCM tracks

include("${TEST_SOURCE_DIR}/test_common.cmake")

set(tests
    d10_30 13 "x"
    d10_40 12 "x"
    d10_50 11 "x"
)

run_tests("${tests}" 3)
