/*
 * Copyright (C) 2006-2011, British Broadcasting Corporation
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <assert.h>

#include <mxf/mxf.h>
#include <mxf/mxf_app.h>
#include <mxf/mxf_uu_metadata.h>
#include <mxf/mxf_macros.h>
#include "write_archive_mxf.h"
#include "timecode_index.h"


/* (2^16 - 8) / 16*/
#define MAX_STRONG_REF_ARRAY_COUNT      4095

/* we expect the URL to be the filename used on the LTO tape which is around 16 characters */
/* the value is much larger here to allow for unforseen changes or for testing purposes */
/* note that the filename will always be truncated if it exceeds this length (-1) */
#define NETWORK_LOCATOR_URL_SIZE        256

/* values used to identify fields that are not present */
#define INVALID_MONTH_VALUE             99
#define INVALID_DURATION_VALUE          (-1)
#define INVALID_ITEM_NO                 0

#define MAX_AUDIO_SEQUENCE_SIZE         5

#define UNKNOWN_AUDIO_SEQUENCE_OFFSET   255

#define MIN_LLEN                        4
#define ESS_ELEMENT_LLEN                4



typedef struct
{
    int haveSystemItem;
    int haveVideo;
    int audioNum;
} EssWriteState;

struct ArchiveMXFWriter
{
    mxfRational frameRate;
    int numAudioTracks;
    int includeCRC32;
    uint16_t blockAlign;
    uint32_t systemItemSize;
    uint32_t videoFrameSize;
    uint32_t audioElementSize;

    MXFFile *mxfFile;

    mxfUMID tapeSourcePackageUID;
    mxfUMID fileSourcePackageUID;
    mxfUMID materialPackageUID;

    MXFList pseFailures;

    mxfUTF16Char *tempString;

    TimecodeIndex vitcIndex;
    TimecodeIndex ltcIndex;

    mxfLength duration;

    uint32_t audioSequence[MAX_AUDIO_SEQUENCE_SIZE];
    uint8_t audioSequenceSize;
    uint8_t audioSequenceOffset;
    uint32_t inputAudioSequence[MAX_AUDIO_SEQUENCE_SIZE];

    EssWriteState essWriteState;

    uint64_t headerMetadataFilePos;
    uint64_t bodyFilePos;
    mxfTimestamp now;

    MXFDataModel *dataModel;
    MXFList *partitions;
    MXFHeaderMetadata *headerMetadata;
    MXFIndexTableSegment *indexSegment;

    /* these are references and should not be free'd */
    MXFPartition *headerPartition;
    MXFPartition *footerPartition;
    MXFMetadataSet *prefaceSet;
    MXFMetadataSet *identSet;
    MXFMetadataSet *contentStorageSet;
    MXFMetadataSet *materialPackageSet;
    MXFMetadataSet *sourcePackageSet;
    MXFMetadataSet *tapeSourcePackageSet;
    MXFMetadataSet *sourcePackageTrackSet;
    MXFMetadataSet *materialPackageTrackSet;
    MXFMetadataSet *sequenceSet;
    MXFMetadataSet *sourceClipSet;
    MXFMetadataSet *dmSet;
    MXFMetadataSet *fileDMFrameworkSet;
    MXFMetadataSet *tapeDMFrameworkSet;
    MXFMetadataSet *dmFrameworkSet;
    MXFMetadataSet *timecodeComponentSet;
    MXFMetadataSet *essContainerDataSet;
    MXFMetadataSet *multipleDescriptorSet;
    MXFMetadataSet *descriptorSet;
    MXFMetadataSet *cdciDescriptorSet;
    MXFMetadataSet *bwfDescriptorSet;
    MXFMetadataSet *tapeDescriptorSet;
    MXFMetadataSet *videoMaterialPackageTrackSet;
    MXFMetadataSet *videoSequenceSet;
    MXFMetadataSet *networkLocatorSet;

    /* duration items that are updated when writing is completed */
    MXFMetadataItem *durationItems[2 + (MAX_ARCHIVE_AUDIO_TRACKS + 1) * 4];
    int numDurationItems;
    /* container duration items that are created when writing is completed */
    MXFMetadataSet *descriptorSets[MAX_ARCHIVE_AUDIO_TRACKS + 2];
    int numDescriptorSets;

    MXFList vtrErrorTrackSets;
    MXFList pseFailureTrackSets;
    MXFList digiBetaDropoutTrackSets;
    MXFList timecodeBreakTrackSets;
};


typedef struct
{
    const mxfUL *ecLabel;
    mxfRational frameRate;
    uint8_t signalStandard;
    uint8_t frameLayout;
    uint32_t width;
    uint32_t height;
    int32_t videoLineMap[2];
} VideoFormat;

static const VideoFormat VIDEO_FORMATS[] =
{
    {&MXF_EC_L(SD_Unc_625_50i_422_135_FrameWrapped),    {25, 1},        MXF_SIGNAL_STANDARD_ITU601,     MXF_MIXED_FIELDS,  720,    576,   {23, 336}},
    {&MXF_EC_L(SD_Unc_525_5994i_422_135_FrameWrapped),  {30000, 1001},  MXF_SIGNAL_STANDARD_ITU601,     MXF_MIXED_FIELDS,  720,    486,   {21, 283}},
    {&MXF_EC_L(HD_Unc_1080_50i_422_FrameWrapped),       {25, 1},        MXF_SIGNAL_STANDARD_SMPTE274M,  MXF_MIXED_FIELDS,  1920,   1080,  {21, 584}},
    {&MXF_EC_L(HD_Unc_1080_5994i_422_FrameWrapped),     {30000, 1001},  MXF_SIGNAL_STANDARD_SMPTE274M,  MXF_MIXED_FIELDS,  1920,   1080,  {21, 584}},
    {&MXF_EC_L(HD_Unc_1080_25p_422_FrameWrapped),       {25, 1},        MXF_SIGNAL_STANDARD_SMPTE274M,  MXF_FULL_FRAME,    1920,   1080,  {42, 0}},
    {&MXF_EC_L(HD_Unc_1080_2997p_422_FrameWrapped),     {30000, 1001},  MXF_SIGNAL_STANDARD_SMPTE274M,  MXF_FULL_FRAME,    1920,   1080,  {42, 0}},
    {&MXF_EC_L(HD_Unc_1080_50p_422_FrameWrapped),       {50, 1},        MXF_SIGNAL_STANDARD_SMPTE274M,  MXF_FULL_FRAME,    1920,   1080,  {42, 0}},
    {&MXF_EC_L(HD_Unc_1080_5994p_422_FrameWrapped),     {60000, 1001},  MXF_SIGNAL_STANDARD_SMPTE274M,  MXF_FULL_FRAME,    1920,   1080,  {42, 0}},
    {&MXF_EC_L(HD_Unc_720_25p_422_FrameWrapped),        {25, 1},        MXF_SIGNAL_STANDARD_SMPTE296M,  MXF_FULL_FRAME,    1280,   720,   {26, 0}},
    {&MXF_EC_L(HD_Unc_720_2997p_422_FrameWrapped),      {30000, 1001},  MXF_SIGNAL_STANDARD_SMPTE296M,  MXF_FULL_FRAME,    1280,   720,   {26, 0}},
    {&MXF_EC_L(HD_Unc_720_50p_422_FrameWrapped),        {50, 1},        MXF_SIGNAL_STANDARD_SMPTE296M,  MXF_FULL_FRAME,    1280,   720,   {26, 0}},
    {&MXF_EC_L(HD_Unc_720_5994p_422_FrameWrapped),      {60000, 1001},  MXF_SIGNAL_STANDARD_SMPTE296M,  MXF_FULL_FRAME,    1280,   720,   {26, 0}},
};


typedef struct
{
    mxfRational frameRate;
    uint32_t sequence[MAX_AUDIO_SEQUENCE_SIZE + 1];
} AudioSequence;

static const AudioSequence AUDIO_SEQUENCES[] =
{
    {{25,    1},     {1920, 0}},
    {{50,    1},     {960, 0}},
    {{30000, 1001},  {1602, 1601, 1602, 1601, 1602, 0}},
    {{60000, 1001},  {801, 801, 800, 801, 801, 0}},
};



static const uint64_t FIXED_BODY_OFFSET = 0x8000;

/* Note: in many places the code assume KAG size equals 1 */
static const uint32_t KAG_SIZE = 1;


static const mxfUUID IDENT_PRODUCT_UID =
    {0x9e, 0x26, 0x08, 0xb1, 0xc9, 0xfe, 0x44, 0x48, 0x88, 0xdf, 0x26, 0x94, 0xcf, 0x77, 0x59, 0x9a};
static const mxfUTF16Char *IDENT_COMPANY_NAME   = L"BBC";
static const mxfUTF16Char *IDENT_PRODUCT_NAME   = L"BBC Archive MXF Writer";
static const mxfUTF16Char *IDENT_VERSION_STRING = L"Version August 2011";


static const mxfKey UNC_BASE_ELEMENT_KEY          = MXF_UNC_EE_K(0x00, 0x00, 0x00);
static const mxfKey WAV_BASE_ELEMENT_KEY          = MXF_AES3BWF_EE_K(0x00, 0x00, 0x00);
static const mxfKey TIMECODE_SYS_ITEM_ELEMENT_KEY = MXF_SS1_ELEMENT_KEY(0x01, 0x00);

static const uint32_t BODY_SID  = 1;
static const uint32_t INDEX_SID = 2;

static const mxfRational AUDIO_SAMPLING_RATE = {48000, 1};

static const mxfUTF16Char *VTR_ERRORS_TRACK_NAME       = L"VTR Errors";
static const mxfUTF16Char *PSE_FAILURES_TRACK_NAME     = L"PSE Failures";
static const mxfUTF16Char *DIGIBETA_DROPOUT_TRACK_NAME = L"DigiBeta Dropouts";
static const mxfUTF16Char *TIMECODE_BREAK_TRACK_NAME   = L"Timecode Breaks";

static const char *LTO_FORMAT_STRING           = "LTO";
static const mxfUTF16Char *LTO_FORMAT_STRING_W = L"LTO";

static const int INFAX_DATA_STRING_SEPARATOR = '|';

/* size of infax data set (including InstanceUID) with all fields + the minimum KLV fill + 32 (error margin) */
static const uint64_t FIXED_INFAX_SET_ALLOCATION_SIZE =
    (16 + MIN_LLEN + 4 + 16 + COMPLETE_INFAX_EXTERNAL_SIZE + 16 + MIN_LLEN + 32);



static int check_supported_formats(const mxfRational *frameRate, uint8_t signalStandard, uint8_t frameLayout,
                                   size_t *index)
{
    size_t i;
    for (i = 0; i < ARRAY_SIZE(VIDEO_FORMATS); i++)
    {
        if (memcmp(frameRate, &VIDEO_FORMATS[i].frameRate, sizeof(*frameRate)) == 0 &&
            signalStandard == VIDEO_FORMATS[i].signalStandard &&
            frameLayout == VIDEO_FORMATS[i].frameLayout)
        {
            *index = i;
            return 1;
        }
    }

    return 0;
}

static int init_audio_sequence(ArchiveMXFWriter *output, const mxfRational *frameRate)
{
    size_t i;
    for (i = 0; i < ARRAY_SIZE(AUDIO_SEQUENCES); i++)
    {
        if (memcmp(frameRate, &AUDIO_SEQUENCES[i].frameRate, sizeof(*frameRate)) == 0)
        {
            uint8_t j = 0;
            while (AUDIO_SEQUENCES[i].sequence[j])
            {
                output->audioSequence[j] = AUDIO_SEQUENCES[i].sequence[j];
                j++;
            }
            output->audioSequenceSize = j;
            return 1;
        }
    }

    return 0;
}

