# Test creating an MXF RDD 9 file with RDD 6 from test1.xml and interlaced video.

set(test_name 4)
include("${TEST_SOURCE_DIR}/test_common.cmake")


# 24-bit PCM
set(create_test_audio ${CREATE_TEST_ESSENCE}
    -t 42
    -d 24
    audio_${test_name}
)

# MPEG-2 LG
set(create_test_video ${CREATE_TEST_ESSENCE}
    -t 14
    -d 24
    video_${test_name}
)

run_test(rdd9 25 mpeg2lg_422p_hl_1080i "9,572" 4)
