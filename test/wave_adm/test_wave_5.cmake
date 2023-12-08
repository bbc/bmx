# Create a Wave file from the sample Wave+ADM file and double-check it equals adm_1.wave.

set(test_name "wave_5")
include("${TEST_SOURCE_DIR}/test_common.cmake")


set(create_command ${RAW2BMX}
    --regtest
    -t wave
    -o ${output_file}
    --adm-wave-chunk axml,adm_itu2076
    --wave ${TEST_SOURCE_DIR}/adm_1.wav
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