static uint32_t get_audio_element_size(const mxfRational *frameRate, uint32_t audioQuantBits)
{
    uint32_t maxNumSamples = 0;
    uint32_t dataSize;
    uint32_t fillSize;
    uint32_t elementInKAGSize;
    size_t i, j = 0;

    for (i = 0; i < ARRAY_SIZE(AUDIO_SEQUENCES); i++)
    {
        if (memcmp(frameRate, &AUDIO_SEQUENCES[i].frameRate, sizeof(*frameRate)) == 0)
        {
            break;
        }
    }
    if (i >= ARRAY_SIZE(AUDIO_SEQUENCES))
    {
        mxf_log_error("Unsupported frame rate for audio sample sequence" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
        return 0;
    }

    while (AUDIO_SEQUENCES[i].sequence[j])
    {
        if (AUDIO_SEQUENCES[i].sequence[j] > maxNumSamples)
            maxNumSamples = AUDIO_SEQUENCES[i].sequence[j];
        j++;
    }

    dataSize = mxfKey_extlen + ESS_ELEMENT_LLEN + get_audio_frame_size(maxNumSamples, audioQuantBits);
    fillSize = (j > 1 ? mxfKey_extlen + MIN_LLEN : 0);
    elementInKAGSize = (dataSize + fillSize) % KAG_SIZE;
    if (elementInKAGSize > 0) {
        fillSize += KAG_SIZE - elementInKAGSize;
        while (fillSize < (uint32_t)MIN_LLEN + mxfKey_extlen)
            fillSize += KAG_SIZE;
    }

    return dataSize + fillSize;
}

static int check_and_update_audio_sequence(ArchiveMXFWriter *output, uint32_t dataSize)
{
    uint32_t numSamples;
    int64_t position;

    numSamples = dataSize / output->blockAlign;
    CHK_ORET(dataSize == numSamples * output->blockAlign);

    /* Note: output->duration is updated after writing the video data, i.e. it starts at 1 for the first audio data */
    CHK_ORET(output->duration > 0);
    position = output->duration - 1;

    /* check tracks have the same number of samples as the first track */
    if (output->essWriteState.audioNum > 0)
    {
        if (output->inputAudioSequence[position % output->audioSequenceSize] != numSamples)
        {
            mxf_log_error("Audio track sample counts differ" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
            return 0;
        }

        return 1;
    }

    output->inputAudioSequence[position % output->audioSequenceSize] = numSamples;

    if (position < (int64_t)output->audioSequenceSize)
    {
        if (output->audioSequenceOffset != UNKNOWN_AUDIO_SEQUENCE_OFFSET)
        {
            /* update the sequence offset */
            output->audioSequenceOffset = 0;
            while (output->audioSequenceOffset < output->audioSequenceSize)
            {
                int64_t i;
                for (i = 0; i <= position; i++)
                {
                    if (output->inputAudioSequence[i] !=
                            output->audioSequence[(i + output->audioSequenceOffset) % output->audioSequenceSize])
                    {
                        break;
                    }
                }
                if (i > position)
                    break;

                output->audioSequenceOffset++;
            }
            if (output->audioSequenceOffset >= output->audioSequenceSize)
            {
                if (output->audioSequenceSize == 1)
                {
                    mxf_log_error("Invalid audio sample count" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
                    return 0;
                }
                else
                {
                    mxf_log_warn("Unknown audio sample sequence" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
                    output->audioSequenceOffset = UNKNOWN_AUDIO_SEQUENCE_OFFSET;
                    return 1;
                }
            }
        }
    }
    else if (output->audioSequenceSize == 1 || output->audioSequenceOffset != UNKNOWN_AUDIO_SEQUENCE_OFFSET)
    {
        if (numSamples != output->audioSequence[(position + output->audioSequenceOffset) % output->audioSequenceSize])
        {
            if (output->audioSequenceSize == 1)
            {
                mxf_log_error("Invalid audio sample count" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
                return 0;
            }
            else
            {
                mxf_log_warn("Unknown audio sample sequence" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
                output->audioSequenceOffset = UNKNOWN_AUDIO_SEQUENCE_OFFSET;
                return 1;
            }
        }
    }

    return 1;
}


static void set_null_infax_data(InfaxData *infaxData)
{
    memset(infaxData, 0, sizeof(InfaxData));
    infaxData->txDate.month    = INVALID_MONTH_VALUE;
    infaxData->stockDate.month = INVALID_MONTH_VALUE;
    infaxData->duration        = INVALID_DURATION_VALUE;
    infaxData->itemNo          = INVALID_ITEM_NO;
}

static int convert_string(const char *input, mxfUTF16Char **tempString)
{
    size_t len = mxf_utf8_to_utf16(NULL, input, 0);
    CHK_ORET(len != (size_t)(-1));

    SAFE_FREE(*tempString);
    CHK_MALLOC_ARRAY_ORET(*tempString, mxfUTF16Char, len + 1);
    memset(*tempString, 0, sizeof(mxfUTF16Char) * (len + 1));

    mxf_utf8_to_utf16(*tempString, input, len + 1);

    return 1;
}

static void free_archive_mxf_file(ArchiveMXFWriter **output)
{
    if (*output == NULL)
    {
        return;
    }

    SAFE_FREE((*output)->tempString);

    clear_timecode_index(&(*output)->vitcIndex);
    clear_timecode_index(&(*output)->ltcIndex);

    mxf_free_index_table_segment(&(*output)->indexSegment);
    mxf_free_file_partitions(&(*output)->partitions);
    mxf_free_header_metadata(&(*output)->headerMetadata);
    mxf_free_data_model(&(*output)->dataModel);

    mxf_clear_list(&(*output)->vtrErrorTrackSets);
    mxf_clear_list(&(*output)->pseFailureTrackSets);
    mxf_clear_list(&(*output)->digiBetaDropoutTrackSets);
    mxf_clear_list(&(*output)->timecodeBreakTrackSets);

    mxf_file_close(&(*output)->mxfFile);

    SAFE_FREE(*output);
}


static int verify_essence_write_state(ArchiveMXFWriter *output, int writeSystemItem, int writeVideo, int writeAudio)
{
    assert(writeSystemItem || writeVideo || writeAudio);

    if (writeSystemItem)
    {
        if (output->essWriteState.haveSystemItem)
        {
            mxf_log_error("Timecode already written" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
            return 0;
        }
    }
    else if (writeVideo)
    {
        if (!output->essWriteState.haveSystemItem)
        {
            mxf_log_error("Must first write timecode before video frame" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
            return 0;
        }
        if (output->essWriteState.haveVideo)
        {
            mxf_log_error("Video frame already written" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
            return 0;
        }
    }
    else /* writeAudio */
    {
        if (!output->essWriteState.haveVideo)
        {
            mxf_log_error("Must write video frame before audio frames" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
            return 0;
        }
    }

    return 1;
}

static void update_essence_write_state(ArchiveMXFWriter *output, int writeSystemItem, int writeVideo, int writeAudio)
{
    assert(writeSystemItem || writeVideo || writeAudio);

    if (writeSystemItem)
    {
        output->essWriteState.haveSystemItem = 1;
    }
    else if (writeVideo)
    {
        output->essWriteState.haveVideo = 1;
        if (output->numAudioTracks == 0)
        {
            output->essWriteState.haveSystemItem = 0;
            output->essWriteState.haveVideo = 0;
            output->essWriteState.audioNum = 0;
        }
    }
    else // writeAudio
    {
        output->essWriteState.audioNum++;
        if (output->essWriteState.audioNum >= output->numAudioTracks)
        {
            output->essWriteState.haveSystemItem = 0;
            output->essWriteState.haveVideo = 0;
            output->essWriteState.audioNum = 0;
        }
    }
}

static void convert_timecode_to_12m(ArchiveTimecode *t, uint8_t *t12m)
{
    /* the format follows the specification of the TimecodeArray property
       defined in SMPTE 405M, table 2, which follows section 8.2 of SMPTE 331M */

    t12m[0] = ((t->frame % 10) & 0x0f) | (((t->frame / 10) & 0x3) << 4);
    t12m[1] = ((t->sec   % 10) & 0x0f) | (((t->sec   / 10) & 0x7) << 4);
    t12m[2] = ((t->min   % 10) & 0x0f) | (((t->min   / 10) & 0x7) << 4);
    t12m[3] = ((t->hour  % 10) & 0x0f) | (((t->hour  / 10) & 0x3) << 4);

    if (t->dropFrame)
        t12m[0] |= 0x40;

    /* Binary Group Data is not used */
    memset(&t12m[4], 0, 4);
}

static int is_empty_string(const char *str, int includingSpace)
{
    const char *strPtr = str;

    if (str == NULL)
    {
        return 1;
    }

    while (*strPtr != '\0')
    {
        if (!includingSpace || !isspace(*strPtr))
        {
            return 0;
        }
        strPtr++;
    }

    return 1;
}

static int set_infax_data(MXFMetadataSet *dmFrameworkSet, const InfaxData *infaxData)
{
    mxfTimestamp dateOnly;
    mxfUTF16Char *tempString = NULL;
    MXFMetadataItem *item;

#define ELSE_REMOVE_ITEM(prop) \
    else if (mxf_have_item(dmFrameworkSet, &MXF_ITEM_K(APP_InfaxFramework, prop))) \
    { \
        CHK_OFAIL(mxf_remove_item(dmFrameworkSet, &MXF_ITEM_K(APP_InfaxFramework, prop), &item)); \
        mxf_free_item(&item); \
    }

#define SET_STRING_ITEM(field, includingSpace, prop, size) \
    if (!is_empty_string(infaxData->field, includingSpace)) \
    { \
        CHK_OFAIL(convert_string(infaxData->field, &tempString)); \
        CHK_OFAIL(mxf_set_fixed_size_utf16string_item(dmFrameworkSet, &MXF_ITEM_K(APP_InfaxFramework, prop), tempString, size)); \
    } \
    ELSE_REMOVE_ITEM(prop)

#define SET_DATE_ITEM(field, prop) \
    if (infaxData->field.month != INVALID_MONTH_VALUE) \
    { \
        /* only the date part is relevant */ \
        dateOnly = infaxData->field; \
        dateOnly.hour  = 0; \
        dateOnly.min   = 0; \
        dateOnly.sec   = 0; \
        dateOnly.qmsec = 0; \
        CHK_OFAIL(mxf_set_timestamp_item(dmFrameworkSet, &MXF_ITEM_K(APP_InfaxFramework, prop), &dateOnly)); \
    } \
    ELSE_REMOVE_ITEM(prop)

#define SET_INT_ITEM(field, invalidValue, setFunc, prop) \
    if (infaxData->field != invalidValue) \
    { \
        CHK_OFAIL(setFunc(dmFrameworkSet, &MXF_ITEM_K(APP_InfaxFramework, prop), infaxData->field)); \
    } \
    ELSE_REMOVE_ITEM(prop)


    SET_STRING_ITEM(format, 1, APP_Format, FORMAT_SIZE)
    SET_STRING_ITEM(progTitle, 0, APP_ProgrammeTitle, PROGTITLE_SIZE)
    SET_STRING_ITEM(epTitle, 0, APP_EpisodeTitle, EPTITLE_SIZE)
    SET_DATE_ITEM(txDate, APP_TransmissionDate)
    SET_STRING_ITEM(magPrefix, 1, APP_MagazinePrefix, MAGPREFIX_SIZE)
    SET_STRING_ITEM(progNo, 1, APP_ProgrammeNumber, PROGNO_SIZE)
    SET_STRING_ITEM(prodCode, 1, APP_ProductionCode, PRODCODE_SIZE)
    SET_STRING_ITEM(spoolStatus, 1, APP_SpoolStatus, SPOOLSTATUS_SIZE)
    SET_DATE_ITEM(stockDate, APP_StockDate)
    SET_STRING_ITEM(spoolDesc, 0, APP_SpoolDescriptor, SPOOLDESC_SIZE)
    SET_STRING_ITEM(memo, 0, APP_Memo, MEMO_SIZE)
    SET_INT_ITEM(duration, INVALID_DURATION_VALUE, mxf_set_int64_item, APP_Duration)
    SET_STRING_ITEM(spoolNo, 1, APP_SpoolNumber, SPOOLNO_SIZE);
    SET_STRING_ITEM(accNo, 1, APP_AccessionNumber, ACCNO_SIZE);
    SET_STRING_ITEM(catDetail, 1, APP_CatalogueDetail, CATDETAIL_SIZE);
    SET_INT_ITEM(itemNo, INVALID_ITEM_NO, mxf_set_uint32_item, APP_ItemNumber)


    SAFE_FREE(tempString);
    return 1;

fail:
    SAFE_FREE(tempString);
    return 0;
}



int prepare_archive_mxf_file(const char *filename, const mxfRational *frameRate, uint8_t signalStandard,
                             uint8_t frameLayout, uint32_t componentDepth, const mxfRational *aspectRatio,
                             int numAudioTracks, uint32_t audioQuantBits, int includeCRC32, int64_t startPosition,
                             ArchiveMXFWriter **output)
{
    MXFFile *mxfFile = NULL;
    int result;

    CHK_ORET(mxf_disk_file_open_new(filename, &mxfFile));

    result = prepare_archive_mxf_file_2(&mxfFile, filename, frameRate, signalStandard, frameLayout, componentDepth,
                                        aspectRatio, numAudioTracks, audioQuantBits, includeCRC32, startPosition,
                                        output);
    if (!result)
    {
        if (mxfFile != NULL)
        {
            mxf_file_close(&mxfFile);
        }
    }

    return result;
}

int prepare_archive_mxf_file_2(MXFFile **mxfFile, const char *filename, const mxfRational *frameRate,
                               uint8_t signalStandard, uint8_t frameLayout, uint32_t componentDepth,
                               const mxfRational *aspectRatio, int numAudioTracks, uint32_t audioQuantBits,
                               int includeCRC32, int64_t startPosition, ArchiveMXFWriter **output)
{
    ArchiveMXFWriter *newOutput;
    int64_t filePos;
    mxfUUID uuid;
    int i;
    uint32_t videoTrackNum;
    uint32_t audioTrackNum;
    uint32_t deltaOffset;
    char nameBuffer[256];
    uint8_t *arrayElement;
    InfaxData nullInfaxData;
    mxfLocalTag assignedTag;
    uint16_t roundedTCBase = (uint16_t)(frameRate->numerator / (float)frameRate->denominator + 0.5);
    int64_t tapeLen = 120 * 60 * 60 * roundedTCBase;
    size_t formatIndex = 0;

    CHK_ORET(componentDepth == 8 || componentDepth == 10);
    CHK_ORET(numAudioTracks <= MAX_ARCHIVE_AUDIO_TRACKS);

    CHK_ORET(check_supported_formats(frameRate, signalStandard, frameLayout, &formatIndex));

    set_null_infax_data(&nullInfaxData);
    videoTrackNum = MXF_UNC_TRACK_NUM(0x00, 0x00, 0x00);
    audioTrackNum = MXF_AES3BWF_TRACK_NUM(0x00, 0x00, 0x00);

    CHK_MALLOC_ORET(newOutput, ArchiveMXFWriter);
    memset(newOutput, 0, sizeof(ArchiveMXFWriter));
    mxf_generate_umid(&newOutput->tapeSourcePackageUID);
    mxf_generate_umid(&newOutput->fileSourcePackageUID);
    mxf_generate_umid(&newOutput->materialPackageUID);
    initialise_timecode_index(&newOutput->vitcIndex, frameRate, 512);
    initialise_timecode_index(&newOutput->ltcIndex, frameRate, 512);
    mxf_initialise_list(&newOutput->vtrErrorTrackSets, NULL);
    mxf_initialise_list(&newOutput->pseFailureTrackSets, NULL);
    mxf_initialise_list(&newOutput->digiBetaDropoutTrackSets, NULL);
    mxf_initialise_list(&newOutput->timecodeBreakTrackSets, NULL);


    newOutput->mxfFile = *mxfFile;
    *mxfFile = NULL;
    newOutput->frameRate = *frameRate;
    newOutput->numAudioTracks = numAudioTracks;
    newOutput->includeCRC32 = includeCRC32;
    newOutput->blockAlign = (uint16_t)((audioQuantBits + 7) / 8);
    mxf_get_timestamp_now(&newOutput->now);

    CHK_ORET(init_audio_sequence(newOutput, frameRate));

    newOutput->systemItemSize = 28;
    if (includeCRC32)
    {
        newOutput->systemItemSize += 12 + (1 + numAudioTracks) * 4;
    }
    newOutput->videoFrameSize = get_video_frame_size(frameRate, signalStandard, componentDepth);
    newOutput->audioElementSize = get_audio_element_size(frameRate, audioQuantBits);


    CHK_OFAIL(mxf_create_file_partitions(&newOutput->partitions));


    /* minimum llen */

    mxf_file_set_min_llen(newOutput->mxfFile, MIN_LLEN);


    /*
     * Write the Header Partition Pack
     */

    CHK_OFAIL(mxf_append_new_partition(newOutput->partitions, &newOutput->headerPartition));
    /* partition is open because the LTO and source videotape Infax data needs to be filled in and is incomplete
       because the durations are not yet known */
    newOutput->headerPartition->key = MXF_PP_K(OpenIncomplete, Header);
    newOutput->headerPartition->kagSize = KAG_SIZE;
    newOutput->headerPartition->bodySID = BODY_SID;
    newOutput->headerPartition->indexSID = INDEX_SID;
    newOutput->headerPartition->operationalPattern = MXF_OP_L(1a, MultiTrack_Stream_Internal);
    if (numAudioTracks > 0)
    {
        CHK_OFAIL(mxf_append_partition_esscont_label(newOutput->headerPartition, &MXF_EC_L(MultipleWrappings)));
        CHK_OFAIL(mxf_append_partition_esscont_label(newOutput->headerPartition, VIDEO_FORMATS[formatIndex].ecLabel));
        CHK_OFAIL(mxf_append_partition_esscont_label(newOutput->headerPartition, &MXF_EC_L(BWFFrameWrapped)));
    }
    else
    {
        CHK_OFAIL(mxf_append_partition_esscont_label(newOutput->headerPartition, VIDEO_FORMATS[formatIndex].ecLabel));
    }
    CHK_OFAIL(mxf_write_partition(newOutput->mxfFile, newOutput->headerPartition));


    CHK_OFAIL((filePos = mxf_file_tell(newOutput->mxfFile)) >= 0);
    newOutput->headerMetadataFilePos = (uint64_t)filePos;


    /*
     * Create the header metadata
     */

    /* load the baseline data model and BBC extensions */
    CHK_OFAIL(mxf_load_data_model(&newOutput->dataModel));
    CHK_OFAIL(mxf_app_load_extensions(newOutput->dataModel));
    CHK_OFAIL(mxf_finalise_data_model(newOutput->dataModel));


    /* create the header metadata */
    CHK_OFAIL(mxf_create_header_metadata(&newOutput->headerMetadata, newOutput->dataModel));
    newOutput->numDurationItems = 0;
    newOutput->numDescriptorSets = 0;


    /* register the Infax data set items in the primer pack, so that addition of new items in the
    update_archive_mxf_file() function have their local tag registered in the primer pack because
    the primer pack is not extendable in update_archive_mxf_file() */
#define REGISTER_ENTRY(prop) \
    CHK_OFAIL(mxf_register_primer_entry(newOutput->headerMetadata->primerPack, \
                                        &MXF_ITEM_K(APP_InfaxFramework, prop), \
                                        g_Null_LocalTag, &assignedTag));

    REGISTER_ENTRY(APP_Format)
    REGISTER_ENTRY(APP_ProgrammeTitle)
    REGISTER_ENTRY(APP_EpisodeTitle)
    REGISTER_ENTRY(APP_TransmissionDate)
    REGISTER_ENTRY(APP_MagazinePrefix)
    REGISTER_ENTRY(APP_ProgrammeNumber)
    REGISTER_ENTRY(APP_ProductionCode)
    REGISTER_ENTRY(APP_SpoolStatus)
    REGISTER_ENTRY(APP_StockDate)
    REGISTER_ENTRY(APP_SpoolDescriptor)
    REGISTER_ENTRY(APP_Memo)
    REGISTER_ENTRY(APP_Duration)
    REGISTER_ENTRY(APP_SpoolNumber)
    REGISTER_ENTRY(APP_AccessionNumber)
    REGISTER_ENTRY(APP_CatalogueDetail)
    REGISTER_ENTRY(APP_ItemNumber)


    /* Preface */
    CHK_OFAIL(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(Preface), &newOutput->prefaceSet));
    CHK_OFAIL(mxf_set_timestamp_item(newOutput->prefaceSet, &MXF_ITEM_K(Preface, LastModifiedDate), &newOutput->now));
    CHK_OFAIL(mxf_set_version_type_item(newOutput->prefaceSet, &MXF_ITEM_K(Preface, Version), MXF_PREFACE_VER(1, 2)));
    CHK_OFAIL(mxf_set_ul_item(newOutput->prefaceSet, &MXF_ITEM_K(Preface, OperationalPattern), &MXF_OP_L(1a, MultiTrack_Stream_Internal)));
    if (newOutput->numAudioTracks > 0)
    {
        CHK_OFAIL(mxf_alloc_array_item_elements(newOutput->prefaceSet, &MXF_ITEM_K(Preface, EssenceContainers), mxfUL_extlen, 3, &arrayElement));
        mxf_set_ul(&MXF_EC_L(MultipleWrappings), arrayElement);
        mxf_set_ul(VIDEO_FORMATS[formatIndex].ecLabel, &arrayElement[mxfUL_extlen]);
        mxf_set_ul(&MXF_EC_L(BWFFrameWrapped), &arrayElement[mxfUL_extlen * 2]);
    }
    else
    {
        CHK_OFAIL(mxf_alloc_array_item_elements(newOutput->prefaceSet, &MXF_ITEM_K(Preface, EssenceContainers), mxfUL_extlen, 1, &arrayElement));
        mxf_set_ul(VIDEO_FORMATS[formatIndex].ecLabel, arrayElement);
    }
    CHK_OFAIL(mxf_alloc_array_item_elements(newOutput->prefaceSet, &MXF_ITEM_K(Preface, DMSchemes), mxfUL_extlen, 1, &arrayElement));
    mxf_set_ul(&MXF_DM_L(APP_PreservationDescriptiveScheme), arrayElement);
    CHK_OFAIL(mxf_set_uint32_item(newOutput->prefaceSet, &MXF_ITEM_K(Preface, APP_VTRErrorCount), 0));
    CHK_OFAIL(mxf_set_uint32_item(newOutput->prefaceSet, &MXF_ITEM_K(Preface, APP_PSEFailureCount), 0));


    /* Preface - Identification */
    mxf_generate_uuid(&uuid);
    CHK_OFAIL(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(Identification), &newOutput->identSet));
    CHK_OFAIL(mxf_add_array_item_strongref(newOutput->prefaceSet, &MXF_ITEM_K(Preface, Identifications), newOutput->identSet));
    CHK_OFAIL(mxf_set_uuid_item(newOutput->identSet, &MXF_ITEM_K(Identification, ThisGenerationUID), &uuid));
    CHK_OFAIL(mxf_set_utf16string_item(newOutput->identSet, &MXF_ITEM_K(Identification, CompanyName), IDENT_COMPANY_NAME));
    CHK_OFAIL(mxf_set_utf16string_item(newOutput->identSet, &MXF_ITEM_K(Identification, ProductName), IDENT_PRODUCT_NAME));
    CHK_OFAIL(mxf_set_utf16string_item(newOutput->identSet, &MXF_ITEM_K(Identification, VersionString), IDENT_VERSION_STRING));
    CHK_OFAIL(mxf_set_uuid_item(newOutput->identSet, &MXF_ITEM_K(Identification, ProductUID), &IDENT_PRODUCT_UID));
    CHK_OFAIL(mxf_set_timestamp_item(newOutput->identSet, &MXF_ITEM_K(Identification, ModificationDate), &newOutput->now));
    CHK_OFAIL(mxf_set_product_version_item(newOutput->identSet, &MXF_ITEM_K(Identification, ToolkitVersion), mxf_get_version()));
    CHK_OFAIL(mxf_set_utf16string_item(newOutput->identSet, &MXF_ITEM_K(Identification, Platform), mxf_get_platform_wstring()));

    /* Preface - ContentStorage */
    CHK_OFAIL(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(ContentStorage), &newOutput->contentStorageSet));
    CHK_OFAIL(mxf_set_strongref_item(newOutput->prefaceSet, &MXF_ITEM_K(Preface, ContentStorage), newOutput->contentStorageSet));


    /* Preface - ContentStorage - EssenceContainerData */
    CHK_OFAIL(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(EssenceContainerData), &newOutput->essContainerDataSet));
    CHK_OFAIL(mxf_add_array_item_strongref(newOutput->contentStorageSet, &MXF_ITEM_K(ContentStorage, EssenceContainerData), newOutput->essContainerDataSet));
    CHK_OFAIL(mxf_set_umid_item(newOutput->essContainerDataSet, &MXF_ITEM_K(EssenceContainerData, LinkedPackageUID), &newOutput->fileSourcePackageUID));
    CHK_OFAIL(mxf_set_uint32_item(newOutput->essContainerDataSet, &MXF_ITEM_K(EssenceContainerData, IndexSID), INDEX_SID));
    CHK_OFAIL(mxf_set_uint32_item(newOutput->essContainerDataSet, &MXF_ITEM_K(EssenceContainerData, BodySID), BODY_SID));


    /* Preface - ContentStorage - MaterialPackage */
    CHK_OFAIL(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(MaterialPackage), &newOutput->materialPackageSet));
    CHK_OFAIL(mxf_add_array_item_strongref(newOutput->contentStorageSet, &MXF_ITEM_K(ContentStorage, Packages), newOutput->materialPackageSet));
    CHK_OFAIL(mxf_set_umid_item(newOutput->materialPackageSet, &MXF_ITEM_K(GenericPackage, PackageUID), &newOutput->materialPackageUID));
    CHK_OFAIL(mxf_set_timestamp_item(newOutput->materialPackageSet, &MXF_ITEM_K(GenericPackage, PackageCreationDate), &newOutput->now));
    CHK_OFAIL(mxf_set_timestamp_item(newOutput->materialPackageSet, &MXF_ITEM_K(GenericPackage, PackageModifiedDate), &newOutput->now));
    CHK_OFAIL(mxf_set_utf16string_item(newOutput->materialPackageSet, &MXF_ITEM_K(GenericPackage, Name), L"Ingex Archive ingested material"));
    /* Name will be updated when complete_archive_mxf_file() is called */


    /* Preface - ContentStorage - MaterialPackage - Timecode Track */
    CHK_OFAIL(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(Track), &newOutput->materialPackageTrackSet));
    CHK_OFAIL(mxf_add_array_item_strongref(newOutput->materialPackageSet, &MXF_ITEM_K(GenericPackage, Tracks), newOutput->materialPackageTrackSet));
    mxf_snprintf(nameBuffer, sizeof(nameBuffer), "TC%d", 1);
    CHK_OFAIL(convert_string(nameBuffer, &newOutput->tempString));
    CHK_OFAIL(mxf_set_utf16string_item(newOutput->materialPackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackName), newOutput->tempString));
    CHK_OFAIL(mxf_set_uint32_item(newOutput->materialPackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackID), 1));
    CHK_OFAIL(mxf_set_uint32_item(newOutput->materialPackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackNumber), 0));
    CHK_OFAIL(mxf_set_rational_item(newOutput->materialPackageTrackSet, &MXF_ITEM_K(Track, EditRate), frameRate));
    CHK_OFAIL(mxf_set_position_item(newOutput->materialPackageTrackSet, &MXF_ITEM_K(Track, Origin), 0));

    /* Preface - ContentStorage - MaterialPackage - Timecode Track - TimecodeComponent */
    CHK_OFAIL(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(TimecodeComponent), &newOutput->timecodeComponentSet));
    CHK_OFAIL(mxf_set_strongref_item(newOutput->materialPackageTrackSet, &MXF_ITEM_K(GenericTrack, Sequence), newOutput->timecodeComponentSet));
    CHK_OFAIL(mxf_set_ul_item(newOutput->timecodeComponentSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &MXF_DDEF_L(Timecode)));
    CHK_OFAIL(mxf_set_length_item(newOutput->timecodeComponentSet, &MXF_ITEM_K(StructuralComponent, Duration), -1)); /* updated when writing completed */
    CHK_OFAIL(mxf_set_uint16_item(newOutput->timecodeComponentSet, &MXF_ITEM_K(TimecodeComponent, RoundedTimecodeBase), roundedTCBase));
    CHK_OFAIL(mxf_set_boolean_item(newOutput->timecodeComponentSet, &MXF_ITEM_K(TimecodeComponent, DropFrame), 0));
    CHK_OFAIL(mxf_set_position_item(newOutput->timecodeComponentSet, &MXF_ITEM_K(TimecodeComponent, StartTimecode), 0));

    CHK_OFAIL(mxf_get_item(newOutput->timecodeComponentSet, &MXF_ITEM_K(StructuralComponent, Duration), &newOutput->durationItems[newOutput->numDurationItems++]));


    /* Preface - ContentStorage - MaterialPackage - Timeline Tracks */
    for (i = 0; i < newOutput->numAudioTracks + 1; i++)
    {
        int isPicture = (i == 0);

        /* Preface - ContentStorage - MaterialPackage - Timeline Track */
        CHK_OFAIL(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(Track), &newOutput->materialPackageTrackSet));
        CHK_OFAIL(mxf_add_array_item_strongref(newOutput->materialPackageSet, &MXF_ITEM_K(GenericPackage, Tracks), newOutput->materialPackageTrackSet));
        if (isPicture)
        {
            mxf_snprintf(nameBuffer, sizeof(nameBuffer), "V%d", 1);
            CHK_OFAIL(convert_string(nameBuffer, &newOutput->tempString));
            CHK_OFAIL(mxf_set_utf16string_item(newOutput->materialPackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackName), newOutput->tempString));
        }
        else
        {
            mxf_snprintf(nameBuffer, sizeof(nameBuffer), "A%d", i);
            CHK_OFAIL(convert_string(nameBuffer, &newOutput->tempString));
            CHK_OFAIL(mxf_set_utf16string_item(newOutput->materialPackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackName), newOutput->tempString));
        }
        CHK_OFAIL(mxf_set_uint32_item(newOutput->materialPackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackID), i + 2));
        CHK_OFAIL(mxf_set_uint32_item(newOutput->materialPackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackNumber), 0));
        if (isPicture)
        {
            CHK_OFAIL(mxf_set_rational_item(newOutput->materialPackageTrackSet, &MXF_ITEM_K(Track, EditRate), frameRate));
        }
        else
        {
            CHK_OFAIL(mxf_set_rational_item(newOutput->materialPackageTrackSet, &MXF_ITEM_K(Track, EditRate), frameRate));
        }
        CHK_OFAIL(mxf_set_position_item(newOutput->materialPackageTrackSet, &MXF_ITEM_K(Track, Origin), 0));

        /* Preface - ContentStorage - MaterialPackage - Timeline Track - Sequence */
        CHK_OFAIL(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(Sequence), &newOutput->sequenceSet));
        CHK_OFAIL(mxf_set_strongref_item(newOutput->materialPackageTrackSet, &MXF_ITEM_K(GenericTrack, Sequence), newOutput->sequenceSet));
        if (isPicture)
        {
            CHK_OFAIL(mxf_set_ul_item(newOutput->sequenceSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &MXF_DDEF_L(Picture)));
            CHK_OFAIL(mxf_set_length_item(newOutput->sequenceSet, &MXF_ITEM_K(StructuralComponent, Duration), -1)); /* updated when writing completed */
        }
        else
        {
            CHK_OFAIL(mxf_set_ul_item(newOutput->sequenceSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &MXF_DDEF_L(Sound)));
            CHK_OFAIL(mxf_set_length_item(newOutput->sequenceSet, &MXF_ITEM_K(StructuralComponent, Duration), -1)); /* updated when writing completed */
        }

        CHK_OFAIL(mxf_get_item(newOutput->sequenceSet, &MXF_ITEM_K(StructuralComponent, Duration), &newOutput->durationItems[newOutput->numDurationItems++]));

        /* Preface - ContentStorage - MaterialPackage - Timeline Track - Sequence - SourceClip */
        CHK_OFAIL(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(SourceClip), &newOutput->sourceClipSet));
        CHK_OFAIL(mxf_add_array_item_strongref(newOutput->sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), newOutput->sourceClipSet));
        if (isPicture)
        {
            CHK_OFAIL(mxf_set_ul_item(newOutput->sourceClipSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &MXF_DDEF_L(Picture)));
            CHK_OFAIL(mxf_set_length_item(newOutput->sourceClipSet, &MXF_ITEM_K(StructuralComponent, Duration), -1)); /* updated when writing completed */
        }
        else
        {
            CHK_OFAIL(mxf_set_ul_item(newOutput->sourceClipSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &MXF_DDEF_L(Sound)));
            CHK_OFAIL(mxf_set_length_item(newOutput->sourceClipSet, &MXF_ITEM_K(StructuralComponent, Duration), -1)); /* updated when writing completed */
        }
        CHK_OFAIL(mxf_set_position_item(newOutput->sourceClipSet, &MXF_ITEM_K(SourceClip, StartPosition), 0));
        CHK_OFAIL(mxf_set_uint32_item(newOutput->sourceClipSet, &MXF_ITEM_K(SourceClip, SourceTrackID), i + 1));
        CHK_OFAIL(mxf_set_umid_item(newOutput->sourceClipSet, &MXF_ITEM_K(SourceClip, SourcePackageID), &newOutput->fileSourcePackageUID));

        CHK_OFAIL(mxf_get_item(newOutput->sourceClipSet, &MXF_ITEM_K(StructuralComponent, Duration), &newOutput->durationItems[newOutput->numDurationItems++]));
    }



    /* Preface - ContentStorage - SourcePackage */
    CHK_OFAIL(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(SourcePackage), &newOutput->sourcePackageSet));
    CHK_OFAIL(mxf_add_array_item_strongref(newOutput->contentStorageSet, &MXF_ITEM_K(ContentStorage, Packages), newOutput->sourcePackageSet));
    CHK_OFAIL(mxf_set_umid_item(newOutput->sourcePackageSet, &MXF_ITEM_K(GenericPackage, PackageUID), &newOutput->fileSourcePackageUID));
    CHK_OFAIL(mxf_set_timestamp_item(newOutput->sourcePackageSet, &MXF_ITEM_K(GenericPackage, PackageCreationDate), &newOutput->now));
    CHK_OFAIL(mxf_set_timestamp_item(newOutput->sourcePackageSet, &MXF_ITEM_K(GenericPackage, PackageModifiedDate), &newOutput->now));


    /* Preface - ContentStorage - SourcePackage - Timeline Tracks */
    for (i = 0; i < newOutput->numAudioTracks + 1; i++)
    {
        int isPicture = (i == 0);

        /* Preface - ContentStorage - SourcePackage - Timeline Track */
        CHK_OFAIL(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(Track), &newOutput->sourcePackageTrackSet));
        CHK_OFAIL(mxf_add_array_item_strongref(newOutput->sourcePackageSet, &MXF_ITEM_K(GenericPackage, Tracks), newOutput->sourcePackageTrackSet));
        if (isPicture)
        {
            mxf_snprintf(nameBuffer, sizeof(nameBuffer), "V%d", 1);
            CHK_OFAIL(convert_string(nameBuffer, &newOutput->tempString));
            CHK_OFAIL(mxf_set_utf16string_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackName), newOutput->tempString));
        }
        else
        {
            mxf_snprintf(nameBuffer, sizeof(nameBuffer), "A%d", i);
            CHK_OFAIL(convert_string(nameBuffer, &newOutput->tempString));
            CHK_OFAIL(mxf_set_utf16string_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackName), newOutput->tempString));
        }
        CHK_OFAIL(mxf_set_uint32_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackID), i + 1));
        if (isPicture)
        {
            mxf_complete_essence_element_track_num(&videoTrackNum, 1, MXF_UNC_FRAME_WRAPPED_EE_TYPE, 1);
            CHK_OFAIL(mxf_set_uint32_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackNumber), videoTrackNum));
            CHK_OFAIL(mxf_set_rational_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(Track, EditRate), frameRate));
        }
        else
        {
            mxf_complete_essence_element_track_num(&audioTrackNum, newOutput->numAudioTracks, MXF_BWF_FRAME_WRAPPED_EE_TYPE, i);
            CHK_OFAIL(mxf_set_uint32_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackNumber), audioTrackNum));
            CHK_OFAIL(mxf_set_rational_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(Track, EditRate), frameRate));
        }
        CHK_OFAIL(mxf_set_position_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(Track, Origin), 0));

        /* Preface - ContentStorage - SourcePackage - Timeline Track - Sequence */
        CHK_OFAIL(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(Sequence), &newOutput->sequenceSet));
        CHK_OFAIL(mxf_set_strongref_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, Sequence), newOutput->sequenceSet));
        if (isPicture)
        {
            CHK_OFAIL(mxf_set_ul_item(newOutput->sequenceSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &MXF_DDEF_L(Picture)));
            CHK_OFAIL(mxf_set_length_item(newOutput->sequenceSet, &MXF_ITEM_K(StructuralComponent, Duration), -1)); /* updated when writing completed */
        }
        else
        {
            CHK_OFAIL(mxf_set_ul_item(newOutput->sequenceSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &MXF_DDEF_L(Sound)));
            CHK_OFAIL(mxf_set_length_item(newOutput->sequenceSet, &MXF_ITEM_K(StructuralComponent, Duration), -1)); /* updated when writing completed */
        }

        CHK_OFAIL(mxf_get_item(newOutput->sequenceSet, &MXF_ITEM_K(StructuralComponent, Duration), &newOutput->durationItems[newOutput->numDurationItems++]));

        /* Preface - ContentStorage - SourcePackage - Timeline Track - Sequence - SourceClip */
        CHK_OFAIL(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(SourceClip), &newOutput->sourceClipSet));
        CHK_OFAIL(mxf_add_array_item_strongref(newOutput->sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), newOutput->sourceClipSet));
        if (isPicture)
        {
            CHK_OFAIL(mxf_set_ul_item(newOutput->sourceClipSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &MXF_DDEF_L(Picture)));
            CHK_OFAIL(mxf_set_length_item(newOutput->sourceClipSet, &MXF_ITEM_K(StructuralComponent, Duration), -1)); /* updated when writing completed */
            CHK_OFAIL(mxf_set_uint32_item(newOutput->sourceClipSet, &MXF_ITEM_K(SourceClip, SourceTrackID), 1));
            CHK_OFAIL(mxf_set_position_item(newOutput->sourceClipSet, &MXF_ITEM_K(SourceClip, StartPosition), startPosition));
        }
        else
        {
            CHK_OFAIL(mxf_set_ul_item(newOutput->sourceClipSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &MXF_DDEF_L(Sound)));
            CHK_OFAIL(mxf_set_length_item(newOutput->sourceClipSet, &MXF_ITEM_K(StructuralComponent, Duration), -1)); /* updated when writing completed */
            CHK_OFAIL(mxf_set_uint32_item(newOutput->sourceClipSet, &MXF_ITEM_K(SourceClip, SourceTrackID), i + 1));
            CHK_OFAIL(mxf_set_position_item(newOutput->sourceClipSet, &MXF_ITEM_K(SourceClip, StartPosition), startPosition));
        }
        CHK_OFAIL(mxf_set_umid_item(newOutput->sourceClipSet, &MXF_ITEM_K(SourceClip, SourcePackageID), &newOutput->tapeSourcePackageUID));

        CHK_OFAIL(mxf_get_item(newOutput->sourceClipSet, &MXF_ITEM_K(StructuralComponent, Duration), &newOutput->durationItems[newOutput->numDurationItems++]));
    }

    /* Preface - ContentStorage - SourcePackage - MultipleDescriptor */
    CHK_OFAIL(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(MultipleDescriptor), &newOutput->multipleDescriptorSet));
    CHK_OFAIL(mxf_set_strongref_item(newOutput->sourcePackageSet, &MXF_ITEM_K(SourcePackage, Descriptor), newOutput->multipleDescriptorSet));
    CHK_OFAIL(mxf_set_rational_item(newOutput->multipleDescriptorSet, &MXF_ITEM_K(FileDescriptor, SampleRate), frameRate));
    CHK_OFAIL(mxf_set_ul_item(newOutput->multipleDescriptorSet, &MXF_ITEM_K(FileDescriptor, EssenceContainer), &MXF_EC_L(MultipleWrappings)));

    newOutput->descriptorSets[newOutput->numDescriptorSets++] = newOutput->multipleDescriptorSet;  /* ContainerDuration updated when writing completed */


    /* Preface - ContentStorage - SourcePackage - MultipleDescriptor - NetworkLocator */
    CHK_OFAIL(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(NetworkLocator), &newOutput->networkLocatorSet));
    CHK_OFAIL(mxf_add_array_item_strongref(newOutput->multipleDescriptorSet, &MXF_ITEM_K(GenericDescriptor, Locators), newOutput->networkLocatorSet));
    CHK_OFAIL(convert_string(filename, &newOutput->tempString));
    CHK_OFAIL(mxf_set_fixed_size_utf16string_item(newOutput->networkLocatorSet, &MXF_ITEM_K(NetworkLocator, URLString), newOutput->tempString, NETWORK_LOCATOR_URL_SIZE));


    /* Preface - ContentStorage - SourcePackage - MultipleDescriptor - CDCIEssenceDescriptor */
    CHK_OFAIL(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(CDCIEssenceDescriptor), &newOutput->cdciDescriptorSet));
    CHK_OFAIL(mxf_add_array_item_strongref(newOutput->multipleDescriptorSet, &MXF_ITEM_K(MultipleDescriptor, SubDescriptorUIDs), newOutput->cdciDescriptorSet));
    CHK_OFAIL(mxf_set_uint32_item(newOutput->cdciDescriptorSet, &MXF_ITEM_K(FileDescriptor, LinkedTrackID), 1));
    CHK_OFAIL(mxf_set_rational_item(newOutput->cdciDescriptorSet, &MXF_ITEM_K(FileDescriptor, SampleRate), frameRate));
    if (componentDepth != 8)
    {
        CHK_OFAIL(mxf_set_ul_item(newOutput->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, PictureEssenceCoding), &MXF_CMDEF_L(UNC_10B_422_INTERLEAVED)));
    }
    CHK_OFAIL(mxf_set_uint8_item(newOutput->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, FrameLayout), frameLayout));
    CHK_OFAIL(mxf_set_uint8_item(newOutput->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, SignalStandard), signalStandard));
    CHK_OFAIL(mxf_set_ul_item(newOutput->cdciDescriptorSet, &MXF_ITEM_K(FileDescriptor, EssenceContainer), VIDEO_FORMATS[formatIndex].ecLabel));
    if (componentDepth == 8)
    {
        CHK_OFAIL(mxf_set_uint32_item(newOutput->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, StoredWidth), VIDEO_FORMATS[formatIndex].width));
    }
    else
    {
        CHK_OFAIL(mxf_set_uint32_item(newOutput->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, StoredWidth), (VIDEO_FORMATS[formatIndex].width + 47) / 48 * 48));
    }
    CHK_OFAIL(mxf_set_uint32_item(newOutput->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, StoredHeight), VIDEO_FORMATS[formatIndex].height));
    CHK_OFAIL(mxf_set_uint32_item(newOutput->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, DisplayWidth), VIDEO_FORMATS[formatIndex].width));
    CHK_OFAIL(mxf_set_uint32_item(newOutput->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, DisplayHeight), VIDEO_FORMATS[formatIndex].height));
    CHK_OFAIL(mxf_alloc_array_item_elements(newOutput->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, VideoLineMap), 4, 2, &arrayElement));
    mxf_set_int32(VIDEO_FORMATS[formatIndex].videoLineMap[0], arrayElement);
    mxf_set_int32(VIDEO_FORMATS[formatIndex].videoLineMap[1], &arrayElement[4]);
    if (aspectRatio->numerator > 0 && aspectRatio->denominator > 0)
    {
        CHK_OFAIL(mxf_set_rational_item(newOutput->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, AspectRatio), aspectRatio));
    }
    else
    {
        mxfRational unknownAspectRatio = {0, 0};
        CHK_OFAIL(mxf_set_rational_item(newOutput->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, AspectRatio), &unknownAspectRatio));
    }
    CHK_OFAIL(mxf_set_uint32_item(newOutput->cdciDescriptorSet, &MXF_ITEM_K(CDCIEssenceDescriptor, ComponentDepth), componentDepth));
    CHK_OFAIL(mxf_set_uint32_item(newOutput->cdciDescriptorSet, &MXF_ITEM_K(CDCIEssenceDescriptor, HorizontalSubsampling), 2));
    CHK_OFAIL(mxf_set_uint32_item(newOutput->cdciDescriptorSet, &MXF_ITEM_K(CDCIEssenceDescriptor, VerticalSubsampling), 1));

    newOutput->descriptorSets[newOutput->numDescriptorSets++] = newOutput->cdciDescriptorSet;  /* ContainerDuration updated when writing completed */



    for (i = 0; i < newOutput->numAudioTracks; i++)
    {
        /* Preface - ContentStorage - SourcePackage - MultipleDescriptor - WAVEssenceDescriptor */
        CHK_OFAIL(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(WaveAudioDescriptor), &newOutput->bwfDescriptorSet));
        CHK_OFAIL(mxf_add_array_item_strongref(newOutput->multipleDescriptorSet, &MXF_ITEM_K(MultipleDescriptor, SubDescriptorUIDs), newOutput->bwfDescriptorSet));
        CHK_OFAIL(mxf_set_uint32_item(newOutput->bwfDescriptorSet, &MXF_ITEM_K(FileDescriptor, LinkedTrackID), i + 2));
        CHK_OFAIL(mxf_set_rational_item(newOutput->bwfDescriptorSet, &MXF_ITEM_K(FileDescriptor, SampleRate), frameRate));
        CHK_OFAIL(mxf_set_ul_item(newOutput->bwfDescriptorSet, &MXF_ITEM_K(FileDescriptor, EssenceContainer), &MXF_EC_L(BWFFrameWrapped)));
        CHK_OFAIL(mxf_set_rational_item(newOutput->bwfDescriptorSet, &MXF_ITEM_K(GenericSoundEssenceDescriptor, AudioSamplingRate), &AUDIO_SAMPLING_RATE));
        CHK_OFAIL(mxf_set_boolean_item(newOutput->bwfDescriptorSet, &MXF_ITEM_K(GenericSoundEssenceDescriptor, Locked), 1));
        CHK_OFAIL(mxf_set_uint32_item(newOutput->bwfDescriptorSet, &MXF_ITEM_K(GenericSoundEssenceDescriptor, ChannelCount), 1));
        CHK_OFAIL(mxf_set_uint32_item(newOutput->bwfDescriptorSet, &MXF_ITEM_K(GenericSoundEssenceDescriptor, QuantizationBits), audioQuantBits));
        CHK_OFAIL(mxf_set_uint16_item(newOutput->bwfDescriptorSet, &MXF_ITEM_K(WaveAudioDescriptor, BlockAlign), newOutput->blockAlign));
        CHK_OFAIL(mxf_set_uint32_item(newOutput->bwfDescriptorSet, &MXF_ITEM_K(WaveAudioDescriptor, AvgBps), 48000 * newOutput->blockAlign));

        newOutput->descriptorSets[newOutput->numDescriptorSets++] = newOutput->bwfDescriptorSet;  /* ContainerDuration updated when writing completed */
    }

    /* Preface - ContentStorage - SourcePackage - DM Track */
    CHK_OFAIL(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(StaticTrack), &newOutput->sourcePackageTrackSet));
    CHK_OFAIL(mxf_add_array_item_strongref(newOutput->sourcePackageSet, &MXF_ITEM_K(GenericPackage, Tracks), newOutput->sourcePackageTrackSet));
    mxf_snprintf(nameBuffer, sizeof(nameBuffer), "DM%d", 1);
    CHK_OFAIL(convert_string(nameBuffer, &newOutput->tempString));
    CHK_OFAIL(mxf_set_utf16string_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackName), newOutput->tempString));
    CHK_OFAIL(mxf_set_uint32_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackID), newOutput->numAudioTracks + 2));
    CHK_OFAIL(mxf_set_uint32_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackNumber), 0));

    /* Preface - ContentStorage - SourcePackage - DM Track - Sequence */
    CHK_OFAIL(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(Sequence), &newOutput->sequenceSet));
    CHK_OFAIL(mxf_set_strongref_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, Sequence), newOutput->sequenceSet));
    CHK_OFAIL(mxf_set_ul_item(newOutput->sequenceSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &MXF_DDEF_L(DescriptiveMetadata)));

    /* Preface - ContentStorage - SourcePackage - DM Track - Sequence - DMSegment */
    CHK_OFAIL(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(DMSegment), &newOutput->dmSet));
    CHK_OFAIL(mxf_add_array_item_strongref(newOutput->sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), newOutput->dmSet));
    CHK_OFAIL(mxf_set_ul_item(newOutput->dmSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &MXF_DDEF_L(DescriptiveMetadata)));

    /* Preface - ContentStorage - SourcePackage - DM Track - Sequence - DMSegment - DMFramework */
    CHK_OFAIL(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(APP_InfaxFramework), &newOutput->fileDMFrameworkSet));
    CHK_OFAIL(mxf_set_strongref_item(newOutput->dmSet, &MXF_ITEM_K(DMSegment, DMFramework), newOutput->fileDMFrameworkSet));
    mxf_set_fixed_set_space_allocation(newOutput->fileDMFrameworkSet, FIXED_INFAX_SET_ALLOCATION_SIZE);
    CHK_OFAIL(set_infax_data(newOutput->fileDMFrameworkSet, &nullInfaxData));
    /* APP_InfaxFramework will be filled in when update_mxf_file() is called */


    /* Preface - ContentStorage - tape SourcePackage */
    CHK_ORET(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(SourcePackage), &newOutput->tapeSourcePackageSet));
    CHK_ORET(mxf_add_array_item_strongref(newOutput->contentStorageSet, &MXF_ITEM_K(ContentStorage, Packages), newOutput->tapeSourcePackageSet));
    CHK_ORET(mxf_set_umid_item(newOutput->tapeSourcePackageSet, &MXF_ITEM_K(GenericPackage, PackageUID), &newOutput->tapeSourcePackageUID));
    CHK_ORET(mxf_set_timestamp_item(newOutput->tapeSourcePackageSet, &MXF_ITEM_K(GenericPackage, PackageCreationDate), &newOutput->now));
    CHK_ORET(mxf_set_timestamp_item(newOutput->tapeSourcePackageSet, &MXF_ITEM_K(GenericPackage, PackageModifiedDate), &newOutput->now));
    CHK_ORET(mxf_set_utf16string_item(newOutput->tapeSourcePackageSet, &MXF_ITEM_K(GenericPackage, Name), L"Source tape"));
    /* Name will be updated when complete_archive_mxf_file() is called */

    /* Preface - ContentStorage - tape SourcePackage - video Timeline Track */
    CHK_ORET(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(Track), &newOutput->sourcePackageTrackSet));
    CHK_ORET(mxf_add_array_item_strongref(newOutput->tapeSourcePackageSet, &MXF_ITEM_K(GenericPackage, Tracks), newOutput->sourcePackageTrackSet));
    CHK_ORET(mxf_set_uint32_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackID), 1));
    mxf_snprintf(nameBuffer, sizeof(nameBuffer), "V%d", 1);
    CHK_ORET(convert_string(nameBuffer, &newOutput->tempString));
    CHK_ORET(mxf_set_utf16string_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackName), newOutput->tempString));
    CHK_ORET(mxf_set_uint32_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackNumber), 1));
    CHK_ORET(mxf_set_rational_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(Track, EditRate), frameRate));
    CHK_ORET(mxf_set_position_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(Track, Origin), 0));

    /* Preface - ContentStorage - tape SourcePackage - video Timeline Track - Sequence */
    CHK_ORET(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(Sequence), &newOutput->sequenceSet));
    CHK_ORET(mxf_set_strongref_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, Sequence), newOutput->sequenceSet));
    CHK_ORET(mxf_set_ul_item(newOutput->sequenceSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &MXF_DDEF_L(Picture)));
    CHK_ORET(mxf_set_length_item(newOutput->sequenceSet, &MXF_ITEM_K(StructuralComponent, Duration), tapeLen));

    /* Preface - ContentStorage - tape SourcePackage - video Timeline Track - Sequence - SourceClip */
    CHK_ORET(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(SourceClip), &newOutput->sourceClipSet));
    CHK_ORET(mxf_add_array_item_strongref(newOutput->sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), newOutput->sourceClipSet));
    CHK_ORET(mxf_set_ul_item(newOutput->sourceClipSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &MXF_DDEF_L(Picture)));
    CHK_ORET(mxf_set_length_item(newOutput->sourceClipSet, &MXF_ITEM_K(StructuralComponent, Duration), tapeLen));
    CHK_ORET(mxf_set_position_item(newOutput->sourceClipSet, &MXF_ITEM_K(SourceClip, StartPosition), 0));
    CHK_ORET(mxf_set_umid_item(newOutput->sourceClipSet, &MXF_ITEM_K(SourceClip, SourcePackageID), &g_Null_UMID));
    CHK_ORET(mxf_set_uint32_item(newOutput->sourceClipSet, &MXF_ITEM_K(SourceClip, SourceTrackID), 0));


    for (i = 0; i < newOutput->numAudioTracks; i++)
    {
        /* Preface - ContentStorage - tape SourcePackage - audio Timeline Track */
        CHK_ORET(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(Track), &newOutput->sourcePackageTrackSet));
        CHK_ORET(mxf_add_array_item_strongref(newOutput->tapeSourcePackageSet, &MXF_ITEM_K(GenericPackage, Tracks), newOutput->sourcePackageTrackSet));
        CHK_ORET(mxf_set_uint32_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackID), i + 2));
        mxf_snprintf(nameBuffer, sizeof(nameBuffer), "A%d", i + 1);
        CHK_ORET(convert_string(nameBuffer, &newOutput->tempString));
        CHK_ORET(mxf_set_utf16string_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackName), newOutput->tempString));
        CHK_ORET(mxf_set_uint32_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackNumber), i + 1));
        CHK_ORET(mxf_set_rational_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(Track, EditRate), frameRate));
        CHK_ORET(mxf_set_position_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(Track, Origin), 0));

        /* Preface - ContentStorage - tape SourcePackage - audio Timeline Track - Sequence */
        CHK_ORET(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(Sequence), &newOutput->sequenceSet));
        CHK_ORET(mxf_set_strongref_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, Sequence), newOutput->sequenceSet));
        CHK_ORET(mxf_set_ul_item(newOutput->sequenceSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &MXF_DDEF_L(Sound)));
        CHK_ORET(mxf_set_length_item(newOutput->sequenceSet, &MXF_ITEM_K(StructuralComponent, Duration), tapeLen));

        /* Preface - ContentStorage - tape SourcePackage - audio Timeline Track - Sequence - SourceClip */
        CHK_ORET(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(SourceClip), &newOutput->sourceClipSet));
        CHK_ORET(mxf_add_array_item_strongref(newOutput->sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), newOutput->sourceClipSet));
        CHK_ORET(mxf_set_ul_item(newOutput->sourceClipSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &MXF_DDEF_L(Sound)));
        CHK_ORET(mxf_set_length_item(newOutput->sourceClipSet, &MXF_ITEM_K(StructuralComponent, Duration), tapeLen));
        CHK_ORET(mxf_set_position_item(newOutput->sourceClipSet, &MXF_ITEM_K(SourceClip, StartPosition), 0));
        CHK_ORET(mxf_set_umid_item(newOutput->sourceClipSet, &MXF_ITEM_K(SourceClip, SourcePackageID), &g_Null_UMID));
        CHK_ORET(mxf_set_uint32_item(newOutput->sourceClipSet, &MXF_ITEM_K(SourceClip, SourceTrackID), 0));
    }

    /* Preface - ContentStorage - tape SourcePackage - timecode Timeline Track */
    CHK_ORET(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(Track), &newOutput->sourcePackageTrackSet));
    CHK_ORET(mxf_add_array_item_strongref(newOutput->tapeSourcePackageSet, &MXF_ITEM_K(GenericPackage, Tracks), newOutput->sourcePackageTrackSet));
    CHK_ORET(mxf_set_uint32_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackID), newOutput->numAudioTracks + 2));
    CHK_ORET(mxf_set_uint32_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackNumber), 0));
    mxf_snprintf(nameBuffer, sizeof(nameBuffer), "T%d", 1);
    CHK_ORET(convert_string(nameBuffer, &newOutput->tempString));
    CHK_ORET(mxf_set_utf16string_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackName), newOutput->tempString));
    CHK_ORET(mxf_set_rational_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(Track, EditRate), frameRate));
    CHK_ORET(mxf_set_position_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(Track, Origin), 0));

    /* Preface - ContentStorage - tape SourcePackage - timecode Timeline Track - Sequence */
    CHK_ORET(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(Sequence), &newOutput->sequenceSet));
    CHK_ORET(mxf_set_strongref_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, Sequence), newOutput->sequenceSet));
    CHK_ORET(mxf_set_ul_item(newOutput->sequenceSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &MXF_DDEF_L(Timecode)));
    CHK_ORET(mxf_set_length_item(newOutput->sequenceSet, &MXF_ITEM_K(StructuralComponent, Duration), tapeLen));

    /* Preface - ContentStorage - tape SourcePackage - Timecode Track - Sequence - TimecodeComponent */
    CHK_ORET(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(TimecodeComponent), &newOutput->timecodeComponentSet));
    CHK_ORET(mxf_add_array_item_strongref(newOutput->sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), newOutput->timecodeComponentSet));
    CHK_ORET(mxf_set_ul_item(newOutput->timecodeComponentSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &MXF_DDEF_L(Timecode)));
    CHK_ORET(mxf_set_length_item(newOutput->timecodeComponentSet, &MXF_ITEM_K(StructuralComponent, Duration), tapeLen));
    CHK_ORET(mxf_set_uint16_item(newOutput->timecodeComponentSet, &MXF_ITEM_K(TimecodeComponent, RoundedTimecodeBase), roundedTCBase));
    CHK_ORET(mxf_set_boolean_item(newOutput->timecodeComponentSet, &MXF_ITEM_K(TimecodeComponent, DropFrame), 0));
    CHK_ORET(mxf_set_position_item(newOutput->timecodeComponentSet, &MXF_ITEM_K(TimecodeComponent, StartTimecode), 0));


    /* Preface - ContentStorage - tape SourcePackage - DM Track */
    CHK_OFAIL(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(StaticTrack), &newOutput->sourcePackageTrackSet));
    CHK_OFAIL(mxf_add_array_item_strongref(newOutput->tapeSourcePackageSet, &MXF_ITEM_K(GenericPackage, Tracks), newOutput->sourcePackageTrackSet));
    mxf_snprintf(nameBuffer, sizeof(nameBuffer), "DM%d", 1);
    CHK_OFAIL(convert_string(nameBuffer, &newOutput->tempString));
    CHK_OFAIL(mxf_set_utf16string_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackName), newOutput->tempString));
    CHK_OFAIL(mxf_set_uint32_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackID), newOutput->numAudioTracks + 3));
    CHK_OFAIL(mxf_set_uint32_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackNumber), 0));

    /* Preface - ContentStorage - tape SourcePackage - DM Track - Sequence */
    CHK_OFAIL(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(Sequence), &newOutput->sequenceSet));
    CHK_OFAIL(mxf_set_strongref_item(newOutput->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, Sequence), newOutput->sequenceSet));
    CHK_OFAIL(mxf_set_ul_item(newOutput->sequenceSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &MXF_DDEF_L(DescriptiveMetadata)));

    /* Preface - ContentStorage - tape SourcePackage - DM Track - Sequence - DMSegment */
    CHK_OFAIL(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(DMSegment), &newOutput->dmSet));
    CHK_OFAIL(mxf_add_array_item_strongref(newOutput->sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), newOutput->dmSet));
    CHK_OFAIL(mxf_set_ul_item(newOutput->dmSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &MXF_DDEF_L(DescriptiveMetadata)));

    /* Preface - ContentStorage - tape SourcePackage - DM Track - Sequence - DMSegment - DMFramework */
    CHK_OFAIL(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(APP_InfaxFramework), &newOutput->tapeDMFrameworkSet));
    CHK_OFAIL(mxf_set_strongref_item(newOutput->dmSet, &MXF_ITEM_K(DMSegment, DMFramework), newOutput->tapeDMFrameworkSet));
    mxf_set_fixed_set_space_allocation(newOutput->tapeDMFrameworkSet, FIXED_INFAX_SET_ALLOCATION_SIZE);
    /* APP_InfaxFramework will be completed in when complete_archive_mxf_file() is called */


    /* Preface - ContentStorage - tape SourcePackage - TapeDescriptor */
    CHK_ORET(mxf_create_set(newOutput->headerMetadata, &MXF_SET_K(TapeDescriptor), &newOutput->tapeDescriptorSet));
    CHK_ORET(mxf_set_strongref_item(newOutput->tapeSourcePackageSet, &MXF_ITEM_K(SourcePackage, Descriptor), newOutput->tapeDescriptorSet));



    /*
     * Write the Header Metadata
     */

    CHK_OFAIL(mxf_mark_header_start(newOutput->mxfFile, newOutput->headerPartition));
    CHK_OFAIL(mxf_write_header_metadata(newOutput->mxfFile, newOutput->headerMetadata));
    CHK_OFAIL(mxf_mark_header_end(newOutput->mxfFile, newOutput->headerPartition));


    /*
     * Write the index table segment
     */

    CHK_OFAIL(mxf_mark_index_start(newOutput->mxfFile, newOutput->headerPartition));

    CHK_OFAIL(mxf_create_index_table_segment(&newOutput->indexSegment));
    mxf_generate_uuid(&newOutput->indexSegment->instanceUID);
    newOutput->indexSegment->indexEditRate = *frameRate;
    newOutput->indexSegment->indexDuration = 0; /* although valid, will be updated when writing completed */
    newOutput->indexSegment->editUnitByteCount = mxfKey_extlen + ESS_ELEMENT_LLEN + newOutput->systemItemSize +
                                                 mxfKey_extlen + ESS_ELEMENT_LLEN + newOutput->videoFrameSize +
                                                 newOutput->numAudioTracks * newOutput->audioElementSize;
    newOutput->indexSegment->indexSID = INDEX_SID;
    newOutput->indexSegment->bodySID = BODY_SID;
    deltaOffset = 0;
    CHK_OFAIL(mxf_default_add_delta_entry(NULL, 0, newOutput->indexSegment, 0, 0, deltaOffset));
    deltaOffset += mxfKey_extlen + ESS_ELEMENT_LLEN + newOutput->systemItemSize;
    CHK_OFAIL(mxf_default_add_delta_entry(NULL, 0, newOutput->indexSegment, 0, 0, deltaOffset));
    deltaOffset += mxfKey_extlen + ESS_ELEMENT_LLEN + newOutput->videoFrameSize;
    for (i = 0; i < newOutput->numAudioTracks; i++)
    {
        CHK_OFAIL(mxf_default_add_delta_entry(NULL, 0, newOutput->indexSegment, 0, 0, deltaOffset));
        deltaOffset += newOutput->audioElementSize;
    }

    CHK_OFAIL(mxf_write_index_table_segment(newOutput->mxfFile, newOutput->indexSegment));

    /* allocate space for any future header metadata extensions */
    CHK_OFAIL((filePos = mxf_file_tell(newOutput->mxfFile)) >= 0);
    CHK_OFAIL((uint64_t)filePos < FIXED_BODY_OFFSET - 17); /* min fill is 17 */
    CHK_OFAIL(mxf_fill_to_position(newOutput->mxfFile, FIXED_BODY_OFFSET));

    CHK_OFAIL(mxf_mark_index_end(newOutput->mxfFile, newOutput->headerPartition));


    /*
     * Update the header partition pack
     */

    CHK_ORET(mxf_update_partitions(newOutput->mxfFile, newOutput->partitions));


    *output = newOutput;
    return 1;

