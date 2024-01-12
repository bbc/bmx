# Create a Wave file from mxf_8 and double-check it equals wave 3

set(test_name "wave_9")
include("${TEST_SOURCE_DIR}/test_common.cmake")


if(TEST_MODE STREQUAL "check")
    set(input_mxf_file test_mxf_8.mxf)
    set(compare_file test_wave_3.wav)
elseif(TEST_MODE STREQUAL "samples")
    set(input_mxf_file ${BMX_TEST_SAMPLES_DIR}/test_mxf_8.mxf)
    set(compare_file ${BMX_TEST_SAMPLES_DIR}/test_wave_3.wav)
else()
    set(input_mxf_file test_mxf_8.mxf)
    set(compare_file test_wave_3.wav)
endif()

set(create_command ${BMXTRANSWRAP}
    --regtest
    -t wave
    -o ${output_file}
    ${input_mxf_file}
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

execute_process(COMMAND ${CMAKE_COMMAND}
    -E compare_files ${output_file} ${compare_file}
    RESULT_VARIABLE compare_result
)
if(NOT compare_result EQUAL 0)
    message(FATAL_ERROR "Output file '${output_file}' differs from original source '${compare_file}'")
endif()
