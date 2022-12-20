# Test creating an AS-11 RDD9 MXF file containing MPEG-2 LG.

include("${TEST_SOURCE_DIR}/test_common.cmake")

run_test(rdd9_422p_1080i50 rdd9 mpeg2lg_422p_hl_1080i 14 14)
