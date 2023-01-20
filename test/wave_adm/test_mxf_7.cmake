# Create an MXF from wave_7.

set(test_name "mxf_7")
include("${TEST_SOURCE_DIR}/test_common.cmake")


if(TEST_MODE STREQUAL "check")
    set(input_wave_file test_wave_7.wav)
elseif(TEST_MODE STREQUAL "samples")
    set(input_wave_file ${BMX_TEST_SAMPLES_DIR}/test_wave_7.wav)
else()
    set(input_wave_file test_wave_7.wav)
endif()

set(create_command ${RAW2BMX}
    --regtest
    -t op1a
    -f 25
    -o ${output_file}
    --adm-wave-chunk axml,adm_itu2076
    --wave ${input_wave_file}
)

run_test_a(
    "${TEST_MODE}"
    "${BMX_TEST_WITH_VALGRIND}"
    ""
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
