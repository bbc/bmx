include("${CMAKE_CURRENT_SOURCE_DIR}/../testing.cmake")

setup_test_dir("avc_separate_fields")

set(args
    "${common_args}"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/test_avc_separate_fields.cmake"
)
setup_test("avc_separate_fields" "bmx_avc_separate_fields" "${args}")