fail:
    free_archive_mxf_file(&newOutput);
    return 0;
}


int write_system_item(ArchiveMXFWriter *output, ArchiveTimecode vitc, ArchiveTimecode ltc, const uint32_t *crc32,
                      int numCRC32)
{
    uint8_t t12m[8];
    int i;

    CHK_ORET(!output->includeCRC32 || numCRC32 == 1 + output->numAudioTracks);

    CHK_ORET(verify_essence_write_state(output, 1, 0, 0));

    CHK_ORET(add_timecode_to_index(&output->vitcIndex, &vitc));
    CHK_ORET(add_timecode_to_index(&output->ltcIndex, &ltc));

    CHK_ORET(mxf_write_fixed_kl(output->mxfFile, &TIMECODE_SYS_ITEM_ELEMENT_KEY, ESS_ELEMENT_LLEN,
                                output->systemItemSize));

    /* timecode */

    CHK_ORET(mxf_write_uint16(output->mxfFile, 0x0102)); /* local tag */
    CHK_ORET(mxf_write_uint16(output->mxfFile, 24)); /* len */

    CHK_ORET(mxf_write_array_header(output->mxfFile, 2, 8)); /* VITC and LTC SMPTE-12M timecodes */
    convert_timecode_to_12m(&vitc, t12m);
    CHK_ORET(mxf_file_write(output->mxfFile, t12m, 8) == 8);
    convert_timecode_to_12m(&ltc, t12m);
    CHK_ORET(mxf_file_write(output->mxfFile, t12m, 8) == 8);

    /* CRC-32 */

    if (output->includeCRC32)
    {
        CHK_ORET(mxf_write_uint16(output->mxfFile, 0xffff)); /* local tag */
        CHK_ORET(mxf_write_uint16(output->mxfFile, 8 + numCRC32 * 4)); /* len */

        CHK_ORET(mxf_write_array_header(output->mxfFile, numCRC32, 4));
        for (i = 0; i < numCRC32; i++)
        {
            CHK_ORET(mxf_write_uint32(output->mxfFile, crc32[i]));
        }
    }

    update_essence_write_state(output, 1, 0, 0);

    return 1;
}

