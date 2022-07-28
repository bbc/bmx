# IMF ProRes Track Files

The [IMF Flavour](imf_track_files.md) of bmxtranswrap and raw2bmx can be used to create Image Track Files conforming to [SMPTE RDD 45](https://ieeexplore.ieee.org/document/8233487) Application ProRes or [SMPTE RDD 59-1](https://github.com/SMPTE/rdd59-1) Application DPP (ProRes).

RDD 45 specifies certain [SMPTE RDD 36](https://ieeexplore.ieee.org/document/7438722) ProRes bitstream parameters that must be set. bmx will populate the Essence Descriptor from the bitstream and other values supplied on the command line.

The following example creates an Image Track File, from a Hybrid Log-Gamma (HLG) bitstream, that conforms to version 1.2 of the [BBC UHD Delivery Document](https://www.dropbox.com/s/tkvwxksgy3izpca/TechnicalDeliveryStandardsBBCUHDiPlayerSupplement.pdf?dl=0):

```text
raw2bmx -o VIDEO.mxf \
-t imf \
-f 25 \
-c 10 \
--color-siting cositing \
--black-level 64 \
--white-level 940 \
--color-range 897 \
--rdd36_422_hq prores_hlg_source.raw
```

## Audio Track Files

For completeness, the following examples demonstrate the creation of Audio Track Files conforming to the BBC Delivery Document, and with wide applicability as they follow [IMF User Group Best Practice](https://www.imfug.com/TR/audio-track-files/).

```text
raw2bmx -o AUDIO.mxf \
-t imf \
--track-mca-labels as11 labels.txt \
--wave source.wav
```

where `labels.txt` (format described in [mca_labels_format.md](mca_labels_format.md)) contains, for a 5.1 surround soundfield,

``` text
0
chL, chan=0
chR, chan=1
chC, chan=2
chLFE, chan=3
chLs, chan=4
chRs, chan=5
sg51, lang=en-GB, mcaaudiocontentkind=PRM, mcaaudioelementkind=FCMP, mcatitle=n/a, mcatitleversion=n/a
```

and for a stereo soundfield,

```text
0
chL, chan=0
chR, chan=1
sgST, lang=en-GB, mcaaudiocontentkind=PRM, mcaaudioelementkind=FCMP, mcatitle=n/a, mcatitleversion=n/a
```
