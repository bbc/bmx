# Test creating an IMF Track File containing RDD 36.
# First test create an IMF audio Track File.
# Second test transwraps the IMF audio Track File.

include("${TEST_SOURCE_DIR}/../testing.cmake")


if(TEST_MODE STREQUAL "check")
    set(output_file_1 test_rdd36_1.mxf)
elseif(TEST_MODE STREQUAL "samples")
    file(MAKE_DIRECTORY ${BMX_TEST_SAMPLES_DIR})

    set(output_file_1 ${BMX_TEST_SAMPLES_DIR}/test_rdd36_1.mxf)
else()
    set(output_file_1 test_rdd36_1.mxf)
endif()

set(create_test_video ${CREATE_TEST_ESSENCE}
    -t 55
    -d 3
    video_rdd36
)

set(create_command ${RAW2BMX}
    --regtest
    -t imf
    -o ${output_file_1}
    --clip test
    --active-width 1910
    --active-height 530
    --active-x-offset 10
    --active-y-offset 10
    --display-primaries 35400,14600,8500,39850,6550,2300
    --display-white-point 15635,16450
    --display-max-luma 10000000
    --display-min-luma 50
    --center-cut-4-3
    --rdd36_422 video_rdd36
)

run_test_a(
    "${TEST_MODE}"
    "${BMX_TEST_WITH_VALGRIND}"
    "${create_test_video}"
    ""
    ""
    "${create_command}"
    ""
    ""
    ""
    "${output_file_1}"
    "test_rdd36_1.md5"
    ""
    ""
)


if(TEST_MODE STREQUAL "check")
    set(output_file test_rdd36_2.mxf)
elseif(TEST_MODE STREQUAL "samples")
    set(output_file ${BMX_TEST_SAMPLES_DIR}/test_rdd36_2.mxf)
else()
    set(output_file test_rdd36_2.mxf)
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
    "test_rdd36_2.md5"
    ""
    ""
)
