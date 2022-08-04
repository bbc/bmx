# Timed Text

The `bmx` Timed Text implementation supports embedding TTML Profiles for Internet Media Subtitles and Captions [IMSC 1.0.1](https://www.w3.org/TR/ttml-imsc1.0.1/) and [IMSC 1.1](https://www.w3.org/TR/ttml-imsc1.1/). The Timed Text is embedded based on SMPTE ST 2067-2:2016 and ST 2067-5:2013, which forms part of the Interoperable Master Format (IMF) set of SMPTE specifications. These specifications reference SMPTE ST 429-5:2017, which is part of the D-Cinema Packaging specifications.

## Timed Text Range

The zero point assumed in the Timed Text may not correspond to the MXF file's zero point. E.g. the Timed Text refers to a media position 0 for the first subtitle but that actually refers to position 750 in the MXF file.

The Timed Text can be re-positioned by adding a Filler at the start of the Timed Text Track in the Material Package before the Source Clip referencing the Track in the File Package. The Filler defines the offset of the Timed Text's zero point relative to the MXF file's zero point. E.g. if the MXF A/V content starts at 09:59:30:00 and the programme, including subtitles, start at 10:00:00:00 then a 30 second Filler is added to align the Timed Text to the start of the programme.

## Operational Patterns

The MXF Operational Pattern signalled by the bmx implementation in the MXF file can be different from the baseline OP1a based on 2 factors:

* are there more than 1 track in the file?
* are Fillers used to align the Timed Text?

If there is more than 1 track then the Operational Pattern needs to be in the `b` row because the Timed Text uses clip wrapping as opposed to frame wrapping. This requires there to be a separate Essence Container and therefore a separate File Package for each Timed Text track.

If Fillers are used in the Timed Text track then the Operational Pattern column moves to either `2` or `3`. It moves to `2` if there are no other audio, video or data tracks, or the other data tracks are Timed Text that have the exact same Filler usage. If those conditions are not met then it moves to `3`.

The set of Operational Patterns that can result from including Timed Text in the bmx implementation is therefore OP1a, OP1b, OP2a, OP2b or OP3b.

## Writing Support

Timed Text is supported in the OP1a writer classes. However, it will signal higher operational patterns depending on the contents of the file and the Timed Text timing relationship.

The Timed Text XML document is written to an Essence Container and ancillary resources are written to Generic Stream Containers. The Timed Text Index Table, Essence and Generic Stream Containers are written in separate partitions after the Header Partition and after RP 2057 XML Generic Stream Container partitions. Placing the Timed Text data before the audio and video data allows applications to start playing the content, including subtitles or captions, before the file is available in its entirety.

The `OP1ATimedTextTrack` class can be used to create a Timed Text track. It is configured using the `OP1ATimedTextTrack::SetSource()` method which is passed a `TimedTextManifest` object which specifies the source content and associated properties.

### Commandline Utilities: `raw2bmx`

The `raw2bmx` utility can be used to embed Timed Text XML and ancillary font or image resources in MXF. It uses a manifest file to describe the source filenames and properties. The structure of the manifest file is described in the [Manifest File Format](#manifest-file-format) section.

*Example 1*: Creates an OP1a file including a timed text track alongside video and audio tracks. The manifest file, `manifest.txt`, references the Timed Text XML file and ancillary resource files.

```bash
raw2bmx -t op1a -y 10:00:00:00 --avci video.h264 --wave audio.wav --tt manifest.txt
```

*Example 2*: Creates a (mono-essence) IMF Timed Text Track File. An edit rate (`-f`) and duration (`--dur`) are required in this case as they are not parsed from the Timed Text XML file (assuming they are specified in there).

```bash
raw2bmx -t imf -f 25 --dur 100 --tt manifest.txt
```

### Manifest File Format

The manifest file is a text file containing properties related to a Timed Text XML document and any associated ancillary image or font resources.

The manifest file consists of a set of name/value property pairs separated by a newline. The name/value pair uses the first ':' character as a separator. A '#' character signals the start of a comment.

The manifest file must start with properties associated with the Timed Text XML document and this can be followed by properties associated with each ancillary resource. Each ancillary resource must start with the `resource_file` property and this is used to detect when a new ancillary resource is being defined.

The properties for the Timed Text XML document are as follows.

* `file` (*required*): The filename of the Timed Text XML document. A relative filename is relative to the location of the manifest file.
* `profile` (*required*): The IMSC profile designator suffix, one of `imsc1/text`, `imsc1/image`, `imsc1.1/text` or `imsc1.1/image`.
* `encoding` (*required*): The XML encoding, e.g. `UTF-8`.
* `resource_id` (*optional*): The identifier associated with the Timed Text XML. The value is a UUID URN.
* `languages` (*optional*): A list of RFC 5646 language tags separated by a ',' character.
* `start` (*optional*): Specifies the non-zero start position for the Timed Text. The value is either a timecode (HH:MM:SS:FF or HH:MM:SS;FF) or a position in media edit rate. It is assumed to be 0 if not set.

The properties for the ancillary resource are as follows.

* `resource_file` (*required*): The filename of the resource. A relative filename is relative to the location of the manifest file.
* `resource_id` (*required*): The identifier associated with the resource. The value is a UUID URN.
* `mime_type` (*required*): The MIME type for the resource.

An example manifest is shown below. It describes a Timed Text XML document `image_example.xml` that references a font resource contained in `font.ttf` and an image resource contained in `image.png`. The Timed Text starts at timecode 10:00:00:00.

```text
file: image_example.xml
profile: imsc1/image
encoding: UTF-8
languages: en,fr
start: 10:00:00:00

resource_file: font.ttf
mime_type: application/x-font-opentype
resource_id: urn:uuid:fe16f8e4-57a9-56b5-a93c-6c27d6f61619  # == uuid.uuid5(uuid.UUID('b6cc57a0-87e7-4e75-b1c3-3359f3ae8817'), 'Arial')

resource_file: image.png
resource_id: urn:uuid:173efd37-3b1e-535c-917f-e126e25bd505  # == uuid.uuid5(uuid.NAMESPACE_URL, 'image.png')
mime_type: image/png
```

## Reading Support

The `MXFTimedTextTrackReader` class is provided in the `mxf_reader` to read the Timed Text metadata and essence. This class is not quite the same as the other `MXFFileTrackReader` classes because it can't be used to read frame-by-frame. This breaks the original design of `bmx` to some degree.

The inherited `MXFFileTrackReader` methods should not be used and instead the `MXFTimedTextTrackReader` class provides a `MXFTimedTextTrackReader::GetManifest()` method to get a manifest of the Timed Text XML and related ancillary resources. The Timed Text XML can be read using the `MXFTimedTextTrackReader::ReadTimedText()` method and the ancillary resources using either `MXFTimedTextTrackReader::ReadAncillaryResourceById()` or `MXFTimedTextTrackReader::ReadAncillaryResourceByStreamId()`.

### Commandline Utilities: `mxf2raw`

The `mxf2raw` utility can be used to show metadata about the Timed Text tracks and used for extracting the essence data to files. The Timed Text tracks will have metadata shown similar to the extract below. It shows the properties in the Timed Text data file descriptor and sub-descriptors. A non-zero Timed Text offset, which corresponds to the `start` field in the manifest, is shown in the `timed_text_offset` field in the Track information.

```text
    Track #1:
      essence_kind    : Data
      essence_type    : Timed_Text
      ec_label        : urn:smpte:ul:060e2b34.0401010a.0d010301.02130101
      edit_rate       : 25/1
      duration        : 00:45:30:00 (count='68250')
      timed_text_offset: 00:00:30:00 (count='750')
      Packages: (1)
        Package #0:
          Material:
            package_uid     : urn:smpte:umid:060a2b34.01010105.01010f20.13000000.c5239c49.1a6d884f.ba77974a.708ebd1a
            track_id        : 3001
            track_number    : 0
          FileSource:
            package_uid     : urn:smpte:umid:060a2b34.01010105.01010f20.13000000.5c86a914.c37cec4e.a811a0e3.dfeade5a
            track_id        : 3001
            track_number    : 0x17010b01
            file_uri        : file:///tmp/timed_text.mxf
      DataDescriptor:
        TimedTextDescriptor:
          profile         : http://www.w3.org/ns/ttml/profile/imsc1/image
          encoding        : UTF-8
          languages       : en
          AncillaryResources: (2)
            Element #0:
              resource_id     : urn:uuid:fe16f8e4-57a9-56b5-a93c-6c27d6f61619
              mime_type       : application/x-font-opentype
              stream_id       : 12
            Element #1:
              resource_id     : urn:uuid:173efd37-3b1e-535c-917f-e126e25bd505
              mime_type       : image/png
              stream_id       : 13
```
