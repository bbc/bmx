# IMF ProRes Track Files

The [IMF Flavour](imf_track_files.md) of bmxtranswrap and raw2bmx can be used to create Image Track Files conforming to [SMPTE RDD 45](https://ieeexplore.ieee.org/document/8233487) Application ProRes or [SMPTE RDD 59-1](https://github.com/SMPTE/rdd59-1) Application DPP (ProRes).

RDD 45 specifies certain [SMPTE RDD 36](https://ieeexplore.ieee.org/document/7438722) ProRes bitstream parameters that must be set. bmx will populate the Essence Descriptor from the bitstream and other values supplied on the command line.

The following example creates an Image Track File, from a Hybrid Log-Gamma (HLG) bitstream, that conforms to version 1.2 of the [BBC UHD Delivery Document](https://www.dropbox.com/s/tkvwxksgy3izpca/TechnicalDeliveryStandardsBBCUHDiPlayerSupplement.pdf?dl=0):

```bash
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

Creation of the corresponding Audio Track Files is described in [imf_audio_track_files.md](imf_audio_track_files.md).
