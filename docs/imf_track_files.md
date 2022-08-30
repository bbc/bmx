# IMF Track Files

The MXF OP1a writer has an Interoperable Master Format (IMF) flavour that helps with the creation of IMF Track Files (as defined in [SMPTE ST 2067-2](https://ieeexplore.ieee.org/document/9097478)). The flavour makes it easier to create IMF Track Files by pre-selecting certain required options. However, it does not have knowledge of all the Track Files defined for IMF (across SMPTE ST 2067-2, the IMF Applications, the IMF Plug-ins, etc) and it does not enforce compliance. For example, it does not check that the input essence and metadata will result in a compliant Track File.

The IMF flavour is enabled using the `-t imf` clip type option in bmxtranswrap and raw2bmx. This results in the following settings for OP1a,

* fill after the header metadata is at least 8192 bytes (`--head-fill 8192`)
* index table follows the essence, even for clip-wrapped audio (`--index-follows`)
* creates partitions every 60 seconds for frame-wrapped essence (`--part 60`)
* audio clip-wrapping (`--clip-wrap`)
* combines audio into a single track by default (`--track-map singlemca`)
* adds a Primary Package property (`--primary-package`)
* disables creation of timecode tracks (`--no-tc-track`)
* adds the audio Channel Assignment label descriptor property and sets it to the IMF label by default (`--audio-layout imf`). Use the `--audio-layout` option to override this default value.
* add the Display F2 Offset and sets it to 0 for progressive JPEG 2000 and RDD 36 video (`--display-f2-offset 0`)

The settings can be found in the code by searching for the OP1a define `OP1A_IMF_FLAVOUR` and the descriptor define `MXFDESC_IMF_FLAVOUR`. E.g. see `if ((flavour & OP1A_IMF_FLAVOUR))` in `src/mxf_op1a/OP1AFile.cpp` for the bulk of the settings.

IMF Track Files contain only a single essence component. bmx provides support for creating such files. If the input MXF files contain multiple essence tracks then use options such as `--disable-video`, `--disable-audio`, `--disable-data` and `--track-map` to disable tracks.

A number of topics related to creating complete IMF Track Files can be found in the [docs/](./) directory, including Multi-channel Audio Labels, Timed Text, JPEG 2000 and ProRes.

The bmxtranswrap and raw2bmx tools allow setting metadata defined in [SMPTE ST 2067-2](https://ieeexplore.ieee.org/document/9097478),

* Reference Image Edit Rate (`--ref-image-edit-rate`)
* Reference Audio Alignment Level (`--ref-audio-align-level`)
* Active Height (`--active-height`)
* Active Width (`--active-width`)
* Active X Offset (`--active-x-offset`)
* Active Y Offset (`--active-y-offset`)
* Alternative Center Cuts (`--center-cut-4-3` and `--center-cut-14-9`)

and [SMPTE ST 2067-21 - Application #2E](https://ieeexplore.ieee.org/document/9097487),

* Mastering Display Primaries (`--display-primaries`)
* Mastering Display White Point Chromaticity (`--display-white-point`)
* Mastering Display Maximum Luminance (`--display-max-luma`)
* Mastering Display Minimum Luminance (`--display-min-luma`)
