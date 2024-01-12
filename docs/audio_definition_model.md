# (BETA) -- Audio Definition Model (ADM) and other audio metadata RIFF Chunks

## Quick start: Example: convert Wave+ADM (BW64) to IMF ADM Audio Track File (MXF)

Input: `wav_adm.wav` -- BW64 Wave file containing audio data, a \<chna\> chunk, and ADM XML metadata in an \<axml\> chunk

Output: `adm_audio_track_file.mxf` -- an IMF ADM Audio Track File containing the same information, with all required MXF signalling added

The ADM XML metadata embedded in the Wave file:
* defines two ADM audioProgramme entities
* does not conform to any identified Profile or Level

Example command:

`raw2bmx -t imf -o adm_audio_track_file.mxf --audio-layout adm --track-mca-labels x mca.txt --adm-wave-chunk axml,adm_itu2076 --wave wav_adm.wav`

where `mca.txt` contains:

```text
0
ADM, chunk_id=axml, RFC5646SpokenLanguage="en-GB", MCATitle="Default", MCATitleVersion="n/a", MCAContent="PRM", MCAUseClass="FCMP", ADMAudioProgrammeID="APR_1001"
ADM, chunk_id=axml, MCATitle="No Commentary", MCATitleVersion="n/a", MCAContent="PRM", MCAUseClass="FCMP", ADMAudioProgrammeID="APR_1002"
```

## SMPTE ST 2131 and SMPTE ST 2067-204

SMPTE ST 2131 defines a mechanism for mapping Resource Interchange File Format (RIFF) Chunks containing audio metadata to MXF files to augment MXF Sound Tracks. It defines additional MXF support specifically for the mapping and labeling of content described by Audio Definition Model (ADM) metadata (Recommendation ITU-R BS.2076) which is often carried in Broadcast Wave 64-bit (BW64) RIFF files (Recommendation ITU-R BS.2088).

SMPTE ST 2067-204 defines a plug-in mechanism to add audio with ADM metadata to IMF Compositions.

Both documents are currently available as Public Committee Drafts (PCD):
* https://github.com/SMPTE/st2131
* https://github.com/SMPTE/st2067-204

## bmx support

Provisional support for SMPTE ST 2131 (PCD) has been implemented in `bmx`. This allows the creation of MXF files for a variety of use cases, including ADM Audio Track Files for use in IMF Compositions (per SMPTE ST 2067-204 (PCD)).

