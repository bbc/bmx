# Test creating an AS02 MXF file containing uncompressed video and 2 PCM tracks

include("${TEST_SOURCE_DIR}/test_common.cmake")

set(tests
    unc_720p 20 "x"
    unc_1080i 18 "x"
    unc_1080p 19 "x"
    unc_3840 45 "x"
    unc 17 "x"
)

run_tests("${tests}" 3)
