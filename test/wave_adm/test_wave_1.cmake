# Create a Wave file from mxf_1 and map the channels back
# into the original order as in adm_1.wav, and double-check it equals adm_1.wave.

set(test_name "wave_1")
include("${TEST_SOURCE_DIR}/test_common.cmake")


if(TEST_MODE STREQUAL "check")
    set(input_mxf_file test_mxf_1.mxf)
elseif(TEST_MODE STREQUAL "samples")
    set(input_mxf_file ${BMX_TEST_SAMPLES_DIR}/test_mxf_1.mxf)
else()
    set(input_mxf_file test_mxf_1.mxf)
endif()

set(create_command ${BMXTRANSWRAP}
    --regtest
    -t wave
    -o ${output_file}
    --track-map "2,3,0,1"
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
    -E compare_files ${output_file} ${TEST_SOURCE_DIR}/adm_1.wav
    RESULT_VARIABLE compare_result
)
if(NOT compare_result EQUAL 0)
    message(FATAL_ERROR "Output file '${output_file}' differs from original source '${TEST_SOURCE_DIR}/adm_1.wav'")
endif()
