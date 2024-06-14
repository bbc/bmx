cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

# Set the policy CMP0091 to new to allow use of CMAKE_MSVC_RUNTIME_LIBRARY,
# unless it has already been set
if(POLICY CMP0091)
    cmake_policy(GET CMP0091 CMP0091_value)
    if(NOT CMP0091_value)
        cmake_policy(SET CMP0091 NEW)
        cmake_policy(GET CMP0091 CMP0091_value)
    endif()
else()
    set(CMP0091_value OLD)
endif()

project(bmx
    VERSION 1.2
    DESCRIPTION "A C++ library and set of utilities to read and write the SMPTE ST 377-1 MXF file format"
    HOMEPAGE_URL https://github.com/bbc/bmx
    LANGUAGES C CXX
)

include("${CMAKE_CURRENT_LIST_DIR}/cmake/options.cmake")

if(MSVC AND CMP0091_value STREQUAL NEW)
    if(BMX_SET_MSVC_RUNTIME STREQUAL "MD")
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    elseif(BMX_SET_MSVC_RUNTIME STREQUAL "MT")
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()
endif()

if(BUILD_SHARED_LIBS OR NOT DEFINED CMAKE_POSITION_INDEPENDENT_CODE)
    # Ensure that static library code can be linked into the dynamic libraries (-fPIC compile option)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

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
    add_compile_options(/W3 /EHsc)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)

    if(CMP0091_value STREQUAL OLD AND NOT BMX_SET_MSVC_RUNTIME STREQUAL "default")
        # Update compiler flags to use a particular runtime
        macro(update_msvc_runtime_flags flags)
            if(BMX_SET_MSVC_RUNTIME STREQUAL "MD")
                # Use MultiThreadedDLL runtime
                string(REGEX REPLACE "/MT" "/MD" ${flags} "${${flags}}")
            elseif(BMX_SET_MSVC_RUNTIME STREQUAL "MT")
                # Use MultiThreaded runtime
                string(REGEX REPLACE "/MD" "/MT" ${flags} "${${flags}}")
            endif()
        endmacro()

        update_msvc_runtime_flags(CMAKE_C_FLAGS)
        update_msvc_runtime_flags(CMAKE_CXX_FLAGS)
        foreach(suffix _DEBUG _RELEASE _MINSIZEREL _RELWITHDEBINFO)
            update_msvc_runtime_flags(CMAKE_C_FLAGS${suffix})
            update_msvc_runtime_flags(CMAKE_CXX_FLAGS${suffix})
        endforeach()
    endif()
else()
    add_compile_options(-Wextra -Wall $<$<COMPILE_LANGUAGE:C>:-Werror=implicit-function-declaration>)

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
if(NOT BMX_BUILD_LIB_ONLY)
    if(BMX_BUILD_APPS)
        add_subdirectory(apps)
    endif()
    if(BMX_BUILD_TOOLS)
        add_subdirectory(tools)
    endif()
endif()
