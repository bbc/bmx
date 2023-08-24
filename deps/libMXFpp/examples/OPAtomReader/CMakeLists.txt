add_library(opatomreader
    FixedSizeEssenceParser.cpp
    FrameOffsetIndexTable.cpp
    OPAtomClipReader.cpp
    OPAtomContentPackage.cpp
    OPAtomTrackReader.cpp
    PCMEssenceParser.cpp
    RawEssenceParser.cpp
    VariableSizeEssenceParser.cpp
)

set(opatomreader_headers
    FixedSizeEssenceParser.h
    FrameOffsetIndexTable.h
    OPAtomClipReader.h
    OPAtomContentPackage.h
    OPAtomTrackReader.h
    PCMEssenceParser.h
    RawEssenceParser.h
    VariableSizeEssenceParser.h
)

target_include_directories(opatomreader PRIVATE
    ${PROJECT_BINARY_DIR}
)
target_compile_definitions(opatomreader PRIVATE
    HAVE_CONFIG_H=1
)

target_link_libraries(opatomreader PUBLIC
    libmxfpp_examples_common
    MXFpp
    ${MXF_link_lib}
)

set_target_properties(opatomreader PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

include("${PROJECT_SOURCE_DIR}/cmake/source_filename.cmake")
set_source_filename(opatomreader "${CMAKE_CURRENT_LIST_DIR}" "libMXF++")

install(TARGETS opatomreader
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)
install(FILES ${opatomreader_headers} DESTINATION include/libMXF++/examples/OPAtomReader)


add_executable(test_opatomreader
    test_opatomreader.cpp
)

target_link_libraries(test_opatomreader
    opatomreader
    MXFpp
)

include("${PROJECT_SOURCE_DIR}/cmake/source_filename.cmake")
set_source_filename(test_opatomreader "${CMAKE_CURRENT_LIST_DIR}" "libMXF++")
