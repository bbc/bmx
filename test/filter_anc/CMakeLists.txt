include("${CMAKE_CURRENT_SOURCE_DIR}/../testing.cmake")

setup_test_dir("filter_anc")

set(tests
    filter_anc_1
    filter_anc_2
    filter_anc_3
    filter_anc_4
    filter_anc_5
    filter_anc_6
)

foreach(test ${tests})
    set(args
        "${common_args}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/test_${test}.cmake"
    )
    setup_test("filter_anc" "bmx_${test}" "${args}")
endforeach()
