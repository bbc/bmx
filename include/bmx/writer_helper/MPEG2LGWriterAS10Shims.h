#ifndef BMX_MPEG2_AS10_SHIMS_H_
#define BMX_MPEG2_AS10_SHIMS_H_

#include <bmx/BMXShimNames.h>

//akl: ok, lot's of defs, 90ies style :-)
#define D_SHIMNAME         "shimname"
#define D_SINGLESEQUENCE   "issinglesequence"
#define D_ASPECTRATIO      "aspectratio"
#define D_SAMPLERATE       "samplerate"
#define D_BITRATE          "bitrate"
#define D_BITRATE_DELTA    "bitratedelta"
#define D_ISCONSTBITRATE   "isconstantbitrate"
#define D_ISPROGRESSIVE    "isprogressive"
#define D_UNIQUERESOLUTION "isuniqueresolution"
#define D_HSIZE            "hsize"
#define D_VSIZE            "vsize"
#define D_CROMA            "croma"
#define D_COLORPRIMARIES   "colorprimaries"
#define D_ISTFF            "istff"
#define D_ISLOWDELAY       "islowdelay"
#define D_ISCLOSEDGOP      "isclosedgop"
#define D_CONSTGOP         "isidenticalgop"
#define D_MAXGOP           "maxgop"
#define D_MAXBFRAMES       "maxbframes"
#define D_CONSTBFRAMES     "isconstbframes"

#define D_BOOL_IS_ANY      2 
#define D_RATIONAL_IS_ANY  ZERO_RATIONAL

MpegDefaults as10_high_hd_2014_mpeg_values = { 
  D_AS10_HIGH_HD_2014,
{ D_SINGLESEQUENCE,   2, 0, false, 0 },
{ D_ASPECTRATIO,      ASPECT_RATIO_16_9, ASPECT_RATIO_16_9, false, 0 },
{ D_SAMPLERATE,       D_RATIONAL_IS_ANY, D_RATIONAL_IS_ANY, false, 0 },
{ D_BITRATE,          50000000, 0, false, 0 },
{ D_BITRATE_DELTA,    1000000, 0, false, 0 },
{ D_ISCONSTBITRATE,   1, 0, false, 0 },
{ D_ISPROGRESSIVE,    D_BOOL_IS_ANY, 0, false, 0 },
{ D_UNIQUERESOLUTION, 1, 0, false, 0 },
{ D_HSIZE,            1920, 0, false, 0 },
{ D_VSIZE,            1080, 0, false, 0 },
{ D_CROMA,            422, 0, false, 0 },
{ D_COLORPRIMARIES,   1, 0, false, 0 },
{ D_ISTFF,            1, 0, false, 0 },
{ D_ISLOWDELAY,       0, 0, false, 0 },
{ D_ISCLOSEDGOP,      1, 0, false, 0 },
{ D_CONSTGOP,         1, 0, false, 0 },
{ D_MAXGOP,           16, 0, false, 0 },
{ D_MAXBFRAMES,       2, 0, false, 0 },
{ D_CONSTBFRAMES,     0, 0, false, 0 }
};

MpegDefaults as10_cnn_hd_2012_mpeg_values = { 
  D_AS10_CNN_HD_2012,
{ D_SINGLESEQUENCE,   2, 0, false, 0 },
{ D_ASPECTRATIO,      ASPECT_RATIO_16_9, ASPECT_RATIO_16_9, false, 0 },
{ D_SAMPLERATE,       D_RATIONAL_IS_ANY, D_RATIONAL_IS_ANY, false, 0 },
{ D_BITRATE,          35000000, 0, false, 0 },
{ D_BITRATE_DELTA,    0, 0, false, 0 },
{ D_ISCONSTBITRATE,   0, 0, false, 0 },
{ D_ISPROGRESSIVE,    D_BOOL_IS_ANY, 0, false, 0 },
{ D_UNIQUERESOLUTION, 1, 0, false, 0 },
{ D_HSIZE,            1440, 0, false, 0 },
{ D_VSIZE,            1080, 0, false, 0 },
{ D_CROMA,            420, 0, false, 0 },
{ D_COLORPRIMARIES,   1, 0, false, 0 },
{ D_ISTFF,            1, 0, false, 0 },
{ D_ISLOWDELAY,       0, 0, false, 0 },
{ D_ISCLOSEDGOP,      1, 0, false, 0 },
{ D_CONSTGOP,         1, 0, false, 0 },
{ D_MAXGOP,           16, 0, false, 0 },
{ D_MAXBFRAMES,       2, 0, false, 0 },
{ D_CONSTBFRAMES,     0, 0, false, 0 }
};

