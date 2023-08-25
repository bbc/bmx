/*
 * Avid labels, keys, etc.
 *
 * Copyright (C) 2011, British Broadcasting Corporation
 * All Rights Reserved.
 *
 * Author: Philip de Nier
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the British Broadcasting Corporation nor the names
 *       of its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef MXF_AVID_LABELS_AND_KEYS_H_
#define MXF_AVID_LABELS_AND_KEYS_H_


#ifdef __cplusplus
extern "C"
{
#endif



#define MXF_AVID_LABEL(byte11, byte12, byte13, byte14, byte15, byte16) \
    {0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01, 0x0e, 0x04, byte11, byte12, byte13, byte14, byte15, byte16}

#define MXF_AVID_EE_K(byte13, byte14, byte15, byte16) \
    {0x06, 0x0e, 0x2b, 0x34, 0x01, 0x02, 0x01, 0x01, 0x0e, 0x04, 0x03, 0x01, byte13, byte14, byte15, byte16}



/*
 *
 * Picture/Sound coding labels
 *
 */



/* 10-bit uncompressed CDCI */

static const mxfUL MXF_CMDEF_L(AvidUncSD10Bit)  = MXF_AVID_LABEL(0x03, 0x01, 0x01, 0x03, 0x01, 0x00);
static const mxfUL MXF_CMDEF_L(AvidUncHD10Bit)  = MXF_AVID_LABEL(0x03, 0x01, 0x01, 0x03, 0x02, 0x00);


/* MJPEG */

#define MXF_AVID_MJPEG_CMDEF_L(byte15, byte16)      MXF_AVID_LABEL(0x02, 0x01, 0x02, 0x01, byte15, byte16)

static const mxfUL MXF_CMDEF_L(AvidMJPEG201_NTSC)   = MXF_AVID_MJPEG_CMDEF_L(0x01, 0x01);
static const mxfUL MXF_CMDEF_L(AvidMJPEG201_PAL)    = MXF_AVID_MJPEG_CMDEF_L(0x01, 0x02);
static const mxfUL MXF_CMDEF_L(AvidMJPEG101_NTSC)   = MXF_AVID_MJPEG_CMDEF_L(0x01, 0x03);
static const mxfUL MXF_CMDEF_L(AvidMJPEG101_PAL)    = MXF_AVID_MJPEG_CMDEF_L(0x01, 0x04);
static const mxfUL MXF_CMDEF_L(AvidMJPEG31_NTSC)    = MXF_AVID_MJPEG_CMDEF_L(0x01, 0x05);
static const mxfUL MXF_CMDEF_L(AvidMJPEG31_PAL)     = MXF_AVID_MJPEG_CMDEF_L(0x01, 0x06);
static const mxfUL MXF_CMDEF_L(AvidMJPEG21_NTSC)    = MXF_AVID_MJPEG_CMDEF_L(0x01, 0x07);
static const mxfUL MXF_CMDEF_L(AvidMJPEG21_PAL)     = MXF_AVID_MJPEG_CMDEF_L(0x01, 0x08);

static const mxfUL MXF_CMDEF_L(AvidMJPEG151s_NTSC)  = MXF_AVID_MJPEG_CMDEF_L(0x02, 0x01);
static const mxfUL MXF_CMDEF_L(AvidMJPEG151s_PAL)   = MXF_AVID_MJPEG_CMDEF_L(0x02, 0x02);
static const mxfUL MXF_CMDEF_L(AvidMJPEG41s_NTSC)   = MXF_AVID_MJPEG_CMDEF_L(0x02, 0x03);
static const mxfUL MXF_CMDEF_L(AvidMJPEG41s_PAL)    = MXF_AVID_MJPEG_CMDEF_L(0x02, 0x04);
static const mxfUL MXF_CMDEF_L(AvidMJPEG21s_NTSC)   = MXF_AVID_MJPEG_CMDEF_L(0x02, 0x05);
static const mxfUL MXF_CMDEF_L(AvidMJPEG21s_PAL)    = MXF_AVID_MJPEG_CMDEF_L(0x02, 0x06);

