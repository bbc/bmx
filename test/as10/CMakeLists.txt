include("${CMAKE_CURRENT_SOURCE_DIR}/../testing.cmake")

setup_test_dir("as10")

set(args
    "${common_args}"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/test_as10.cmake"
)
setup_test("as10" "bmx_as10" "${args}")
