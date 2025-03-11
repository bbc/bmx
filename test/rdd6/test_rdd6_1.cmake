# Test creating an MXF OP1a file with RDD 6 from test1.xml and interlaced video.

set(test_name 1)
include("${TEST_SOURCE_DIR}/test_common.cmake")


# 24-bit PCM
set(create_test_audio ${CREATE_TEST_ESSENCE}
    -t 42
    -d 3
    audio_${test_name}
)

# AVCI
set(create_test_video ${CREATE_TEST_ESSENCE}
    -t 7
    -d 3
    video_${test_name}
)

run_test(op1a 25 avci100_1080i "9,572" 4)