static const mxfUL MXF_CMDEF_L(AvidMJPEG351p_NTSC)  = MXF_AVID_MJPEG_CMDEF_L(0x03, 0x01);
static const mxfUL MXF_CMDEF_L(AvidMJPEG351p_PAL)   = MXF_AVID_MJPEG_CMDEF_L(0x03, 0x02);
static const mxfUL MXF_CMDEF_L(AvidMJPEG281p_NTSC)  = MXF_AVID_MJPEG_CMDEF_L(0x03, 0x03);
static const mxfUL MXF_CMDEF_L(AvidMJPEG281p_PAL)   = MXF_AVID_MJPEG_CMDEF_L(0x03, 0x04);
static const mxfUL MXF_CMDEF_L(AvidMJPEG141p_NTSC)  = MXF_AVID_MJPEG_CMDEF_L(0x03, 0x07);
static const mxfUL MXF_CMDEF_L(AvidMJPEG141p_PAL)   = MXF_AVID_MJPEG_CMDEF_L(0x03, 0x08);
static const mxfUL MXF_CMDEF_L(AvidMJPEG31p_NTSC)   = MXF_AVID_MJPEG_CMDEF_L(0x03, 0x0b);
static const mxfUL MXF_CMDEF_L(AvidMJPEG31p_PAL)    = MXF_AVID_MJPEG_CMDEF_L(0x03, 0x0c);
static const mxfUL MXF_CMDEF_L(AvidMJPEG21p_NTSC)   = MXF_AVID_MJPEG_CMDEF_L(0x03, 0x0d);
static const mxfUL MXF_CMDEF_L(AvidMJPEG21p_PAL)    = MXF_AVID_MJPEG_CMDEF_L(0x03, 0x0e);

static const mxfUL MXF_CMDEF_L(AvidMJPEG101m_NTSC)  = MXF_AVID_MJPEG_CMDEF_L(0x04, 0x01);
static const mxfUL MXF_CMDEF_L(AvidMJPEG101m_PAL)   = MXF_AVID_MJPEG_CMDEF_L(0x04, 0x02);
static const mxfUL MXF_CMDEF_L(AvidMJPEG41m_NTSC)   = MXF_AVID_MJPEG_CMDEF_L(0x04, 0x03);
static const mxfUL MXF_CMDEF_L(AvidMJPEG41m_PAL)    = MXF_AVID_MJPEG_CMDEF_L(0x04, 0x04);

static const mxfUL MXF_CMDEF_L(AvidMJPEG81m_NTSC)   = MXF_AVID_MJPEG_CMDEF_L(0x05, 0x01);
static const mxfUL MXF_CMDEF_L(AvidMJPEG81m_PAL)    = MXF_AVID_MJPEG_CMDEF_L(0x05, 0x02);
static const mxfUL MXF_CMDEF_L(AvidMJPEG31m_NTSC)   = MXF_AVID_MJPEG_CMDEF_L(0x05, 0x03);
static const mxfUL MXF_CMDEF_L(AvidMJPEG31m_PAL)    = MXF_AVID_MJPEG_CMDEF_L(0x05, 0x04);


/* DNxHD */

static const mxfUL MXF_CMDEF_L(DNxHD) = MXF_AVID_LABEL(0x02, 0x01, 0x02, 0x04, 0x01, 0x00);



/*
 *
 * Essence container labels
 *
 */

#define MXF_AVID_EC_L(byte14, byte15, byte16)       MXF_AVID_LABEL(0x03, 0x01, 0x02, byte14, byte15, byte16);



/* AAF-KLV - This label is used in the File Descriptor. The Partition Pack contains the coding specific label */

static const mxfUL MXF_EC_L(AvidAAFKLVEssenceContainer) =
    {0x06, 0x0e, 0x2b, 0x34, 0x01, 0x01, 0x01, 0xff, 0x4b, 0x46, 0x41, 0x41, 0x00, 0x0d, 0x4d, 0x4f};


/* MJPEG */

static const mxfUL MXF_EC_L(AvidMJPEGClipWrapped) = MXF_AVID_EC_L(0x01, 0x00, 0x00);


/* IMX 30/40/50 - Avid labels have register version byte set to 0x01, whilst the corresponding D10 labels,
   e.g. MXF_EC_L(D10_50_625_50_picture_only), have register version byte set to 0x02 */

#define MXF_AVID_IMX_EC_L(byte15) \
    {0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01, 0x0d, 0x01, 0x03, 0x01, 0x02, 0x01, byte15, 0x7f}

static const mxfUL MXF_EC_L(AvidIMX50_625_50) = MXF_AVID_IMX_EC_L(0x01);
static const mxfUL MXF_EC_L(AvidIMX50_525_60) = MXF_AVID_IMX_EC_L(0x02);
static const mxfUL MXF_EC_L(AvidIMX40_625_50) = MXF_AVID_IMX_EC_L(0x03);
static const mxfUL MXF_EC_L(AvidIMX40_525_60) = MXF_AVID_IMX_EC_L(0x04);
static const mxfUL MXF_EC_L(AvidIMX30_625_50) = MXF_AVID_IMX_EC_L(0x05);
static const mxfUL MXF_EC_L(AvidIMX30_525_60) = MXF_AVID_IMX_EC_L(0x06);


/* MPEG */

/* Label observed in XDCAM HD422 files produced by Media Composer 3.0
   and label found in Avid file containing XDCAM proxy (MPEG-4) material */
