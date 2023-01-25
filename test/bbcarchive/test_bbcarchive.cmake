# Test reading and writing BBC Archive Preservation project MXF files.

include("${TEST_SOURCE_DIR}/../testing.cmake")


if(TEST_MODE STREQUAL "check")
    set(output_file test.mxf)
    set(output_info_file test.xml)
elseif(TEST_MODE STREQUAL "samples")
    file(MAKE_DIRECTORY ${BMX_TEST_SAMPLES_DIR})

    set(output_file ${BMX_TEST_SAMPLES_DIR}/test_bbcarchive_1080_25i.mxf)
    set(output_info_file ${BMX_TEST_SAMPLES_DIR}/test_bbcarchive_1080_25i.xml.bin)
else()
    set(output_file test.mxf)
    set(output_info_file ${TEST_SOURCE_DIR}/bbcarchive_1080_25i.xml.bin)
endif()

set(create_command ${TEST_WRITE_ARCHIVE_MXF}
    --regtest
    --num-audio 6
    --format 1080i25
    --16by9
    --crc32
    3
    ${output_file}
)

set(read_command ${MXF2RAW}
    --regtest
    --app
    --app-events dptv
    --info-file ${output_info_file}
    ${output_file}
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
    "bbcarchive_1080_25i.md5"
    "${output_info_file};bbcarchive_1080_25i.xml.bin"
    ""
)
