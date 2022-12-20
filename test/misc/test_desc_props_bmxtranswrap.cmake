# Test creating an MXF file with essence descriptor options, then transwrapped with more options used.

set(test_name desc_props_bmxtranswrap)
include("${TEST_SOURCE_DIR}/test_common.cmake")


set(create_command_1 ${RAW2BMX}
    --regtest
    -t op1a
    -f 25
    -o test_pass1_${test_name}.mxf
    --frame-layout segmentedframe
    --black-level 65
    --white-level 938
    --color-range 899
    --avc_high_422_intra video_${test_name}
    -q 24 --locked true --pcm audio_${test_name}_1
    -q 24 --locked true --pcm audio_${test_name}_2
)

set(create_command_2 ${BMXTRANSWRAP}
    --regtest
    -t op1a
    -o ${output_file}
    --signal-std st428
    --transfer-ch bt2020
    --coding-eq gbr
    --color-prim dcdm
    --color-siting quincunx
    test_pass1_${test_name}.mxf
)

run_test_a(
    "${TEST_MODE}"
    "${BMX_TEST_WITH_VALGRIND}"
    "${create_test_audio_1}"
    "${create_test_audio_2}"
    "${create_test_video}"
    "${create_command_1}"
    "${create_command_2}"
    ""
    ""
    "${output_file}"
    "${test_name}.md5"
    ""
    ""
)