int write_video_frame(ArchiveMXFWriter *output, const uint8_t *data, uint32_t size)
{
    mxfKey eeKey = UNC_BASE_ELEMENT_KEY;

    if (size != output->videoFrameSize)
    {
        mxf_log_error("Invalid video frame size %ld; expecting %ld" LOG_LOC_FORMAT, size, output->videoFrameSize, LOG_LOC_PARAMS);
        return 0;
    }

    CHK_ORET(verify_essence_write_state(output, 0, 1, 0));

    mxf_complete_essence_element_key(&eeKey, 1, MXF_UNC_FRAME_WRAPPED_EE_TYPE, 1);

    CHK_ORET(mxf_write_fixed_kl(output->mxfFile, &eeKey, ESS_ELEMENT_LLEN, size));
    CHK_ORET(mxf_file_write(output->mxfFile, data, size) == size);

    output->duration++;
    update_essence_write_state(output, 0, 1, 0);

    return 1;
}

int write_audio_frame(ArchiveMXFWriter *output, const uint8_t *data, uint32_t size)
{
    mxfKey eeKey = WAV_BASE_ELEMENT_KEY;

    CHK_ORET(verify_essence_write_state(output, 0, 0, 1));

    CHK_ORET(check_and_update_audio_sequence(output, size));

    mxf_complete_essence_element_key(&eeKey, output->numAudioTracks, MXF_BWF_FRAME_WRAPPED_EE_TYPE,
                                     output->essWriteState.audioNum + 1);

    CHK_ORET(mxf_write_fixed_kl(output->mxfFile, &eeKey, ESS_ELEMENT_LLEN, size));
    CHK_ORET(mxf_file_write(output->mxfFile, data, size) == size);
    if (mxfKey_extlen + ESS_ELEMENT_LLEN + size < output->audioElementSize)
    {
        CHK_ORET(mxf_write_fill(output->mxfFile, output->audioElementSize - (mxfKey_extlen + ESS_ELEMENT_LLEN + size)));
    }

    update_essence_write_state(output, 0, 0, 1);

    return 1;
}

