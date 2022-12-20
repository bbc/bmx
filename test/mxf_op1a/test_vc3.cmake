# Test creating an MXF OP1a file containing VC3 video and 2 PCM tracks

include("${TEST_SOURCE_DIR}/test_common.cmake")

set(tests
    vc3_720p_1250 37 "x"
    vc3_720p_1251 38 "x"
    vc3_720p_1252 38 "x"
    vc3_1080i_1241 34 "x"
    vc3_1080i_1242 35 "x"
    vc3_1080i_1243 36 "x"
    vc3_1080p_1235 31 "x"
    vc3_1080p_1237 32 "x"
    vc3_1080p_1238 33 "x"
    vc3_1080p_1253 40 "x"
)

run_tests("${tests}" 3)
