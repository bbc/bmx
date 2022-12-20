include("${CMAKE_CURRENT_SOURCE_DIR}/../testing.cmake")

setup_test_dir("bmxtranswrap")

set(args
    "${common_args}"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/test_bmxtranswrap.cmake"
)
setup_test("bmxtranswrap" "bmx_bmxtranswrap" "${args}")
