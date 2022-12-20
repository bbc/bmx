# Test creating an MXF file with RDD6 from test3.xml on line 0 and progressive video.

set(test_name 3)
include("${TEST_SOURCE_DIR}/test_common.cmake")


# 24-bit PCM
set(create_test_audio ${CREATE_TEST_ESSENCE}
    -t 42
    -d 3
    audio_${test_name}
)

# Unc 3840
set(create_test_video ${CREATE_TEST_ESSENCE}
    -t 45
    -d 3
    video_${test_name}
)

run_test(50 unc_3840 "0" 4)
