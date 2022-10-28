# Audio Definition Model (ADM) Support

A SMPTE project named "31FS ST Mapping ADM to MXF" started in March 2022 to "define a means of mapping audio metadata RIFF chunks to MXF with specific consideration of the requirements related to ADM metadata (for example, with reference to ADM carriage in BW64 files per Rec. ITU-R BS.2088-1)."

Provisional support for ADM has been implemented in `bmx` to allow samples files to be created to support the SMPTE project. The current levek of support in the `bmx` implementation is as follows:

- Reading Wave (BW64) with access to Wave chunks (e.g. ADM \<axml\>) and a parsed ADM \<chna\> chunk
- Writing Wave (BW64) with additional Wave chunks (e.g. ADM \<axml\>) and a serialised ADM \<chna\> chunk
- Reading MXF that includes Wave chunks and ADM \<chna\> descriptors
- Writing MXF with Wave chunks and ADM \<chna\> descriptors
- Converting Wave+ADM to MXF+ADM using raw2bmx
- Converting Wave+ADM to Wave+ADM using raw2bmx
- Converting MXF+ADM to Wave+ADM using bmxtranswrap
- Creating Wave+ADM with Wave / raw PCM, \<chna\> text file and (optionally) Wave chunk file inputs (e.g. containing \<axml\> chunk data)
- Preservation of ADM links after remapping audio channels using the `--track-map` option. The option can be used to reorder, omit, group and add silence channels.
- Option to add a ADM Soundfield Group MCA label and identify ADM chunks
- Set the Channel Assignment audio descriptor property for ADM-described content labelling

The known limitations in the current support are:

- mxf2raw doesn't yet show ADM metadata presence or allow ADM chunks or a \<chna\> text file to be extracted.
- Re-wrapping a file using raw2bmx or bmxtranswrap with an offset or duration change may result in the ADM metadata and links becoming invalid. E.g. an audio object is no longer available in the new duration or the start offset has changed. The \<axml\>, \<bxml\> or \<sxml\> chunks are not parsed to ensure that the re-wrap retains valid ADM.
- \> 4 GB chunk size is only supported for the data chunk. The ADM chunks are unlikely to be that large but it's worth adding support just in case that assumption is wrong.

## Wave+ADM to MXF+ADM mapping process

The process of mapping Wave+ADM to MXF+ADM is basically as follows:

- A \<chna\> chunk is converted to a ADM_CHNASubDescriptor descriptor in each track using the audio channel mapping defined by the `--track-map` option
- The `--wave-chunk` and `--adm-wave-chunk` options are used to select the (non-builtin) chunks to transfer to the MXF file
    - The builtin chunks that can't be selected are: \<JUNK\>, \<ds64\>, \<fmt\>, \<fact\>, \<bext\>, \<data\> and \<chna\>
    - The chunk data is copied into MXF generic streams and each will have a RIFFChunkDefinitionSubDescriptor associated with it
    - Audio tracks containing channels originating from the input file will reference all the chunks from that input file using RIFFChunkReferencesSubDescriptors
- The `--adm-wave-chunk` option is also used to signal that the chunk contains ADM audio metadata. Profile and level labels can be provided with the option
    - A ADMAudioMetadataSubDescriptor descriptor is created, alongside the RIFFChunkDefinitionSubDescriptor, to identify these ADM audio metadata chunks. This descriptor holds the profile and level labels to which the metadata complies
- ADM Soundfield Group labels are required to reference the chunk containing ADM audio metadata and described using a ADMAudioMetadataSubDescriptor descriptor. The ADM `chunk_id` property is used in the labels text file to identify the chunk data and it is converted to the MXF generic stream ID

## Creating a Wave+ADM sample file

A Wave+ADM sample file can be created using the following example commandline given a Wave file, a \<chna\> text file and an \<axml\> XML file:

`raw2bmx -t wave -o output.wav --wave-chunk-data axml.xml axml --chna-audio-ids chna.txt --wave input.wav`

