include("${CMAKE_CURRENT_SOURCE_DIR}/../testing.cmake")

setup_test_dir("d10_mxf")

set(args
    "${common_args}"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/test_d10_mxf.cmake"
)
setup_test("d10_mxf" "bmx_d10_mxf" "${args}")
