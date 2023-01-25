add_executable(bmxparse
    bmxparse.cpp
)

target_include_directories(bmxparse PRIVATE
    "${PROJECT_BINARY_DIR}"
)
target_compile_definitions(bmxparse PRIVATE
    HAVE_CONFIG_H
)

target_link_libraries(bmxparse PRIVATE
    bmx
)

include("${PROJECT_SOURCE_DIR}/cmake/source_filename.cmake")
set_source_filename(bmxparse "${CMAKE_CURRENT_LIST_DIR}" "bmx")

install(TARGETS bmxparse DESTINATION bin)