The \<axml\> file `axml.xml` contains the data that will be written into the \<axml\> chunk. The \<chna\> text file `chna.txt` lists the audio identifiers that make up the \<chna\> chunk. The format of the text file is described in [\<chna\> Text file Definition Format](#chna-text-file-definition-format).

If the source Wave file contained an \<axml\> chunk then the `--wave-chunk-data` option will override it. If the source Wave file contained a \<chna\> chunk then the `--chna-audio-ids chna.txt` option will override it.

A Wave+ADM sample file can be created without a \<axml\> chunk, i.e. just a \<chna\> chunk, using the following example commandline given a Wave file (which doesn't have a \<axml\> chunk!) and a \<chna\> text file:

`raw2bmx -t wave -o output.wav --chna-audio-ids chna.txt --wave input.wav`

Raw PCM files can also be used as input, e.g. replace `--wave input.wave` with:

`-s 48000 -q 24 --audio-chan 2 --pcm audio_0_1.pcm -s 48000 -q 24 --audio-chan 2 --pcm audio_2_3.pcm`

## Converting a Wave+ADM to MXF+ADM

A Wave+ADM can be converted to MXF+ADM using the following example commandline:

`raw2bmx -t op1a -o output.mxf --adm-wave-chunk axml,adm_itu2076 --wave input.wave`

The \<axml\> chunk in this example is known to contain ADM metadata and therefore the `--adm-wave-chunk` option is used. If it doesn't contain ADM metadata and should still be transferred to MXF then use the `--wave-chunk` option. The \<chna\> chunk (mapped to a MXF descriptor) is also written to the output file.

The default layout is to have mono-audio MXF tracks. This can be changed using the `--track-map` option. For example, this command will group stereo pairs into MXF tracks:

`raw2bmx -t op1a -o output.mxf --track-map '0,1;2,3' --wave input.wave`

Note that the channel index is zero-based in the `--track-map` option.

## Converting a MXF+ADM to Wave+ADM

A MXF+ADM can be converted to Wave+ADM using the following example commandline:

`bmxtranswrap -t wave -o output.wav input.mxf`

The `--track-map` option can be used to change the audio channels. For example, if only the first sereo pair is required:

`bmxtranswrap -t wave -o output.wav --track-map '0,1' input.mxf`

## Converting a Wave+ADM to Wave+ADM

A Wave+ADM can be converted to another Wave+ADM using the following example commandline:

`raw2bmx -t wave -o output.wave --wave-chunk axml --wave input.wave`

The `--wave-chunk` option ensures that the \<axml\> chunk is copied over (in addition to the \<chna\> chunk). The `--adm-wave-chunk` isn't required here because the Wave file format doesn't need to know which chunks contain ADM audio metadata.

The `--track-map` option can be used to change what is output. For example, reorder the stereo pairs:

`raw2bmx -t wave -o output.wave --track-map '2,3,0,1' --wave-chunk axml --wave input.wave`

## Add ADM MCA labels to MXF+ADM

A ADM Soundfield Group Label Subdescriptor is created from a [MCA labels text file](./mca_labels_format.md) if the list of properties associated with the Soundfield Group contains an ADM property (the ADM properties have the prefix "ADM") or it is a ADM Soundfield Group label and there are no Audio Channels that reference it.

A ADM Soundfield Group label can be added using the `ADM` symbol in a [MCA labels text file](./mca_labels_format.md).

`mca.txt`:

```text
0
chL, chan=0
chR, chan=1
sgST, lang=eng, mca_title="Example Programme"
ADM, chunk_id=axml, adm_audio_object_id="AO_1001"

1
chL, chan=0
chR, chan=1
sgST, lang=eng, mca_title="Example Programme"
ADM, chunk_id=axml, adm_audio_object_id="AO_1002"
```

Note the (required) `chunk_id` property for the ADM Soundfield Group Label which identifies the \<axml\> chunk containing the ADM audio metadata. The `--adm-wave-chunk` option must be used to signal that it contains ADM audio metadata.

The MCA labels defined in `mca.txt` can be added and the ADM-described content label can be set in the audio descriptor Channel Assignment property using the following example commandline: (_note_: the `x` in `--track-mca-labels` is a legacy option component and will be ignored)

`raw2bmx -t op1a -o output.mxf --track-map '0,1' --track-mca-labels x mca.txt --audio-layout adm --adm-wave-chunk axml --wave input.wave`

The Soundfield Groups that use the ADM Soundfield Group Label Subdescriptor are shown using a `(ADM)` suffix in the summary output of `mxf2raw`. E.g. `ADM(ADM)` in the example `mca.txt` above. If the `--mca-detail` option is used and the Soundfield Group uses the ADM Soundfield Group Label Subdescriptor then the name used in the `SoundfieldGroups` array is `ADMSoundfieldGroup`.

## \<chna\> Text File Definition Format

The \<chna\> text file contains the list of ADM audio identifiers that make up the \<chna\> chunk. The number of tracks and UIDs is automatically calculated.

The \<chna\> text file consists of a set of name/value property pairs separated by a newline. The name/value pair uses the first ':' character as a separator. A '#' character signals the start of a comment.

The properties for each audio identifier are as follows.

- `track_index`: The index of the track in the Wave file, starting from 1. A value 0 represents a null / placeholder entry.
- `uid`: The audioTrackUID value of the track.
- `track_ref`: The audioTrackFormatID reference for the track.
- `pack_ref`: The audioPackFormatID reference for the track.

Each audio identifier is started by a `track_index` property.

A `track_index` 0 represents a null / placeholder entry that is used to support Wave file updates. These entries are stripped when mapping to MXF.

An example \<chna\> text file is shown below. It describes 2 stereo pairs, each with "front left" and "front right" channels.

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