int abort_archive_mxf_file(ArchiveMXFWriter **output)
{
    free_archive_mxf_file(output);
    return 1;
}

int complete_archive_mxf_file(ArchiveMXFWriter **outputRef, const InfaxData *sourceInfaxData,
                              const PSEFailure *pseFailures, long numPSEFailures,
                              const VTRError *vtrErrors, long numVTRErrors,
                              const DigiBetaDropout *digiBetaDropouts, long numDigiBetaDropouts,
                              const TimecodeBreak *timecodeBreaks, long numTimecodeBreaks)
{
    ArchiveMXFWriter *output = *outputRef;
    int i;
    int64_t filePos;
    long j;
    MXFListIterator iter;
    const PSEFailure *pseFailure;
    const VTRError *vtrError;
    const DigiBetaDropout *digiBetaDropout;
    const TimecodeBreak *timecodeBreak;
    uint32_t nextTrackID;
    int numTracks;
    TimecodeIndexSearcher vitcIndexSearcher;
    TimecodeIndexSearcher ltcIndexSearcher;
    int64_t errorPosition;
    long numVTRErrorsLocated = 0;
    long errorIndex;
    long failureIndex;
    long digiBetaDropoutIndex;
    long timecodeBreakIndex;
    char mpName[64];
    int vitcIndexIsNull;
    int ltcIndexIsNull;
    int useVTRLTC = 1;
    int setUseVTRLTC = 0;

    vitcIndexIsNull = is_null_timecode_index(&output->vitcIndex);
    ltcIndexIsNull = is_null_timecode_index(&output->ltcIndex);


    /* update the PSE failure and VTR error counts */

    CHK_ORET(mxf_set_uint32_item(output->prefaceSet, &MXF_ITEM_K(Preface, APP_VTRErrorCount), numVTRErrors));
    CHK_ORET(mxf_set_uint32_item(output->prefaceSet, &MXF_ITEM_K(Preface, APP_PSEFailureCount), numPSEFailures));
    CHK_ORET(mxf_set_uint32_item(output->prefaceSet, &MXF_ITEM_K(Preface, APP_DigiBetaDropoutCount), numDigiBetaDropouts));
    CHK_ORET(mxf_set_uint32_item(output->prefaceSet, &MXF_ITEM_K(Preface, APP_TimecodeBreakCount), numTimecodeBreaks));


    /* update the header metadata durations and audio sequence offset */

    for (i = 0; i < output->numDurationItems; i++)
    {
        CHK_ORET(mxf_set_length_item(output->durationItems[i]->set, &output->durationItems[i]->key, output->duration));
    }
    for (i = 0; i < output->numDescriptorSets; i++)
    {
        CHK_ORET(mxf_set_length_item(output->descriptorSets[i], &MXF_ITEM_K(FileDescriptor, ContainerDuration), output->duration));

        if (output->audioSequenceSize > 1 &&
            output->audioSequenceOffset != UNKNOWN_AUDIO_SEQUENCE_OFFSET &&
            mxf_equals_key(&output->descriptorSets[i]->key, &MXF_SET_K(WaveAudioDescriptor)))
        {
            CHK_ORET(mxf_set_uint8_item(output->descriptorSets[i], &MXF_ITEM_K(WaveAudioDescriptor, SequenceOffset), output->audioSequenceOffset));
        }
    }


    /* add the tape Infax items to the set */

    CHK_ORET(set_infax_data(output->tapeDMFrameworkSet, sourceInfaxData));


    /* update the header metadata MaterialPackage and tape SourcePackage Names */

    CHK_ORET(convert_string(sourceInfaxData->spoolNo, &output->tempString));
    CHK_ORET(mxf_set_utf16string_item(output->tapeSourcePackageSet, &MXF_ITEM_K(GenericPackage, Name), output->tempString));

    mxf_snprintf(mpName, sizeof(mpName), "Archive preservation %s", sourceInfaxData->spoolNo);
    CHK_ORET(convert_string(mpName, &output->tempString));
    CHK_ORET(mxf_set_utf16string_item(output->materialPackageSet, &MXF_ITEM_K(GenericPackage, Name), output->tempString));


    /* re-write the header metadata */

    CHK_ORET(mxf_file_seek(output->mxfFile, output->headerMetadataFilePos, SEEK_SET));
    CHK_ORET(mxf_mark_header_start(output->mxfFile, output->headerPartition));
    CHK_ORET(mxf_write_header_metadata(output->mxfFile, output->headerMetadata));
    CHK_ORET(mxf_mark_header_end(output->mxfFile, output->headerPartition));


    /* re-write the header index table segment */

    CHK_ORET(mxf_mark_index_start(output->mxfFile, output->headerPartition));

    output->indexSegment->indexDuration = output->duration;
    CHK_ORET(mxf_write_index_table_segment(output->mxfFile, output->indexSegment));

    /* fill space to body position */
    CHK_ORET((filePos = mxf_file_tell(output->mxfFile)) >= 0);
    CHK_ORET((uint64_t)filePos < FIXED_BODY_OFFSET - 17); /* min fill is 17 */
    CHK_ORET(mxf_fill_to_position(output->mxfFile, FIXED_BODY_OFFSET));

    CHK_ORET(mxf_mark_index_end(output->mxfFile, output->headerPartition));



    /* Write the Footer Partition Pack */

    CHK_ORET(mxf_file_seek(output->mxfFile, 0, SEEK_END));
    CHK_ORET(mxf_append_new_from_partition(output->partitions, output->headerPartition, &output->footerPartition));
    /* The preservation system will store the MXF file on an LTO. However, to support usage outside the
    preservation system where the file is not stored on an LTO, the partition is defined to be closed */
    output->footerPartition->key = MXF_PP_K(ClosedComplete, Footer);
    output->footerPartition->indexSID = INDEX_SID;

    CHK_ORET(mxf_write_partition(output->mxfFile, output->footerPartition));




    /* update the metadata with just the references to the Tracks for the VTR errors, etc */
    /* the individual VTR errors etc. will be written straight to file
       at the end of the header metadata. This is done to avoid the memory hit that results
       from storing the (potentially large) errors and failures metadata sets in memory */

    nextTrackID = output->numAudioTracks + 3;

    /* PSE failures Track(s) */

    if (numPSEFailures > 0)
    {
        /* if num PSE failures exceeds MAX_STRONG_REF_ARRAY_COUNT then the failures are
           written in > 1 tracks */
        numTracks = (numPSEFailures + MAX_STRONG_REF_ARRAY_COUNT - 1) / MAX_STRONG_REF_ARRAY_COUNT;
        for (i = 0; i < numTracks; i++)
        {
            /* Preface - ContentStorage - SourcePackage - DM Event Track */
            CHK_ORET(mxf_create_set(output->headerMetadata, &MXF_SET_K(EventTrack), &output->sourcePackageTrackSet));
            CHK_ORET(mxf_add_array_item_strongref(output->sourcePackageSet, &MXF_ITEM_K(GenericPackage, Tracks), output->sourcePackageTrackSet));
            CHK_ORET(mxf_set_utf16string_item(output->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackName), PSE_FAILURES_TRACK_NAME));
            CHK_ORET(mxf_set_uint32_item(output->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackID), nextTrackID++));
            CHK_ORET(mxf_set_uint32_item(output->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackNumber), 0));
            CHK_ORET(mxf_set_rational_item(output->sourcePackageTrackSet, &MXF_ITEM_K(EventTrack, EventEditRate), &output->frameRate));
            CHK_ORET(mxf_set_position_item(output->sourcePackageTrackSet, &MXF_ITEM_K(EventTrack, EventOrigin), 0));

            /* remove Track set from header metadata, leaving just the reference in the sourcePackageSet
               The Track will be written when the failures are written */
            CHK_ORET(mxf_remove_set(output->headerMetadata, output->sourcePackageTrackSet));
            CHK_ORET(mxf_append_list_element(&output->pseFailureTrackSets, output->sourcePackageTrackSet));
        }
    }

    /* VTR errors Track(s) */

    if (numVTRErrors > 0)
    {
        initialise_timecode_index_searcher(&output->vitcIndex, &vitcIndexSearcher);
        initialise_timecode_index_searcher(&output->ltcIndex, &ltcIndexSearcher);

        /* count the number of VTR errors that can be matched to a track position */
        for (j = 0; j < numVTRErrors; j++)
        {
            vtrError = (const VTRError*)&vtrErrors[j];

            if (vitcIndexIsNull && ltcIndexIsNull)
            {
                mxf_log_warn("SDI LTC and VITC indexes are both null\n");
                break;
            }
            else if (vitcIndexIsNull)
            {
                if (find_position(&ltcIndexSearcher, &vtrError->ltcTimecode, &errorPosition))
                {
                    numVTRErrorsLocated++;
                }
            }
            else if (ltcIndexIsNull)
            {
                if (find_position(&vitcIndexSearcher, &vtrError->vitcTimecode, &errorPosition))
                {
                    numVTRErrorsLocated++;
                }
            }
            else
            {
                if ((!setUseVTRLTC || useVTRLTC) &&
                    find_position(&ltcIndexSearcher, &vtrError->ltcTimecode, &errorPosition))
                {
                    numVTRErrorsLocated++;
                    if (!setUseVTRLTC)
                    {
                        useVTRLTC = 1;
                        setUseVTRLTC = 1;
                    }
                }
                else if ((!setUseVTRLTC || !useVTRLTC) &&
                         find_position(&vitcIndexSearcher, &vtrError->vitcTimecode, &errorPosition))
                {
                    numVTRErrorsLocated++;
                    if (!setUseVTRLTC)
                    {
                        useVTRLTC = 0;
                        setUseVTRLTC = 1;
                    }
                }
            }
        }

        if (numVTRErrorsLocated <= 0)
        {
            mxf_log_warn("Failed to find the position of any the %ld VTR errors" LOG_LOC_FORMAT, numVTRErrors, LOG_LOC_PARAMS);
        }
        else
        {
            /* if numVTRErrorsLocated exceeds MAX_STRONG_REF_ARRAY_COUNT then the errors are
               written in > 1 tracks */
            numTracks = (numVTRErrorsLocated + MAX_STRONG_REF_ARRAY_COUNT - 1) / MAX_STRONG_REF_ARRAY_COUNT;
            for (j = 0; j < numTracks; j++)
            {
                /* Preface - ContentStorage - SourcePackage - DM Event Track */
                CHK_ORET(mxf_create_set(output->headerMetadata, &MXF_SET_K(EventTrack), &output->sourcePackageTrackSet));
                CHK_ORET(mxf_add_array_item_strongref(output->sourcePackageSet, &MXF_ITEM_K(GenericPackage, Tracks), output->sourcePackageTrackSet));
                CHK_ORET(mxf_set_utf16string_item(output->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackName), VTR_ERRORS_TRACK_NAME));
                CHK_ORET(mxf_set_uint32_item(output->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackID), nextTrackID++));
                CHK_ORET(mxf_set_uint32_item(output->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackNumber), 0));
                CHK_ORET(mxf_set_rational_item(output->sourcePackageTrackSet, &MXF_ITEM_K(EventTrack, EventEditRate), &output->frameRate));
                CHK_ORET(mxf_set_position_item(output->sourcePackageTrackSet, &MXF_ITEM_K(EventTrack, EventOrigin), 0));

                /* remove Track set from header metadata, leaving just the reference in the sourcePackageSet
                   The Track will be written when the errors are written */
                CHK_ORET(mxf_remove_set(output->headerMetadata, output->sourcePackageTrackSet));
                CHK_ORET(mxf_append_list_element(&output->vtrErrorTrackSets, output->sourcePackageTrackSet));
            }

        }

    }

    /* DigiBeta dropout Track(s) */

    if (numDigiBetaDropouts > 0)
    {
        /* if num dropouts exceeds MAX_STRONG_REF_ARRAY_COUNT then the failures are
           written in > 1 tracks */
        numTracks = (numDigiBetaDropouts + MAX_STRONG_REF_ARRAY_COUNT - 1) / MAX_STRONG_REF_ARRAY_COUNT;
        for (i = 0; i < numTracks; i++)
        {
            /* Preface - ContentStorage - SourcePackage - DM Event Track */
            CHK_ORET(mxf_create_set(output->headerMetadata, &MXF_SET_K(EventTrack), &output->sourcePackageTrackSet));
            CHK_ORET(mxf_add_array_item_strongref(output->sourcePackageSet, &MXF_ITEM_K(GenericPackage, Tracks), output->sourcePackageTrackSet));
            CHK_ORET(mxf_set_utf16string_item(output->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackName), DIGIBETA_DROPOUT_TRACK_NAME));
            CHK_ORET(mxf_set_uint32_item(output->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackID), nextTrackID++));
            CHK_ORET(mxf_set_uint32_item(output->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackNumber), 0));
            CHK_ORET(mxf_set_rational_item(output->sourcePackageTrackSet, &MXF_ITEM_K(EventTrack, EventEditRate), &output->frameRate));
            CHK_ORET(mxf_set_position_item(output->sourcePackageTrackSet, &MXF_ITEM_K(EventTrack, EventOrigin), 0));

            /* remove Track set from header metadata, leaving just the reference in the sourcePackageSet
               The Track will be written when the dropouts are written */
            CHK_ORET(mxf_remove_set(output->headerMetadata, output->sourcePackageTrackSet));
            CHK_ORET(mxf_append_list_element(&output->digiBetaDropoutTrackSets, output->sourcePackageTrackSet));
        }
    }

    /* Timecode break Track(s) */

    if (numTimecodeBreaks > 0)
    {
        /* if num breaks exceeds MAX_STRONG_REF_ARRAY_COUNT then the failures are
           written in > 1 tracks */
        numTracks = (numTimecodeBreaks + MAX_STRONG_REF_ARRAY_COUNT - 1) / MAX_STRONG_REF_ARRAY_COUNT;
        for (i = 0; i < numTracks; i++)
        {
            /* Preface - ContentStorage - SourcePackage - DM Event Track */
            CHK_ORET(mxf_create_set(output->headerMetadata, &MXF_SET_K(EventTrack), &output->sourcePackageTrackSet));
            CHK_ORET(mxf_add_array_item_strongref(output->sourcePackageSet, &MXF_ITEM_K(GenericPackage, Tracks), output->sourcePackageTrackSet));
            CHK_ORET(mxf_set_utf16string_item(output->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackName), TIMECODE_BREAK_TRACK_NAME));
            CHK_ORET(mxf_set_uint32_item(output->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackID), nextTrackID++));
            CHK_ORET(mxf_set_uint32_item(output->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackNumber), 0));
            CHK_ORET(mxf_set_rational_item(output->sourcePackageTrackSet, &MXF_ITEM_K(EventTrack, EventEditRate), &output->frameRate));
            CHK_ORET(mxf_set_position_item(output->sourcePackageTrackSet, &MXF_ITEM_K(EventTrack, EventOrigin), 0));

            /* remove Track set from header metadata, leaving just the reference in the sourcePackageSet
               The Track will be written when the breaks are written */
            CHK_ORET(mxf_remove_set(output->headerMetadata, output->sourcePackageTrackSet));
            CHK_ORET(mxf_append_list_element(&output->timecodeBreakTrackSets, output->sourcePackageTrackSet));
        }
    }



    /* register the VTR error etc. items that will be written later
       This must be done so that the local tags are registered in the Primer */
    CHK_ORET(mxf_register_set_items(output->headerMetadata, &MXF_SET_K(DMSegment)));
    CHK_ORET(mxf_register_set_items(output->headerMetadata, &MXF_SET_K(APP_VTRReplayErrorFramework)));
    CHK_ORET(mxf_register_set_items(output->headerMetadata, &MXF_SET_K(APP_PSEAnalysisFramework)));
    CHK_ORET(mxf_register_set_items(output->headerMetadata, &MXF_SET_K(APP_DigiBetaDropoutFramework)));
    CHK_ORET(mxf_register_set_items(output->headerMetadata, &MXF_SET_K(APP_TimecodeBreakFramework)));


    /* write the footer header metadata created thus far */

    CHK_ORET(mxf_mark_header_start(output->mxfFile, output->footerPartition));
    CHK_ORET(mxf_write_header_metadata(output->mxfFile, output->headerMetadata));


    /* write the timecode breaks Track(s) and child items */

    if (mxf_get_list_length(&output->timecodeBreakTrackSets) > 0)
    {
        timecodeBreakIndex = 0;
        mxf_initialise_list_iter(&iter, &output->timecodeBreakTrackSets);
        while (mxf_next_list_iter_element(&iter))
        {
            output->sourcePackageTrackSet = (MXFMetadataSet*)mxf_get_iter_element(&iter);
            CHK_ORET(mxf_add_set(output->headerMetadata, output->sourcePackageTrackSet));

            /* Preface - ContentStorage - SourcePackage - DM Event Track - Sequence */
            CHK_ORET(mxf_create_set(output->headerMetadata, &MXF_SET_K(Sequence), &output->sequenceSet));
            CHK_ORET(mxf_set_strongref_item(output->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, Sequence), output->sequenceSet));
            CHK_ORET(mxf_set_ul_item(output->sequenceSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &MXF_DDEF_L(DescriptiveMetadata)));

            for (j = 0; j < MAX_STRONG_REF_ARRAY_COUNT && timecodeBreakIndex < numTimecodeBreaks; j++, timecodeBreakIndex++)
            {
                timecodeBreak = (const TimecodeBreak*)&timecodeBreaks[timecodeBreakIndex];

                /* Preface - ContentStorage - SourcePackage - DM Event Track - Sequence - DMSegment */
                CHK_ORET(mxf_create_set(output->headerMetadata, &MXF_SET_K(DMSegment), &output->dmSet));
                CHK_ORET(mxf_add_array_item_strongref(output->sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), output->dmSet));
                CHK_ORET(mxf_set_ul_item(output->dmSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &MXF_DDEF_L(DescriptiveMetadata)));
                CHK_ORET(mxf_set_position_item(output->dmSet, &MXF_ITEM_K(DMSegment, EventStartPosition), timecodeBreak->position));
                CHK_ORET(mxf_set_length_item(output->dmSet, &MXF_ITEM_K(StructuralComponent, Duration), 1));

                /* Preface - ContentStorage - SourcePackage - DM Event Track - Sequence - DMSegment - DMFramework */
                CHK_ORET(mxf_create_set(output->headerMetadata, &MXF_SET_K(APP_TimecodeBreakFramework), &output->dmFrameworkSet));
                CHK_ORET(mxf_set_strongref_item(output->dmSet, &MXF_ITEM_K(DMSegment, DMFramework), output->dmFrameworkSet));
                CHK_ORET(mxf_set_uint16_item(output->dmFrameworkSet, &MXF_ITEM_K(APP_TimecodeBreakFramework, APP_TimecodeType), timecodeBreak->timecodeType));


                /* write the DMSegment and DMFramework to file */
                CHK_ORET(mxf_write_set(output->mxfFile, output->dmSet));
                CHK_ORET(mxf_write_set(output->mxfFile, output->dmFrameworkSet));

                /* remove and free the sets to limit memory usage */
                CHK_ORET(mxf_remove_set(output->headerMetadata, output->dmSet));
                mxf_free_set(&output->dmSet);
                CHK_ORET(mxf_remove_set(output->headerMetadata, output->dmFrameworkSet));
                mxf_free_set(&output->dmFrameworkSet);
            }

            /* write the Track and Sequence to file */
            CHK_ORET(mxf_write_set(output->mxfFile, output->sourcePackageTrackSet));
            CHK_ORET(mxf_write_set(output->mxfFile, output->sequenceSet));

            /* remove and free the sets to limit memory usage */
            CHK_ORET(mxf_remove_set(output->headerMetadata, output->sourcePackageTrackSet));
            mxf_free_set(&output->sourcePackageTrackSet);
            CHK_ORET(mxf_remove_set(output->headerMetadata, output->sequenceSet));
            mxf_free_set(&output->sequenceSet);
        }
    }


    /* write the digibeta dropouts Track(s) and child items */

    if (mxf_get_list_length(&output->digiBetaDropoutTrackSets) > 0)
    {
        digiBetaDropoutIndex = 0;
        mxf_initialise_list_iter(&iter, &output->digiBetaDropoutTrackSets);
        while (mxf_next_list_iter_element(&iter))
        {
            output->sourcePackageTrackSet = (MXFMetadataSet*)mxf_get_iter_element(&iter);
            CHK_ORET(mxf_add_set(output->headerMetadata, output->sourcePackageTrackSet));

            /* Preface - ContentStorage - SourcePackage - DM Event Track - Sequence */
            CHK_ORET(mxf_create_set(output->headerMetadata, &MXF_SET_K(Sequence), &output->sequenceSet));
            CHK_ORET(mxf_set_strongref_item(output->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, Sequence), output->sequenceSet));
            CHK_ORET(mxf_set_ul_item(output->sequenceSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &MXF_DDEF_L(DescriptiveMetadata)));

            for (j = 0; j < MAX_STRONG_REF_ARRAY_COUNT && digiBetaDropoutIndex < numDigiBetaDropouts; j++, digiBetaDropoutIndex++)
            {
                digiBetaDropout = (const DigiBetaDropout*)&digiBetaDropouts[digiBetaDropoutIndex];

                /* Preface - ContentStorage - SourcePackage - DM Event Track - Sequence - DMSegment */
                CHK_ORET(mxf_create_set(output->headerMetadata, &MXF_SET_K(DMSegment), &output->dmSet));
                CHK_ORET(mxf_add_array_item_strongref(output->sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), output->dmSet));
                CHK_ORET(mxf_set_ul_item(output->dmSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &MXF_DDEF_L(DescriptiveMetadata)));
                CHK_ORET(mxf_set_position_item(output->dmSet, &MXF_ITEM_K(DMSegment, EventStartPosition), digiBetaDropout->position));
                CHK_ORET(mxf_set_length_item(output->dmSet, &MXF_ITEM_K(StructuralComponent, Duration), 1));

                /* Preface - ContentStorage - SourcePackage - DM Event Track - Sequence - DMSegment - DMFramework */
                CHK_ORET(mxf_create_set(output->headerMetadata, &MXF_SET_K(APP_DigiBetaDropoutFramework), &output->dmFrameworkSet));
                CHK_ORET(mxf_set_strongref_item(output->dmSet, &MXF_ITEM_K(DMSegment, DMFramework), output->dmFrameworkSet));
                CHK_ORET(mxf_set_int32_item(output->dmFrameworkSet, &MXF_ITEM_K(APP_DigiBetaDropoutFramework, APP_Strength), digiBetaDropout->strength));


                /* write the DMSegment and DMFramework to file */
                CHK_ORET(mxf_write_set(output->mxfFile, output->dmSet));
                CHK_ORET(mxf_write_set(output->mxfFile, output->dmFrameworkSet));

                /* remove and free the sets to limit memory usage */
                CHK_ORET(mxf_remove_set(output->headerMetadata, output->dmSet));
                mxf_free_set(&output->dmSet);
                CHK_ORET(mxf_remove_set(output->headerMetadata, output->dmFrameworkSet));
                mxf_free_set(&output->dmFrameworkSet);
            }

            /* write the Track and Sequence to file */
            CHK_ORET(mxf_write_set(output->mxfFile, output->sourcePackageTrackSet));
            CHK_ORET(mxf_write_set(output->mxfFile, output->sequenceSet));

            /* remove and free the sets to limit memory usage */
            CHK_ORET(mxf_remove_set(output->headerMetadata, output->sourcePackageTrackSet));
            mxf_free_set(&output->sourcePackageTrackSet);
            CHK_ORET(mxf_remove_set(output->headerMetadata, output->sequenceSet));
            mxf_free_set(&output->sequenceSet);
        }
    }

    /* write the PSE failures Track(s) and child items */

    if (mxf_get_list_length(&output->pseFailureTrackSets) > 0)
    {
        failureIndex = 0;
        mxf_initialise_list_iter(&iter, &output->pseFailureTrackSets);
        while (mxf_next_list_iter_element(&iter))
        {
            output->sourcePackageTrackSet = (MXFMetadataSet*)mxf_get_iter_element(&iter);
            CHK_ORET(mxf_add_set(output->headerMetadata, output->sourcePackageTrackSet));

            /* Preface - ContentStorage - SourcePackage - DM Event Track - Sequence */
            CHK_ORET(mxf_create_set(output->headerMetadata, &MXF_SET_K(Sequence), &output->sequenceSet));
            CHK_ORET(mxf_set_strongref_item(output->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, Sequence), output->sequenceSet));
            CHK_ORET(mxf_set_ul_item(output->sequenceSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &MXF_DDEF_L(DescriptiveMetadata)));

            for (j = 0; j < MAX_STRONG_REF_ARRAY_COUNT && failureIndex < numPSEFailures; j++, failureIndex++)
            {
                pseFailure = (const PSEFailure*)&pseFailures[failureIndex];

                /* Preface - ContentStorage - SourcePackage - DM Event Track - Sequence - DMSegment */
                CHK_ORET(mxf_create_set(output->headerMetadata, &MXF_SET_K(DMSegment), &output->dmSet));
                CHK_ORET(mxf_add_array_item_strongref(output->sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), output->dmSet));
                CHK_ORET(mxf_set_ul_item(output->dmSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &MXF_DDEF_L(DescriptiveMetadata)));
                CHK_ORET(mxf_set_position_item(output->dmSet, &MXF_ITEM_K(DMSegment, EventStartPosition), pseFailure->position));
                CHK_ORET(mxf_set_length_item(output->dmSet, &MXF_ITEM_K(StructuralComponent, Duration), 1));

                /* Preface - ContentStorage - SourcePackage - DM Event Track - Sequence - DMSegment - DMFramework */
                CHK_ORET(mxf_create_set(output->headerMetadata, &MXF_SET_K(APP_PSEAnalysisFramework), &output->dmFrameworkSet));
                CHK_ORET(mxf_set_strongref_item(output->dmSet, &MXF_ITEM_K(DMSegment, DMFramework), output->dmFrameworkSet));
                CHK_ORET(mxf_set_int16_item(output->dmFrameworkSet, &MXF_ITEM_K(APP_PSEAnalysisFramework, APP_RedFlash), pseFailure->redFlash));
                CHK_ORET(mxf_set_int16_item(output->dmFrameworkSet, &MXF_ITEM_K(APP_PSEAnalysisFramework, APP_SpatialPattern), pseFailure->spatialPattern));
                CHK_ORET(mxf_set_int16_item(output->dmFrameworkSet, &MXF_ITEM_K(APP_PSEAnalysisFramework, APP_LuminanceFlash), pseFailure->luminanceFlash));
                CHK_ORET(mxf_set_boolean_item(output->dmFrameworkSet, &MXF_ITEM_K(APP_PSEAnalysisFramework, APP_ExtendedFailure), pseFailure->extendedFailure));


                /* write the DMSegment and DMFramework to file */
                CHK_ORET(mxf_write_set(output->mxfFile, output->dmSet));
                CHK_ORET(mxf_write_set(output->mxfFile, output->dmFrameworkSet));

                /* remove and free the sets to limit memory usage */
                CHK_ORET(mxf_remove_set(output->headerMetadata, output->dmSet));
                mxf_free_set(&output->dmSet);
                CHK_ORET(mxf_remove_set(output->headerMetadata, output->dmFrameworkSet));
                mxf_free_set(&output->dmFrameworkSet);
            }

            /* write the Track and Sequence to file */
            CHK_ORET(mxf_write_set(output->mxfFile, output->sourcePackageTrackSet));
            CHK_ORET(mxf_write_set(output->mxfFile, output->sequenceSet));

            /* remove and free the sets to limit memory usage */
            CHK_ORET(mxf_remove_set(output->headerMetadata, output->sourcePackageTrackSet));
            mxf_free_set(&output->sourcePackageTrackSet);
            CHK_ORET(mxf_remove_set(output->headerMetadata, output->sequenceSet));
            mxf_free_set(&output->sequenceSet);
        }
    }



    /* write the VTR errors Track(s) and child items */

    if (mxf_get_list_length(&output->vtrErrorTrackSets) > 0)
    {
        initialise_timecode_index_searcher(&output->vitcIndex, &vitcIndexSearcher);
        initialise_timecode_index_searcher(&output->ltcIndex, &ltcIndexSearcher);

        errorIndex = 0;
        mxf_initialise_list_iter(&iter, &output->vtrErrorTrackSets);
        while (mxf_next_list_iter_element(&iter))
        {
            output->sourcePackageTrackSet = (MXFMetadataSet*)mxf_get_iter_element(&iter);
            CHK_ORET(mxf_add_set(output->headerMetadata, output->sourcePackageTrackSet));

            /* Preface - ContentStorage - SourcePackage - DM Event Track - Sequence */
            CHK_ORET(mxf_create_set(output->headerMetadata, &MXF_SET_K(Sequence), &output->sequenceSet));
            CHK_ORET(mxf_set_strongref_item(output->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, Sequence), output->sequenceSet));
            CHK_ORET(mxf_set_ul_item(output->sequenceSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &MXF_DDEF_L(DescriptiveMetadata)));

            for (j = 0; j < MAX_STRONG_REF_ARRAY_COUNT && errorIndex < numVTRErrors; j++, errorIndex++)
            {
                vtrError = (const VTRError*)&vtrErrors[errorIndex];

                /* find the position associated with the VITC and LTC timecode */
                if (vitcIndexIsNull && ltcIndexIsNull)
                {
                    /* shouldn't even be here if both indexes are null */
                    /* TODO: add assert(false) here */
                    break;
                }
                else if (vitcIndexIsNull)
                {
                    if (!find_position(&ltcIndexSearcher, &vtrError->ltcTimecode, &errorPosition))
                    {
                        mxf_log_warn("Failed to find the (LTC) position of the VTR error %ld" LOG_LOC_FORMAT, errorIndex, LOG_LOC_PARAMS);
                        continue;
                    }
                }
                else if (ltcIndexIsNull)
                {
                    if (!find_position(&vitcIndexSearcher, &vtrError->vitcTimecode, &errorPosition))
                    {
                        mxf_log_warn("Failed to find the (VITC) position of the VTR error %ld" LOG_LOC_FORMAT, errorIndex, LOG_LOC_PARAMS);
                        continue;
                    }
                }
                else
                {
                    if (useVTRLTC)
                    {
                        if (!find_position(&ltcIndexSearcher, &vtrError->ltcTimecode, &errorPosition))
                        {
                            mxf_log_warn("Failed to find the (LTC) position of the VTR error %ld" LOG_LOC_FORMAT, errorIndex, LOG_LOC_PARAMS);
                            continue;
                        }
                    }
                    else
                    {
                        if (!find_position(&vitcIndexSearcher, &vtrError->vitcTimecode, &errorPosition))
                        {
                            mxf_log_warn("Failed to find the (VITC) position of the VTR error %ld" LOG_LOC_FORMAT, errorIndex, LOG_LOC_PARAMS);
                            continue;
                        }
                    }
                }

                /* Preface - ContentStorage - SourcePackage - DM Event Track - Sequence - DMSegment */
                CHK_ORET(mxf_create_set(output->headerMetadata, &MXF_SET_K(DMSegment), &output->dmSet));
                CHK_ORET(mxf_add_array_item_strongref(output->sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), output->dmSet));
                CHK_ORET(mxf_set_ul_item(output->dmSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &MXF_DDEF_L(DescriptiveMetadata)));
                CHK_ORET(mxf_set_position_item(output->dmSet, &MXF_ITEM_K(DMSegment, EventStartPosition), errorPosition));
                CHK_ORET(mxf_set_length_item(output->dmSet, &MXF_ITEM_K(StructuralComponent, Duration), 1));

                /* Preface - ContentStorage - SourcePackage - DM Event Track - Sequence - DMSegment - DMFramework */
                CHK_ORET(mxf_create_set(output->headerMetadata, &MXF_SET_K(APP_VTRReplayErrorFramework), &output->dmFrameworkSet));
                CHK_ORET(mxf_set_strongref_item(output->dmSet, &MXF_ITEM_K(DMSegment, DMFramework), output->dmFrameworkSet));
                CHK_ORET(mxf_set_uint8_item(output->dmFrameworkSet, &MXF_ITEM_K(APP_VTRReplayErrorFramework, APP_VTRErrorCode), vtrError->errorCode));


                /* write the DMSegment and DMFramework to file */
                CHK_ORET(mxf_write_set(output->mxfFile, output->dmSet));
                CHK_ORET(mxf_write_set(output->mxfFile, output->dmFrameworkSet));

                /* remove and free the sets to limit memory usage */
                CHK_ORET(mxf_remove_set(output->headerMetadata, output->dmSet));
                mxf_free_set(&output->dmSet);
                CHK_ORET(mxf_remove_set(output->headerMetadata, output->dmFrameworkSet));
                mxf_free_set(&output->dmFrameworkSet);
            }

            /* write the Track and Sequence to file */
            CHK_ORET(mxf_write_set(output->mxfFile, output->sourcePackageTrackSet));
            CHK_ORET(mxf_write_set(output->mxfFile, output->sequenceSet));

            /* remove and free the sets to limit memory usage */
            CHK_ORET(mxf_remove_set(output->headerMetadata, output->sourcePackageTrackSet));
            mxf_free_set(&output->sourcePackageTrackSet);
            CHK_ORET(mxf_remove_set(output->headerMetadata, output->sequenceSet));
            mxf_free_set(&output->sequenceSet);
        }
    }


    /* indicate end of header metadata writing */
    CHK_ORET(mxf_mark_header_end(output->mxfFile, output->footerPartition));



    /* Write the Footer Index Table Segment */

    CHK_ORET(mxf_mark_index_start(output->mxfFile, output->footerPartition));
    mxf_generate_uuid(&output->indexSegment->instanceUID);
    CHK_ORET(mxf_write_index_table_segment(output->mxfFile, output->indexSegment));
    CHK_ORET(mxf_mark_index_end(output->mxfFile, output->footerPartition));


    /* write the random index pack */

    CHK_ORET(mxf_write_rip(output->mxfFile, output->partitions));



    /* Update the Partition Packs and re-write them */

    /* header partition is open because the LTO Infax data needs to be filled in
       and it doesn't include the PSE failures and VTR errors */
    output->headerPartition->key = MXF_PP_K(OpenComplete, Header);
    CHK_ORET(mxf_update_partitions(output->mxfFile, output->partitions));



    free_archive_mxf_file(outputRef);
    return 1;
}


