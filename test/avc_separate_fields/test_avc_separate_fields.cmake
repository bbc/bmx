# Test creating and transwrapping separate field encoded AVC

include("${TEST_SOURCE_DIR}/../testing.cmake")


if(TEST_MODE STREQUAL "check")
    set(output_file_1 test_1.mxf)
elseif(TEST_MODE STREQUAL "samples")
    file(MAKE_DIRECTORY ${BMX_TEST_SAMPLES_DIR})

    set(output_file_1 ${BMX_TEST_SAMPLES_DIR}/test_avc_separate_fields_1.mxf)
else()
    set(output_file_1 test_1.mxf)
endif()

set(create_command ${RAW2BMX}
    --regtest-real
    -t op1a
    -o ${output_file_1}
    --clip test
    --avc "${TEST_SOURCE_DIR}/separate_fields_gop.h264"
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
    "${output_file_1}"
    "test_avc_separate_fields_1.md5"
    ""
    ""
)


if(TEST_MODE STREQUAL "check")
    set(output_file_2 test.mxf)
elseif(TEST_MODE STREQUAL "samples")
    set(output_file_2 ${BMX_TEST_SAMPLES_DIR}/test_avc_separate_fields_2.mxf)
else()
    set(output_file_2 test.mxf)
endif()

set(create_command ${BMXTRANSWRAP}
    --regtest
    -t op1a
    -o ${output_file_2}
    --clip test
    ${output_file_1}
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
    "${output_file_2}"
    "test_avc_separate_fields_2.md5"
    ""
    ""
)
