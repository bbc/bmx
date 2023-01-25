include("${CMAKE_CURRENT_SOURCE_DIR}/../testing.cmake")

setup_test_dir("wave")

set(tests
    write
    read
    sound_only_from_raw
    sound_only_transwrap
)

foreach(test ${tests})
    set(args
        "${common_args}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/test_wave_${test}.cmake"
    )
    setup_test("wave" "bmx_wave_${test}" "${args}")
endforeach()
