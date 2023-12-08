# Create an MXF from a Wave+ADM and AVC-Intra,
# but don't include <chna> or any other Wave chunks.

set(test_name "mxf_10")
include("${TEST_SOURCE_DIR}/test_common.cmake")


set(create_command ${RAW2BMX}
    --regtest
    -t op1a
    -f 25
    -o ${output_file}
    --track-map "2,3\;0,1"
    --avci100_1080i video_1_${test_name}
    --no-chna-chunk
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
