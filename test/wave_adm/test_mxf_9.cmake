# Create mono track MXF from wave_3.

set(test_name "mxf_9")
include("${TEST_SOURCE_DIR}/test_common.cmake")


if(TEST_MODE STREQUAL "check")
    set(input_wave_file test_wave_3.wav)
elseif(TEST_MODE STREQUAL "samples")
    set(input_wave_file ${BMX_TEST_SAMPLES_DIR}/test_wave_3.wav)
else()
    set(input_wave_file test_wave_3.wav)
endif()

set(create_command ${RAW2BMX}
    --regtest
    -t op1a
    -f 25
    -o ${output_file}
    --track-map mono
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