static int update_header_metadata(MXFFile *mxfFile, uint64_t headerByteCount, const InfaxData *infaxData,
                                  const char *newFilename)
{
    mxfKey key;
    uint8_t llen;
    uint64_t len;
    MXFDataModel *dataModel = NULL;
    MXFHeaderMetadata *headerMetadata = NULL;
    uint64_t count;
    MXFMetadataSet *frameworkSet;
    mxfUTF16Char formatString[FORMAT_SIZE];
    int ltoInfaxSetFoundAndUpdated;
    int networkLocatorSetFoundAndUpdated;
    mxfUTF16Char *tempString = NULL;
    MXFMetadataSet *networkLocatorSet;
    int ltoInfaxSetFound;


    /* load the data model */

    CHK_OFAIL(mxf_load_data_model(&dataModel));
    CHK_OFAIL(mxf_app_load_extensions(dataModel));
    CHK_OFAIL(mxf_finalise_data_model(dataModel));


    /* find the LTO Infax metadata set and update and rewrite it */

    CHK_OFAIL(mxf_read_next_nonfiller_kl(mxfFile, &key, &llen, &len));
    CHK_OFAIL(mxf_is_header_metadata(&key));

    CHK_OFAIL(mxf_create_header_metadata(&headerMetadata, dataModel));
    mxf_free_primer_pack(&headerMetadata->primerPack);
    CHK_OFAIL(mxf_read_primer_pack(mxfFile, &headerMetadata->primerPack));

    count = mxfKey_extlen + llen + len;

    ltoInfaxSetFoundAndUpdated = 0;
    networkLocatorSetFoundAndUpdated = 0;
    while (count < headerByteCount &&
            (!ltoInfaxSetFoundAndUpdated || !networkLocatorSetFoundAndUpdated))
    {
        CHK_OFAIL(mxf_read_kl(mxfFile, &key, &llen, &len));
        count += mxfKey_extlen + llen;

        if (mxf_is_filler(&key))
        {
            CHK_OFAIL(mxf_skip(mxfFile, len));
        }
        else
        {
            /* the LTO Infax set is identifiable by a missing format string, or
               if the file has already been updated for some reason, then the
               format string == LTO_FORMAT_STRING_W */
            if (mxf_equals_key(&key, &MXF_SET_K(APP_InfaxFramework)))
            {
                ltoInfaxSetFound = 0;
                CHK_OFAIL(mxf_read_and_return_set(mxfFile, &key, len, headerMetadata, 1, &frameworkSet) == 1);
                if (mxf_have_item(frameworkSet, &MXF_ITEM_K(APP_InfaxFramework, APP_Format)))
                {
                    CHK_OFAIL(mxf_get_utf16string_item(frameworkSet, &MXF_ITEM_K(APP_InfaxFramework, APP_Format), formatString));
                    if (wcslen(formatString) == 0 || wcscmp(formatString, LTO_FORMAT_STRING_W) == 0)
                    {
                        ltoInfaxSetFound = 1;
                    }
                }
                else
                {
                    ltoInfaxSetFound = 1;
                }

                if (ltoInfaxSetFound)
                {
                    /* update set and allocate the same fixed space */
                    mxf_set_fixed_set_space_allocation(frameworkSet, FIXED_INFAX_SET_ALLOCATION_SIZE);
                    CHK_OFAIL(set_infax_data(frameworkSet, infaxData));

                    /* re-write set */
                    CHK_OFAIL(mxf_file_seek(mxfFile, - mxfKey_extlen - llen - len, SEEK_CUR));
                    CHK_OFAIL(mxf_write_set(mxfFile, frameworkSet));
                    ltoInfaxSetFoundAndUpdated = 1;
                }
            }

            /* update the NetworkLocator URL which holds the filename of 'this' file */
            else if (mxf_equals_key(&key, &MXF_SET_K(NetworkLocator)))
            {
                /* we are assuming there is only 1 NetworkLocator object in the header metadata */
                CHK_OFAIL(!networkLocatorSetFoundAndUpdated);

                /* update set (and debug check the length hasn't changed) */
                CHK_OFAIL(mxf_read_and_return_set(mxfFile, &key, len, headerMetadata, 1, &networkLocatorSet) == 1);
                CHK_OFAIL(convert_string(newFilename, &tempString));
                CHK_OFAIL(mxf_set_fixed_size_utf16string_item(networkLocatorSet, &MXF_ITEM_K(NetworkLocator, URLString),
                                                              tempString, NETWORK_LOCATOR_URL_SIZE));
                CHK_OFAIL(llen + len + mxfKey_extlen == mxf_get_set_size(mxfFile, networkLocatorSet));

                /* re-write set */
                CHK_OFAIL(mxf_file_seek(mxfFile, - mxfKey_extlen - llen - len, SEEK_CUR));
                CHK_OFAIL(mxf_write_set(mxfFile, networkLocatorSet));
                networkLocatorSetFoundAndUpdated = 1;
            }

            else
            {
                CHK_OFAIL(mxf_skip(mxfFile, len));
            }
        }
        count += len;
    }
    CHK_OFAIL(ltoInfaxSetFoundAndUpdated && networkLocatorSetFoundAndUpdated);


    SAFE_FREE(tempString);
    mxf_free_header_metadata(&headerMetadata);
    mxf_free_data_model(&dataModel);
    return 1;

fail:
    SAFE_FREE(tempString);
    mxf_free_header_metadata(&headerMetadata);
    mxf_free_data_model(&dataModel);
    return 0;
}

