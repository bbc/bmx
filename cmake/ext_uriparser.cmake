if(uriparser_link_lib)
    return()
endif()


if(MSVC OR BMX_BUILD_URIPARSER_SOURCE)
    include(FetchContent)

    set(URIPARSER_BUILD_DOCS OFF CACHE INTERNAL "")
    set(URIPARSER_BUILD_TESTS OFF CACHE INTERNAL "")
    set(URIPARSER_BUILD_TOOLS OFF CACHE INTERNAL "")

    if(EXISTS "${PROJECT_SOURCE_DIR}/deps/uriparser")
        FetchContent_Declare(liburiparser
            SOURCE_DIR "${PROJECT_SOURCE_DIR}/deps/uriparser"
        )
        message("-- Build using uriparser in deps/")
    else()
        FetchContent_Declare(liburiparser
            GIT_REPOSITORY https://github.com/uriparser/uriparser
            GIT_TAG uriparser-0.9.7
        )
        message("-- Build using uriparser from git repo")
    endif()

    FetchContent_GetProperties(liburiparser)
    if(NOT liburiparser_POPULATED)
        FetchContent_Populate(liburiparser)
        add_subdirectory(${liburiparser_SOURCE_DIR} ${liburiparser_BINARY_DIR})
    endif()

    set(uriparser_link_lib uriparser)
else()
    include(FindPkgConfig)

    pkg_search_module(PC_uriparser uriparser liburiparser QUIET IMPORTED_TARGET)
    if(PC_uriparser_FOUND)
        set(uriparser_link_lib PkgConfig::PC_uriparser)
    else()
        find_library(LIB_uriparser uriparser)
        if(NOT LIB_uriparser)
            message(FATAL_ERROR "uriparser dependency not found")
        endif()

        find_path(uriparser_include_dirs
            NAMES uriparser/Uri.h
        )

        add_library(liburiparser UNKNOWN IMPORTED)
        set_target_properties(liburiparser PROPERTIES
            IMPORTED_LOCATION ${LIB_uriparser_LIBRARY}
            INTERFACE_INCLUDE_DIRECTORIES ${uriparser_include_dirs}
        )
        set(uriparser_link_lib liburiparser)
    endif()
endif()
