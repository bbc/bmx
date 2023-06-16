if(MXF_link_lib)
    return()
endif()


if(BMX_BUILD_LIBMXF_LIB)
    include(FindPkgConfig)

    pkg_search_module(PC_libMXF libMXF REQUIRED IMPORTED_TARGET)
    message("-- Build using libMXF pkg-config '${PC_libMXF_MODULE_NAME}'")

    set(MXF_link_lib PkgConfig::PC_libMXF)
else()
    if(NOT EXISTS ${PROJECT_SOURCE_DIR}/deps/libMXF/CMakeLists.txt)
        message(FATAL_ERROR
            "libMXF submodule source code does not exist at 'deps/libMXF'\n"
            "Run 'git submodule update --init' to fetch the source"
        )
    endif()

    include(FetchContent)

    set(LIBMXF_SET_MSVC_RUNTIME ${BMX_SET_MSVC_RUNTIME} CACHE INTERNAL "")
    if(BMX_BUILD_LIB_ONLY)
        set(LIBMXF_BUILD_LIB_ONLY ON CACHE BOOL "Build MXF library only")
    endif()

    FetchContent_Declare(libMXF
        SOURCE_DIR ${PROJECT_SOURCE_DIR}/deps/libMXF
    )

    FetchContent_GetProperties(libMXF)
    if(NOT libMXF_POPULATED)
        FetchContent_Populate(libMXF)
        add_subdirectory(${libmxf_SOURCE_DIR} ${libmxf_BINARY_DIR})
    endif()

    set(MXF_link_lib MXF)
endif()
