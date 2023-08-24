# Option to only build the library
option(LIBMXF_BUILD_LIB_ONLY "Build MXF library only" OFF)

# Option to build testing
# This option is ignored if BUILD_TESTING is defined and falsy
option(LIBMXF_BUILD_TESTING "Build libMXF testing" ON)

# Option to build all the examples
option(LIBMXF_BUILD_EXAMPLES "Build all the examples" OFF)

# Option to build the archive example
option(LIBMXF_BUILD_ARCHIVE "Build the archive example" OFF)

# Option to build the avidmxfinfo example
option(LIBMXF_BUILD_AVIDMXFINFO "Build the avidmxfinfo example" OFF)

# Option to build the reader example
option(LIBMXF_BUILD_READER "Build the reader example" OFF)

# Option to build the transfertop2 example
option(LIBMXF_BUILD_TRANSFERTOP2 "Build the transfertop2 example" OFF)

# Option to build the writeaviddv50 example
option(LIBMXF_BUILD_WRITEAVIDDV50 "Build the writeaviddv50 example" OFF)

# Option to build the writeavidmxf example
option(LIBMXF_BUILD_WRITEAVIDMXF "Build the writeavidmxf example" OFF)

# Option to build all the tools
option(LIBMXF_BUILD_TOOLS "Build all the tools" OFF)

# Option to build the MXFDump tool
option(LIBMXF_BUILD_MXFDUMP "Build the MXFDump tool" ON)

# Option to change the directory where test sample files are written
set(LIBMXF_TEST_SAMPLES_DIR "test_samples" CACHE STRING "Directory for writing test sample files")

if(UNIX)
    # Option to build a shared object library
    option(BUILD_SHARED_LIBS "Build using shared libraries" ON)

    # Run tests with valgrind
    option(LIBMXF_TEST_WITH_VALGRIND "Run tests with valgrind" OFF)
elseif(MSVC)
    # Shared library currently not supported
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build using shared libraries")

    # Option to set to use the MultiThreadedDLL runtime
    option(LIBMXF_SET_MSVC_RUNTIME "Enable setting MSVC runtime to MultiThreadedDLL" ON)
endif()