int update_archive_mxf_file(const char *filePath, const char *newFilename, const InfaxData *ltoInfaxData)
{
    MXFFile *mxfFile = NULL;
    int result;

    CHK_ORET(filePath != NULL);

    CHK_ORET(mxf_disk_file_open_modify(filePath, &mxfFile));

    result = update_archive_mxf_file_2(&mxfFile, newFilename, ltoInfaxData);
    if (!result)
    {
        if (mxfFile != NULL)
        {
            mxf_file_close(&mxfFile);
        }
    }

    return result;
}

int update_archive_mxf_file_2(MXFFile **mxfFileIn, const char *newFilename, const InfaxData *ltoInfaxData)
{
    mxfKey key;
    uint8_t llen;
    uint64_t len;
    MXFPartition *headerPartition = NULL;
    MXFPartition *footerPartition = NULL;
    MXFFile *mxfFile = NULL;

    CHK_ORET(*mxfFileIn != NULL && newFilename != NULL);
    CHK_ORET(strcmp(ltoInfaxData->format, LTO_FORMAT_STRING) == 0);

    /* take ownership */
    mxfFile = *mxfFileIn;
    *mxfFileIn = NULL;

    /* open mxf file */
    mxf_file_set_min_llen(mxfFile, MIN_LLEN);


    /* update the header partition header metadata LTO Infax data set */
    /* Note: the header partition remains open and complete because it doesn't
       contain the PSE failure and VTR error data */

    /* read the header partition pack */
    if (!mxf_read_header_pp_kl(mxfFile, &key, &llen, &len))
    {
        mxf_log_error("Could not find header partition pack key" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
        return 0;
    }
    CHK_OFAIL(mxf_read_partition(mxfFile, &key, len, &headerPartition));

    CHK_OFAIL(update_header_metadata(mxfFile, headerPartition->headerByteCount, ltoInfaxData, newFilename));



    /* update the footer partition header metadata LTO Infax data set */

    CHK_OFAIL(mxf_file_seek(mxfFile, headerPartition->footerPartition, SEEK_SET));

    /* read the footer partition pack */
    if (!mxf_read_kl(mxfFile, &key, &llen, &len))
    {
        mxf_log_error("Could not find footer partition pack key" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
        return 0;
    }
    CHK_OFAIL(mxf_read_partition(mxfFile, &key, len, &footerPartition));

    CHK_OFAIL(update_header_metadata(mxfFile, footerPartition->headerByteCount, ltoInfaxData, newFilename));


    /* re-write the footer partition pack with closed and complete status */

    CHK_OFAIL(mxf_file_seek(mxfFile, headerPartition->footerPartition, SEEK_SET));
    footerPartition->key = MXF_PP_K(ClosedComplete, Footer);
    CHK_OFAIL(mxf_write_partition(mxfFile, footerPartition));



    mxf_file_close(&mxfFile);
    mxf_free_partition(&headerPartition);
    mxf_free_partition(&footerPartition);
    return 1;

fail:
    mxf_file_close(&mxfFile);
    mxf_free_partition(&headerPartition);
    mxf_free_partition(&footerPartition);
    return 0;
}

int64_t get_archive_mxf_file_size(ArchiveMXFWriter *writer)
{
    return mxf_file_size(writer->mxfFile);
}

mxfUMID get_material_package_uid(ArchiveMXFWriter *writer)
{
    return writer->materialPackageUID;
}

mxfUMID get_file_package_uid(ArchiveMXFWriter *writer)
{
    return writer->fileSourcePackageUID;
}

mxfUMID get_tape_package_uid(ArchiveMXFWriter *writer)
{
    return writer->tapeSourcePackageUID;
}

uint32_t get_video_frame_size(const mxfRational *frameRate, uint8_t signalStandard, uint32_t componentDepth)
{
    uint32_t width = 0, height = 0;
    switch (signalStandard)
    {
        case MXF_SIGNAL_STANDARD_ITU601:
            width = 720;
            if (frameRate->numerator == 25)
            {
                height = 576;
            }
            else
            {
                height = 486;
            }
            break;
        case MXF_SIGNAL_STANDARD_SMPTE274M:
            width = 1920;
            height = 1080;
            break;
        case MXF_SIGNAL_STANDARD_SMPTE296M:
            width = 1280;
            height = 720;
            break;
        default:
            assert(0);
            break;
    }

    if (componentDepth == 8)
        return width * height * 2;
    else
        return (width + 47) / 48 * 128 * height;
}

uint32_t get_audio_frame_size(uint32_t numSamples, uint32_t audioQuantBits)
{
    return numSamples * ((audioQuantBits + 7) / 8);
}

int get_audio_sample_sequence(const mxfRational *frameRate, uint8_t maxSequenceSize, uint32_t *sequence,
                              uint8_t *sequenceSize)
{
    size_t i;
    for (i = 0; i < ARRAY_SIZE(AUDIO_SEQUENCES); i++)
    {
        if (memcmp(frameRate, &AUDIO_SEQUENCES[i].frameRate, sizeof(*frameRate)) == 0)
        {
            uint8_t j = 0;
            while (AUDIO_SEQUENCES[i].sequence[j] && j < maxSequenceSize)
            {
                sequence[j] = AUDIO_SEQUENCES[i].sequence[j];
                j++;
            }
            if (AUDIO_SEQUENCES[i].sequence[j])
                return 0;

            *sequenceSize = j;
            return 1;
        }
    }

    return 0;
}

uint32_t get_archive_mxf_content_package_size(const mxfRational *frameRate, uint8_t signalStandard,
                                              uint32_t componentDepth, int numAudioTracks, uint32_t audioQuantBits,
                                              int includeCRC32)
{
    uint32_t videoFrameSize = get_video_frame_size(frameRate, signalStandard, componentDepth);
    uint32_t audioElementSize = get_audio_element_size(frameRate, audioQuantBits);
    uint32_t systemItemSize = 28;
    if (includeCRC32)
    {
        systemItemSize += 12 + (1 + numAudioTracks) * 4;
    }

    return mxfKey_extlen + ESS_ELEMENT_LLEN + systemItemSize +
           mxfKey_extlen + ESS_ELEMENT_LLEN + videoFrameSize +
           numAudioTracks * audioElementSize;
}

#define PARSE_STRING(member, size, beStrict) \
{ \
    int cpySize = (int)(endField - startField); \
    if (cpySize < 0) \
    { \
        mxf_log_error("invalid infax string field" LOG_LOC_FORMAT, LOG_LOC_PARAMS); \
        return 0; \
    } \
    else if (cpySize >= size) \
    { \
        if (beStrict) \
        { \
            mxf_log_error("Infax string size (%d) exceeds limit (%d)" \
                          LOG_LOC_FORMAT, endField - startField, size, LOG_LOC_PARAMS); \
            return 0; \
        } \
        else \
        { \
            mxf_log_warn("Infax string size (%d) exceeds limit (%d) - string will be truncated" \
                         LOG_LOC_FORMAT, endField - startField, size, LOG_LOC_PARAMS); \
            cpySize = size - 1; \
        } \
    } \
    \
    if (cpySize > 0) \
    { \
        strncpy(member, startField, cpySize); \
    } \
    member[cpySize] = '\0'; \
}


#define PARSE_DATE(member) \
{ \
    if (endField - startField > 0) \
    { \
        int year, month, day; \
        CHK_ORET(sscanf(startField, "%d-%u-%u", &year, &month, &day) == 3); \
        member.year  = (int16_t)year; \
        member.month = (uint8_t)month; \
        member.day   = (uint8_t)day; \
    } \
    else \
    { \
        member.month = INVALID_MONTH_VALUE; \
    } \
}
#define PARSE_INT64(member, invalid) \
{ \
    if (endField - startField > 0) \
    { \
        CHK_ORET(sscanf(startField, "%" PRId64 "", &member) == 1); \
    } \
    else \
    { \
        member = invalid; \
    } \
}
#define PARSE_UINT32(member) \
{ \
    CHK_ORET(endField - startField > 0); \
    CHK_ORET(sscanf(startField, "%u", &member) == 1); \
}

int parse_infax_data(const char *infaxDataString, InfaxData *infaxData)
{
    const char *startField = infaxDataString;
    const char *endField = NULL;
    int fieldIndex = 0;
    int done = 0;

    CHK_ORET(infaxDataString != NULL);

    set_null_infax_data(infaxData);

    do
    {
        endField = strchr(startField, INFAX_DATA_STRING_SEPARATOR);
        if (endField == NULL)
        {
            done = 1;
            endField = startField + strlen(startField);
        }

        switch (fieldIndex)
        {
            case 0:
                PARSE_STRING(infaxData->format, FORMAT_SIZE, 1)
                break;
            case 1:
                PARSE_STRING(infaxData->progTitle, PROGTITLE_SIZE, 0)
                break;
            case 2:
                PARSE_STRING(infaxData->epTitle, EPTITLE_SIZE, 0)
                break;
            case 3:
                PARSE_DATE(infaxData->txDate)
                break;
            case 4:
                PARSE_STRING(infaxData->magPrefix, MAGPREFIX_SIZE, 1)
                break;
            case 5:
                PARSE_STRING(infaxData->progNo, PROGNO_SIZE, 1)
                break;
            case 6:
                PARSE_STRING(infaxData->prodCode, PRODCODE_SIZE, 1)
                break;
            case 7:
                PARSE_STRING(infaxData->spoolStatus, SPOOLSTATUS_SIZE, 1)
                break;
            case 8:
                PARSE_DATE(infaxData->stockDate)
                break;
            case 9:
                PARSE_STRING(infaxData->spoolDesc, SPOOLDESC_SIZE, 0)
                break;
            case 10:
                PARSE_STRING(infaxData->memo, MEMO_SIZE, 0)
                break;
            case 11:
                PARSE_INT64(infaxData->duration, INVALID_DURATION_VALUE)
                break;
            case 12:
                PARSE_STRING(infaxData->spoolNo, SPOOLNO_SIZE, 1)
                break;
            case 13:
                PARSE_STRING(infaxData->accNo, ACCNO_SIZE, 1)
                break;
            case 14:
                PARSE_STRING(infaxData->catDetail, CATDETAIL_SIZE, 1)
                break;
            case 15:
                PARSE_UINT32(infaxData->itemNo)
                break;
            default:
                mxf_log_error("Invalid Infax data string ('%s')" LOG_LOC_FORMAT, infaxDataString, LOG_LOC_PARAMS);
                return 0;
        }

        if (!done)
        {
            startField = endField + 1;
            fieldIndex++;
        }
    }
    while (!done);

    CHK_ORET(fieldIndex == 15);

    return 1;
}


/* The CRC32_TABLE was generated using the following code taken from http://www.w3.org/TR/PNG-CRCAppendix.html:
   // Table of CRCs of all 8-bit messages.
   unsigned long crc_table[256];

   // Flag: has the table been computed? Initially false.
   int crc_table_computed = 0;

   // Make the table for a fast CRC.
   void make_crc_table(void)
   {
     unsigned long c;
     int n, k;

     for (n = 0; n < 256; n++) {
       c = (unsigned long) n;
       for (k = 0; k < 8; k++) {
         if (c & 1)
           c = 0xedb88320L ^ (c >> 1);
         else
           c = c >> 1;
       }
       crc_table[n] = c;
     }
     crc_table_computed = 1;
   }
*/

static const uint32_t g_crc32Table[256] =
{
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
    0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
    0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
    0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
    0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
    0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
    0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
    0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
    0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
    0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
    0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
    0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
    0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
    0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
    0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
    0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
    0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
    0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
    0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
    0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
    0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
    0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

uint32_t calc_crc32(const uint8_t *data, uint32_t size)
{
    uint32_t i;
    uint32_t crc32 = 0;

    crc32 ^= 0xffffffff;

    for (i = 0; i < size; i++)
        crc32 = g_crc32Table[(crc32 ^ data[i]) & 0xff] ^ (crc32 >> 8);

    return crc32 ^ 0xffffffff;
}

