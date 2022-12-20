include("${CMAKE_CURRENT_SOURCE_DIR}/../testing.cmake")

setup_test_dir("timed_text")

foreach(test_num RANGE 1 10)
    set(args
        "${common_args}"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/test_${test_num}.cmake
    )
    setup_test("timed_text" "bmx_timed_text_${test_num}" "${args}")
endforeach()
