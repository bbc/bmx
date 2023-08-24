# libMXF

libMXF is a low-level C library for reading and writing the [SMPTE ST 377-1 MXF file format](https://ieeexplore.ieee.org/document/7292073).

libMXF was originally developed as part of the [Ingex Project](http://ingex.sourceforge.net/) where it supported MXF transfer, playback and storage applications. libMXF was also used in the [BBC Archive Preservation Project](https://www.bbc.co.uk/rd/publications/whitepaper275) to migrate BBC archive content from video tapes to files.

The [MXFDump](./tools/MXFDump) MXF text dumper utility from the [AAF SDK](https://sourceforge.net/projects/aaf/) is provided in this project for convenience.

## Examples

A number of applications and library code can be found in the [examples](./examples) directory. These are not part of the core library and are not required to build [libMXF++](../libMXFpp) or [bmx](../../).

* [archive](./examples/archive): library code and utilities used in the [BBC Archive Preservation Project](https://www.bbc.co.uk/rd/publications/whitepaper275).
* [avidmxfinfo](./examples/avidmxfinfo): library and utility for extracting metadata about Avid MXF OP-Atom files. This utility has been superseded by
`mxf2raw` in the [bmx](../../) project.
* [reader](./examples/reader): library code used by the [Ingex Player](http://ingex.sourceforge.net/) for reading MXF files.
* [transfertop2](./examples/transfertop2): utilities used in the [TransferToP2](http://ingex.sourceforge.net/TransferToP2.html) application to allow edited sequences to be transferred from an editing system to a P2 card.
* [vlc](./examples/vlc): legacy code that was written to test how easy it would be to support MXF in [VLC](https://www.videolan.org/vlc/).
* [writeaviddv50](./examples/writeaviddv50): example utility for writing DV 50 MBit/s video in Avid MXF OP-Atom files.
* [writeavidmxf](./examples/writeavidmxf): library code and utility for writing Avid MXF OP-Atom files. This utility has been superseded by `raw2bmx` in the [bmx](../../) project.

## Build, Test and Install

libMXF is developed on Ubuntu Linux but is supported on other Unix-like systems and Windows.

The [cmake](https://cmake.org/) build system is used, with minimum version **3.12**. The build requires the `git` tool along with the C/C++ compilers.

The build process has been tested on Ubuntu, Debian, MacOS and Windows (Microsoft Visual C++ 2017 v15.9.51 and later versions).

### Dependencies

The uuid library (Ubuntu / Debian package name `uuid-dev`) is required for non-Apple Unix-like systems.

### Commands

A basic commandline process is described here for platforms that default to the Unix Makefiles or Visual Studio cmake generators. See [build.md](./docs/build.md) for more detailed build options.

Replace `<build type>` in the commandlines below with `Debug` or `Release`. Note that the Visual Studio generator supports multiple build types for a configuration, which is why the build type is selected _after_ configuration.

The default generator can be overridden using the cmake `-G` option. The list of available generators is shown at the end of the output of `cmake --help`.

Start by creating a build directory and change into it. The commandlines below use a `out/build` build directory in the working tree, which follows the approach taken by Microsoft Visual Studio C++.

#### Unix-like (Unix Makefiles)

```bash
mkdir -p out/build
cd out/build
cmake ../../ -DCMAKE_BUILD_TYPE=<build type>
cmake --build .
make test
sudo make install
```

Run `ldconfig` to update the runtime linker cache. This avoids library link errors similar to "error while loading shared libraries".

```bash
sudo /sbin/ldconfig
```

#### Windows (Visual Studio)

```console
mkdir out\build
cd out\build
cmake ..\..\
cmake --build . --config <build type>
ctest -C <build type>
cmake --build . --config <build type> --target install
```

## Source and Binary Distributions

Source distributions, including dependencies for the Windows build, and Windows binaries are made available as in [bmx](../../).

## License

The libMXF library is provided under the BSD 3-clause license. See the [COPYING](./COPYING) file provided with this library for more details.
