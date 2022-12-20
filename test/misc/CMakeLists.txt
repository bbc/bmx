include("${CMAKE_CURRENT_SOURCE_DIR}/../testing.cmake")

setup_test_dir("misc")

set(tests
    desc_props_bmxtranswrap
    desc_props_raw2bmx
)

foreach(test ${tests})
    set(args
        "${common_args}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/test_${test}.cmake"
    )
    setup_test("misc" "bmx_misc_${test}" "${args}")
endforeach()
