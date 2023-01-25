# Test creating an D10 MXF file with text objects.

set(test_name d10)
include("${TEST_SOURCE_DIR}/test_common.cmake")

# 24-bit PCM
set(create_test_audio ${CREATE_TEST_ESSENCE}
    -t 42
    -d 3
    audio_${test_name}
)

# D10
set(create_test_video ${CREATE_TEST_ESSENCE}
    -t 11
    -d 3
    video_${test_name}
)

set_create_command(d10 d10_50)

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
    "${output_file}"
    "test_d10.md5"
    "${output_info_file};info_d10.xml.bin"
    "\
${output_text_file_prefix}_0.xml;utf8.xml.bin;\
${output_text_file_prefix}_1.xml;utf16be.xml.bin;\
${output_text_file_prefix}_2.xml;utf16le.xml.bin;\
${output_text_file_prefix}_3.xml;utf8.xml.bin;\
${output_text_file_prefix}_4.xml;other.xml.bin;\
${output_text_file_prefix}_5.xml;other.xml.bin;\
${output_text_file_prefix}_6.xml;utf8_noprolog.xml.bin"
)
