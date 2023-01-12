add_library(bmx_app_writers STATIC
    InputTrack.cpp
    OutputTrack.cpp
    TrackMapper.cpp
)

target_include_directories(bmx_app_writers PRIVATE
    "${PROJECT_BINARY_DIR}"
)
target_compile_definitions(bmx_app_writers PRIVATE
    HAVE_CONFIG_H
)

target_link_libraries(bmx_app_writers PUBLIC
    bmx
)

include("${PROJECT_SOURCE_DIR}/cmake/source_filename.cmake")
set_source_filename(bmx_app_writers "${CMAKE_CURRENT_LIST_DIR}" "bmx")
