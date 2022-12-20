include("${CMAKE_CURRENT_SOURCE_DIR}/../testing.cmake")

setup_test_dir("mca")

set(tests
    track_map
    labels
)

foreach(test ${tests})
    set(args
        "${common_args}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/test_${test}.cmake"
    )
    setup_test("mca" "bmx_mca_${test}" "${args}")
endforeach()
