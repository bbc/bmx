# IMF Audio Track Files

The following examples demonstrate how the [IMF Flavour](./imf_track_files.md) of raw2bmx can be used to create Audio Track Files, from multichannel WAV files, that conform to version 1.2 of the [BBC UHD Delivery Document](https://www.dropbox.com/s/tkvwxksgy3izpca/TechnicalDeliveryStandardsBBCUHDiPlayerSupplement.pdf?dl=0). They have wide general applicability as they follow [IMF User Group Best Practice](https://www.imfug.com/TR/audio-track-files/).

```bash
raw2bmx -o {Type}_{fp_uuid}.mxf \
-t imf \
--track-mca-labels as11 labels.txt \
--wave source.wav
```

where `labels.txt` (format described in [mca_labels_format.md](./mca_labels_format.md)) contains, for a 5.1 surround soundfield,

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

Creation of the corresponding Image Track File is described in [imf_prores_track_files.md](./imf_prores_track_files.md).
