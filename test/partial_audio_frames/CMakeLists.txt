include("${CMAKE_CURRENT_SOURCE_DIR}/../testing.cmake")

setup_test_dir("partial_audio_frames")

foreach(test RANGE 1 2)
    set(args
        "${common_args}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/test_${test}.cmake"
    )
    setup_test("partial_audio_frames" "bmx_partial_audio_frames_${test}" "${args}")
endforeach()
