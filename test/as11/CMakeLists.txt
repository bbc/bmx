include("${CMAKE_CURRENT_SOURCE_DIR}/../testing.cmake")

setup_test_dir("as11")

set(tests
    avci
    d10
    mpeg2lg
)

foreach(test ${tests})
    set(args
        "${common_args}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/test_${test}.cmake"
    )
    setup_test("as11" "bmx_as11_${test}" "${args}")
endforeach()
