if(LIBMXF_BUILD_TOOLS)
    add_subdirectory(extract_avid_extensions)
endif()

if(LIBMXF_BUILD_TOOLS)
    add_subdirectory(extract_jpeg2000_labels)
endif()

if(LIBMXF_BUILD_TOOLS OR LIBMXF_BUILD_MXFDUMP)
    add_subdirectory(MXFDump)
endif()
