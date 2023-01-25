# Test creating an MXF OP1a file containing AVC video and 2 PCM tracks

include("${TEST_SOURCE_DIR}/test_common.cmake")

set(tests
    avc_high_422_intra 8 "x"
    avci50_720p 30 25
    avci50_720p 30 50
    avci50_1080i 9 "x"
    avci50_1080p 10 25
    avci50_1080p 10 50
    avci100_720p 29 25
    avci100_720p 29 50
    avci100_1080i 7 "x"
    avci100_1080p 8 25
    avci100_1080p 8 50
    avci200_720p 48 50
    avci200_1080i 46 "x"
    avci200_1080p 47 50
)

run_tests("${tests}" 3)