MpegDefaults as10_nrk_hd_2012_mpeg_values = { 
  D_AS10_NRK_HD_2012,
{ D_SINGLESEQUENCE,   2, 0, false, 0 },
{ D_ASPECTRATIO,      ASPECT_RATIO_16_9, ASPECT_RATIO_16_9, false, 0 },
{ D_SAMPLERATE,       D_RATIONAL_IS_ANY, D_RATIONAL_IS_ANY, false, 0 },
{ D_BITRATE,          50000000, 0, false, 0 },
{ D_BITRATE_DELTA,    1000000, 0, false, 0 },
{ D_ISCONSTBITRATE,   1, 0, false, 0 },
{ D_ISPROGRESSIVE,    D_BOOL_IS_ANY, 0, false, 0 },
{ D_UNIQUERESOLUTION, 1, 0, false, 0 },
{ D_HSIZE,            1920, 0, false, 0 },
{ D_VSIZE,            1080, 0, false, 0 },
{ D_CROMA,            422, 0, false, 0 },
{ D_COLORPRIMARIES,   1, 0, false, 0 },
{ D_ISTFF,            1, 0, false, 0 },
{ D_ISLOWDELAY,       0, 0, false, 0 },
{ D_ISCLOSEDGOP,      1, 0, false, 0 },
{ D_CONSTGOP,         1, 0, false, 0 },
{ D_MAXGOP,           16, 0, false, 0 },
{ D_MAXBFRAMES,       2, 0, false, 0 },
{ D_CONSTBFRAMES,     0, 0, false, 0 }
};

MpegDefaults as10_jvc_hd_35_vbr_2012_mpeg_values = { 
   D_AS10_JVC_HD_35_VBR,
{ D_SINGLESEQUENCE,   2, 0, false, 0 },
{ D_ASPECTRATIO,      ASPECT_RATIO_16_9, ASPECT_RATIO_16_9, false, 0 },
{ D_SAMPLERATE,       D_RATIONAL_IS_ANY, D_RATIONAL_IS_ANY, false, 0 },
{ D_BITRATE,          35000000, 0, false, 0 },
{ D_BITRATE_DELTA,    0, 0, false, 0 },
{ D_ISCONSTBITRATE,   0, 0, false, 0 },
{ D_ISPROGRESSIVE,    D_BOOL_IS_ANY, 0, false, 0 },
{ D_UNIQUERESOLUTION, 1, 0, false, 0 },
{ D_HSIZE,            1440, 0, false, 0 },
{ D_VSIZE,            1080, 0, false, 0 },
{ D_CROMA,            420, 0, false, 0 },
{ D_COLORPRIMARIES,   1, 0, false, 0 },
{ D_ISTFF,            1, 0, false, 0 },
{ D_ISLOWDELAY,       0, 0, false, 0 },
{ D_ISCLOSEDGOP,      1, 0, false, 0 },
{ D_CONSTGOP,         1, 0, false, 0 },
{ D_MAXGOP,           16, 0, false, 0 },
{ D_MAXBFRAMES,       2, 0, false, 0 },
{ D_CONSTBFRAMES,     0, 0, false, 0 }
};

