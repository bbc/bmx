set(MXFpp_sources
    AvidHeaderMetadata.cpp
    DataModel.cpp
    File.cpp
    HeaderMetadata.cpp
    IndexTable.cpp
    MXFException.cpp
    MXFTypes.cpp
    MXFVersion.cpp
    MetadataSet.cpp
    Partition.cpp
)

set(MXFpp_headers
    AvidHeaderMetadata.h
    DataModel.h
    File.h
    HeaderMetadata.h
    IndexTable.h
    MXF.h
    MXFException.h
    MXFTypes.h
    MXFVersion.h
    MetadataSet.h
    Partition.h
)

add_subdirectory(extensions)
add_subdirectory(metadata)

add_library(MXFpp
    ${MXFpp_sources}
)

target_include_directories(MXFpp PUBLIC
    ${PROJECT_SOURCE_DIR}
)

# Add path to header files if not included in MSVC distribution
if(MSVC)
    include(CheckIncludeFile)

    CHECK_INCLUDE_FILE(inttypes.h have_inttypes_header)
    if(NOT have_inttypes_header)
        target_include_directories(bmx PUBLIC
            ${PROJECT_SOURCE_DIR}/msvc_build/include/inttypes
        )
    endif()
endif()

target_include_directories(MXFpp PRIVATE
    ${PROJECT_BINARY_DIR}
)
target_compile_definitions(MXFpp PRIVATE
    HAVE_CONFIG_H
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # Use target_link_libraries to add linker options. This is equivalent to
    # target_link_options which was added in cmake 3.13
    target_link_libraries(MXFpp PRIVATE -Wl,--no-undefined)
endif()

target_link_libraries(MXFpp PUBLIC
    ${MXF_link_lib}
)

# Add the git version tracking library code
include("${PROJECT_SOURCE_DIR}/cmake/git_version.cmake")
target_link_libraries(MXFpp PRIVATE libmxfpp_git_version)
if(NOT BUILD_SHARED_LIBS)
    # Add the libmxfpp_git_version object files to the static MXFpp library
    target_sources(MXFpp PRIVATE $<TARGET_OBJECTS:libmxfpp_git_version>)
endif()

set_target_properties(MXFpp PROPERTIES
    OUTPUT_NAME MXF++
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

include("${PROJECT_SOURCE_DIR}/cmake/source_filename.cmake")
set_source_filename(MXFpp "${CMAKE_CURRENT_LIST_DIR}" "libMXF++")

install(TARGETS MXFpp
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)
foreach(include_file ${MXFpp_headers})
    get_filename_component(dir ${include_file} DIRECTORY)
    install(FILES ${include_file} DESTINATION include/libMXF++/${dir})
endforeach()

if(UNIX)
    configure_file(../${PROJECT_NAME}.pc.in ${PROJECT_NAME}.pc @ONLY)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc DESTINATION lib/pkgconfig)
endif()
