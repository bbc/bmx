include("${CMAKE_CURRENT_SOURCE_DIR}/../testing.cmake")

setup_test_dir("avid_mxf")

set(tests
    avc
    avc_gf
    avid_alpha
    d10
    dv
    mpeg2lg
    unc
    vc3
)

foreach(test ${tests})
    set(args
        "${common_args}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/test_${test}.cmake"
    )
    setup_test("avid_mxf" "bmx_avid_mxf_${test}" "${args}")
endforeach()
