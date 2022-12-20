# Test creating an IMF Track File containing Timed Text.
# First test create an IMF audio Track File.
# Second test transwraps the IMF audio Track File.

include("${TEST_SOURCE_DIR}/../testing.cmake")


if(TEST_MODE STREQUAL "check")
    set(output_file_1 test_1.mxf)
elseif(TEST_MODE STREQUAL "samples")
    file(MAKE_DIRECTORY ${BMX_TEST_SAMPLES_DIR})

    set(output_file_1 ${BMX_TEST_SAMPLES_DIR}/test_timed_text_1.mxf)
else()
    set(output_file_1 test_1.mxf)
endif()

set(create_command ${RAW2BMX}
    --regtest
    -t imf
    -o ${output_file_1}
    --clip test
    -f 25
    --dur 100
    --tt "${TEST_SOURCE_DIR}/../timed_text/manifest_2.txt"
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
    "test_timed_text_1.md5"
    ""
    ""
)


if(TEST_MODE STREQUAL "check")
    set(output_file test.mxf)
elseif(TEST_MODE STREQUAL "samples")
    set(output_file ${BMX_TEST_SAMPLES_DIR}/test_timed_text_2.mxf)
else()
    set(output_file test.mxf)
endif()

set(create_command ${BMXTRANSWRAP}
    --regtest
    -t imf
    -o ${output_file}
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
    "${read_command}"
    "${output_file}"
    "test_timed_text_2.md5"
    ""
    ""
)
