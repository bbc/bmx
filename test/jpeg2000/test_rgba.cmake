# Test creating and transwrapping an MXF file with JPEG 2000 RGB essence.

include("${TEST_SOURCE_DIR}/../testing.cmake")


if(TEST_MODE STREQUAL "samples")
    file(MAKE_DIRECTORY ${BMX_TEST_SAMPLES_DIR})
endif()


if(TEST_MODE STREQUAL "check")
    set(output_file_3 test_3.mxf)
    set(output_info_file_3 info_3.xml)
elseif(TEST_MODE STREQUAL "samples")
    set(output_file_3 ${BMX_TEST_SAMPLES_DIR}/test_3.mxf)
    set(output_info_file_3 ${BMX_TEST_SAMPLES_DIR}/info_3.xml.bin)
else()
    set(output_file_3 test_3.mxf)
    set(output_info_file_3 ${TEST_SOURCE_DIR}/info_3.xml.bin)
endif()

set(create_command ${RAW2BMX}
    --regtest
    -t imf
    -o ${output_file_3}
    --clip test
    -f 25
    -a 16:9
    --frame-layout fullframe
    --transfer-ch hlg
    --coding-eq bt2020
    --color-prim bt2020
    --comp-max-ref 940
    --comp-min-ref 64
    --scan-dir 0
    --display-primaries 35400,14600,8500,39850,6550,2300
    --display-white-point 15635,16450
    --display-max-luma 10000000
    --display-min-luma 50
    --fill-pattern-gaps
    --j2c_rgba "${TEST_SOURCE_DIR}/image_rgb_%d.j2c"
)

set(read_command ${MXF2RAW}
    --regtest
    --info
    --info-format xml
    --info-file ${output_info_file_3}
    ${output_file_3}
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
    "${output_file_3}"
    "test_3.md5"
    "${output_info_file_3};info_3.xml.bin"
    ""
)


if(TEST_MODE STREQUAL "check")
    set(output_file_4 test_4.mxf)
    set(output_info_file_4 info_4.xml)
elseif(TEST_MODE STREQUAL "samples")
    set(output_file_4 ${BMX_TEST_SAMPLES_DIR}/test_4.mxf)
    set(output_info_file_4 ${BMX_TEST_SAMPLES_DIR}/info_4.xml.bin)
else()
    set(output_file_4 test_4.mxf)
    set(output_info_file_4 ${TEST_SOURCE_DIR}/info_4.xml.bin)
endif()

set(create_command ${BMXTRANSWRAP}
    --regtest
    -t imf
    -o ${output_file_4}
    --clip test
    ${output_file_3}
)

set(read_command ${MXF2RAW}
    --regtest
    --info
    --info-format xml
    --info-file ${output_info_file_4}
    ${output_file_4}
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
    "${output_file_4}"
    "test_4.md5"
    "${output_info_file_4};info_4.xml.bin"
    ""
)
