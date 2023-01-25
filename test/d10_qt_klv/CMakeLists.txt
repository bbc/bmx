include("${CMAKE_CURRENT_SOURCE_DIR}/../testing.cmake")

setup_test_dir("d10_qt_klv")

set(args
    "${common_args}"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/test_d10_qt_klv.cmake"
)
setup_test("d10_qt_klv" "bmx_d10_qt_klv" "${args}")
