# Building libMXF++

## Configure Options

The libMXF++ cmake configure options can be found in [cmake/options.cmake](../cmake/options.cmake). Additionaly, there are [libMXF options](../../libMXF/cmake/options.cmake).

Most options are booleans and can be set to `ON` or `OFF` using the `cmake` commandline option `-D<option>=<value>`. E.g. `-DLIBMXFPP_BUILD_EXAMPLES=OFF` will disable building the examples and `-DLIBMXFPP_BUILD_TOOLS=ON` will enable building the tools.

The default configuration will result in the following outputs:

- libMXF++ library
- libMXF library
- MXFDump tool (from the AAF SDK)

The `BUILD_SHARED_LIBS` option can be used to select building shared or static libraries on Unix-like systems. Shared libraries are currently not supported on Windows.

## Build Dependencies From Source

The [libMXF](../../libMXF/) dependency is provided alongside libMXF++.

The `LIBMXFPP_BUILD_LIBMXF_LIB` option can be used to build from an installed [libMXF](../../libMXF/).

## Windows Build

The Microsoft Visual Studio C++ build currently supports static libraries only. The Multi-threaded DLL runtime library unless the `LIBMXFPP_SET_MSVC_RUNTIME` configuration option is set to `OFF`.

## Git Library Version String

The MXF library version string (Identification::VersionString) includes the output of `git describe`. This git commit information is provided to the cmake build by the [cmake-git-version-tracking utility](https://github.com/andrew-hardin/cmake-git-version-tracking.git), modified with [this patch](../cmake/git_version_904dbda.patch).

If no `.git` files are present in the source tree then the string defined in the [fallback_git_version.h](../fallback_git_version.h) header file is used instead. This fallback is used in source-only packages.

If a checkout of the cmake-git-version-tracking utility is present in a `deps/` directory than that will be used. See [git_version.cmake](../cmake/git_version.cmake) for the git URL and tag used by default.

## Testing

The tests can be run using `ctest -C <build type>` on Windows or `ctest` / `make test` on Unix-like systems.

The ctest `-j<num cpu>` option can be used to run tests in parallel. E.g. `ctest -j4`. Note that the `-j` option in `make` doesn't work in this case.

The ctest `-V` option will show cmake debug messages as well as the test command output and errors.

### Test With Valgrind

The tests can be run using the [valgrind tool](https://valgrind.org/) by using the `LIBMXFPP_TEST_WITH_VALGRIND` configuration option. The valgrind tool must be installed first (the package is named `valgrind` in Debian / Ubuntu).

The test commands are run through valgrind as `valgrind --leak-check=full -q --error-exitcode=1 <test command>`. This is useful for exposing memory access issues and memory leaks.
