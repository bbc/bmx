# Building bmx

## Configure Options

The bmx cmake configure options can be found in [cmake/options.cmake](../cmake/options.cmake). Additionaly, there are [libMXF options](https://github.com/bbc/libMXF/cmake/options.cmake) (or in a code checkout at `deps/libMXF/cmake/options.cmake`) and [libMXFpp options](https://github.com/bbc/libMXFpp/cmake/options.cmake) (or in a code checkout at `deps/libMXFpp/cmake/options.cmake`).

Most options are booleans and can be set to `ON` or `OFF` using the `cmake` commandline option `-D<option>=<value>`. E.g. `-DBMX_BUILD_APPS=OFF` will disable building the bmx commandline apps and `-DLIBMXF_BUILD_EXAMPLES=ON` will enable building the libMXF examples.

The `BMX_TEST_SAMPLES_DIR` option accepts a string value that is an absolute or relative (to the build directory) file path for the test sample files.

The default configuration will result in the following outputs:

- bmx commandline apps (e.g. `bmxtranswrap`) and tools (e.g. `h264dump`)
- bmx library
- libMXF library
- libMXF++ library
- MXFDump tool (from the AAF SDK)

The `BUILD_SHARED_LIBS` option can be used to select building shared or static libraries on Unix-like systems. Shared libraries are currently not supported on Windows.

## Build Dependencies From Source

The libMXF and libMXF++ dependencies are included as git submodules in `deps/`. The other dependencies can also be built from source by including them in the `deps/` directory and enabling it using configuration options.

On Unix-like systems the configuration options to build dependencies from source in `deps/` are: `BMX_BUILD_EXPAT_SOURCE` and `BMX_BUILD_URIPARSER_SOURCE`. Checkout the code from GitHub into the `deps/` directory and set the corresponding option to `ON`. See the [ext_expat.cmake](../cmake/ext_expat.cmake) and [ext_uriparser.cmake](../cmake/ext_uriparser.cmake) files for git URL and tags used in the Windows build.

The `BMX_BUILD_LIBMXF_LIB` and `BMX_BUILD_LIBMXFPP_LIB` options can be used to build from installed libraries rather than the submodules in `deps/`.

## Windows Build

The Microsoft Visual Studio C++ build currently supports static libraries only. The Multi-threaded DLL runtime library unless the `BMX_SET_MSVC_RUNTIME` configuration option is set to `OFF`.

## Git Library Version String

The MXF library version string (Identification::VersionString) includes the output of `git describe`. This git commit information is provided to the cmake build by the [cmake-git-version-tracking utility](https://github.com/andrew-hardin/cmake-git-version-tracking.git), modified with [this patch](../cmake/git_version_904dbda.patch).

If no `.git` files are present in the source tree then the string defined in the [fallback_git_version.h](../fallback_git_version.h) header file is used instead. This fallback is used in source-only packages.

If a checkout of the cmake-git-version-tracking utility is present in `deps/` than that will be used. See [git_version.cmake](../cmake/git_version.cmake) for the git URL and tag used by default.

## Testing

The tests can be run using `ctest -C <build type>` on Windows or `ctest` / `make test` on Unix-like systems.

The ctest `-j<num cpu>` option can be used to run tests in parallel. E.g. `ctest -j4`. Note that the `-j` option in `make` doesn't work in this case.

The ctest `-R` option can be used to run a single test or multiple tests that match a name pattern. E.g. `ctest -R bmx_imf` will run the IMF tests and `bmx_imf_timed_text` will run the IMF Timed Text test.

The ctest `-V` option will show cmake debug messages as well as the test command output and errors.

### Test Data

The majority of tests use MD5 checksums to validate outputs. Some tests also have text files that are checked, e.g. the XML output of `mxf2raw` These MD5 checksums and text files can be found in the test directories and are generated using custom cmake build targets. The build target runs the test, but checksums and text files are written to the source test directory and output files aren't validated.

The build target can be run to update checksums and text files when a change has been made to the source code. The changes should be checked first before updating using the samples build target - see the [Test Samples](#test-samples) section.

The build target can be run using the cmake `--target` option, e.g. `cmake--build . --target bmx_test_data` or `make bmx_test_data`.

The `bmx_test_data` build target will generate checksums for all tests. Each test directory has a build target named `bmx_test_<directory name>_data` that will generate checksums for all tests in the directory, e.g. `bmx_test_imf_data`. Each test has a build target named `<test name>_data` to generate a checksum for the test, e.g. `bmx_imf_timed_text_data`.

### Test Samples

The outputs of tests can be generated using custom cmake build targets similar to the test data described in the [Test Data](#test-data) section. The build target names use the `_samples` suffix rather than `_data`.

The build target run the test, but outputs are written to the test samples directory and aren't validated. This is useful when a test fails as it allows a comparison of output files with previous versions of the code.

The samples are written to the `test_samples/` directory in the build directory, unless another directory is configured using the `BMX_TEST_SAMPLES_DIR` option.

### Test With Valgrind

The tests can be run using the [valgrind tool](https://valgrind.org/) by using the `BMX_TEST_WITH_VALGRIND` configuration option. The valgrind tool must be installed first (the package is named `valgrind` in Debian / Ubuntu).

The test commands are run through valgrind as `valgrind --leak-check=full -q --error-exitcode=1 <test command>`. This is useful for exposing memory access issues and memory leaks.

### Large File Test

The large file (> 4GB) support test can be enabled using the `BMX_TEST_LARGE_FILE` configuration option. The test requires ~8.04 GB disk space to run. It will delete the output files once done.