The following formats are supported:
* **Component form**
  * Audio as Wave / raw PCM, \<chna\> text file (`bmx` format as [defined below](#chna-text-file-definition-format)) and Wave/RIFF chunk data files (e.g. ADM XML file)
* **Wave (BW64)**
  * Reading Wave (BW64) with access to Wave chunks (e.g. ADM \<axml\>) and a parsed ADM \<chna\> chunk
  * Writing Wave (BW64) with additional Wave chunks (e.g. ADM \<axml\>) and a serialised ADM \<chna\> chunk
* **MXF**
  * Reading MXF that includes Wave chunks and ADM \<chna\> descriptors
  * Writing MXF with Wave chunks and ADM \<chna\> descriptors
  * Support for the ADMAudioMetadataSubDescriptor
  * Support for the "Audio Labeling Framework for ADM-described Content" including the ADMSoundfieldGroupLabelSubDescriptor, the ADMSoundfield label, and signalling in the Channel Assignment audio descriptor property

The I/O combinations supported by `bmx` tools are:

|                   | Output (Component) | Output (Wave) | Output (MXF) |
|-------------------|--------------------|---------------|--------------|
| Input (Component) |                    | raw2bmx       |              |
| Input (Wave)      |                    | raw2bmx       | raw2bmx      |
| Input (MXF)       | mxf2raw            | bmxtranswrap  |              |

Features:
- Preservation of ADM references in the \<chna\> details after remapping audio channels using the `--track-map` option. The option can be used to reorder, omit, group and add silence channels.
- The \<chna\> "summary" properties (numTracks and numUIDs in Wave files; NumLocalChannels and NumADMAudioTrackUIDs in MXF files) are discarded upon read of Wave/MXF files, and automatically calculated upon write of Wave/MXF files.

The known limitations in the current implementation are:
- mxf2raw doesn't yet show ADM metadata presence or extraction of all related signalling.
- There is no parsing of the ADM metadata. Therefore, re-wrapping a file using raw2bmx or bmxtranswrap with an offset or duration change might result in the ADM metadata and references becoming invalid. For example, an ADM audio object might no longer be available after re-wrapping, or its start offset might have changed.
- \> 4 GB chunk size is only supported for the \<data\> chunk. The ADM chunks are unlikely to be that large but ideally `bmx` would provide support.

## Wave+ADM to MXF+ADM mapping process

The process of mapping Wave+ADM to MXF+ADM is basically as follows:

- A \<chna\> chunk is converted to a ADM_CHNASubDescriptor in each Sound Track using the audio channel mapping defined by the `--track-map` option
    - Any null / placeholder entries (those with a trackIndex/track_index of zero) are discarded
- The `--wave-chunks` and `--adm-wave-chunk` options are used to select the (non built-in) chunks to transfer to the MXF file
    - The built-in chunks that can't be selected are: \<JUNK\>, \<ds64\>, \<fmt\>, \<fact\>, \<bext\>, \<data\> and \<chna\>
    - The chunk data is copied into MXF generic streams and each will have a RIFFChunkDefinitionSubDescriptor associated with it
    - Sound Tracks containing channels originating from the input file will reference all the chunks from that input file using RIFFChunkReferencesSubDescriptors
- The `--adm-wave-chunk` option is the same as `--wave-chunks` except that:
    - It identifies only one chunk at a time (but can be used multiple times) and signals that the chunk contains ADM metadata
    - ADM Profile/Level labels can be provided with the option
    - An ADMAudioMetadataSubDescriptor descriptor is created and it sits alongside the RIFFChunkDefinitionSubDescriptor for the chunk

## Creating a Wave+ADM sample file

A Wave+ADM sample file can be created using the following example commandline given a Wave file, a \<chna\> text file and an \<axml\> XML file:

`raw2bmx -t wave -o output.wav --wave-chunk-data axml.xml axml --chna-audio-ids chna.txt --wave input.wav`

The \<axml\> file `axml.xml` contains the data that will be written into the \<axml\> chunk. The \<chna\> text file `chna.txt` lists the audio identifiers that make up the \<chna\> chunk. The format of the text file is described in [\<chna\> Text file Definition Format](#chna-text-file-definition-format).

A Wave+ADM sample file can be created without a \<axml\> chunk, i.e. just a \<chna\> chunk, using the following example commandline given a Wave file (which doesn't have a \<axml\> chunk!) and a \<chna\> text file:

`raw2bmx -t wave -o output.wav --chna-audio-ids chna.txt --wave input.wav`

Raw PCM files can also be used as input. For example, replace:

`--wave input.wav`

with

`-s 48000 -q 24 --audio-chan 2 --pcm audio_0_1.pcm -s 48000 -q 24 --audio-chan 2 --pcm audio_2_3.pcm`

## Converting a Wave+ADM to MXF+ADM

A Wave+ADM can be converted to MXF+ADM using the following example commandline:

`raw2bmx -t op1a -o output.mxf --adm-wave-chunk axml,adm_itu2076 --wave input.wav`

The \<axml\> chunk in this example is known to contain ADM metadata and therefore the `--adm-wave-chunk` option is used. If it doesn't contain ADM metadata and should still be transferred to MXF then use the `--wave-chunks` option. The \<chna\> chunk (mapped to a MXF SubDescriptor) is also written to the output file.

The default layout is to have mono-audio MXF tracks. This can be changed using the `--track-map` option. For example, this command will group two stereo pairs into MXF tracks:

`raw2bmx -t op1a -o output.mxf --track-map '0,1;2,3' --adm-wave-chunk axml,adm_itu2076 --wave input.wav`

Note that the channel index is zero-based in the `--track-map` option.

## Converting a MXF+ADM to Wave+ADM

A MXF+ADM can be converted to Wave+ADM using the following example commandline:

`bmxtranswrap -t wave -o output.wav input.mxf`

The `--track-map` option can be used to change the audio channels. For example, if only the first stereo pair is required:

`bmxtranswrap -t wave -o output.wav --track-map '0,1' input.mxf`

## Converting a Wave+ADM to Wave+ADM

A Wave+ADM can be converted to another Wave+ADM using the following example commandline:

`raw2bmx -t wave -o output.wav --wave-chunks axml --wave input.wav`

The `--wave-chunks` option ensures that the \<axml\> chunk is copied over (in addition to the \<chna\> chunk). The `--adm-wave-chunk` isn't required here because the Wave file format doesn't need to know which chunks contain ADM audio metadata.

The `--wave-chunk-data` option could be used instead of `--wave-chunks` to override the \<axml\> chunk with the contents of an XML file. The `--chna-audio-ids` option could be used to override the \<chna\> chunk.

The `--track-map` option can be used to change what is output. For example, reorder the stereo pairs:

`raw2bmx -t wave -o output.wav --track-map '2,3,0,1' --wave-chunks axml --wave input.wav`

## Extracting ADM metadata and \<chna\> information from an MXF file

The ADM metadata and \<chna\> information in an MXF file can be extracted to external files using the following example commandline:

`mxf2raw --chna-text-out chna --wave-chunks-out chunk input.mxf`

## Add ADM MCA labels to MXF+ADM

### MCA labels text file

An [MCA labels text file](./mca_labels_format.md) is used to specify the Multichannel Audio Labeling (MCA) to add to an MXF file. This is an example:

```text
0
ADM, chunk_id=axml, RFC5646SpokenLanguage="en-GB", MCATitle="Default", MCATitleVersion="n/a", MCAContent="PRM", MCAUseClass="FCMP", ADMAudioProgrammeID="APR_1001"
ADM, chunk_id=axml, MCATitle="No Commentary", MCATitleVersion="n/a", MCAContent="PRM", MCAUseClass="FCMP", ADMAudioProgrammeID="APR_1002"
```

Each soundfield group (SG) "label line" in the text file results in the creation of a SoundfieldGroupLabelSubDescriptor unless a `chunk_id` property is present on that line (in which case a ADMSoundfieldGroupLabelSubDescriptor is created). In this example two ADMSoundfieldGroupLabelSubDescriptor instances are created.

The `chunk_id` property gives the ID of the chunk carrying the associated ADM metadata. This chunk must be mapped using the `--adm-wave-chunk` commandline option (so that it's properly signalled as containing ADM metadata). In this example, both soundfield groups reference the \<axml\> chunk.

The ADM Soundfield Group Label (with MCA Tag Symbol "ADM") can be specified using the symbol "ADM" at the start of the "label line". This is used by both soundfield groups in this example.

Note that `bmx` permits both the SoundfieldGroupLabelSubDescriptor and the ADMSoundfieldGroupLabelSubDescriptor to use any soundfield group label.

The following extra properties can be used on any line where the `chunk_id` property is present:
* ADMAudioProgrammeID
* ADMAudioContentID
* ADMAudioObjectID

### MXF creation commandline for MCA

Example:

`raw2bmx -t op1a -o output.mxf --track-map '0,1' --track-mca-labels x mca.txt --audio-layout adm --adm-wave-chunk axml --wave input.wav`

In this example the MCA labels text file is called `mca.txt`.

(_note_: the `x` in `--track-mca-labels` is a legacy option component and will be ignored)

`--audio-layout adm` populates the Channel Assignment audio descriptor property with the AudioLabelingFrameworkADMContent label.

### MXF Inspection with `mxf2raw`

The soundfield groups that use the ADMSoundfieldGroupLabelSubDescriptor are shown using a `(ADM)` suffix in the summary output of `mxf2raw`. For example, both soundfield groups in the example above will appear as `ADM(ADM)`. If the `--mca-detail` option is used and the soundfield group uses the ADMSoundfieldGroupLabelSubDescriptor then the name used in the `SoundfieldGroups` array is `ADMSoundfieldGroup`.

## \<chna\> Text File Format

`bmx` uses a \<chna\> text file for input/output of \<chna\> information. This is the information stored in the \<chna\> chunk in Wave files and in the ADM_CHNASubDescriptor in MXF files.

The \<chna\> text file contains a list of ADM audio identifiers. There is a set of name/value property pairs for each audio identifier, separated by a newline. The name/value pair uses the first ':' character as a separator. A '#' character signals the start of a comment.

The properties for each audio identifier are as follows:

- `track_index`: The index of the track in the Wave file or the index of the channel in the MXF Sound Track. The index *starts from 1* in both cases. A value 0 represents a null / placeholder entry.
- `uid`: The audioTrackUID value
- `track_ref`: The linked audioTrackFormatID or audioChannelFormatID value
- `pack_ref`: The linked audioPackFormatID value

Each audio identifier is started by a `track_index` property.

Note that the additional \<chna\> properties are not stored in the \<chna\> text file (numTracks and numUIDs in Wave files; NumLocalChannels and NumADMAudioTrackUIDs in MXF files). These are discarded upon read of Wave/MXF files, and automatically calculated upon write of Wave/MXF files.

An example \<chna\> text file is shown below. It describes two stereo pairs, each with "front left" and "front right" channels.

```text
track_index: 1
uid: ATU_00000001
track_ref: AT_00010001_01
pack_ref: AP_00010002

track_index: 2
uid: ATU_00000002
track_ref: AT_00010002_01
pack_ref: AP_00010002

track_index: 3
uid: ATU_00000003
track_ref: AT_00010001_01
pack_ref: AP_00010002

track_index: 4
uid: ATU_00000004
track_ref: AT_00010002_01
pack_ref: AP_00010002
```
