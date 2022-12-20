include("${CMAKE_CURRENT_SOURCE_DIR}/../testing.cmake")

setup_test_dir("rdd9_mxf")

set(tests
    ancvbi
    mpeg2lg
)

foreach(test ${tests})
    set(args
        "${common_args}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/test_${test}.cmake"
    )
    setup_test("rdd9_mxf" "bmx_rdd9_mxf_${test}" "${args}")
endforeach()
