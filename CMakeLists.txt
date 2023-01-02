cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

include("${CMAKE_CURRENT_LIST_DIR}/cmake/options.cmake")

if(MSVC AND BMX_SET_MSVC_RUNTIME AND CMAKE_VERSION VERSION_GREATER_EQUAL 3.15.0)
    # cmake version >= 3.15: Use MultiThreadedDLL runtime
    cmake_policy(SET CMP0091 NEW)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

if(BUILD_SHARED_LIBS)
    # Ensure that static library code can be linked into the dynamic libraries (-fPIC compile option)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()
set(CMAKE_CXX_STANDARD 11)

project(bmx
    VERSION 0.1.3
    DESCRIPTION "A C++ library and set of utilities to read and write the SMPTE ST 377-1 MXF file format"
    HOMEPAGE_URL https://github.com/bbc/bmx
    LANGUAGES C CXX
)
set(PROJECT_VERSION_RELEASE 2)

# Set the test samples output directory
if(BMX_TEST_SAMPLES_DIR STREQUAL "")
    set(new_samples_dir ${CMAKE_CURRENT_BINARY_DIR}/test_samples)
else()
    get_filename_component(new_samples_dir
        ${BMX_TEST_SAMPLES_DIR}
        REALPATH
        BASE_DIR ${CMAKE_CURRENT_BINARY_DIR}
    )
endif()
if(NOT new_samples_dir STREQUAL ${BMX_TEST_SAMPLES_DIR})
    set(BMX_TEST_SAMPLES_DIR ${new_samples_dir})
    message("-- Test samples output directory: ${BMX_TEST_SAMPLES_DIR}")
endif()

if(MSVC)
    add_compile_options(/W3)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)

    if(BMX_SET_MSVC_RUNTIME AND CMAKE_VERSION VERSION_LESS 3.15.0)
        # cmake version < 3.15: Update compiler flags to use the MultiThreadedDLL runtime
        macro(update_msvc_runtime_flags flags)
            string(REGEX REPLACE "/MT" "/MD" ${flags} "${${flags}}")
        endmacro()

        update_msvc_runtime_flags(CMAKE_C_FLAGS)
        update_msvc_runtime_flags(CMAKE_C_FLAGS_DEBUG)
        update_msvc_runtime_flags(CMAKE_C_FLAGS_RELEASE)
        update_msvc_runtime_flags(CMAKE_CXX_FLAGS)
        update_msvc_runtime_flags(CMAKE_CXX_FLAGS_DEBUG)
        update_msvc_runtime_flags(CMAKE_CXX_FLAGS_RELEASE)
    endif()
else()
    add_compile_options(-W -Wall -O2)

    # Enable large file support on 32-bit systems.
    add_definitions(
        -D_FILE_OFFSET_BITS=64
        -D_LARGEFILE_SOURCE
        -D_LARGEFILE64_SOURCE
    )
endif()

if(BMX_BUILD_TESTING AND (NOT DEFINED BUILD_TESTING OR BUILD_TESTING))
    enable_testing()
endif()
add_custom_target(bmx_test_samples)
add_custom_target(bmx_test_data)

include("${PROJECT_SOURCE_DIR}/cmake/libmxf.cmake")
include("${PROJECT_SOURCE_DIR}/cmake/libmxfpp.cmake")
include("${PROJECT_SOURCE_DIR}/cmake/ext_uuid.cmake")
include("${PROJECT_SOURCE_DIR}/cmake/ext_expat.cmake")
include("${PROJECT_SOURCE_DIR}/cmake/ext_uriparser.cmake")
if(BMX_BUILD_WITH_LIBCURL)
    include("${PROJECT_SOURCE_DIR}/cmake/ext_libcurl.cmake")
endif()

configure_file(config.h.in config.h)

add_subdirectory(include)
add_subdirectory(src)
add_subdirectory(test)
if(BMX_BUILD_APPS)
    add_subdirectory(apps)
endif()
if(BMX_BUILD_TOOLS)
    add_subdirectory(tools)
endif()
