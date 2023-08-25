add_executable(extract_avid_extensions
    extract_avid_extensions.c
)
target_include_directories(extract_avid_extensions PUBLIC
    ${PROJECT_SOURCE_DIR}
)
target_link_libraries(extract_avid_extensions MXF)

include("${PROJECT_SOURCE_DIR}/cmake/source_filename.cmake")
set_source_filename(extract_avid_extensions "${CMAKE_CURRENT_LIST_DIR}" "libMXF")
