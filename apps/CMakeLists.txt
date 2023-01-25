add_subdirectory(writers)

add_subdirectory(bmxparse)
add_subdirectory(bmxtranswrap)
add_subdirectory(mxf2raw)
add_subdirectory(raw2bmx)


add_executable(bmxtimecode
    bmxtimecode.cpp
)

target_include_directories(bmxtimecode PRIVATE
    "${PROJECT_BINARY_DIR}"
)
target_compile_definitions(bmxtimecode PRIVATE
    HAVE_CONFIG_H
)

target_link_libraries(bmxtimecode PRIVATE
    bmx
)

include("${PROJECT_SOURCE_DIR}/cmake/source_filename.cmake")
set_source_filename(bmxtimecode "${CMAKE_CURRENT_LIST_DIR}" "bmx")

install(TARGETS bmxtimecode DESTINATION bin)
