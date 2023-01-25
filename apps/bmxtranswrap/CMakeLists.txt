add_executable(bmxtranswrap
    bmxtranswrap.cpp
    MXFInputTrack.cpp
)

target_include_directories(bmxtranswrap PRIVATE
    "${PROJECT_BINARY_DIR}"
)
target_compile_definitions(bmxtranswrap PRIVATE
    HAVE_CONFIG_H
)

target_link_libraries(bmxtranswrap PRIVATE
    bmx_app_writers
)

include("${PROJECT_SOURCE_DIR}/cmake/source_filename.cmake")
set_source_filename(bmxtranswrap "${CMAKE_CURRENT_LIST_DIR}" "bmx")

install(TARGETS bmxtranswrap DESTINATION bin)