static const mxfUL MXF_EC_L(AvidMPEGClipWrapped) = MXF_AVID_EC_L(0x03, 0x00, 0x00);


/* DNxHD */

#define MXF_AVID_DNXHD_EC_L(byte15, byte16)     MXF_AVID_EC_L(0x06, byte15, byte16)

static const mxfUL MXF_EC_L(DNxHD1080p1235ClipWrapped)  = MXF_AVID_DNXHD_EC_L(0x01, 0x01);
static const mxfUL MXF_EC_L(DNxHD1080p1237ClipWrapped)  = MXF_AVID_DNXHD_EC_L(0x01, 0x02);
static const mxfUL MXF_EC_L(DNxHD1080p1238ClipWrapped)  = MXF_AVID_DNXHD_EC_L(0x01, 0x03);
static const mxfUL MXF_EC_L(DNxHD1080p1253ClipWrapped)  = MXF_AVID_DNXHD_EC_L(0x01, 0x04);
static const mxfUL MXF_EC_L(DNxHD1080p1259ClipWrapped)  = MXF_AVID_DNXHD_EC_L(0x01, 0x06);
static const mxfUL MXF_EC_L(DNxHD1080i1241ClipWrapped)  = MXF_AVID_DNXHD_EC_L(0x02, 0x01);
static const mxfUL MXF_EC_L(DNxHD1080i1242ClipWrapped)  = MXF_AVID_DNXHD_EC_L(0x02, 0x02);
static const mxfUL MXF_EC_L(DNxHD1080i1243ClipWrapped)  = MXF_AVID_DNXHD_EC_L(0x02, 0x03);
static const mxfUL MXF_EC_L(DNxHD1080i1244ClipWrapped)  = MXF_AVID_DNXHD_EC_L(0x02, 0x04);
static const mxfUL MXF_EC_L(DNxHD1080i1260ClipWrapped)  = MXF_AVID_DNXHD_EC_L(0x02, 0x08);
static const mxfUL MXF_EC_L(DNxHD720p1250ClipWrapped)   = MXF_AVID_DNXHD_EC_L(0x03, 0x01);
static const mxfUL MXF_EC_L(DNxHD720p1251ClipWrapped)   = MXF_AVID_DNXHD_EC_L(0x03, 0x02);
static const mxfUL MXF_EC_L(DNxHD720p1252ClipWrapped)   = MXF_AVID_DNXHD_EC_L(0x03, 0x03);
static const mxfUL MXF_EC_L(DNxHD720p1258ClipWrapped)   = MXF_AVID_DNXHD_EC_L(0x03, 0x04);


/* 10-bit uncompressed CDCI */

static const mxfUL MXF_EC_L(AvidUnc10Bit625ClipWrapped)     = MXF_AVID_EC_L(0x07, 0x01, 0x0a);
static const mxfUL MXF_EC_L(AvidUnc10Bit525ClipWrapped)     = MXF_AVID_EC_L(0x07, 0x01, 0x09);
static const mxfUL MXF_EC_L(AvidUnc10Bit1080iClipWrapped)   = MXF_AVID_EC_L(0x07, 0x02, 0x01);
static const mxfUL MXF_EC_L(AvidUnc10Bit1080pClipWrapped)   = MXF_AVID_EC_L(0x07, 0x02, 0x02);
static const mxfUL MXF_EC_L(AvidUnc10Bit720pClipWrapped)    = MXF_AVID_EC_L(0x07, 0x02, 0x03);


/* Uncompressed RGBA */

static const mxfUL MXF_EC_L(AvidUncRGBA)    = MXF_AVID_EC_L(0x08, 0x01, 0x00);



/*
 *
 * Essence element keys
 *
 */


/* DV */

static const mxfKey MXF_EE_K(DVClipWrapped) = MXF_DV_EE_K(0x01, MXF_DV_CLIP_WRAPPED_EE_TYPE, 0x01);


/* MJPEG */

static const mxfKey MXF_EE_K(AvidMJPEGClipWrapped) = MXF_AVID_EE_K(0x15, 0x01, 0x01, 0x01);

#define MXF_AVID_MJPEG_PICT_TRACK_NUM   MXF_TRACK_NUM(0x15, 0x01, 0x01, 0x01)


/* MPEG */

/* Label observed in XDCAM HD422 files produced by Media Composer 3.0 */
static const mxfKey MXF_EE_K(AvidMPEGClipWrapped) = MXF_AVID_EE_K(0x15, 0x01, 0x03, 0x01);

#define MXF_AVID_MPEG_PICT_TRACK_NUM  MXF_TRACK_NUM(0x15, 0x01, 0x03, 0x01)


/* DNxHD */

