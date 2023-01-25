# Test creating an AS-11 MXF OP1a file containing AVCI.

include("${TEST_SOURCE_DIR}/test_common.cmake")

run_test(avci100_1080i50 op1a avci100_1080i 7 3)
