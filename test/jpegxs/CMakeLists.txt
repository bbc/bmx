include("${CMAKE_CURRENT_SOURCE_DIR}/../testing.cmake")

setup_test_dir("jpegxs")

set(tests
    rgba
    cdci
)

foreach(test ${tests})
    set(args
        "${common_args}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/test_${test}.cmake"
    )
    setup_test("jpegxs" "bmx_jpegxs_${test}" "${args}")
endforeach()
