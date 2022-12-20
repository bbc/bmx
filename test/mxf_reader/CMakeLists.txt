include("${CMAKE_CURRENT_SOURCE_DIR}/../testing.cmake")

setup_test_dir("mxf_reader")

set(tests
    avci
    d10
    dv
    mpeg2lg
    unc
)

foreach(test ${tests})
    set(args
        "${common_args}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/test_${test}.cmake"
    )
    setup_test("mxf_reader" "bmx_mxf_reader_${test}" "${args}")
endforeach()
