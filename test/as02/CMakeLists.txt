include("${CMAKE_CURRENT_SOURCE_DIR}/../testing.cmake")

setup_test_dir("as02")

set(tests
    avci
    d10
    dv
    mpeg2lg
    soundonly
    unc
)

foreach(test ${tests})
    set(args
        "${common_args}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/test_${test}.cmake"
    )
    setup_test("as02" "bmx_as02_${test}" "${args}")
endforeach()
