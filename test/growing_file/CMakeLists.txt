include("${CMAKE_CURRENT_SOURCE_DIR}/../testing.cmake")

setup_test_dir("growing_file")

set(args
    "${common_args}"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/test_growing_file.cmake"
)
setup_test("growing_file" "bmx_growing_file" "${args}")
