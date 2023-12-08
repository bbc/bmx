# Create an MXF file from a Wave+ADM and AVC-Intra.
# Map only audio 2 channels to a stereo track, with the channels in reverse order.

set(test_name "mxf_2")
include("${TEST_SOURCE_DIR}/test_common.cmake")


set(create_command ${RAW2BMX}
    --regtest
    -t op1a
    -f 25
    -o ${output_file}
    --track-map "3,2"
    --avci100_1080i video_1_${test_name}
    --adm-wave-chunk axml,adm_itu2076
    --wave ${TEST_SOURCE_DIR}/adm_1.wav
)

run_test_a(
    "${TEST_MODE}"
    "${BMX_TEST_WITH_VALGRIND}"
    "${create_test_video_1}"
    ""
    ""
    "${create_command}"
    ""
    ""
    ""
    "${output_file}"
    "test_${test_name}.md5"
    ""
    ""
)
