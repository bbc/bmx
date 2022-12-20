# Option to build a shared object library
option(BUILD_SHARED_LIBS "Build using shared libraries" $<IF:MSVC:OFF:ON>)

# Option to build testing
# This option is ignored if BUILD_TESTING is defined and falsy
option(BMX_BUILD_TESTING "Build testing" ON)

# Option to build all the apps
option(BMX_BUILD_APPS "Build all the apps" OFF)

# Option to build all the tools
option(BMX_BUILD_TOOLS "Build all the tools" OFF)

# Option to change the directory where test sample files are written
set(BMX_TEST_SAMPLES_DIR "test_samples" CACHE STRING "Directory for writing test sample files")

if(UNIX)
    # Run tests with valgrind
    option(BMX_TEST_WITH_VALGRIND "Run tests with valgrind" OFF)

    # Option to build libMXF from an installed library found using pkg-config
    option(BMX_BUILD_LIBMXF_LIB "Build libMXF from installed library" OFF)

    # Option to build libMXF++ from an installed library found using pkg-config
    option(BMX_BUILD_LIBMXFPP_LIB "Build libMXF++ from installed library" OFF)

    # Build with libcurl for reading MXF files over HTTP(s)
    option(BMX_BUILD_WITH_LIBCURL "Build with libcurl library" OFF)

    # Option to build expat from source in deps/ or from the git repo
    option(BMX_BUILD_EXPAT_SOURCE "Build expat from source" OFF)

    # Option to build uriparser from source in deps/ or from the git repo
    option(BMX_BUILD_URIPARSER_SOURCE "Build uriparser from source" OFF)
elseif(MSVC)
    # Option to set to use the MultiThreadedDLL runtime
    option(BMX_SET_MSVC_RUNTIME "Enable setting MSVC runtime to MultiThreadedDLL" ON)
endif()
