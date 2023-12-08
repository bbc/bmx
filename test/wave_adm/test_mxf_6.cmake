# Create an MXF file from a Wave+ADM and AVC-Intra.
# Map the audio channels to mono, mono and stereo tracks.
# Add ADM MCA labels from mca_2.txt.

set(test_name "mxf_6")
include("${TEST_SOURCE_DIR}/test_common.cmake")


if(TEST_MODE STREQUAL "check")
    set(output_info_file test_${test_name}.xml)
elseif(TEST_MODE STREQUAL "samples")
    set(output_info_file ${BMX_TEST_SAMPLES_DIR}/test_${test_name}.xml.bin)
else()
    set(output_info_file ${TEST_SOURCE_DIR}/test_${test_name}.xml.bin)
endif()

set(create_command ${RAW2BMX}
    --regtest
    -t op1a
    -f 25
    -o ${output_file}
    --track-map "0\;1\;2,3"
    --track-mca-labels x ${TEST_SOURCE_DIR}/mca_2.txt
    --audio-layout adm
    --avci100_1080i video_1_${test_name}
    --adm-wave-chunk axml,adm_itu2076
    --wave ${TEST_SOURCE_DIR}/adm_1.wav
)

set(read_command ${MXF2RAW}
    --regtest
    --info
    --info-format xml
    --info-file ${output_info_file}
    --mca-detail
    ${output_file}
)

run_test_a(
    "${TEST_MODE}"
    "${BMX_TEST_WITH_VALGRIND}"
    "${create_test_video_1}"
    ""
    ""
    "${create_command}"
    ""
    ""
    "${read_command}"
    "${output_file}"
    "test_${test_name}.md5"
    "${output_info_file};test_${test_name}.xml.bin"
    ""
)
