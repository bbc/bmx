if(MXF_link_lib)
    return()
endif()


if(LIBMXFPP_BUILD_LIBMXF_LIB)
    include(FindPkgConfig)

    pkg_search_module(PC_libMXF libMXF REQUIRED IMPORTED_TARGET)
    message("-- Build using libMXF pkg-config '${PC_libMXF_MODULE_NAME}'")

    set(MXF_link_lib PkgConfig::PC_libMXF)
else()
    if(NOT EXISTS ${PROJECT_SOURCE_DIR}/../libMXF/CMakeLists.txt)
        message(FATAL_ERROR
            "libMXF source code does not exist at '../libMXF'"
        )
    endif()

    include(FetchContent)

    if(LIBMXFPP_BUILD_LIB_ONLY)
        set(LIBMXF_BUILD_LIB_ONLY ON CACHE BOOL "Build libMXF only")
    endif()

    FetchContent_Declare(libMXF
        SOURCE_DIR ${PROJECT_SOURCE_DIR}/../libMXF
    )

    FetchContent_GetProperties(libMXF)
    if(NOT libMXF_POPULATED)
        FetchContent_Populate(libMXF)
        add_subdirectory(${libmxf_SOURCE_DIR} ${libmxf_BINARY_DIR})
    endif()

    set(MXF_link_lib MXF)
endif()
