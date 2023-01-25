set(bmx_sources)

add_subdirectory(apps)
add_subdirectory(as02)
add_subdirectory(as10)
add_subdirectory(as11)
add_subdirectory(avid_mxf)
add_subdirectory(clip_writer)
add_subdirectory(common)
add_subdirectory(d10_mxf)
add_subdirectory(essence_parser)
add_subdirectory(frame)
add_subdirectory(mxf_helper)
add_subdirectory(mxf_op1a)
add_subdirectory(mxf_reader)
add_subdirectory(rdd9_mxf)
add_subdirectory(st436)
add_subdirectory(wave)
add_subdirectory(writer_helper)

add_library(bmx
    ${bmx_sources}
)

target_include_directories(bmx PUBLIC
    ${PROJECT_SOURCE_DIR}/include
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

    CHECK_INCLUDE_FILE(dirent.h have_dirent_header)
    if(NOT have_dirent_header)
        target_include_directories(bmx PRIVATE
            ${PROJECT_SOURCE_DIR}/msvc_build/include/dirent
        )
    endif()
endif()

target_include_directories(bmx PRIVATE
    ${PROJECT_BINARY_DIR}
    ${PROJECT_SOURCE_DIR}
)
target_compile_definitions(bmx PRIVATE
    HAVE_CONFIG_H
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # Use target_link_libraries to add linker options. This is equivalent to
    # target_link_options which was added in cmake 3.13
    target_link_libraries(bmx PRIVATE -Wl,--no-undefined)
endif()

target_link_libraries(bmx
    PUBLIC
        ${MXF_link_lib}
        ${MXFpp_link_lib}
    PRIVATE
        ${uuid_link_lib}
        ${expat_link_lib}
        ${uriparser_link_lib}
)

if(BMX_BUILD_WITH_LIBCURL)
    target_link_libraries(bmx PRIVATE
        ${libcurl_link_lib}
    )
endif()

# Add the git version tracking library code
include("${PROJECT_SOURCE_DIR}/cmake/git_version.cmake")
target_link_libraries(bmx PRIVATE
    bmx_git_version
)

set_target_properties(bmx PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

include("${PROJECT_SOURCE_DIR}/cmake/source_filename.cmake")
set_source_filename(bmx "${CMAKE_CURRENT_LIST_DIR}" "bmx")

install(TARGETS bmx
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)
# headers are installed in the include/CMakeLists.txt

if(UNIX)
    configure_file(../${PROJECT_NAME}.pc.in ${PROJECT_NAME}.pc @ONLY)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc DESTINATION lib/pkgconfig)
endif()
