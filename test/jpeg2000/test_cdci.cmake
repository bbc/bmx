# Test creating and transwrapping an MXF file with JPEG 2000 CDCI essence.

include("${TEST_SOURCE_DIR}/../testing.cmake")


if(TEST_MODE STREQUAL "samples")
    file(MAKE_DIRECTORY ${BMX_TEST_SAMPLES_DIR})
endif()


if(TEST_MODE STREQUAL "check")
    set(output_file_1 test_1.mxf)
    set(output_info_file_1 info_1.xml)
elseif(TEST_MODE STREQUAL "samples")
    set(output_file_1 ${BMX_TEST_SAMPLES_DIR}/test_1.mxf)
    set(output_info_file_1 ${BMX_TEST_SAMPLES_DIR}/info_1.xml)
else()
    set(output_file_1 test_1.mxf)
    set(output_info_file_1 ${TEST_SOURCE_DIR}/info_1.xml.bin)
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
    --display-primaries 35400,14600,8500,39850,6550,2300
    --display-white-point 15635,16450
    --display-max-luma 10000000
    --display-min-luma 50
    --fill-pattern-gaps
    --j2c_cdci "${TEST_SOURCE_DIR}/image_yuv_%d.j2c"
)

set(read_command ${MXF2RAW}
    --regtest
    --info
    --info-format xml
    --info-file ${output_info_file_1}
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
    "${output_file_1}"
    "test_1.md5"
    "${output_info_file_1};info_1.xml.bin"
    ""
)


if(TEST_MODE STREQUAL "check")
    set(output_file_2 test_2.mxf)
    set(output_info_file_2 info_2.xml)
elseif(TEST_MODE STREQUAL "samples")
    set(output_file_2 ${BMX_TEST_SAMPLES_DIR}/test_2.mxf)
    set(output_info_file_2 ${BMX_TEST_SAMPLES_DIR}/info_2.xml.bin)
else()
    set(output_file_2 test_2.mxf)
    set(output_info_file_2 ${TEST_SOURCE_DIR}/info_2.xml.bin)
endif()

set(create_command ${BMXTRANSWRAP}
    --regtest
    -t imf
    -o ${output_file_2}
    --clip test
    ${output_file_1}
)

set(read_command ${MXF2RAW}
    --regtest
    --info
    --info-format xml
    --info-file ${output_info_file_2}
    ${output_file_2}
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
    "${output_file_2}"
    "test_2.md5"
    "${output_info_file_2};info_2.xml.bin"
    ""
)
