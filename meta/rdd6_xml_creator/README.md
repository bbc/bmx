# RDD 6 XML Creator

The [SMPTE RDD 6](https://ieeexplore.ieee.org/document/7290141) (Dolby E Audio Metadata) XML Creator is a basic HTML web page application that creates an XML file suitable for `bmxtranswrap` to embed a static RDD 6 serial bitstream in an MXF file.

Copy the [index.html](./index.html) and supporting directories ([css](./css) and [js](./js)) to a local file system, or use a clone of this Git repository.

Open `index.html` in a web browser and fill-in the RDD 6 metadata. Select "Save XML" to export to a local file.

>The file name includes a hash of the file contents (`var filename = "dpp_rdd6_" + hash_djb2(rdd6_xml) + ".xml"`) and therefore files will have a different name if the contents are different.

The exported XML file can be passed to the `bmxtranswrap` tool using the `--rdd6 <file>` commandline option.

The RDD 6 XML can be extracted from an MXF file using `mxf2raw` and the `--rdd6 <frames> <file>` option.

> Set the `<frames>` argument to a single MXF frame (e.g. `10`) or sequence of MXF frames (e.g. `0-10`) to extract the RDD 6 metadata. The RDD 6 metadata is the same in each frame except for the program description which is spread across multiple frames. Each frame has 1 character of the description text, a description start byte (0x02) or a description end byte (0x03).

The XML Schema for the RDD 6 XML can be found in [../rdd6/rdd6.xsd](../rdd6/rdd6.xsd).