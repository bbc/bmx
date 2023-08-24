add_executable(archive_mxf_info
    archive_mxf_info.c
)

target_link_libraries(archive_mxf_info
    MXF
)

include("${PROJECT_SOURCE_DIR}/cmake/source_filename.cmake")
set_source_filename(archive_mxf_info "${CMAKE_CURRENT_LIST_DIR}" "libMXF")

install(TARGETS archive_mxf_info DESTINATION bin)
