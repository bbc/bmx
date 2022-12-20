# Test creating an MXF OP1a file containing AVC video and 2 PCM tracks

include("${TEST_SOURCE_DIR}/test_common.cmake")

set(tests
    mpeg2lg_422p_hl_1080i 14 "x"
    mpeg2lg_mp_h14_1080i 15 "x"
    mpeg2lg_mp_hl_1920_1080i 16 "x"
)

run_tests("${tests}" 24)
