# IMF ProRes Track Files

The [IMF Flavour](./imf_track_files.md) of `bmxtranswrap` and `raw2bmx` can be used to create Image Track Files conforming to [SMPTE RDD 45](https://ieeexplore.ieee.org/document/8233487) Application ProRes or [SMPTE RDD 59-1](https://ieeexplore.ieee.org/document/9999622) Application DPP (ProRes) (which is based on RDD 45).

RDD 45 specifies certain [SMPTE RDD 36](https://ieeexplore.ieee.org/document/7438722) ProRes bitstream parameters that must be set. bmx will populate the Essence Descriptor from the bitstream and other values supplied on the command line or as defaults.

Here is a snippet of `rdd36dump` output for a suitable [ITU-R BT.2100](https://www.itu.int/rec/R-REC-BT.2100) UHD, HLG transfer function, 25Hz, YCbCr video source bitstream, showing the parameters that are mentioned in RDD 45:

```text
horizontal_size: 3840
vertical_size: 2160
chroma_format: 2 (4:2:2)
interlace_mode: 0 (Progressive frame)
aspect_ratio_information: 0 (Unknown/unspecified)
frame_rate_code: 0 (Unknown/unspecified)
color_primaries: 9 (ITU-R BT.2020)
transfer_characteristic: 18 (HLG (reserved))
matrix_coefficients: 9 (ITU-R BT.2020 (NCL))
alpha_channel_type: 0 (Not present)
```

Note that RDD 45 specifies that `aspect_ratio_information` and `frame_rate_code` be ignored by readers, but despite this, if they are not set to "Unknown" they should be the correct values to avoid confusion.

The following example creates an Image Track File, from a bitstream formatted as above, that conforms to version 1.2 of the [BBC UHD Delivery Document](https://www.dropbox.com/s/tkvwxksgy3izpca/TechnicalDeliveryStandardsBBCUHDiPlayerSupplement.pdf?dl=0):

```bash
raw2bmx -o {Type}_{fp_uuid}.mxf \
-t imf \
-f 25 \
-c 10 \
--color-siting cositing \
--black-level 64 \
--white-level 940 \
--color-range 897 \
--rdd36_422_hq prores_hlg_source.raw
```

Creation of the corresponding Audio Track Files is described in [imf_audio_track_files.md](./imf_audio_track_files.md).
