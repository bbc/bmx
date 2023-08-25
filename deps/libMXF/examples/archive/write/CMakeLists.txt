add_library(writearchivemxf
    timecode_index.c
    write_archive_mxf.c
)

set(writearchivemxf_headers
    write_archive_mxf.h
)

target_include_directories(writearchivemxf PUBLIC
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/examples/archive/write
)

target_include_directories(writearchivemxf PRIVATE
    ${PROJECT_BINARY_DIR}
)
target_compile_definitions(writearchivemxf PRIVATE
    HAVE_CONFIG_H=1
)

target_link_libraries(writearchivemxf PUBLIC
    MXF
)

set_target_properties(writearchivemxf PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

include("${PROJECT_SOURCE_DIR}/cmake/source_filename.cmake")
set_source_filename(writearchivemxf "${CMAKE_CURRENT_LIST_DIR}" "libMXF")

install(TARGETS writearchivemxf
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)
install(FILES ${writearchivemxf_headers} DESTINATION include/mxf/examples/archive)


add_executable(update_archive_mxf
    update_archive_mxf.c
)
target_link_libraries(update_archive_mxf
    writearchivemxf
    MXF
)

include("${PROJECT_SOURCE_DIR}/cmake/source_filename.cmake")
set_source_filename(update_archive_mxf "${CMAKE_CURRENT_LIST_DIR}" "libMXF")
