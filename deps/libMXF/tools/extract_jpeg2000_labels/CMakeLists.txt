add_executable(extract_jpeg2000_labels
    extract_jpeg2000_labels.c
)
target_include_directories(extract_jpeg2000_labels PUBLIC
    ${PROJECT_SOURCE_DIR}
)
target_link_libraries(extract_jpeg2000_labels MXF)

include("${PROJECT_SOURCE_DIR}/cmake/source_filename.cmake")
set_source_filename(extract_jpeg2000_labels "${CMAKE_CURRENT_LIST_DIR}" "libMXF")
