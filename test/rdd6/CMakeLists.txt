include("${CMAKE_CURRENT_SOURCE_DIR}/../testing.cmake")

setup_test_dir("rdd6")

set(tests
    rdd6_1
    rdd6_2
    rdd6_3
)

foreach(test ${tests})
    set(args
        "${common_args}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/test_${test}.cmake"
    )
    setup_test("rdd6" "bmx_${test}" "${args}")
endforeach()