MpegDefaults as10_jvc_hd_25_cbr_2012_mpeg_values = { 
  D_AS10_JVC_HD_25_CBR,
{ D_SINGLESEQUENCE,   2, 0, false, 0 },
{ D_ASPECTRATIO,      ASPECT_RATIO_16_9, ASPECT_RATIO_16_9, false, 0 },
{ D_SAMPLERATE,       D_RATIONAL_IS_ANY, D_RATIONAL_IS_ANY, false, 0 },
{ D_BITRATE,          25000000, 0, false, 0 },
{ D_BITRATE_DELTA,    0, 0, false, 0 },
{ D_ISCONSTBITRATE,   1, 0, false, 0 },
{ D_ISPROGRESSIVE,    D_BOOL_IS_ANY, 0, false, 0 },
{ D_UNIQUERESOLUTION, 1, 0, false, 0 },
{ D_HSIZE,            1440, 0, false, 0 },
{ D_VSIZE,            1080, 0, false, 0 },
{ D_CROMA,            420, 0, false, 0 },
{ D_COLORPRIMARIES,   1, 0, false, 0 },
{ D_ISTFF,            1, 0, false, 0 },
{ D_ISLOWDELAY,       0, 0, false, 0 },
{ D_ISCLOSEDGOP,      1, 0, false, 0 },
{ D_CONSTGOP,         1, 0, false, 0 },
{ D_MAXGOP,           16, 0, false, 0 },
{ D_MAXBFRAMES,       2, 0, false, 0 },
{ D_CONSTBFRAMES,     0, 0, false, 0 }
}; 

descriptor AS10_MXF_Descriptors_Arr[] = {
	{ "SampleRate",        0, "" },
	{ "SignalStandard",   -1, "smpte-274m" },

	{ "FrameLayout",       0, "" },

	{ "StoredWidth",       0, "" },
	{ "StoredHeight",      0, "" },

	{ "SampledWidth",      0, "" },
	{ "SampledHeight",     0, "" },

	{ "SampledXOffset",   -1, "0" },
	{ "SampledYOffset",   -1, "0" },

	{ "DisplayHeight",     0, "" },
	{ "DisplayWidth",      0, "" },

	{ "DisplayF2Offset",  -1, "0" },
	{ "DisplayXOffset",   -1, "0" },
	{ "DisplayYOffset",   -1, "0" },

	{ "AspectRatio",       0, "16/9" },
	{ "VideoLineMap",      0,  "" },
	{ "CaptureGamma",     -1, "bt.709" },

	{ "ImageStartOffset", -1, "0" },
	{ "ImageEndOffset",   -1, "0" },
	{ "ImageStartOffset", -1, "0" },
	{ "ImageEndOffset",   -1, "0" },

	{ "ImageAlignmentOffset",  -1, "0" },
	{ "FieldDominance",        -1, "1" },
	{ "PictureEssenceCodinng",  0, "" },

	{ "ComponentDepth",       -1, "8bts" },

	{ "HorizontalSubsampling",-1, "42n" },
	{ "VerticalSubsampling",   0, "42n" },

	{ "ColorSiting",          -1, "hd422" },
	{ "ReversedByteOrder",    -1, "false" },
	{ "PaddingBits",          -1, "0" },

	{ "BlackRefLevel",        -1, "16" },
	{ "WhiteRefLevel",        -1, "235" },
	{ "ColorRange",           -1, "255" },

	{ "SingleSequence",       0, "" },
	{ "ConstantBframe",       0, "" },
	{ "CodedContentType",     0, "" },
	{ "LowDelay",            -1, "false" },

	{ "ClosedGOP",            0, "" },
	{ "IdenticalGOP",         0, "" },
	{ "MaxGOP",               0, "" },
	{ "BPictureCount",       -1, "2" },
	{ "BitRate",              0, "" },

	{ "ProfileAndLevel",      0, "" },

	{ "", 0, "" }
};

#endif