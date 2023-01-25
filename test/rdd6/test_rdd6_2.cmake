# Test creating an MXF file with RDD6 from test2.xml and interlaced video.

set(test_name 2)
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

run_test(25 avci100_1080i "9,572" 4)
