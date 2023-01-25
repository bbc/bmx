# bmx Changelog

## v1.1

### Breaking changes

* None

### Features

* Moved to cmake build system ([PR 3](https://github.com/bbc/bmx/pull/3))
  * Replaced absolute source file paths in log messages to relative file paths
  * Interpret forward slash as backward slash in Windows file URIs
  * Generate library version from git tag

### Bug fixes

* MPEG-2 LG 576i: fix coding equations and remove signal standard and transfer characteristic
* Fixed various build warnings and memory deallocs as part of cmake build change

## Snapshot 2022-08-31 (v0.1)

### Features

* Add IMF flavour for outputting SMPTE ST 2067-2 IMF Track Files
* Add various options to support IMF Track Files
* Support arbitrary audio durations (not video frame aligned) when transwrapping to audio-only files
* Support additional MCA label properties
* Add limited OP1a support for MPEG2 Long GOP 576i
* Add `--check-end-tolerance` bmxtranswrap option to allow for shorter duration

### Bug fixes

* Default to `singlemca` track layout for clip-wrapped audio
* Fix temporal offsets for AVC long GOPs (commit 8285326)
* Various index table fixes

## Snapshot 2021-07-07

### Features

* OP-1A: add support for JPEG 2000.
* D-10: handle KLV wrapped D-10 video originating from Quicktime.
* Add a Dockerfile for creating Docker images.
* AS-11: add and update labels for X1, X5, X6 and X9.
* raw2bmx: add support for pipes.
* OP1a: add support for IMSC 1 Timed Text.
* movdump: various updates and fixes.
* Avid: add support for AVC / H.264.
* bmxtranswrap/raw2bmx: add KAG size and AES-3 flavour options.

### Bug fixes

* Apply KAG after the Primer Pack.
* Avid: fix physical source track count for multichannel audio.

## Snapshot 2017-08-14

### Features

* Add support for SMPTE RDD-36 (ProRes) in OP-1A
* RDD-6: support frame rates other than 25 Hz
* AS-11: change header metadata fill size to 4 MB

### Bug fixes

* D-10: set Locked property to 'true' by default
* RDD-9: include all descriptor properties listed in RDD-9
* align generic stream partition to KAG

## Snapshot 2017-01-17

### Features

* AS-11: update specification labels following specification status updates
* AS-11: change 'Programme' to 'Program' in MCA label names
* AS-11: allow no Channel ID label property in audio tracks containing a single audio channel
* AS-11: add unused audio channel label
* VC-2: merge branch into master
* Avid/raw2bmx: support auxiliary timecode tracks
* reinstate legacy AVC-Intra label names for Ingex player
* bmxtimecode: replace --drop/non-drop with --output parameter
* AVC: map HLG label from AVC bitstream

### Bug fixes

* AVC-Intra: don't attempt to parse AVC when there is no SPS+PPS
* mxf2raw: fix incomplete BBC Archive data model
* movdump: fix printing >32-bit atom sizes for files truncated to <32-bit size

## Snapshot 2016-07-05

### Features

* raw2bmx supports same AS-11 functionality as bmxtranswrap
* add option to set / override various AVC MXF descriptor properties
* merged-in the AVC branch
* add embedded XML support to RDD-9/D-10
* add MCA label support to RDD-9
* add AS-11 X7 and X8 labels
* changed the meaning of the 'repeat' option in MCA labels definition file
* add audio channel assignment label support to RDD-9
* support track mapping to all clip types
* support silence channels and tracks in track mapping
* support drop frame timecode in AS-11 segmentation definition file
* add option to set MXF header metadata filler size
* added the bmxtimecode utility

### Bug fixes

* fix RP2057 scheme label version byte
* apps: remove bogus AS-10 "Unknown framework..." warning
* h264dump: fix dpb_output_delay property name

## Snapshot 2016-03-30

### Features

* preliminary support for AMWA AS-11 X1 to X4. The variants that don't use
  RP 2057 AVC-Intra, e.g. Long GOP, require the 'avc' branch.
* support for AMWA AS-10
* support for embedding XML / text objects (SMPTE RP 2057)
* support for multi-channel audio tracks and labels (SMPTE ST 377-4)
* support for reading non-seekable MXF streams containing AVC-Intra or D-10
* support for memory-mapped MXF file access in Visual C++ builds only
* bmxparse application showing raw essence properties extracted by the essence
  parser classes
* default Material Package Track Number is now 0. The AMWA AS-02 and AS-11 UK
  DPP flavours set it to a non-zero value
* removed frame rate restrictions from VC-3 (Avid DNxHD)
* apps: add options to disable tracks by type
* avid: always output display and sampled x/y offset properties
* mxf2raw: show display x/y offset if present in file

### Bug fixes

* mxf2raw: fix UL/UUID detection for the URN text representation
* h264dump: fix buffering period SEI parsing
* avid: include pre-charge in physical source package timecode calculation

### Notes

* raw2bmx is currently missing some of the AS-11 X1 to X4 functionality,
  including MCA labelling. The workaround it to wrap the raw essence first using
  raw2bmx and then re-wrap using bmxtranswrap.

## Snapshot 2015-06-03

### Features

* support AVC-Intra Class 200
* add RIP to D-10 flavour
* RDD-9 flavour supports ANC and VBI data, SMPTE ST 436
* add basic (not optimised) file access over HTTP (Linux only)
* RDD-6 XML: change linear level to dB levels and change dialnorm to a signed value
* add support for VC-3 1244, 1258, 1259 and 1260 compression schemes
* apps: add option to set Material Package Track Number to 0
* apps: change default flavour to OP-1A
* bmxtranswrap: option to copy across AS-11 descriptive metadata
* bmxtranswrap: support stdin as input
* raw2bmx: assume variable D-10 input frame sizes and add option to override
* raw2bmx: support multi-channel raw PCM inputs
* mxf2raw: default to showing information if no options are provided
* h264dump: show rbsp_stop_one_bit info
* movdump: add stbl soun output

### Bug fixes

* AS-11: fix Track ID clash between data track (e.g. ST 436) and DM tracks
* mxf2raw: fixed display x/y offset to be a 32-bit signed integer
* mxf2raw: fix --file-chksum-only output
* avoid recursive Source Clips references and check for duplicate Package identifiers
* MSVC: updates to support msbuild usage
* movdump: use _stati64 to fix large file support on Windows

## Snapshot 2014-11-25
