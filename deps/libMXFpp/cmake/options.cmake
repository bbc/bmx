# Option to only build the library
option(LIBMXFPP_BUILD_LIB_ONLY "Build MXF++ and MXF libraries only" OFF)

# Option to build libMXF++ testing
# This option is ignored if BUILD_TESTING is defined and falsy
option(LIBMXFPP_BUILD_TESTING "Build libMXF++ testing" ON)

# Option to build all the examples
option(LIBMXFPP_BUILD_EXAMPLES "Build all the examples" OFF)

# Option to build the D10 writer example
option(LIBMXFPP_BUILD_D10_Writer "Build the D10 writer example" OFF)

# Option to build the OPAtom reader example
option(LIBMXFPP_BUILD_OPATOM_READER "Build the OPAtom reader example" OFF)

# Option to build all the tools
option(LIBMXFPP_BUILD_TOOLS "Build all the tools" OFF)

# Option to build the generate classes tool
option(LIBMXFPP_BUILD_GEN_CLASSES "Build the generate classes tool" ON)

if(UNIX)
    # Option to build a shared object library
    option(BUILD_SHARED_LIBS "Build using shared libraries" ON)

    # Run tests with valgrind
    option(LIBMXFPP_TEST_WITH_VALGRIND "Run tests with valgrind" OFF)

    # Option to build libMXF from an installed library found using pkg-config
    option(LIBMXFPP_BUILD_LIBMXF_LIB "Build libMXF from installed library" OFF)
elseif(MSVC)
    # Shared library currently not supported
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build using shared libraries")

    # Option to set to use the MultiThreadedDLL runtime
    option(LIBMXFPP_SET_MSVC_RUNTIME "Enable setting MSVC runtime to MultiThreadedDLL" ON)
endif()
