# bmx Library and Utilities

bmx is a library and set of utilities to read and write the [SMPTE ST 377-1 MXF file format](https://ieeexplore.ieee.org/document/7292073).

bmx is used to support standardisation efforts in the broadcast industry. It provides utilities for creating standard compliant sample files. It serves as an example implementation for MXF file format standards.

bmx provides a set of commandline applications:

* **raw2bmx**: create MXF files from raw essence files
* **bmxtranswrap**: re-wrap from one MXF file to another MXF file
* **mxf2raw**: output MXF file metadata and raw essence
* **bmxparse**: text dump raw essence files using the bmx library's parser class

bmx provides a set of file format text dumper and essence extraction tools:

* **h264dump**: text dump raw H.264 bitstream files
* **j2cdump**: text dump raw JPEG 2000 codestreams
* **jp2extract**: extract JPEG 2000 codestream from a JP2 file (ISO/IEC 15444-1 / ITU T.800 Annex I)
* **movdump**: text dump Quicktime / MP4 files
* **rdd36dump**: text dump SMPTE RDD 36 (Apple ProRes) bitstream files
* **vc2dump**:  text dump SMPTE ST 2042 VC-2 bitstream files
* **MXFDump**: text dumper for MXF files from the [AAF SDK](https://sourceforge.net/projects/aaf/). This utility is made available and built as part of libMXF.

The following input and output wrapper formats and flavours are supported:

* [AMWA AS-02](https://www.amwa.tv/specifications) MXF Versioning
* [AMWA AS-10](https://www.amwa.tv/specifications) MXF for Production
* [AMWA AS-11](https://www.amwa.tv/specifications) Media Contribution File Formats (MXF)
* [SMPTE ST 378](https://ieeexplore.ieee.org/document/7291764) MXF OP-1A
* [SMPTE RDD 9](https://ieeexplore.ieee.org/document/7290714) MXF MPEG Long GOP (Sony XDCAM)
* [SMPTE ST 386](https://ieeexplore.ieee.org/document/7291350) MXF D-10 (Sony MPEG IMX)
* [Avid native MXF OP-Atom](https://www.avid.com/static/resources/common/documents/mxf.pdf)
* [WAV](https://en.wikipedia.org/wiki/WAV)

The following essence formats are supported:

* [SMPTE RP 2027](https://ieeexplore.ieee.org/document/7290936) AVC-Intra video, class 50 / 100 / 200
* [SMPTE ST 356](https://ieeexplore.ieee.org/document/7290684) D-10 video, 30 / 40 / 50 MBit/s
* [DV](https://en.wikipedia.org/wiki/DV) video, 25 / 50 / 100 MBit/s
* [MPEG-2](https://www.itu.int/rec/T-REC-H.262) Long GOP video, 422P@HL, MP@HL (1920 and 1440) and MP@H14
* [JPEG 2000](https://www.itu.int/rec/T-REC-T.800) video
* [H.264](https://www.itu.int/rec/T-REC-H.264) video
* [SMPTE ST 2019](https://ieeexplore.ieee.org/document/7291983) VC-3 video (Avid DNxHD)
* [SMPTE ST 2042](https://ieeexplore.ieee.org/document/7967896) VC-2 video
* [SMPTE RDD 36](https://ieeexplore.ieee.org/document/7438722) video (Apple ProRes)
* Uncompressed video, UYVY / v210
* Avid [MJPEG](https://en.wikipedia.org/wiki/Motion_JPEG) video
* [WAV](https://en.wikipedia.org/wiki/WAV) PCM audio
* [SMPTE ST 436](https://ieeexplore.ieee.org/document/7290051) encapsulated ANC and VBI data
* [IMSC 1 Timed Text](https://www.w3.org/TR/ttml-imsc1.0.1/)


## Topics

A number of topics are described in more detail in the [docs/](docs/) directory,
including the following:

* [Timed Text](./docs/timed_text.md)
* [JPEG 2000](./docs/jpeg_2000.md)
* [RDD 6 XML Creator](./meta/rdd6_xml_creator/README.md)


## Build and Installation

bmx is developed on Ubuntu Linux but is supported on other Unix-like systems using the autotools build system. A set of Microsoft Visual C++ project files are provided for Windows.


### Dependencies

The following libraries must be installed to build bmx. The (Ubuntu) debian package names and versions are shown in brackets.

* libMXF (libmxf >= 1.0.4)
* libMXF++ (libmxf++ >= 1.0.4)
* [uriparser](https://github.com/uriparser/uriparser) (liburiparser-dev >= 0.7.2, <= 0.8).
* [expat](https://github.com/libexpat/libexpat) (libexpat1-dev >= 2.1)
* uuid, Unix-like systems only (uuid-dev)

The [libcurl](https://curl.haxx.se/libcurl/) (libcurl4-openssl-dev >= 7.22.0) library is optional for Unix-like systems and provides support for reading MXF files over HTTP(S).


### Unix-like Systems Build

Install the development versions of the dependency libraries. The bmx library can then be built from source using autotools as follows,
```bash
./autogen.sh
./configure
make
```

Run configure as shown below to see a list of build configuration options,
```bash
./configure -h
```

Add the `--with-curl` option to the configure line to include libcurl and therefore support reading MXF files over HTTP(S).

If you get library link errors similar to "error while loading shared libraries" then run
```
sudo /sbin/ldconfig
```
to update the runtime linker cache. E.g. the libMXF library was built and installed previously and the linker cache needs to be updated with the result.

There are a number of regression tests that can be run using
```bash
make check
```

Finally, the library and utilities can be installed using
```bash
sudo make install
```


### Microsoft Visual Studio C++ Build

The Visual Studio 2010 build solution and project files can be found in the [msvc_build/vs10](./msvc_build/vs10) directory. These files can be upgraded to any more recent version when importing into the IDE.

The main build solution file is [bmx.sln](./msvc_build/vs10/bmx.sln). It is used to build the library and MXF applications. The build solution assumes the following directories are present at the same directory level as bmx: `libMXF/`, `libMXF++/`, `uriparser/` and `expat/`.

The source distributions will contain a copy of the expat and uriparser libraries. See [Source and Binary Distributions](#source-and-binary-distributions) below.

A local copy of the expat and uriparser project files are included in the bmx build directory, i.e. the project files in the external repository are not used. The build solution file will build the dependency libraries.

The build depends on the `bmx_scm_version.h` header file in the root directory to provide the most recent git commit identifier. This file is generated automatically using the [gen_scm_version.sh](./gen_scm_version.sh) script when building using autotools and is included in the source distribution package. You are likely missing this file if you are using the source code directly from the git repository then and will need to create it manually.

The [tools.sln](./msvc_build/vs10/tools.sln) build solution file is used to build the text dumper tools.


## Docker

A [Dockerfile](./Dockerfile) is provided for building a Docker image containing a set of tools from bmx, libMXF and AAF SDK.

The Dockerfile contains a **build** and **runtime** layer:

* **build**: builds and checks the libMXF, libMXF++ and bmx code.
* **runtime**: provides the commandline tool executables built in the `build` layer.

### Build

The runtime Docker image can be built using docker build,

`DOCKER_BUILDKIT=1 docker build -t bmxtools .`

The `LIBMXF_GIT` and `LIBMXFPP_GIT` ARGS default to the Sourceforge git URLs. These can be changed by using the `--build-arg` commandline argument.

If you are behind a proxy then remember to use the `--build-arg` commandline arguments to set the Docker proxy ARGS.

### Run

The [docker/bmx_docker.sh](./docker/bmx_docker.sh) shell script is provided to make it easier to run the Docker container. Run

`./docker/bmx_docker.sh`

to see the usage and list of available tools.

The script maps the file user and group ID so that files have the user's identity when written to the output directory on the host. This is needed if Docker runs the container as the root user.

The script bind mounts input and output directories on the host to `/input` and `/output` directories on the container. The default is to set input and output directories to the current host directory.

If the input and output directories are the same then that directory is also bind mounted to the container's working directory. If the input or output directory is the current directory then it is also bind mounted to the container's working directory.

This example runs `bmxtranswrap` from input `./in.mxf` to output `./out.mxf`.

`./docker/bmx_docker.sh bmxtranswrap -o out.mxf in.mxf`

This example runs `bmxtranswrap` from input `./in.mxf` to output `/tmp/out.mxf`.

`./docker/bmx_docker.sh --output /tmp bmxtranswrap -o /output/out.mxf in.mxf`

This example runs `bmxtranswrap` from input `./sources/in.mxf` to output `./dests/out.mxf`.

`./docker/bmx_docker.sh --input ./sources --output ./dests bmxtranswrap -o /output/out.mxf /input/in.mxf`


## Source and Binary Distributions

Source distributions, including dependencies for the Windows build, and Windows binaries are made [available on SourceForge](https://sourceforge.net/projects/bmxlib/files/).


## Development Process

Software development of new features and bug fixes is done using repositories on GitHub. Once a feature or a bug fix is complete it is copied over to the repository on Sourceforge.

Source and binary distributions are generally only created when a new feature is required for creating standard compliant sample files for example, or when a release hasn't been made for a long time.


## License

The bmx library is provided under the BSD 3-clause license. See the [COPYING](./COPYING) file provided with this library for more details.
