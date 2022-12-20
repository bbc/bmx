include("${CMAKE_CURRENT_SOURCE_DIR}/../testing.cmake")

setup_test_dir("mxf_op1a")

set(tests
    ancvbi
    avc
    d10
    dv
    indexfollows
    mpeg2lg
    rdd36
    soundonly
    unc
    vc2
    vc3
)

foreach(test ${tests})
    set(args
        "${common_args}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/test_${test}.cmake"
    )
    setup_test("mxf_op1a" "bmx_mxf_op1a_${test}" "${args}")
endforeach()
