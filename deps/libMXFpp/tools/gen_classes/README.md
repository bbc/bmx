# Generate metadata classes

The `gen_classes` tool is used to generate C++ headers and source files from the sets and items defined in the [libMXF data model](https://github.com/bbc/libMXF/blob/main/mxf/mxf_baseline_data_model.h).

This tool hasn't been updated or improved for a long time and this means the process often requires manual changes to be made. A typical process involves generating the classes and then manually adding the missing bits, fixing where required and changing to match the current code.

The C++ headers and source files can be generated using the `libMXFpp_generate_classes` build target,

```bash
cmake --build . --target libMXFpp_generate_classes
```

or

```bash
make libMXFpp_generate_classes
```

This will create a `generated_classes` directory in the cmake top-level binary output directory.

Copy the new files and `Metadata.h` to the [libMXF++/metadata](../../libMXF%2B%2B/metadata/) directory. Copy the new `REGISTER_CLASS` lines output to the terminal output into the `HeaderMetadata::initialiseObjectFactory` method in [libMXF++/HeaderMetadata.cpp](../../libMXF%2B%2B/HeaderMetadata.cpp).

Modify the new headers and source code as required.
