include("${CMAKE_CURRENT_SOURCE_DIR}/../testing.cmake")

setup_test_dir("large_file")

set(args
    "${common_args}"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/test.cmake"
)
setup_test("large_file" "bmx_large_file" "${args}")


# Add a cleanup fixture so that the large test files don't hang around.
add_test(NAME bmx_large_file_cleanup
    COMMAND ${CMAKE_COMMAND} -E remove -f video test_raw2bmx.mxf test_bmxtranswrap.mxf
)
set_tests_properties(bmx_large_file_cleanup PROPERTIES
    FIXTURES_CLEANUP bmx_large_file_fixture
)
set_tests_properties(bmx_large_file PROPERTIES
    FIXTURES_REQUIRED bmx_large_file_fixture
)

# Do the same cleanup for the samples and data targets (if they succeed).
add_custom_command(TARGET bmx_test_large_file_samples
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E remove -f video test_raw2bmx.mxf test_bmxtranswrap.mxf
)
add_custom_command(TARGET bmx_test_large_file_data
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E remove -f video test_raw2bmx.mxf test_bmxtranswrap.mxf
)
