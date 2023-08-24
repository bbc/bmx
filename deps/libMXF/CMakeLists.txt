cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.15.0)
    # Set policy CMP0091 to new to allow use of CMAKE_MSVC_RUNTIME_LIBRARY
    cmake_policy(SET CMP0091 NEW)
endif()

project(libMXF
    VERSION 1.1
    DESCRIPTION "Low-level C library for reading and writing the SMPTE ST 377-1 MXF file format"
    HOMEPAGE_URL https://github.com/bbc/libMXF
    LANGUAGES C CXX
)

include("${CMAKE_CURRENT_LIST_DIR}/cmake/options.cmake")

if(MSVC AND LIBMXF_SET_MSVC_RUNTIME AND CMAKE_VERSION VERSION_GREATER_EQUAL 3.15.0)
    # cmake version >= 3.15: Use MultiThreadedDLL runtime
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

if(BUILD_SHARED_LIBS)
    # Ensure that static library code can be linked into the dynamic libraries (-fPIC compile option)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()
set(CMAKE_CXX_STANDARD 11)

# Set the test samples output directory
if(LIBMXF_TEST_SAMPLES_DIR STREQUAL "")
    set(new_samples_dir ${CMAKE_CURRENT_BINARY_DIR}/test_samples)
else()
    get_filename_component(new_samples_dir
        ${LIBMXF_TEST_SAMPLES_DIR}
        REALPATH
        BASE_DIR ${CMAKE_CURRENT_BINARY_DIR}
    )
endif()
if(NOT new_samples_dir STREQUAL ${LIBMXF_TEST_SAMPLES_DIR})
    set(LIBMXF_TEST_SAMPLES_DIR ${new_samples_dir})
    message("-- Test samples output directory: ${LIBMXF_TEST_SAMPLES_DIR}")
endif()

if(MSVC)
    add_compile_options(/W3)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)

    if(LIBMXF_SET_MSVC_RUNTIME AND CMAKE_VERSION VERSION_LESS 3.15.0)
        # cmake version < 3.15: Update compiler flags to use the MultiThreadedDLL runtime
        macro(update_msvc_runtime_flags flags)
            string(REGEX REPLACE "/MT" "/MD" ${flags} "${${flags}}")
        endmacro()

        update_msvc_runtime_flags(CMAKE_C_FLAGS)
        update_msvc_runtime_flags(CMAKE_CXX_FLAGS)
        foreach(suffix _DEBUG _RELEASE _MINSIZEREL _RELWITHDEBINFO)
            update_msvc_runtime_flags(CMAKE_C_FLAGS${suffix})
            update_msvc_runtime_flags(CMAKE_CXX_FLAGS${suffix})
        endforeach()
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

if(LIBMXF_BUILD_TESTING AND (NOT DEFINED BUILD_TESTING OR BUILD_TESTING))
    enable_testing()
endif()
add_custom_target(libMXF_test_samples)
add_custom_target(libMXF_test_data)

include("${PROJECT_SOURCE_DIR}/cmake/ext_uuid.cmake")

configure_file(config.h.in config.h)

add_subdirectory(mxf)
if(LIBMXF_BUILD_TESTING AND (NOT DEFINED BUILD_TESTING OR BUILD_TESTING))
    add_subdirectory(test)
endif()
if(NOT LIBMXF_BUILD_LIB_ONLY)
    add_subdirectory(examples)
    add_subdirectory(tools)
endif()
