# MCA Labels Format

The `--track-mca-labels` option in raw2bmx and bmxtranswrap can be used to add Multi-channel Audio Labels as specified in SMPTE ST 377-4:2021. The format for the text file that is passed to that option is described in this document.

## General Layout

The text file contains one of the following line types:

- audio track index
- audio label
- empty

A `#` character can be used to add a comment.

The file defines the set of audio labels to include for each MXF audio track. The audio tracks are identified by an integer index starting from 0 and incrementing by 1 in the order that the audio tracks are written (the Track ID order).

A set of labels for an audio track starts with a line containing the index and is followed by lines defining audio labels. There can be multiple channel (CH) label lines, 0 or 1 soundfield group (SG) label lines and 0 or more group of soundfield groups (GOSG) label lines.

An empty line is used to signal the start of a new audio track.

## Label Line

A label line starts with the value of the Tag Symbol label property, followed by a comma separated list of name/value pairs of properties for that label. The list of supported symbols can be found in the [AS11WriterHelper.cpp](../src/as11/AS11WriterHelper.cpp) source code file. The symbol identifies ether a CH (e.g. `chL` for Left), SG (e.g. `sg51` for 5.1) or GOSG (e.g. `ggMPg` for Main Program).

### Label Line Properties

A CH must have a `chan` property that is the channel number, starting from 0, if the track contains multiple audio channels (labelled or not).

The `lang` property is used to set the RFC 5646 Spoken Language label property.

The `id` property is used to identify a SG or GOSG label in the file. It has an alpha-numeric value. It is used to set the link from a CH to a SG and from a SG to GOSGs.

If multiple labels have the same `id` then they will be copies of the same label. As a result, the first one that appears in the file should have all its properties set and all others with the same `id` should have no other properties set, apart from possibly `repeat` (see below). This results in the properties being the same as those of the first one.

The `id` property is only needed if links are made to audio labels in other tracks, which is done by assigning them the same `id` value. Links will be automatically created for labels that appear in the same track.

The `repeat` property is used to decide whether to repeat a SG or GOSG label that is already defined in another track with the same `id`. It is a boolean property that defaults to True and can be set to False using the value `false`, `0` or `no`.

The first instance of a SG or GOSG label with a given `id` will always be stored in the MXF track's file descriptor. Subsequent SG or GOSG labels with the same `id` will be stored as a copy of the earlier label if `repeat` is True.

## Example Files

This example below is an English language Main Program 5.1.

Each CH is labelled for the placement in the 5.1 SG. The SG is linked to a Main Program GOSG.

```text
0
chL, chan=0
chC, chan=1
chR, chan=2
chLs, chan=3
chRs, chan=4
chLFE, chan=5
sg51
ggMPg, lang=en
```

The example below is for a French language Alternative Program stereo pair.

The 1st audio track contains the left channel and the 2nd audio track the right channel. The SG and GOSG labels are not repeated in the 2nd track, but the `id` properties link them to the labels in the 1st track.

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
