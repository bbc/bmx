if(MXFpp_link_lib)
    return()
endif()


if(BMX_BUILD_LIBMXFPP_LIB)
    include(FindPkgConfig)

    pkg_search_module(PC_libMXFpp libMXF++ REQUIRED IMPORTED_TARGET)
    message("-- Build using LIBMXF++ pkg-config '${PC_libMXFpp_MODULE_NAME}'")

    set(MXFpp_link_lib PkgConfig::PC_libMXFpp)
else()
    if(NOT EXISTS ${PROJECT_SOURCE_DIR}/deps/libMXFpp/CMakeLists.txt)
        message(FATAL_ERROR
            "libMXF submodule source code does not exist at 'deps/libMXFpp'\n"
            "Run 'git submodule update --init' to fetch the source"
        )
    endif()

    include(FetchContent)

    set(LIBMXFPP_SET_MSVC_RUNTIME ${BMX_SET_MSVC_RUNTIME} CACHE INTERNAL "")

    FetchContent_Declare(libMXFpp
        SOURCE_DIR ${PROJECT_SOURCE_DIR}/deps/libMXFpp
    )

    FetchContent_GetProperties(libMXFpp)
    if(NOT libMXFpp_POPULATED)
        FetchContent_Populate(libMXFpp)
        add_subdirectory(${libmxfpp_SOURCE_DIR} ${libmxfpp_BINARY_DIR})
    endif()

    set(MXFpp_link_lib MXFpp)
 endif()
