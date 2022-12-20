if(MXF_link_lib)
    return()
endif()


if(BMX_BUILD_LIBMXF_LIB)
    include(FindPkgConfig)

    pkg_search_module(PC_libMXF libMXF REQUIRED IMPORTED_TARGET)
    message("-- Build using libMXF pkg-config '${PC_libMXF_MODULE_NAME}'")

    set(MXF_link_lib PkgConfig::PC_libMXF)
else()
    include(FetchContent)

    set(LIBMXF_SET_MSVC_RUNTIME ${BMX_SET_MSVC_RUNTIME} CACHE INTERNAL "")
    set(LIBMXFPP_SET_MSVC_RUNTIME ${BMX_SET_MSVC_RUNTIME} CACHE INTERNAL "")

    FetchContent_Declare(libMXF
        SOURCE_DIR ${PROJECT_SOURCE_DIR}/deps/libMXF
    )
    FetchContent_MakeAvailable(libMXF)

    set(MXF_link_lib MXF)
endif()
