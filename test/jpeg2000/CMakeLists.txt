include("${CMAKE_CURRENT_SOURCE_DIR}/../testing.cmake")

setup_test_dir("jpeg2000")

set(tests
    rgba
    cdci
)

foreach(test ${tests})
    set(args
        "${common_args}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/test_${test}.cmake"
    )
    setup_test("jpeg2000" "bmx_jpeg2000_${test}" "${args}")
endforeach()
