# Test creating an MXF file with essence descriptor options.

set(test_name desc_props_raw2bmx)
include("${TEST_SOURCE_DIR}/test_common.cmake")


set(create_command ${RAW2BMX}
    --regtest
    -t op1a
    -f 25
    -o ${output_file}
    --signal-std st428
    --frame-layout segmentedframe
    --transfer-ch bt2020
    --coding-eq gbr
    --color-prim dcdm
    --color-siting quincunx
    --black-level 65
    --white-level 938
    --color-range 899
    --avc_high_422_intra video_${test_name}
    -q 24 --locked true --pcm audio_${test_name}_1
    -q 24 --locked true --pcm audio_${test_name}_2
)

run_test_a(
    "${TEST_MODE}"
    "${BMX_TEST_WITH_VALGRIND}"
    "${create_test_audio_1}"
    "${create_test_audio_2}"
    "${create_test_video}"
    "${create_command}"
    ""
    ""
    ""
    "${output_file}"
    "${test_name}.md5"
    ""
    ""
)
