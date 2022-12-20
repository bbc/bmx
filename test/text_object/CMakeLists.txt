include("${CMAKE_CURRENT_SOURCE_DIR}/../testing.cmake")

setup_test_dir("text_object")

set(tests
    d10
    op1a
    rdd9
)

foreach(test ${tests})
    set(args
        "${common_args}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/test_text_object_${test}.cmake"
    )
    setup_test("text_object" "bmx_text_object_${test}" "${args}")
endforeach()
