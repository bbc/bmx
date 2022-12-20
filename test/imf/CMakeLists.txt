include("${CMAKE_CURRENT_SOURCE_DIR}/../testing.cmake")

setup_test_dir("imf")

set(tests
    audio
    jpeg2000
    rdd36
    timed_text
)

foreach(test ${tests})
    set(args
        "${common_args}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/test_${test}.cmake"
    )
    setup_test("imf" "bmx_imf_${test}" "${args}")
endforeach()
