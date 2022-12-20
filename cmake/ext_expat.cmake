if(expat_link_lib)
    return()
endif()


if(MSVC OR BMX_BUILD_EXPAT_SOURCE)
    include(FetchContent)

    set(EXPAT_BUILD_DOCS OFF CACHE INTERNAL "")
    set(EXPAT_BUILD_EXAMPLES OFF CACHE INTERNAL "")
    set(EXPAT_BUILD_TESTS OFF CACHE INTERNAL "")
    set(EXPAT_BUILD_TOOLS OFF CACHE INTERNAL "")
    if(BUILD_SHARED_LIBS)
        set(EXPAT_SHARED_LIBS ON CACHE INTERNAL "")
    else()
        set(EXPAT_SHARED_LIBS OFF CACHE INTERNAL "")
    endif()

    if(EXISTS "${PROJECT_SOURCE_DIR}/deps/libexpat")
        FetchContent_Declare(FT_libexpat
            SOURCE_DIR "${PROJECT_SOURCE_DIR}/deps/libexpat"
        )
    else()
        FetchContent_Declare(FT_libexpat
            GIT_REPOSITORY https://github.com/libexpat/libexpat
            GIT_TAG R_2_5_0
        )
    endif()

    # Use FetchContent_Populate because the CMakeLists.txt is in the expat/ sub-directory
    FetchContent_GetProperties(FT_libexpat)
    if(NOT FT_libexpat_POPULATED)
        FetchContent_Populate(FT_libexpat)

        add_subdirectory("${ft_libexpat_SOURCE_DIR}/expat" ${ft_libexpat_BINARY_DIR})
    endif()

    set(expat_link_lib expat)
else()
    include(FindEXPAT)

    if(NOT EXPAT_FOUND)
        message(FATAL_ERROR "expat dependency not found")
    endif()

    set(expat_link_lib EXPAT::EXPAT)
endif()
