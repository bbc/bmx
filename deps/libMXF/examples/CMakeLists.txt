if(LIBMXF_BUILD_EXAMPLES OR LIBMXF_BUILD_ARCHIVE)
    add_subdirectory(archive)
endif()

if(LIBMXF_BUILD_EXAMPLES OR LIBMXF_BUILD_AVIDMXFINFO)
    add_subdirectory(avidmxfinfo)
endif()

if(LIBMXF_BUILD_EXAMPLES OR LIBMXF_BUILD_READER)
    add_subdirectory(reader)
endif()

if(LIBMXF_BUILD_EXAMPLES OR LIBMXF_BUILD_TRANSFERTOP2)
    add_subdirectory(transfertop2)
endif()

if(LIBMXF_BUILD_EXAMPLES OR LIBMXF_BUILD_WRITEAVIDDV50)
    add_subdirectory(writeaviddv50)
endif()

# Build, but not install, if LIBMXF_BUILD_AVIDMXFINFO OR LIBMXF_BUILD_READER because they
# require the test output files
if(LIBMXF_BUILD_EXAMPLES OR LIBMXF_BUILD_WRITEAVIDMXF OR LIBMXF_BUILD_AVIDMXFINFO OR LIBMXF_BUILD_READER)
    add_subdirectory(writeavidmxf)
endif()
