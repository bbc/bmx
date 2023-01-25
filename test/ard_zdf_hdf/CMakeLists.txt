include("${CMAKE_CURRENT_SOURCE_DIR}/../testing.cmake")

setup_test_dir("ard_zdf_hdf")

set(tests
    avci
    mpeg2lg
)

foreach(test ${tests})
    set(args
        "${common_args}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/test_${test}.cmake"
    )
    setup_test("ard_zdf_hdf" "bmx_ard_zdf_hdf_${test}" "${args}")
endforeach()
