# Test creating a growing Avid MXF file containing Avid Alpha and 2 PCM tracks

include("${TEST_SOURCE_DIR}/test_common.cmake")

set(tests
    avid_alpha_1080i 41 25
)

run_tests("${tests}" 3)