static const mxfKey MXF_EE_K(DNxHD) = MXF_AVID_EE_K(0x15, 0x01, 0x06, 0x01);

#define MXF_DNXHD_PICT_TRACK_NUM  MXF_TRACK_NUM(0x15, 0x01, 0x06, 0x01)


/* IMX 30/40/50 */

static const mxfKey MXF_EE_K(IMX) = MXF_D10_PICTURE_EE_K(0x01);


/* AES/BWAV */

static const mxfKey MXF_EE_K(BWFClipWrapped)  = MXF_AES3BWF_EE_K(0x01, MXF_BWF_CLIP_WRAPPED_EE_TYPE, 0x01);
static const mxfKey MXF_EE_K(AES3ClipWrapped) = MXF_AES3BWF_EE_K(0x01, MXF_AES3_CLIP_WRAPPED_EE_TYPE, 0x01);


/* Uncompressed CDCI */

static const mxfKey MXF_EE_K(UncClipWrapped)            = MXF_UNC_EE_K(0x01, MXF_UNC_CLIP_WRAPPED_EE_TYPE, 0x01);
static const mxfKey MXF_EE_K(AvidUnc10BitClipWrapped)   = MXF_AVID_EE_K(0x15, 0x01, 0x07, 0x01);

#define MXF_AVID_UNC_10BIT_PICT_TRACK_NUM   MXF_TRACK_NUM(0x15, 0x01, 0x07, 0x01)


/* Uncompressed RGBA */

static const mxfKey MXF_EE_K(AvidUncRGBA)   = MXF_AVID_EE_K(0x15, 0x01, 0x08, 0x01);

#define MXF_AVID_UNC_RGBA_PICT_TRACK_NUM    MXF_TRACK_NUM(0x15, 0x01, 0x08, 0x01)



/*
 *
 * Avid CDCIDescriptor::ResolutionID
 *
 */

#define AVID_RESOLUTION_ID(name,id) \
    static const uint32_t g_##name##_ResolutionID = id


AVID_RESOLUTION_ID(AvidUnc10Bit_1080i,  0x07d0);
AVID_RESOLUTION_ID(AvidUnc10Bit_1080p,  0x07d3);
AVID_RESOLUTION_ID(AvidUnc10Bit_720p,   0x07d4);
AVID_RESOLUTION_ID(AvidUnc10Bit_PAL,    0x07e6);
AVID_RESOLUTION_ID(AvidUnc10Bit_NTSC,   0x07e5);
AVID_RESOLUTION_ID(AvidUnc8Bit,         0x00aa);

AVID_RESOLUTION_ID(AvidMJPEG101,    0x4b);
AVID_RESOLUTION_ID(AvidMJPEG21,     0x4c);
AVID_RESOLUTION_ID(AvidMJPEG31,     0x4d);
AVID_RESOLUTION_ID(AvidMJPEG151s,   0x4e);
AVID_RESOLUTION_ID(AvidMJPEG41s,    0x4f);
AVID_RESOLUTION_ID(AvidMJPEG21s,    0x50);
AVID_RESOLUTION_ID(AvidMJPEG201,    0x52);
AVID_RESOLUTION_ID(AvidMJPEG31p,    0x61);
AVID_RESOLUTION_ID(AvidMJPEG21p,    0x62);
AVID_RESOLUTION_ID(AvidMJPEG351p,   0x66);
AVID_RESOLUTION_ID(AvidMJPEG141p,   0x67);
AVID_RESOLUTION_ID(AvidMJPEG281p,   0x68);
AVID_RESOLUTION_ID(AvidMJPEG101m,   0x6e);
AVID_RESOLUTION_ID(AvidMJPEG41m,    0x6f);
AVID_RESOLUTION_ID(AvidMJPEG81m,    0x70);
AVID_RESOLUTION_ID(AvidMJPEG31m,    0x71);

AVID_RESOLUTION_ID(AvidMPEG4,       0x05df);

AVID_RESOLUTION_ID(AvidMPEG_PAL,    0x0fea);
AVID_RESOLUTION_ID(AvidMPEG_NTSC,   0x0fe9);



/*
 *
 * Avid extensions metadata keys
 *
 */

static const mxfKey g_AvidObjectDirectory_key =
    {0x96, 0x13, 0xb3, 0x8a, 0x87, 0x34, 0x87, 0x46, 0xf1, 0x02, 0x96, 0xf0, 0x56, 0xe0, 0x4d, 0x2a};

static const mxfKey g_AvidMetadataRoot_key =
    {0x80, 0x53, 0x08, 0x00, 0x36, 0x21, 0x08, 0x04, 0xb3, 0xb3, 0x98, 0xa5, 0x1c, 0x90, 0x11, 0xd4};



#ifdef __cplusplus
}
#endif


#endif


