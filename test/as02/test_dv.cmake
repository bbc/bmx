# Test creating an AS02 MXF file containing DV video and 2 PCM tracks

include("${TEST_SOURCE_DIR}/test_common.cmake")

set(tests
    dv50 4 "x"
    dv100_720p 6 25
    dv100_1080i 5 "x"
    dvbased25 3 "x"
    iecdv25 2 "x"
)

run_tests("${tests}" 3)
