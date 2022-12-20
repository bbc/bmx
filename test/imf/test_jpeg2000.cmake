# Test creating an IMF Track File containing JPEG 2000.
# First test create an IMF audio Track File.
# Second test transwraps the IMF audio Track File.

include("${TEST_SOURCE_DIR}/../testing.cmake")


if(TEST_MODE STREQUAL "check")
    set(output_file_1 test_jpeg2000_1.mxf)
elseif(TEST_MODE STREQUAL "samples")
    file(MAKE_DIRECTORY ${BMX_TEST_SAMPLES_DIR})

    set(output_file_1 ${BMX_TEST_SAMPLES_DIR}/test_jpeg2000_1.mxf)
else()
    set(output_file_1 test_jpeg2000_1.mxf)
endif()

set(create_command ${RAW2BMX}
    --regtest
    -t imf
    -o ${output_file_1}
    --clip test
    -f 25
    -a 16:9
    --frame-layout fullframe
    --transfer-ch hlg
    --coding-eq bt2020
    --color-prim bt2020
    --color-siting cositing
    --black-level 64
    --white-level 940
    --color-range 897
    --active-width 220
    --active-height 115
    --active-x-offset 10
    --active-y-offset 10
    --display-primaries 35400,14600,8500,39850,6550,2300
    --display-white-point 15635,16450
    --display-max-luma 10000000
    --display-min-luma 50
    --center-cut-14-9
    --fill-pattern-gaps
    --j2c_cdci "${TEST_SOURCE_DIR}/../jpeg2000/image_yuv_%d.j2c"
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
    "test_jpeg2000_1.md5"
    ""
    ""
)


if(TEST_MODE STREQUAL "check")
    set(output_file test_jpeg2000_2.mxf)
elseif(TEST_MODE STREQUAL "samples")
    set(output_file ${BMX_TEST_SAMPLES_DIR}/test_jpeg2000_2.mxf)
else()
    set(output_file test_jpeg2000_2.mxf)
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
    "test_jpeg2000_2.md5"
    ""
    ""
)
