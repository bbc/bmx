add_library(d10mxfop1awriter
    D10ContentPackage.cpp
    D10MXFOP1AWriter.cpp
)

set(d10mxfop1awriter_headers
    D10ContentPackage.h
    D10MXFOP1AWriter.h
)

target_include_directories(d10mxfop1awriter PRIVATE
    ${PROJECT_BINARY_DIR}
)
target_compile_definitions(d10mxfop1awriter PRIVATE
    HAVE_CONFIG_H=1
)

target_link_libraries(d10mxfop1awriter PUBLIC
    libmxfpp_examples_common
    MXFpp
    ${MXF_link_lib}
)

set_target_properties(d10mxfop1awriter PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

include("${PROJECT_SOURCE_DIR}/cmake/source_filename.cmake")
set_source_filename(d10mxfop1awriter "${CMAKE_CURRENT_LIST_DIR}" "libMXF++")

install(TARGETS d10mxfop1awriter
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)
install(FILES ${d10mxfop1awriter_headers} DESTINATION include/libMXF++/examples/D10MXFOP1AWriter)


add_executable(test_d10mxfop1awriter
    test_d10mxfop1awriter.cpp
)

target_link_libraries(test_d10mxfop1awriter
    d10mxfop1awriter
    MXFpp
)

include("${PROJECT_SOURCE_DIR}/cmake/source_filename.cmake")
set_source_filename(test_d10mxfop1awriter "${CMAKE_CURRENT_LIST_DIR}" "libMXF++")
