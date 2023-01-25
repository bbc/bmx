# Test creating an Avid MXF file containing uncompressed video and 2 PCM tracks

include("${TEST_SOURCE_DIR}/test_common.cmake")

set(tests
    unc_720p 20 50
    unc_1080i 18 25
    unc_1080p 19 25
    unc 17 25
)

run_tests("${tests}" 3)
