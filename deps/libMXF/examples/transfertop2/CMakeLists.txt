add_library(avid_mxf_to_p2
    avid_mxf_to_p2.c
    xml_writer.c
)

target_link_libraries(avid_mxf_to_p2
    MXF
)

include("${PROJECT_SOURCE_DIR}/cmake/source_filename.cmake")
set_source_filename(avid_mxf_to_p2 "${CMAKE_CURRENT_LIST_DIR}" "libMXF")


if(LIBMXF_BUILD_TESTING AND (NOT DEFINED BUILD_TESTING OR BUILD_TESTING))
    add_executable(test_avid_mxf_to_p2
        test_avid_mxf_to_p2.c
    )
    target_link_libraries(test_avid_mxf_to_p2
        avid_mxf_to_p2
        MXF
    )

    include("${PROJECT_SOURCE_DIR}/cmake/source_filename.cmake")
    set_source_filename(test_avid_mxf_to_p2 "${CMAKE_CURRENT_LIST_DIR}" "libMXF")
endif()
