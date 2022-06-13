# MCA Labels Format

The `--track-mca-labels` option in raw2bmx and bmxtranswrap can be used to add Multi-channel Audio Labels as specified in SMPTE ST 377-4:2021. The format for the text file that is passed to that option is described in this document.

## General Layout

The text file contains one of the following line types:

- audio track index
- audio channel (CH), soundfield group (SG) or group of soundfield group (GOSG) label
- empty

A `#` character can be used to add a comment.

The file defines the set of audio labels to include for each MXF audio track. The audio tracks are identified by an integer index starting from 0 and incrementing by 1 in the order that the audio tracks are written (the Track ID order).

A set of labels for an audio track starts with a line containing the index and is followed by lines defining audio labels. There can be multiple channel CH label lines, 0 or 1 SG label lines and 0 or more GOSG label lines.

An empty line is used to signal the start of a new audio track.

## Label Line

 A label line consists of the value of the MCA Tag Symbol label property, followed by a comma and a comma-separated list of name`=`value pairs of properties for that label.

 The label identifies either a CH (e.g. `chL` for Left), SG (e.g. `sg51` for 5.1) or GOSG (e.g. `ggMPg` for Main Program). The list of supported labels can be found in the [AppMCALabelHelper.cpp](../src/apps/AppMCALabelHelper.cpp) source code file. The supported names are listed below.

 The label properties are described in the next section, including how to insert characters such as commas into values.

### Label Line Properties

A CH must have a `chan` property that is the channel number, starting from 0, if the track contains multiple audio channels (labelled or not).

The `id` property is used to identify a SG or GOSG label in the file. It has an alpha-numeric value. It is used to set the link from a CH to a SG and from a SG to GOSGs.

If multiple labels have the same `id` then they are instances of the same label. As a result, the first one that appears in the file should have all its properties set and all others with the same `id` need have no other properties set, apart from possibly `repeat` (see below). This results in the properties being the same as those of the first one.

The `id` property is only needed if they are referenced by or reference audio labels in other tracks. This is done by assigning them the same `id` value.

The `repeat` property is used to decide whether to repeat a SG or GOSG label that is already defined in another track with the same `id`. It is a boolean property that defaults to True and can be set to False using the value `false`, `0` or `no`.

The first instance of a SG or GOSG label with a given `id` will always be stored in the MXF track's file descriptor. Subsequent SG or GOSG labels with the same `id` will be stored as a copy of the earlier label if `repeat` is True.

The following string value properties can be used:

- RFC5646SpokenLanguage (alternative is "lang")
- MCATitle
- MCATitleVersion
- MCATitleSubVersion
- MCAEpisode
- MCAPartitionKind
- MCAPartitionNumber
- MCAAudioContentKind
- MCAAudioElementKind
- MCAContent
- MCAUseClass
- MCAContentSubtype
- MCAContentDifferentiator
- MCASpokenLanguageAttribute
- RFC5646AdditionalSpokenLanguages
- MCAAdditionalLanguageAttributes

The "lang" name can be used instead of "RFC5646SpokenLanguage".

The name may omit the "MCA" prefix, e.g. "TitleVersion" is accepted. The name matching is case-insenstive, e.g. "titleversion" is accepted. Underscores are ignored, e.g. "title_version" is accepted.

The property value may be between quotation marks, either single or double quotes.

Some special characters (e.g. used in RFC5646AdditionalSpokenLanguages and MCAAdditionalLanguageAttributes) can be set by using the `\` escape character. They are tab `\t` (0x09), line feed `\n` (0x0a) and carriage return `\r` (0x0d).

The `\` escape character can be used before special characters, e.g. characters such as `,` and `#` when not using quotation marks or `"` when using double quotes. A `\` character can be inserted using a double backslash `\\`.

## Example Files

This example below is an English language Main Program 5.1.

Each CH is labelled for the placement in the 5.1 SG. The SG is linked to a Main Program GOSG. The GOSG has a title (MCATitle) and title version (MCATitleVersion).

```text
0
chL, chan=0
chC, chan=1
chR, chan=2
chLs, chan=3
chRs, chan=4
chLFE, chan=5
sg51
ggMPg, lang=en, title="Examples", title_version="The final cut example"
```

The example below is for a French language Alternative Program stereo pair.

The 1st audio track contains the left channel and the 2nd audio track the right channel. The SG and GOSG labels are not repeated in the 2nd track, but the `id` properties link them to the labels in the 1st track.

The GOSG label shown in the 2nd track is not required because the link is already provided in the 1st track. It is nice to have it in the 2nd track for clarity.

```text
0
chL
sgST, id=sg1
ggAPg, id=gosg1, lang=fr

1
chR
sgST, id=sg1, repeat=false
ggAPg, id=gosg1, repeat=false
```
