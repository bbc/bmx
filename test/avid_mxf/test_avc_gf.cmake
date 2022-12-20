# Test creating a growing Avid MXF file containing AVC video and 2 PCM tracks

include("${TEST_SOURCE_DIR}/test_common.cmake")

set(tests
    avci100_1080i_gf 7 "x"
    avci100_1080i_gfp 7 "x"
)

run_tests("${tests}" 3)
