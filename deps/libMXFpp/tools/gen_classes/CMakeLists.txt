add_executable(gen_classes
    gen_metadata_classes.cpp
)
target_link_libraries(gen_classes PRIVATE
    ${MXF_link_lib}
)

include("${PROJECT_SOURCE_DIR}/cmake/source_filename.cmake")
set_source_filename(gen_classes "${CMAKE_CURRENT_LIST_DIR}" "libMXF++")


add_custom_target(libMXFpp_generate_classes
    COMMAND $<TARGET_FILE:gen_classes> ${CMAKE_BINARY_DIR}/generated_classes
)
