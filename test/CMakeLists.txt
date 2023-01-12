add_executable(create_test_essence
    create_test_essence.cpp
)

include("${PROJECT_SOURCE_DIR}/cmake/source_filename.cmake")
set_source_filename(create_test_essence "${CMAKE_CURRENT_LIST_DIR}" "bmx")

add_executable(file_truncate
    file_truncate.cpp
)

set_source_filename(file_truncate "${CMAKE_CURRENT_LIST_DIR}" "bmx")

add_subdirectory(ard_zdf_hdf)
add_subdirectory(as02)
add_subdirectory(as10)
add_subdirectory(as11)
add_subdirectory(avid_mxf)
if(LIBMXF_BUILD_ARCHIVE OR LIBMXF_BUILD_EXAMPLES)
    add_subdirectory(bbcarchive)
endif()
add_subdirectory(bmxtranswrap)
add_subdirectory(d10_mxf)
add_subdirectory(d10_qt_klv)
add_subdirectory(growing_file)
add_subdirectory(imf)
add_subdirectory(jpeg2000)
if(BMX_TEST_LARGE_FILE)
    add_subdirectory(large_file)
endif()
add_subdirectory(mca)
add_subdirectory(misc)
add_subdirectory(mxf_op1a)
add_subdirectory(mxf_reader)
add_subdirectory(partial_audio_frames)
add_subdirectory(rdd6)
add_subdirectory(rdd9_mxf)
add_subdirectory(text_object)
add_subdirectory(timed_text)
add_subdirectory(wave)
