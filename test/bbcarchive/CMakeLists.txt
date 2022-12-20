include("${CMAKE_CURRENT_SOURCE_DIR}/../testing.cmake")

setup_test_dir("bbcarchive")

list(APPEND common_args
    -D TEST_WRITE_ARCHIVE_MXF=$<TARGET_FILE:test_write_archive_mxf>
)

set(args
    "${common_args}"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/test_bbcarchive.cmake"
)
setup_test("bbcarchive" "bmx_bbcarchive" "${args}")

set_tests_properties("bmx_bbcarchive" PROPERTIES FIXTURES_REQUIRED test_write_archive_mxf)
add_dependencies("bmx_bbcarchive_samples" test_write_archive_mxf)
add_dependencies("bmx_bbcarchive_data" test_write_archive_mxf)
