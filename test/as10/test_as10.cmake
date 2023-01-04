# Test creating and transwrapping an AS-10 MXF file.

include("${TEST_SOURCE_DIR}/../testing.cmake")


set(create_test_audio ${CREATE_TEST_ESSENCE}
    -t 42
    -d 36
    audio
)

set(create_test_video ${CREATE_TEST_ESSENCE}
    -t 53
    -d 36
    video
)


if(TEST_MODE STREQUAL "check")
    set(output_file_1 test_1.mxf)
    set(output_info_file_1 test_1.xml)
elseif(TEST_MODE STREQUAL "samples")
    file(MAKE_DIRECTORY ${BMX_TEST_SAMPLES_DIR})

    set(output_file_1 ${BMX_TEST_SAMPLES_DIR}/test_high_hd_2014_rb.mxf)
    set(output_info_file_1 ${BMX_TEST_SAMPLES_DIR}/test_high_hd_2014_rb.xml.bin)
else()
    set(output_file_1 test_1.mxf)
    set(output_info_file_1 ${TEST_SOURCE_DIR}/high_hd_2014_rb.xml.bin)
endif()

set(create_command ${RAW2BMX}
    --regtest
    -t as10
    -f 25
    -y 22:22:22:22
    --single-pass
    --part 25
    -o ${output_file_1}
    --dm-file as10 ${TEST_SOURCE_DIR}/as10_core_framework.txt
    --shim-name high_hd_2014
    --mpeg-checks
    --print-checks
    --mpeg2lg_422p_hl_1080i video
    -q 24 --locked true --pcm audio
    -q 24 --locked true --pcm audio
    -q 24 --locked true --pcm audio
    -q 24 --locked true --pcm audio
    -q 24 --locked true --pcm audio
    -q 24 --locked true --pcm audio
    -q 24 --locked true --pcm audio
    -q 24 --locked true --pcm audio
)

set(read_command ${MXF2RAW}
    --regtest
    --info
    --info-format xml
    --info-file ${output_info_file_1}
    --as10
    ${output_file_1}
)

run_test_a(
    "${TEST_MODE}"
    "${BMX_TEST_WITH_VALGRIND}"
    "${create_test_audio}"
    "${create_test_video}"
    ""
    "${create_command}"
    ""
    ""
    "${read_command}"
    "${output_file_1}"
    "high_hd_2014_rb.md5"
    "${output_info_file_1};high_hd_2014_rb.xml.bin"
    ""
)


if(TEST_MODE STREQUAL "check")
    set(output_file_2 test_2.mxf)
    set(output_info_file_2 test_2.xml)
elseif(TEST_MODE STREQUAL "samples")
    file(MAKE_DIRECTORY ${BMX_TEST_SAMPLES_DIR})

    set(output_file_2 ${BMX_TEST_SAMPLES_DIR}/test_high_hd_2014_tw.mxf)
    set(output_info_file_2 ${BMX_TEST_SAMPLES_DIR}/test_high_hd_2014_tw.xml.bin)
else()
    set(output_file_2 test_2.mxf)
    set(output_info_file_2 ${TEST_SOURCE_DIR}/high_hd_2014_tw.xml.bin)
endif()

set(create_command ${BMXTRANSWRAP}
    --regtest
    -t as10
    -o ${output_file_2}
    --dm-file as10 ${TEST_SOURCE_DIR}/as10_core_framework.txt
    --shim-name high_hd_2014
    --mpeg-checks
    --max-same-warnings 2
    --print-checks
    ${output_file_1}
)

set(read_command ${MXF2RAW}
    --regtest
    --info
    --info-format xml
    --info-file ${output_info_file_2}
    --as10
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
    "high_hd_2014_tw.md5"
    "${output_info_file_2};high_hd_2014_tw.xml.bin"
    ""
)
