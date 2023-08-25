/*
 * Transfers Avid MXF files to P2
 *
 * Copyright (C) 2006, British Broadcasting Corporation
 * All Rights Reserved.
 *
 * Author: Philip de Nier, Stuart Cunningham
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
#include <assert.h>

#include "xml_writer.h"
#include "avid_mxf_to_p2.h"
#include "icon_avid_to_p2.h"		// provides icon_avid_to_p2[]
#include <mxf/mxf_macros.h>


#define CALL_PROGRESS() \
    if (transfer->progress != NULL) \
    { \
        if (transfer->percentCompleted - transfer->lastCallPercentCompleted >= 0.1f || \
            transfer->percentCompleted == 0.0f || transfer->percentCompleted == 100.0f) \
        { \
            transfer->progress(transfer->percentCompleted); \
            transfer->lastCallPercentCompleted = transfer->percentCompleted; \
        } \
    }


typedef struct
{
    int hour;
    int min;
    int sec;
    int frame;
} Timecode;


static const mxfUUID g_mxfIdentProductUID =
    {0xae, 0x36, 0x89, 0x2c, 0x8e, 0xaf, 0x4c, 0xe4, 0x92, 0x04, 0x0a, 0xf6, 0x7f, 0xa4, 0xfa, 0xd0};
static const mxfUTF16Char *g_mxfIdentCompanyName = L"BBC Research";
static const mxfUTF16Char *g_mxfIdentProductName = L"Ingex Avid to P2 Exporter";
static const mxfUTF16Char *g_mxfIdentVersionString = L"Alpha version";


static const uint32_t g_p2_timecodeTrackID = 0;
static const uint32_t g_p2_timecodeTrackNumber = 1;

static const uint32_t g_p2_bodySID = 2;
static const uint32_t g_p2_indexSID = 1;

static const uint64_t g_p2_fixedStartByteOffset = 32768;
/* 32768 = 32620 + size(BodyPP) + size(essence element KL) = 32620 + 124 + 24 */
static const uint64_t g_p2_fixedBodyPPOffset = 32620;



/* buffer size must be >= max frame size */
#define ESSENCE_BUFFER_SIZE     288000


/* invalid because the registry version, byte8, is 0x01 rather than 0x02, and
   byte14 should be 0x02 when multiple material package tracks are present */
static const mxfUL g_invalidAAFSDKOPAtomLabel =
    {0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01, 0x0d, 0x01, 0x02, 0x01, 0x10, 0x00, 0x00, 0x00};

/* invalid because it isn't a valid generic container label and it is has the
   same value for all essence types. */
/* corresponds to kAAFContainerDef_AAF */
static const mxfUL g_invalidAAFSDKEssenceContainerLabel_1 =
    {0x80, 0x9b, 0x00, 0x60, 0x08, 0x14, 0x3e, 0x6f, 0x43, 0x13, 0xb5, 0x71, 0xd8, 0xba, 0x11, 0xd2};
/* corresponds to kAAFContainerDef_AAFKLV */
static const mxfUL g_invalidAAFSDKEssenceContainerLabel_2 =
    {0x06, 0x0e, 0x2b, 0x34, 0x01, 0x01, 0x01, 0xff, 0x4b, 0x46, 0x41, 0x41, 0x00, 0x0d, 0x4d, 0x4f};


#if defined(_WIN32)
static const char *g_fileSeparator = "\\";
#else
static const char *g_fileSeparator = "/";
#endif
static const char *g_lastClipFilename = "LASTCLIP.TXT";
static const char *g_contentsDirname = "CONTENTS";
static const char *g_videoDirname = "VIDEO";
static const char *g_audioDirname = "AUDIO";
static const char *g_clipDirname = "CLIP";
static const char *g_iconDirname = "ICON";



static int is_invalid_aafsdk_op_atom_label(const mxfUL *label)
{
    return mxf_equals_ul(label, &g_invalidAAFSDKOPAtomLabel);
}

static int is_invalid_aafsdk_esscont_label(const mxfUL *label)
{
    return mxf_equals_ul(label, &g_invalidAAFSDKEssenceContainerLabel_1) ||
        mxf_equals_ul(label, &g_invalidAAFSDKEssenceContainerLabel_2);
}


static int initialise_input_file(AvidMXFFile *input)
{
    memset(input, 0, sizeof(AvidMXFFile));

    return 1;
}

static int open_input_file(const char *filename, AvidMXFFile *input)
{
    if (filename != NULL)
    {
        SAFE_FREE(input->filename);
        CHK_ORET((input->filename = strdup(filename)) != NULL);
    }

    if (!mxf_disk_file_open_read(input->filename, &input->mxfFile))
    {
        mxf_log_error("Failed to open '%s'" LOG_LOC_FORMAT, input->filename, LOG_LOC_PARAMS);
        return 0;
    }

    return 1;
}

static void close_input_file(AvidMXFFile *input)
{
    if (input->mxfFile != NULL)
    {
        mxf_file_close(&input->mxfFile);
    }
}

static void clear_input_file(AvidMXFFile *input)
{
    if (input == NULL)
    {
        return;
    }

    close_input_file(input);
    SAFE_FREE(input->filename);

    mxf_close_essence_element(&input->essenceElement);
    mxf_free_header_metadata(&input->headerMetadata);
    mxf_free_data_model(&input->dataModel);
    mxf_free_partition(&input->headerPartition);

    mxf_free_list(&input->aList);
}

static int initialise_output_file(P2MXFFile *output)
{
    memset(output, 0, sizeof(P2MXFFile));
    CHK_ORET(mxf_create_file_partitions(&output->partitions));

    return 1;
}


static int open_output_file(const char *filename, P2MXFFile *output)
{
    if (filename != NULL)
    {
        SAFE_FREE(output->filename);
        CHK_ORET((output->filename = strdup(filename)) != NULL);
    }

    if (!mxf_disk_file_open_new(output->filename, &output->mxfFile))
    {
        mxf_log_error("Failed to create '%s'" LOG_LOC_FORMAT, output->filename, LOG_LOC_PARAMS);
        return 0;
    }

    return 1;

}

static void close_output_file(P2MXFFile *output)
{
    if (output->mxfFile != NULL)
    {
        mxf_file_close(&output->mxfFile);
    }
}

static void clear_output_file(P2MXFFile *output)
{
    if (output == NULL)
    {
        return;
    }

    close_output_file(output);
    SAFE_FREE(output->filename);

    mxf_close_essence_element(&output->essenceElement);
    mxf_free_header_metadata(&output->headerMetadata);
    mxf_free_index_table_segment(&output->indexSegment);
    mxf_free_file_partitions(&output->partitions);
    mxf_free_data_model(&output->dataModel);
}


static void convert_frames_to_timecode(int64_t frameCount, int fps, int dropFrameFlag, Timecode *timecode)
{
    int64_t workFrameCount = frameCount;
    int64_t numFramesSkipped;

    assert(fps == 25 || fps == 30);
    assert(dropFrameFlag == 0 || fps == 30);

    timecode->hour = (int)(workFrameCount / (60 * 60 * fps));
    workFrameCount %= 60 * 60 * fps;
    timecode->min = (int)(workFrameCount / (60 * fps));
    workFrameCount %= 60 * fps;
    timecode->sec = (int)(workFrameCount / fps);
    timecode->frame = (int)(workFrameCount % fps);

    if (dropFrameFlag)
    {
        /* first 2 frame numbers shall be omitted at the start of each minute,
           except minutes 0, 10, 20, 30, 40 and 50 */

        /* calculate number frames skipped */
        numFramesSkipped = (60-6) * 2 * timecode->hour; /* every whole hour */
        numFramesSkipped += (timecode->min / 10) * 9 * 2; /* every whole 10 min */
        numFramesSkipped += (timecode->min % 10) * 2; /* every whole min, except min 0 */

        /* re-calculate with skipped frames */
        workFrameCount = frameCount + numFramesSkipped;

        timecode->hour = (int)(workFrameCount / (60 * 60 * fps));
        workFrameCount %= 60 * 60 * fps;
        timecode->min = (int)(workFrameCount / (60 * fps));
        workFrameCount %= 60 * fps;
        timecode->sec = (int)(workFrameCount / fps);
        timecode->frame = (int)(workFrameCount % fps);
    }
}


static int preprocess_avid_input(AvidMXFToP2Transfer *transfer, int inputFileIndex, int outputFileIndex)
{
    mxfKey key;
    uint8_t llen;
    uint64_t len;
    uint8_t *avid_arrayData;
    uint32_t avid_arrayDataLen;
    MXFArrayItemIterator avid_arrayIter;
    mxfUMID avid_fileSourcePackageUID;
    mxfUMID umid;
    int foundIt = 0;
    size_t essenceContainerLen;
    mxfUL *essenceContainerLabel;
    MXFListIterator listIter;

    AvidMXFFile *input = &transfer->inputs[inputFileIndex];
    P2MXFFile *output = &transfer->outputs[outputFileIndex];


    /* load the data model for the avid file */

    CHK_ORET(mxf_load_data_model(&input->dataModel));
    CHK_ORET(mxf_avid_load_extensions(input->dataModel));
    CHK_ORET(mxf_finalise_data_model(input->dataModel));


    /* read header partition pack */

    if (!mxf_read_header_pp_kl(input->mxfFile, &key, &llen, &len))
    {
        mxf_log_error("Could not find header partition pack key" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
        return 0;
    }
    CHK_ORET(mxf_read_partition(input->mxfFile, &key, len, &input->headerPartition));


    /* check the operational pattern is OP Atom */

    if (!mxf_is_op_atom(&input->headerPartition->operationalPattern) &&
        !is_invalid_aafsdk_op_atom_label(&input->headerPartition->operationalPattern))
    {
        mxf_log_error("Input file is not OP Atom" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
        return 0;
    }


    /* check the essence container label */

    essenceContainerLen = mxf_get_list_length(&input->headerPartition->essenceContainers);
    if (essenceContainerLen != 1)
    {
        mxf_log_error("Unexpected essence container labels - expecting 1 only" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
        return 0;
    }
    essenceContainerLabel = (mxfUL*)mxf_get_first_list_element(&input->headerPartition->essenceContainers);

    if (mxf_equals_ul(essenceContainerLabel, &MXF_EC_L(IECDV_25_625_50_ClipWrapped)))
    {
        output->isPicture = 1;
        output->sourceTrackNumber = MXF_DV_TRACK_NUM(0x01, 0x02, 0x01);
        output->essElementKey = MXF_EE_K(DVClipWrapped);
        mxf_snprintf(output->codecString, sizeof(output->codecString), "DV25_420");
        mxf_snprintf(output->frameRateString, sizeof(output->frameRateString), "50i");
        output->frameRate.numerator = 25;
        output->frameRate.denominator = 1;
        output->editRate = output->frameRate;
        output->startByteOffset = g_p2_fixedStartByteOffset;
        output->essenceContainerLabel = MXF_EC_L(IECDV_25_625_50_ClipWrapped);
        output->frameSize = 144000;
        output->dataDef = MXF_DDEF_L(Picture);
        output->sourceTrackID = 0;
        output->verticalSubsampling = 2;
        output->horizSubsampling = 2;
        output->pictureEssenceCoding = MXF_CMDEF_L(IECDV_25_625_50);
        output->storedHeight = 288;
        output->storedWidth = 720;
        output->videoLineMap[0] = 23;
        output->videoLineMap[1] = 335;
        mxf_generate_umid(&output->sourcePackageUID);
        transfer->pictureOutputIndex = outputFileIndex;
    }
    else if (mxf_equals_ul(essenceContainerLabel, &MXF_EC_L(IECDV_25_525_60_ClipWrapped)))
    {
        output->isPicture = 1;
        output->sourceTrackNumber = MXF_DV_TRACK_NUM(0x01, 0x02, 0x01);
        output->essElementKey = MXF_EE_K(DVClipWrapped);
        mxf_snprintf(output->codecString, sizeof(output->codecString), "DV25_411");
        mxf_snprintf(output->frameRateString, sizeof(output->frameRateString), "59.94i");
        output->frameRate.numerator = 30000;
        output->frameRate.denominator = 1001;
        output->editRate = output->frameRate;
        output->startByteOffset = g_p2_fixedStartByteOffset;
        output->essenceContainerLabel = MXF_EC_L(IECDV_25_525_60_ClipWrapped);
        output->frameSize = 120000;
        output->dataDef = MXF_DDEF_L(Picture);
        output->sourceTrackID = 0;
        output->verticalSubsampling = 1;
        output->horizSubsampling = 4;
        output->pictureEssenceCoding = MXF_CMDEF_L(IECDV_25_525_60);
        output->storedHeight = 240;
        output->storedWidth = 720;
        output->videoLineMap[0] = 23;
        output->videoLineMap[1] = 285;
        mxf_generate_umid(&output->sourcePackageUID);
        transfer->pictureOutputIndex = outputFileIndex;
    }
    else if (mxf_equals_ul(essenceContainerLabel, &MXF_EC_L(DVBased_25_625_50_ClipWrapped)))
    {
        output->isPicture = 1;
        output->sourceTrackNumber = MXF_DV_TRACK_NUM(0x01, 0x02, 0x01);
        output->essElementKey = MXF_EE_K(DVClipWrapped);
        mxf_snprintf(output->codecString, sizeof(output->codecString), "DV25_411");
        mxf_snprintf(output->frameRateString, sizeof(output->frameRateString), "50i");
        output->frameRate.numerator = 25;
        output->frameRate.denominator = 1;
        output->editRate = output->frameRate;
        output->startByteOffset = g_p2_fixedStartByteOffset;
        output->essenceContainerLabel = MXF_EC_L(DVBased_25_625_50_ClipWrapped);
        output->frameSize = 144000;
        output->dataDef = MXF_DDEF_L(Picture);
        output->sourceTrackID = 0;
        output->verticalSubsampling = 1;
        output->horizSubsampling = 4;
        output->pictureEssenceCoding = MXF_CMDEF_L(DVBased_25_625_50);
        output->storedHeight = 288;
        output->storedWidth = 720;
        output->videoLineMap[0] = 23;
        output->videoLineMap[1] = 335;
        mxf_generate_umid(&output->sourcePackageUID);
        transfer->pictureOutputIndex = outputFileIndex;
    }
    else if (mxf_equals_ul(essenceContainerLabel, &MXF_EC_L(DVBased_25_525_60_ClipWrapped)))
    {
        output->isPicture = 1;
        output->sourceTrackNumber = MXF_DV_TRACK_NUM(0x01, 0x02, 0x01);
        output->essElementKey = MXF_EE_K(DVClipWrapped);
        mxf_snprintf(output->codecString, sizeof(output->codecString), "DV25_411");
        mxf_snprintf(output->frameRateString, sizeof(output->frameRateString), "59.94i");
        output->frameRate.numerator = 30000;
        output->frameRate.denominator = 1001;
        output->editRate = output->frameRate;
        output->startByteOffset = g_p2_fixedStartByteOffset;
        output->essenceContainerLabel = MXF_EC_L(DVBased_25_525_60_ClipWrapped);
        output->frameSize = 120000;
        output->dataDef = MXF_DDEF_L(Picture);
        output->sourceTrackID = 0;
        output->verticalSubsampling = 1;
        output->horizSubsampling = 4;
        output->pictureEssenceCoding = MXF_CMDEF_L(DVBased_25_525_60);
        output->storedHeight = 240;
        output->storedWidth = 720;
        output->videoLineMap[0] = 23;
        output->videoLineMap[1] = 285;
        mxf_generate_umid(&output->sourcePackageUID);
        transfer->pictureOutputIndex = outputFileIndex;
    }
    else if (mxf_equals_ul(essenceContainerLabel, &MXF_EC_L(DVBased_50_625_50_ClipWrapped)))
    {
        output->isPicture = 1;
        output->sourceTrackNumber = MXF_DV_TRACK_NUM(0x01, 0x02, 0x01);
        output->essElementKey = MXF_EE_K(DVClipWrapped);
        mxf_snprintf(output->codecString, sizeof(output->codecString), "DV50_422");
        mxf_snprintf(output->frameRateString, sizeof(output->frameRateString), "50i");
        output->frameRate.numerator = 25;
        output->frameRate.denominator = 1;
        output->editRate = output->frameRate;
        output->startByteOffset = g_p2_fixedStartByteOffset;
        output->essenceContainerLabel = MXF_EC_L(DVBased_50_625_50_ClipWrapped);
        output->frameSize = 288000;
        output->dataDef = MXF_DDEF_L(Picture);
        output->sourceTrackID = 0;
        output->verticalSubsampling = 1;
        output->horizSubsampling = 2;
        output->pictureEssenceCoding = MXF_CMDEF_L(DVBased_50_625_50);
        output->storedHeight = 288;
        output->storedWidth = 720;
        output->videoLineMap[0] = 23;
        output->videoLineMap[1] = 335;
        mxf_generate_umid(&output->sourcePackageUID);
        transfer->pictureOutputIndex = outputFileIndex;
    }
    else if (mxf_equals_ul(essenceContainerLabel, &MXF_EC_L(DVBased_50_525_60_ClipWrapped)))
    {
        output->isPicture = 1;
        output->sourceTrackNumber = MXF_DV_TRACK_NUM(0x01, 0x02, 0x01);
        output->essElementKey = MXF_EE_K(DVClipWrapped);
        mxf_snprintf(output->codecString, sizeof(output->codecString), "DV50_422");
        mxf_snprintf(output->frameRateString, sizeof(output->frameRateString), "59.94i");
        output->frameRate.numerator = 30000;
        output->frameRate.denominator = 1001;
        output->editRate = output->frameRate;
        output->startByteOffset = g_p2_fixedStartByteOffset;
        output->essenceContainerLabel = MXF_EC_L(DVBased_50_525_60_ClipWrapped);
        output->frameSize = 240000;
        output->dataDef = MXF_DDEF_L(Picture);
        output->sourceTrackID = 0;
        output->verticalSubsampling = 1;
        output->horizSubsampling = 2;
        output->pictureEssenceCoding = MXF_CMDEF_L(DVBased_50_525_60);
        output->storedHeight = 240;
        output->storedWidth = 720;
        output->videoLineMap[0] = 23;
        output->videoLineMap[1] = 285;
        mxf_generate_umid(&output->sourcePackageUID);
        transfer->pictureOutputIndex = outputFileIndex;
    }
    else if (mxf_equals_ul(essenceContainerLabel, &MXF_EC_L(BWFClipWrapped)) ||
        mxf_equals_ul(essenceContainerLabel, &MXF_EC_L(AES3ClipWrapped)))
    {
        output->isPicture = 0;
        output->sourceTrackNumber = MXF_AES3BWF_TRACK_NUM(0x01, 0x04, 0x01);
        output->essElementKey = MXF_EE_K(AES3ClipWrapped);
        output->startByteOffset = g_p2_fixedStartByteOffset;
        output->samplingRate.numerator = 48000;
        output->samplingRate.denominator = 1;
        output->editRate = output->samplingRate;
        output->dataDef = MXF_DDEF_L(Sound);
        output->sourceTrackID = 0;
        mxf_generate_umid(&output->sourcePackageUID);
        output->essenceContainerLabel = MXF_EC_L(AES3ClipWrapped);
    }
    else if (is_invalid_aafsdk_esscont_label(essenceContainerLabel))
    {
        mxf_log_error("Invalid AAF SDK essence container label in input file" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
        return 0;
    }
    else
    {
        char labelStr[KEY_STR_SIZE];
        mxf_sprint_label(labelStr, essenceContainerLabel);
        mxf_log_error("Unknown or unsupported essence container label '%s' in input file" LOG_LOC_FORMAT,
            labelStr, LOG_LOC_PARAMS);
        return 0;
    }


    /* create and read the header metadata (filter out meta-dictionary and dictionary except data defs) */

    CHK_ORET(mxf_create_header_metadata(&input->headerMetadata, input->dataModel));

    CHK_ORET(mxf_read_next_nonfiller_kl(input->mxfFile, &key, &llen, &len));
    CHK_ORET(mxf_is_header_metadata(&key));
    CHK_ORET(mxf_avid_read_filtered_header_metadata(input->mxfFile, 0, input->headerMetadata,
        input->headerPartition->headerByteCount, &key, llen, len));


    /* get the top file SourcePackage referenced by Preface::PrimaryPackage if it exists
       or go via the EssenceData::LinkedPackageUID */

    CHK_ORET(mxf_find_singular_set_by_key(input->headerMetadata, &MXF_SET_K(Preface), &input->prefaceSet));

    if (mxf_have_item(input->prefaceSet, &MXF_ITEM_K(Preface, PrimaryPackage)))
    {
        /* the Preface::PrimaryPackage references the top level file SourcePackage */
        CHK_ORET(mxf_get_weakref_item(input->prefaceSet, &MXF_ITEM_K(Preface, PrimaryPackage), &input->sourcePackageSet));
    }
    else
    {
        /* get the EssenceContainerData::LinkedPackageUID */
        CHK_ORET(mxf_find_singular_set_by_key(input->headerMetadata, &MXF_SET_K(EssenceContainerData), &input->essContainerDataSet));
        CHK_ORET(mxf_get_umid_item(input->essContainerDataSet, &MXF_ITEM_K(EssenceContainerData, LinkedPackageUID), &avid_fileSourcePackageUID));

        /* Go through the SourcePackages looking for the one with PackageUID == LinkedPackageUID */
        CHK_ORET(mxf_find_set_by_key(input->headerMetadata, &MXF_SET_K(SourcePackage), &input->aList));
        mxf_initialise_list_iter(&listIter, input->aList);
        foundIt = 0;
        while (!foundIt && mxf_next_list_iter_element(&listIter))
        {
            MXFMetadataSet *set = (MXFMetadataSet*)mxf_get_iter_element(&listIter);

            CHK_ORET(mxf_get_umid_item(set, &MXF_ITEM_K(GenericPackage, PackageUID), &umid));
            if (mxf_equals_umid(&avid_fileSourcePackageUID, &umid))
            {
                input->sourcePackageSet = set;
                foundIt = 1;
            }
        }
        mxf_free_list(&input->aList);
        if (!foundIt)
        {
            mxf_log_error("Could not find the top level file source package" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
            return 0;
        }
    }


    /* get the file SourcePackage Sequence::Duration */

    CHK_ORET(mxf_initialise_array_item_iterator(input->sourcePackageSet, &MXF_ITEM_K(GenericPackage, Tracks), &avid_arrayIter));
    while (mxf_next_array_item_element(&avid_arrayIter, &avid_arrayData, &avid_arrayDataLen))
    {
        CHK_ORET(mxf_get_strongref(input->headerMetadata, avid_arrayData, &input->sourcePackageTrackSet));

        CHK_ORET(mxf_get_strongref_item(input->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, Sequence), &input->sequenceSet));

        CHK_ORET(mxf_get_length_item(input->sequenceSet, &MXF_ITEM_K(StructuralComponent, Duration), &output->duration));
    }


    /* get the descriptor info from the file SourcePackage Descriptor */

    CHK_ORET(mxf_get_strongref_item(input->sourcePackageSet, &MXF_ITEM_K(SourcePackage, Descriptor), &input->descriptorSet));

    if (output->isPicture)
    {
        CHK_ORET(mxf_get_rational_item(input->descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, AspectRatio), &output->aspectRatio));
    }
    else
    {
        CHK_ORET(mxf_get_rational_item(input->descriptorSet, &MXF_ITEM_K(GenericSoundEssenceDescriptor, AudioSamplingRate), &output->samplingRate));
        CHK_ORET(mxf_get_uint32_item(input->descriptorSet, &MXF_ITEM_K(GenericSoundEssenceDescriptor, QuantizationBits), &output->bitsPerSample));
        CHK_ORET(mxf_get_uint16_item(input->descriptorSet, &MXF_ITEM_K(WaveAudioDescriptor, BlockAlign), &output->blockAlign));
        CHK_ORET(mxf_get_uint32_item(input->descriptorSet, &MXF_ITEM_K(WaveAudioDescriptor, AvgBps), &output->avgBps));

        if (output->samplingRate.numerator != 48000 || output->samplingRate.denominator != 1)
        {
            mxf_log_error("Unsupported input audio sampling rate %d/%d" LOG_LOC_FORMAT,
                output->samplingRate.numerator, output->samplingRate.denominator, LOG_LOC_PARAMS);
            return 0;
        }
        if (output->bitsPerSample != 16 && output->bitsPerSample != 24)
        {
            mxf_log_error("Unsupported audio bits per sample %u" LOG_LOC_FORMAT,
                output->bitsPerSample, LOG_LOC_PARAMS);
            return 0;
        }
        if (output->bitsPerSample == 16)
        {
            if (output->blockAlign != 2)
            {
                mxf_log_error("Unsupported audio block alignment %u for %u bit quantization" LOG_LOC_FORMAT,
                    output->blockAlign, output->bitsPerSample, LOG_LOC_PARAMS);
                return 0;
            }
            if (output->avgBps != 96000)
            {
                mxf_log_error("Unsupported audio avg. bps %u for %u bit quantization" LOG_LOC_FORMAT,
                    output->avgBps, output->bitsPerSample, LOG_LOC_PARAMS);
                return 0;
            }
        }
        else //24 bitsPerSample
        {
            if (output->blockAlign != 3)
            {
                mxf_log_error("Unsupported audio block alignment %u for %u bit quantization" LOG_LOC_FORMAT,
                    output->blockAlign, output->bitsPerSample, LOG_LOC_PARAMS);
                return 0;
            }
            if (output->avgBps != 144000)
            {
                mxf_log_error("Unsupported audio avg. bps %u for %u bit quantization" LOG_LOC_FORMAT,
                    output->avgBps, output->bitsPerSample, LOG_LOC_PARAMS);
                return 0;
            }
        }
    }



    /* skip the body partition pack and position at the start of the essence element */

    CHK_ORET(mxf_read_next_nonfiller_kl(input->mxfFile, &key, &llen, &len));
    if (!mxf_is_body_partition_pack(&key))
    {
        mxf_log_error("Expecting the body partition pack" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
        return 0;
    }
    CHK_ORET(mxf_skip(input->mxfFile, len));

    CHK_ORET(mxf_read_next_nonfiller_kl(input->mxfFile, &key, &llen, &len));
    if (!mxf_is_gc_essence_element(&key))
    {
        mxf_log_error("Expecting an essence element" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
        return 0;
    }

    /* number of essence bytes determines the contribution towards the progress % */
    output->essenceBytesLength = len;


    /* calculate the container duration */

    if (output->isPicture)
    {
        output->containerDuration = len / output->frameSize;
    }
    else
    {
        output->containerDuration = len / output->blockAlign;
    }
    if (output->containerDuration != output->duration)
    {
        mxf_log_warn("File container duration %" PRId64 " does not equal track duration %" PRId64 "\n",
            output->containerDuration, output->duration);
    }


    return 1;
}


static int transfer_to_p2(AvidMXFToP2Transfer *transfer, int inputFileIndex, int outputFileIndex)
{
    mxfKey key;
    uint8_t llen;
    uint64_t len;
    mxfUUID uuid;
    MXFPartition *headerPartition;
    MXFPartition *bodyPartition;
    MXFPartition *footerPartition;
    uint8_t buffer[ESSENCE_BUFFER_SIZE];
    uint32_t numRead;
    mxfUTF16Char materialPackageName[256];
    uint32_t essenceReadSize;
    mxfLength frameCount;
    int i;
    float percentCompletedStart = transfer->percentCompleted;
    uint64_t totalBytesRead;
    uint8_t *arrayElement;

    AvidMXFFile *input = &transfer->inputs[inputFileIndex];
    P2MXFFile *output = &transfer->outputs[outputFileIndex];

    mbstowcs(materialPackageName, transfer->clipName, strlen(transfer->clipName) + 1);

    /*
     * Position the input at the essence element and open it
     */
    while (1)
    {
        CHK_ORET(mxf_read_next_nonfiller_kl(input->mxfFile, &key, &llen, &len));
        CHK_ORET(mxf_file_seek(input->mxfFile, len, SEEK_CUR));
        if (mxf_is_body_partition_pack(&key))
        {
            break;
        }
    }

    CHK_ORET(mxf_read_next_nonfiller_kl(input->mxfFile, &key, &llen, &len));
    if (!mxf_is_gc_essence_element(&key))
    {
        mxf_log_error("Expecting an essence element" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
        return 0;
    }

    CHK_ORET(mxf_open_essence_element_read(input->mxfFile, &key, llen, len, &input->essenceElement));



    /* set the minimum llen */
    mxf_file_set_min_llen(output->mxfFile, 4);


    /*
     * Create the P2 header metadata from the Avid header metadata
     */

    /* load the data model */
    CHK_ORET(mxf_load_data_model(&output->dataModel));


    /* create the header metadata */
    CHK_ORET(mxf_create_header_metadata(&output->headerMetadata, output->dataModel));

    /* Preface */
    CHK_ORET(mxf_create_set(output->headerMetadata, &MXF_SET_K(Preface), &output->prefaceSet));
    CHK_ORET(mxf_set_timestamp_item(output->prefaceSet, &MXF_ITEM_K(Preface, LastModifiedDate), &transfer->now));
    CHK_ORET(mxf_set_version_type_item(output->prefaceSet, &MXF_ITEM_K(Preface, Version), MXF_PREFACE_VER(1, 2)));
    CHK_ORET(mxf_set_ul_item(output->prefaceSet, &MXF_ITEM_K(Preface, OperationalPattern), &MXF_OP_L(atom, NTracks_1SourceClip)));
    CHK_ORET(mxf_alloc_array_item_elements(output->prefaceSet, &MXF_ITEM_K(Preface, EssenceContainers), mxfUL_extlen, 1, &arrayElement));
    mxf_set_ul(&output->essenceContainerLabel, arrayElement);
    CHK_ORET(mxf_set_empty_array_item(output->prefaceSet, &MXF_ITEM_K(Preface, DMSchemes), mxfUL_extlen));

    /* Preface - Identification */
    mxf_generate_uuid(&uuid);
    CHK_ORET(mxf_create_set(output->headerMetadata, &MXF_SET_K(Identification), &output->identSet));
    CHK_ORET(mxf_add_array_item_strongref(output->prefaceSet, &MXF_ITEM_K(Preface, Identifications), output->identSet));
    CHK_ORET(mxf_set_uuid_item(output->identSet, &MXF_ITEM_K(Identification, ThisGenerationUID), &uuid));
    CHK_ORET(mxf_set_utf16string_item(output->identSet, &MXF_ITEM_K(Identification, CompanyName), g_mxfIdentCompanyName));
    CHK_ORET(mxf_set_utf16string_item(output->identSet, &MXF_ITEM_K(Identification, ProductName), g_mxfIdentProductName));
    CHK_ORET(mxf_set_utf16string_item(output->identSet, &MXF_ITEM_K(Identification, VersionString), g_mxfIdentVersionString));
    CHK_ORET(mxf_set_uuid_item(output->identSet, &MXF_ITEM_K(Identification, ProductUID), &g_mxfIdentProductUID));
    CHK_ORET(mxf_set_timestamp_item(output->identSet, &MXF_ITEM_K(Identification, ModificationDate), &transfer->now));
    CHK_ORET(mxf_set_product_version_item(output->identSet, &MXF_ITEM_K(Identification, ToolkitVersion), mxf_get_version()));
    CHK_ORET(mxf_set_utf16string_item(output->identSet, &MXF_ITEM_K(Identification, Platform), mxf_get_platform_wstring()));

    /* Preface - ContentStorage */
    CHK_ORET(mxf_create_set(output->headerMetadata, &MXF_SET_K(ContentStorage), &output->contentStorageSet));
    CHK_ORET(mxf_set_strongref_item(output->prefaceSet, &MXF_ITEM_K(Preface, ContentStorage), output->contentStorageSet));


    /* Preface - ContentStorage - EssenceContainerData */
    CHK_ORET(mxf_create_set(output->headerMetadata, &MXF_SET_K(EssenceContainerData), &output->essContainerDataSet));
    CHK_ORET(mxf_add_array_item_strongref(output->contentStorageSet, &MXF_ITEM_K(ContentStorage, EssenceContainerData), output->essContainerDataSet));
    CHK_ORET(mxf_set_umid_item(output->essContainerDataSet, &MXF_ITEM_K(EssenceContainerData, LinkedPackageUID), &output->sourcePackageUID));
    CHK_ORET(mxf_set_uint32_item(output->essContainerDataSet, &MXF_ITEM_K(EssenceContainerData, IndexSID), g_p2_indexSID));
    CHK_ORET(mxf_set_uint32_item(output->essContainerDataSet, &MXF_ITEM_K(EssenceContainerData, BodySID), g_p2_bodySID));


    /* Preface - ContentStorage - MaterialPackage */
    CHK_ORET(mxf_create_set(output->headerMetadata, &MXF_SET_K(MaterialPackage), &output->materialPackageSet));
    CHK_ORET(mxf_add_array_item_strongref(output->contentStorageSet, &MXF_ITEM_K(ContentStorage, Packages), output->materialPackageSet));
    CHK_ORET(mxf_set_umid_item(output->materialPackageSet, &MXF_ITEM_K(GenericPackage, PackageUID), &transfer->globalClipID));
    CHK_ORET(mxf_set_timestamp_item(output->materialPackageSet, &MXF_ITEM_K(GenericPackage, PackageCreationDate), &transfer->now));
    CHK_ORET(mxf_set_timestamp_item(output->materialPackageSet, &MXF_ITEM_K(GenericPackage, PackageModifiedDate), &transfer->now));
    CHK_ORET(mxf_set_utf16string_item(output->materialPackageSet, &MXF_ITEM_K(GenericPackage, Name), materialPackageName));


    /* Preface - ContentStorage - MaterialPackage Tracks */

    /* Preface - ContentStorage - MaterialPackage - Timecode Track */
    if (transfer->pictureOutputIndex >= 0)
    {
        uint16_t roundedTimecodeBase = (uint16_t)((float)transfer->outputs[transfer->pictureOutputIndex].frameRate.numerator /
            (float)transfer->outputs[transfer->pictureOutputIndex].frameRate.denominator + 0.5);

        /* Preface - ContentStorage - MaterialPackage - Timecode Track */
        CHK_ORET(mxf_create_set(output->headerMetadata, &MXF_SET_K(Track), &output->materialPackageTrackSet));
        CHK_ORET(mxf_add_array_item_strongref(output->materialPackageSet, &MXF_ITEM_K(GenericPackage, Tracks), output->materialPackageTrackSet));
        CHK_ORET(mxf_set_uint32_item(output->materialPackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackID), g_p2_timecodeTrackID));
        CHK_ORET(mxf_set_uint32_item(output->materialPackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackNumber), g_p2_timecodeTrackNumber));
        CHK_ORET(mxf_set_rational_item(output->materialPackageTrackSet, &MXF_ITEM_K(Track, EditRate), &transfer->outputs[transfer->pictureOutputIndex].editRate));
        CHK_ORET(mxf_set_position_item(output->materialPackageTrackSet, &MXF_ITEM_K(Track, Origin), 0));

        /* Preface - ContentStorage - MaterialPackage - Timecode Track - Sequence */
        CHK_ORET(mxf_create_set(output->headerMetadata, &MXF_SET_K(Sequence), &output->sequenceSet));
        CHK_ORET(mxf_set_strongref_item(output->materialPackageTrackSet, &MXF_ITEM_K(GenericTrack, Sequence), output->sequenceSet));
        CHK_ORET(mxf_set_ul_item(output->sequenceSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &MXF_DDEF_L(Timecode)));
        CHK_ORET(mxf_set_length_item(output->sequenceSet, &MXF_ITEM_K(StructuralComponent, Duration), transfer->outputs[transfer->pictureOutputIndex].duration));

        /* Preface - ContentStorage - MaterialPackage - Timecode Track - Sequence - TimecodeComponent */
        CHK_ORET(mxf_create_set(output->headerMetadata, &MXF_SET_K(TimecodeComponent), &output->timecodeComponentSet));
        CHK_ORET(mxf_add_array_item_strongref(output->sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), output->timecodeComponentSet));
        CHK_ORET(mxf_set_ul_item(output->timecodeComponentSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &MXF_DDEF_L(Timecode)));
        CHK_ORET(mxf_set_length_item(output->timecodeComponentSet, &MXF_ITEM_K(StructuralComponent, Duration), transfer->outputs[transfer->pictureOutputIndex].duration));
        CHK_ORET(mxf_set_uint16_item(output->timecodeComponentSet, &MXF_ITEM_K(TimecodeComponent, RoundedTimecodeBase), roundedTimecodeBase));
        CHK_ORET(mxf_set_boolean_item(output->timecodeComponentSet, &MXF_ITEM_K(TimecodeComponent, DropFrame), transfer->dropFrameFlag));
        CHK_ORET(mxf_set_position_item(output->timecodeComponentSet, &MXF_ITEM_K(TimecodeComponent, StartTimecode), transfer->timecodeStart));
    }

    /* Preface - ContentStorage - MaterialPackage - Timeline Tracks */
    for (i = 0; i < transfer->numInputs; i++)
    {
        /* Preface - ContentStorage - MaterialPackage - Timeline Track */
        CHK_ORET(mxf_create_set(output->headerMetadata, &MXF_SET_K(Track), &output->materialPackageTrackSet));
        CHK_ORET(mxf_add_array_item_strongref(output->materialPackageSet, &MXF_ITEM_K(GenericPackage, Tracks), output->materialPackageTrackSet));
        CHK_ORET(mxf_set_uint32_item(output->materialPackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackID), transfer->outputs[i].materialTrackID));
        CHK_ORET(mxf_set_uint32_item(output->materialPackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackNumber), transfer->outputs[i].materialTrackNumber));
        CHK_ORET(mxf_set_rational_item(output->materialPackageTrackSet, &MXF_ITEM_K(Track, EditRate), &transfer->outputs[i].editRate));
        CHK_ORET(mxf_set_position_item(output->materialPackageTrackSet, &MXF_ITEM_K(Track, Origin), 0));

        /* Preface - ContentStorage - MaterialPackage - Timeline Track - Sequence */
        CHK_ORET(mxf_create_set(output->headerMetadata, &MXF_SET_K(Sequence), &output->sequenceSet));
        CHK_ORET(mxf_set_strongref_item(output->materialPackageTrackSet, &MXF_ITEM_K(GenericTrack, Sequence), output->sequenceSet));
        CHK_ORET(mxf_set_ul_item(output->sequenceSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &transfer->outputs[i].dataDef));
        CHK_ORET(mxf_set_length_item(output->sequenceSet, &MXF_ITEM_K(StructuralComponent, Duration), transfer->outputs[i].duration));

        /* Preface - ContentStorage - MaterialPackage - Timeline Track - Sequence - SourceClip */
        CHK_ORET(mxf_create_set(output->headerMetadata, &MXF_SET_K(SourceClip), &output->sourceClipSet));
        CHK_ORET(mxf_add_array_item_strongref(output->sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), output->sourceClipSet));
        CHK_ORET(mxf_set_ul_item(output->sourceClipSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &transfer->outputs[i].dataDef));
        CHK_ORET(mxf_set_length_item(output->sourceClipSet, &MXF_ITEM_K(StructuralComponent, Duration), transfer->outputs[i].duration));
        CHK_ORET(mxf_set_position_item(output->sourceClipSet, &MXF_ITEM_K(SourceClip, StartPosition), 0));
        CHK_ORET(mxf_set_umid_item(output->sourceClipSet, &MXF_ITEM_K(SourceClip, SourcePackageID), &transfer->outputs[i].sourcePackageUID));
        CHK_ORET(mxf_set_uint32_item(output->sourceClipSet, &MXF_ITEM_K(SourceClip, SourceTrackID), transfer->outputs[i].sourceTrackID));
    }


    /* Preface - ContentStorage - SourcePackage */
    CHK_ORET(mxf_create_set(output->headerMetadata, &MXF_SET_K(SourcePackage), &output->sourcePackageSet));
    CHK_ORET(mxf_add_array_item_strongref(output->contentStorageSet, &MXF_ITEM_K(ContentStorage, Packages), output->sourcePackageSet));
    CHK_ORET(mxf_set_weakref_item(output->prefaceSet, &MXF_ITEM_K(Preface, PrimaryPackage), output->sourcePackageSet));
    CHK_ORET(mxf_set_umid_item(output->sourcePackageSet, &MXF_ITEM_K(GenericPackage, PackageUID), &output->sourcePackageUID));
    CHK_ORET(mxf_set_timestamp_item(output->sourcePackageSet, &MXF_ITEM_K(GenericPackage, PackageCreationDate), &transfer->now));
    CHK_ORET(mxf_set_timestamp_item(output->sourcePackageSet, &MXF_ITEM_K(GenericPackage, PackageModifiedDate), &transfer->now));

    /* Preface - ContentStorage - SourcePackage - Timeline Track */
    CHK_ORET(mxf_create_set(output->headerMetadata, &MXF_SET_K(Track), &output->sourcePackageTrackSet));
    CHK_ORET(mxf_add_array_item_strongref(output->sourcePackageSet, &MXF_ITEM_K(GenericPackage, Tracks), output->sourcePackageTrackSet));
    CHK_ORET(mxf_set_uint32_item(output->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackID), output->sourceTrackID));
    CHK_ORET(mxf_set_uint32_item(output->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackNumber), output->sourceTrackNumber));
    CHK_ORET(mxf_set_rational_item(output->sourcePackageTrackSet, &MXF_ITEM_K(Track, EditRate), &output->editRate));
    CHK_ORET(mxf_set_position_item(output->sourcePackageTrackSet, &MXF_ITEM_K(Track, Origin), 0));

    /* Preface - ContentStorage - SourcePackage - Timeline Track - Sequence */
    CHK_ORET(mxf_create_set(output->headerMetadata, &MXF_SET_K(Sequence), &output->sequenceSet));
    CHK_ORET(mxf_set_strongref_item(output->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, Sequence), output->sequenceSet));
    CHK_ORET(mxf_set_ul_item(output->sequenceSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &output->dataDef));
    CHK_ORET(mxf_set_length_item(output->sequenceSet, &MXF_ITEM_K(StructuralComponent, Duration), output->duration));

    /* Preface - ContentStorage - SourcePackage - Timeline Track - Sequence - SourceClip */
    CHK_ORET(mxf_create_set(output->headerMetadata, &MXF_SET_K(SourceClip), &output->sourceClipSet));
    CHK_ORET(mxf_add_array_item_strongref(output->sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), output->sourceClipSet));
    CHK_ORET(mxf_set_ul_item(output->sourceClipSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &output->dataDef));
    CHK_ORET(mxf_set_length_item(output->sourceClipSet, &MXF_ITEM_K(StructuralComponent, Duration), output->duration));
    CHK_ORET(mxf_set_position_item(output->sourceClipSet, &MXF_ITEM_K(SourceClip, StartPosition), 0));
    CHK_ORET(mxf_set_umid_item(output->sourceClipSet, &MXF_ITEM_K(SourceClip, SourcePackageID), &g_Null_UMID));
    CHK_ORET(mxf_set_uint32_item(output->sourceClipSet, &MXF_ITEM_K(SourceClip, SourceTrackID), 0));


    if (output->isPicture)
    {
        CHK_ORET(mxf_create_set(output->headerMetadata, &MXF_SET_K(CDCIEssenceDescriptor), &output->cdciDescriptorSet));
        CHK_ORET(mxf_set_strongref_item(output->sourcePackageSet, &MXF_ITEM_K(SourcePackage, Descriptor), output->cdciDescriptorSet));
        CHK_ORET(mxf_set_rational_item(output->cdciDescriptorSet, &MXF_ITEM_K(FileDescriptor, SampleRate), &output->frameRate));
        CHK_ORET(mxf_set_length_item(output->cdciDescriptorSet, &MXF_ITEM_K(FileDescriptor, ContainerDuration), output->containerDuration));
        CHK_ORET(mxf_set_ul_item(output->cdciDescriptorSet, &MXF_ITEM_K(FileDescriptor, EssenceContainer), &output->essenceContainerLabel));
        CHK_ORET(mxf_set_uint8_item(output->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, FrameLayout), MXF_SEPARATE_FIELDS));
        CHK_ORET(mxf_set_uint32_item(output->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, StoredHeight), output->storedHeight));
        CHK_ORET(mxf_set_uint32_item(output->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, StoredWidth), output->storedWidth));
        CHK_ORET(mxf_alloc_array_item_elements(output->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, VideoLineMap), 4, 2, &arrayElement));
        mxf_set_int32(output->videoLineMap[0], arrayElement);
        mxf_set_int32(output->videoLineMap[1], &arrayElement[4]);
        CHK_ORET(mxf_set_rational_item(output->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, AspectRatio), &output->aspectRatio));
        CHK_ORET(mxf_set_uint32_item(output->cdciDescriptorSet, &MXF_ITEM_K(CDCIEssenceDescriptor, ComponentDepth), 8));
        CHK_ORET(mxf_set_ul_item(output->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, PictureEssenceCoding), &output->pictureEssenceCoding));
        CHK_ORET(mxf_set_uint32_item(output->cdciDescriptorSet, &MXF_ITEM_K(CDCIEssenceDescriptor, HorizontalSubsampling), output->horizSubsampling));
        CHK_ORET(mxf_set_uint32_item(output->cdciDescriptorSet, &MXF_ITEM_K(CDCIEssenceDescriptor, VerticalSubsampling), output->verticalSubsampling));
    }
    else
    {
        /* Preface - ContentStorage - SourcePackage - AES3EssenceDescriptor */
        CHK_ORET(mxf_create_set(output->headerMetadata, &MXF_SET_K(AES3AudioDescriptor), &output->aes3DescriptorSet));
        CHK_ORET(mxf_set_strongref_item(output->sourcePackageSet, &MXF_ITEM_K(SourcePackage, Descriptor), output->aes3DescriptorSet));
        CHK_ORET(mxf_set_rational_item(output->aes3DescriptorSet, &MXF_ITEM_K(FileDescriptor, SampleRate), &output->samplingRate));
        CHK_ORET(mxf_set_length_item(output->aes3DescriptorSet, &MXF_ITEM_K(FileDescriptor, ContainerDuration), output->containerDuration));
        CHK_ORET(mxf_set_ul_item(output->aes3DescriptorSet, &MXF_ITEM_K(FileDescriptor, EssenceContainer), &output->essenceContainerLabel));
        CHK_ORET(mxf_set_rational_item(output->aes3DescriptorSet, &MXF_ITEM_K(GenericSoundEssenceDescriptor, AudioSamplingRate), &output->samplingRate));
        CHK_ORET(mxf_set_boolean_item(output->aes3DescriptorSet, &MXF_ITEM_K(GenericSoundEssenceDescriptor, Locked), 1));
        CHK_ORET(mxf_set_uint32_item(output->aes3DescriptorSet, &MXF_ITEM_K(GenericSoundEssenceDescriptor, ChannelCount), 1));
        CHK_ORET(mxf_set_uint32_item(output->aes3DescriptorSet, &MXF_ITEM_K(GenericSoundEssenceDescriptor, QuantizationBits), output->bitsPerSample));
        CHK_ORET(mxf_set_uint16_item(output->aes3DescriptorSet, &MXF_ITEM_K(WaveAudioDescriptor, BlockAlign), output->blockAlign));
        CHK_ORET(mxf_set_uint32_item(output->aes3DescriptorSet, &MXF_ITEM_K(WaveAudioDescriptor, AvgBps), output->avgBps));
    }

    /*
     * Write the Header Partition Pack
     */

    CHK_ORET(mxf_append_new_partition(output->partitions, &headerPartition));
    headerPartition->key = MXF_PP_K(ClosedComplete, Header);
    headerPartition->majorVersion = 1;
    headerPartition->minorVersion = 2;
    headerPartition->kagSize = 0x01;
    headerPartition->indexSID = g_p2_indexSID;
    headerPartition->operationalPattern = MXF_OP_L(atom, NTracks_1SourceClip);
    CHK_ORET(mxf_append_partition_esscont_label(headerPartition, &output->essenceContainerLabel));

    CHK_ORET(mxf_write_partition(output->mxfFile, headerPartition));


    /*
     * Write the Header Metadata
     */

    CHK_ORET(mxf_mark_header_start(output->mxfFile, headerPartition));
    CHK_ORET(mxf_write_header_metadata(output->mxfFile, output->headerMetadata));
    CHK_ORET(mxf_mark_header_end(output->mxfFile, headerPartition));


    /*
     * Write the Index Table Segment
     */

    CHK_ORET(mxf_mark_index_start(output->mxfFile, headerPartition));

    CHK_ORET(mxf_create_index_table_segment(&output->indexSegment));
    mxf_generate_uuid(&output->indexSegment->instanceUID);
    output->indexSegment->indexEditRate = output->editRate;
    output->indexSegment->indexStartPosition = 0;
    output->indexSegment->indexDuration = 0; /* zero value means the indexDuration == container duration */
    if (output->isPicture)
    {
        output->indexSegment->editUnitByteCount = output->frameSize;
    }
    else
    {
        output->indexSegment->editUnitByteCount = output->bitsPerSample / 8;
    }
    output->indexSegment->indexSID = g_p2_indexSID;
    output->indexSegment->bodySID = g_p2_bodySID;
    output->indexSegment->sliceCount = 0;
    output->indexSegment->posTableCount = 0;
    output->indexSegment->deltaEntryArray = NULL;
    output->indexSegment->indexEntryArray = NULL;

    CHK_ORET(mxf_write_index_table_segment(output->mxfFile, output->indexSegment));
    CHK_ORET(mxf_fill_to_position(output->mxfFile, g_p2_fixedBodyPPOffset));

    CHK_ORET(mxf_mark_index_end(output->mxfFile, headerPartition));


    /*
     * Write the Body Partition Pack
     */

    CHK_ORET(mxf_append_new_from_partition(output->partitions, headerPartition, &bodyPartition));
    bodyPartition->key = MXF_PP_K(ClosedComplete, Body);
    bodyPartition->bodySID = g_p2_bodySID;

    CHK_ORET(mxf_write_partition(output->mxfFile, bodyPartition));


    /*
     * Transfer the essence data
     */
    if (output->isPicture)
    {
        essenceReadSize = output->frameSize;
    }
    else
    {
        essenceReadSize = ESSENCE_BUFFER_SIZE;
    }
    CHK_ORET(mxf_open_essence_element_write(output->mxfFile, &output->essElementKey, 8, 0,
        &output->essenceElement));
    frameCount = 0;
    totalBytesRead = 0;
    while (1)
    {
        CHK_ORET(mxf_read_essence_element_data(input->mxfFile, input->essenceElement, essenceReadSize,
            buffer, &numRead));

        /* insert timecode into DV essence */
        if (output->isPicture && transfer->insertTimecode != NULL &&
            numRead == essenceReadSize)
        {
            transfer->insertTimecode(buffer, essenceReadSize,
                frameCount + transfer->timecodeStart, transfer->dropFrameFlag);
            frameCount++;
        }

        if (output->isPicture && numRead > 0 && numRead != essenceReadSize)
        {
            mxf_log_warn("Last essence data frame is wrong size %u\n", numRead);
        }

        if (numRead > 0)
        {
            CHK_ORET(mxf_write_essence_element_data(output->mxfFile, output->essenceElement, buffer, numRead));
        }

        /* report progress */
        totalBytesRead += numRead;
        transfer->percentCompleted = percentCompletedStart +
            output->percentCompletedContribution * (float)((double)totalBytesRead / output->essenceBytesLength);
        CALL_PROGRESS();

        if (numRead < essenceReadSize)
        {
            break;
        }
    }
    CHK_ORET(mxf_finalize_essence_element_write(output->mxfFile, output->essenceElement));
    output->dataSize = mxf_get_essence_element_size(output->essenceElement);
    mxf_close_essence_element(&output->essenceElement);
    mxf_close_essence_element(&input->essenceElement);


    /*
     * Write the Footer Partition Pack
     */

    CHK_ORET(mxf_append_new_from_partition(output->partitions, headerPartition, &footerPartition));
    footerPartition->key = MXF_PP_K(ClosedComplete, Footer);
    footerPartition->indexSID = g_p2_indexSID;

    CHK_ORET(mxf_write_partition(output->mxfFile, footerPartition));


    /*
     * Write the Index Table Segment (copy the previous and set a new InstanceUID)
     */

    CHK_ORET(mxf_mark_index_start(output->mxfFile, footerPartition));

    mxf_generate_uuid(&output->indexSegment->instanceUID);
    CHK_ORET(mxf_write_index_table_segment(output->mxfFile, output->indexSegment));

    CHK_ORET(mxf_mark_index_end(output->mxfFile, footerPartition));


    /*
     * write the random index pack
     */

    CHK_ORET(mxf_write_rip(output->mxfFile, output->partitions));


    /*
     * Update the Partition Packs and re-write them
     */

    CHK_ORET(mxf_update_partitions(output->mxfFile, output->partitions));


    return 1;
}


static int write_icon_bmp(AvidMXFToP2Transfer *transfer)
{
    FILE *iconFile;
    char filename[FILENAME_MAX];
    size_t numWrite;

    mxf_snprintf(filename, sizeof(filename), "%s%s%s%s%s%s%s.BMP",
        transfer->p2path, g_fileSeparator,
        g_contentsDirname, g_fileSeparator,
        g_iconDirname, g_fileSeparator,
        transfer->clipName);

	/* TODO: use ffmpeg to extract a frame, decode it, and scale down to 80x60 */
    if ((iconFile = fopen(filename, "wb")) == NULL)
    {
        mxf_log_error("Failed to open BMP file for write '%s'" LOG_LOC_FORMAT, filename, LOG_LOC_PARAMS);
        return 0;
    }
	/* write the hard-coded "Avid->P2" icon */
    numWrite = fwrite(icon_avid_to_p2, 1, sizeof(icon_avid_to_p2), iconFile);
    if (numWrite != sizeof(icon_avid_to_p2))
    {
        mxf_log_error("Failed to write BMP data for '%s'" LOG_LOC_FORMAT, filename, LOG_LOC_PARAMS);
    }
    fclose(iconFile);

    return (numWrite == sizeof(icon_avid_to_p2));
}

static int write_clip_document(AvidMXFToP2Transfer *transfer)
{
    XMLWriter *writer;
    char filename[FILENAME_MAX];
    char globalClipIDString[128];
    char timestampString[128];
    char buffer[256];
    int i;
    Timecode timecode;

    globalClipIDString[0] = 0;
    for (i = 0; i < 32; i++)
    {
        mxf_snprintf(&globalClipIDString[i*2], sizeof(globalClipIDString) - i * 2, "%02x",
                     ((uint8_t*)&transfer->globalClipID)[i]);
    }

    mxf_snprintf(timestampString, sizeof(timestampString), "%04d-%02u-%02uT%02u:%02u:%02u+00:00",
        transfer->now.year, transfer->now.month, transfer->now.day, transfer->now.hour,
        transfer->now.min, transfer->now.sec);

    mxf_snprintf(filename, sizeof(filename), "%s%s%s%s%s%s%s.XML",
        transfer->p2path, g_fileSeparator,
        g_contentsDirname, g_fileSeparator,
        g_clipDirname, g_fileSeparator,
        transfer->clipName);

    /* Note: the XML writer must use DOS line endings */
    if (!xml_writer_open(filename, &writer))
    {
        mxf_log_error("Failed to open clip xml document '%s'" LOG_LOC_FORMAT, filename, LOG_LOC_PARAMS);
        return 0;
    }

    CHK_OFAIL(xml_writer_element_start(writer, "P2Main"));
    CHK_OFAIL(xml_writer_attribute(writer, "xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance"));

    CHK_OFAIL(xml_writer_element_start(writer, "ClipContent"));

    CHK_OFAIL(xml_writer_element_start(writer, "ClipName"));
    CHK_OFAIL(xml_writer_character_data(writer, transfer->clipName));
    CHK_OFAIL(xml_writer_element_end(writer, "ClipName"));

    CHK_OFAIL(xml_writer_element_start(writer, "GlobalClipID"));
    CHK_OFAIL(xml_writer_character_data(writer, globalClipIDString));
    CHK_OFAIL(xml_writer_element_end(writer, "GlobalClipID"));

    CHK_OFAIL(xml_writer_element_start(writer, "Duration"));
    mxf_snprintf(buffer, sizeof(buffer), "%" PRId64, transfer->duration);
    CHK_OFAIL(xml_writer_character_data(writer, buffer));
    CHK_OFAIL(xml_writer_element_end(writer, "Duration"));

    CHK_OFAIL(xml_writer_element_start(writer, "EditUnit"));
    mxf_snprintf(buffer, sizeof(buffer), "%d/%d", transfer->editUnit.numerator, transfer->editUnit.denominator);
    CHK_OFAIL(xml_writer_character_data(writer, buffer));
    CHK_OFAIL(xml_writer_element_end(writer, "EditUnit"));


    CHK_OFAIL(xml_writer_element_start(writer, "EssenceList"));

    /* start with video */
    if (transfer->pictureOutputIndex >= 0)
    {
        P2MXFFile *output = &transfer->outputs[transfer->pictureOutputIndex];

        CHK_OFAIL(xml_writer_element_start(writer, "Video"));
        CHK_OFAIL(xml_writer_attribute(writer, "ValidAudioFlag", "false"));

        CHK_OFAIL(xml_writer_element_start(writer, "VideoFormat"));
        CHK_OFAIL(xml_writer_character_data(writer, "MXF"));
        CHK_OFAIL(xml_writer_element_end(writer, "VideoFormat"));

        CHK_OFAIL(xml_writer_element_start(writer, "Codec"));
        CHK_OFAIL(xml_writer_character_data(writer, output->codecString));
        CHK_OFAIL(xml_writer_element_end(writer, "Codec"));

        CHK_OFAIL(xml_writer_element_start(writer, "FrameRate"));
        if (output->frameRate.numerator != 25) /* NTSC */
        {
            if (transfer->dropFrameFlag)
            {
                CHK_OFAIL(xml_writer_attribute(writer, "DropFrameFlag", "true"));
            }
            else
            {
                CHK_OFAIL(xml_writer_attribute(writer, "DropFrameFlag", "false"));
            }
        }
        CHK_OFAIL(xml_writer_character_data(writer, output->frameRateString));
        CHK_OFAIL(xml_writer_element_end(writer, "FrameRate"));

        if (output->frameRate.numerator == 25)
        {
            convert_frames_to_timecode(transfer->timecodeStart, 25, 0, &timecode);
        }
        else
        {
            convert_frames_to_timecode(transfer->timecodeStart, 30, transfer->dropFrameFlag, &timecode);
        }
        CHK_OFAIL(xml_writer_element_start(writer, "StartTimecode"));
        mxf_snprintf(buffer, sizeof(buffer), "%02u:%02u:%02u:%02u", timecode.hour, timecode.min, timecode.sec, timecode.frame);
        CHK_OFAIL(xml_writer_character_data(writer, buffer));
        CHK_OFAIL(xml_writer_element_end(writer, "StartTimecode"));

        CHK_OFAIL(xml_writer_element_start(writer, "StartBinaryGroup"));
        CHK_OFAIL(xml_writer_character_data(writer, "00000000"));
        CHK_OFAIL(xml_writer_element_end(writer, "StartBinaryGroup"));

        CHK_OFAIL(xml_writer_element_start(writer, "AspectRatio"));
        mxf_snprintf(buffer, sizeof(buffer), "%d:%d", output->aspectRatio.numerator, output->aspectRatio.denominator);
        CHK_OFAIL(xml_writer_character_data(writer, buffer));
        CHK_OFAIL(xml_writer_element_end(writer, "AspectRatio"));

        CHK_OFAIL(xml_writer_element_start(writer, "VideoIndex"));
        CHK_OFAIL(xml_writer_element_start(writer, "StartByteOffset"));
        mxf_snprintf(buffer, sizeof(buffer), "%" PRId64, output->startByteOffset);
        CHK_OFAIL(xml_writer_character_data(writer, buffer));
        CHK_OFAIL(xml_writer_element_end(writer, "StartByteOffset"));
        CHK_OFAIL(xml_writer_element_start(writer, "DataSize"));
        mxf_snprintf(buffer, sizeof(buffer), "%" PRId64, output->dataSize);
        CHK_OFAIL(xml_writer_character_data(writer, buffer));
        CHK_OFAIL(xml_writer_element_end(writer, "DataSize"));
        CHK_OFAIL(xml_writer_element_end(writer, "VideoIndex"));

        CHK_OFAIL(xml_writer_element_end(writer, "Video"));
    }

    /* the audios */
    for (i = 0; i < transfer->numInputs; i++)
    {
        P2MXFFile *output = &transfer->outputs[i];

        if (!output->isPicture)
        {
            CHK_OFAIL(xml_writer_element_start(writer, "Audio"));

            CHK_OFAIL(xml_writer_element_start(writer, "AudioFormat"));
            CHK_OFAIL(xml_writer_character_data(writer, "MXF"));
            CHK_OFAIL(xml_writer_element_end(writer, "AudioFormat"));

            CHK_OFAIL(xml_writer_element_start(writer, "SamplingRate"));
            CHK_OFAIL(output->samplingRate.numerator == 48000 && output->samplingRate.denominator == 1);
            CHK_OFAIL(xml_writer_character_data(writer, "48000"));
            CHK_OFAIL(xml_writer_element_end(writer, "SamplingRate"));

            CHK_OFAIL(xml_writer_element_start(writer, "BitsPerSample"));
            mxf_snprintf(buffer, sizeof(buffer), "%u", output->bitsPerSample);
            CHK_OFAIL(xml_writer_character_data(writer, buffer));
            CHK_OFAIL(xml_writer_element_end(writer, "BitsPerSample"));

            CHK_OFAIL(xml_writer_element_start(writer, "AudioIndex"));
            CHK_OFAIL(xml_writer_element_start(writer, "StartByteOffset"));
            mxf_snprintf(buffer, sizeof(buffer), "%" PRId64, output->startByteOffset);
            CHK_OFAIL(xml_writer_character_data(writer, buffer));
            CHK_OFAIL(xml_writer_element_end(writer, "StartByteOffset"));
            CHK_OFAIL(xml_writer_element_start(writer, "DataSize"));
            mxf_snprintf(buffer, sizeof(buffer), "%" PRId64, output->dataSize);
            CHK_OFAIL(xml_writer_character_data(writer, buffer));
            CHK_OFAIL(xml_writer_element_end(writer, "DataSize"));
            CHK_OFAIL(xml_writer_element_end(writer, "AudioIndex"));

            CHK_OFAIL(xml_writer_element_end(writer, "Audio"));
        }
    }


    CHK_OFAIL(xml_writer_element_end(writer, "EssenceList"));


    CHK_OFAIL(xml_writer_element_start(writer, "ClipMetadata"));

    CHK_OFAIL(xml_writer_element_start(writer, "UserClipName"));
    CHK_OFAIL(xml_writer_character_data(writer, globalClipIDString));
    CHK_OFAIL(xml_writer_element_end(writer, "UserClipName"));

    CHK_OFAIL(xml_writer_element_start(writer, "DataSource"));
    CHK_OFAIL(xml_writer_character_data(writer, "RENDERING"));
    CHK_OFAIL(xml_writer_element_end(writer, "DataSource"));

    CHK_OFAIL(xml_writer_element_start(writer, "Access"));
    CHK_OFAIL(xml_writer_element_start(writer, "CreationDate"));
    CHK_OFAIL(xml_writer_character_data(writer, timestampString));
    CHK_OFAIL(xml_writer_element_end(writer, "CreationDate"));
    CHK_OFAIL(xml_writer_element_start(writer, "LastUpdateDate"));
    CHK_OFAIL(xml_writer_character_data(writer, timestampString));
    CHK_OFAIL(xml_writer_element_end(writer, "LastUpdateDate"));
    CHK_OFAIL(xml_writer_element_end(writer, "Access"));

    CHK_OFAIL(xml_writer_element_start(writer, "Thumbnail"));
    CHK_OFAIL(xml_writer_element_start(writer, "FrameOffset"));
    CHK_OFAIL(xml_writer_character_data(writer, "0"));
    CHK_OFAIL(xml_writer_element_end(writer, "FrameOffset"));
    CHK_OFAIL(xml_writer_element_start(writer, "ThumbnailFormat"));
    CHK_OFAIL(xml_writer_character_data(writer, "BMP"));
    CHK_OFAIL(xml_writer_element_end(writer, "ThumbnailFormat"));
    CHK_OFAIL(xml_writer_element_start(writer, "Width"));
    CHK_OFAIL(xml_writer_character_data(writer, "80"));
    CHK_OFAIL(xml_writer_element_end(writer, "Width"));
    CHK_OFAIL(xml_writer_element_start(writer, "Height"));
    CHK_OFAIL(xml_writer_character_data(writer, "60"));
    CHK_OFAIL(xml_writer_element_end(writer, "Height"));
    CHK_OFAIL(xml_writer_element_end(writer, "Thumbnail"));

    CHK_OFAIL(xml_writer_element_end(writer, "ClipMetadata"));

    CHK_OFAIL(xml_writer_element_end(writer, "ClipContent"));

    CHK_OFAIL(xml_writer_element_end(writer, "P2Main"));

    xml_writer_close(&writer);
    return 1;

fail:
    fprintf(stderr, "Failed to write clip document\n");
    xml_writer_close(&writer);
    return 0;
}


static int create_p2_clipname(AvidMXFToP2Transfer *transfer)
{
    FILE *lastClipFile;
    char filename[FILENAME_MAX];
    char lastClipName[256];
    int xxxx;
    uint16_t yy;
    uint8_t yy1, yy2;
    int i;
    const uint8_t *globalClipIDPtr;
    int pastLimitAlready;

    mxf_snprintf(filename, sizeof(filename), "%s%s%s", transfer->p2path, g_fileSeparator, g_lastClipFilename);

    if ((lastClipFile = fopen(filename, "r+b")) == NULL)
    {
        mxf_log_warn("Creating new last clip file '%s'\n", filename);
        if ((lastClipFile = fopen(filename, "w+b")) == NULL)
        {
            mxf_log_error("Failed to open last clip file '%s'" LOG_LOC_FORMAT, filename, LOG_LOC_PARAMS);
            goto fail;
        }
        if (fprintf(lastClipFile, "000000\n1.0\n2\n") < 0)
        {
            mxf_log_error("Failed to initialise last clip file '%s'" LOG_LOC_FORMAT, filename, LOG_LOC_PARAMS);
            goto fail;
        }
        if (fseek(lastClipFile, 0, SEEK_SET) != 0)
        {
            mxf_log_error("Failed to seek to start of last clip file '%s'" LOG_LOC_FORMAT, filename, LOG_LOC_PARAMS);
            goto fail;
        }
    }

    /* get the number from the last clip name and increment */
    if (fscanf(lastClipFile, "%s", lastClipName) != 1 ||
        strlen(lastClipName) != 6)
    {
        mxf_log_error("Failed to read last clip name" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
        goto fail;
    }

    xxxx = (lastClipName[0] - '0') * 1000 + (lastClipName[1] - '0') * 100 +
        (lastClipName[2] - '0') * 10 + (lastClipName[3] - '0');

    if (xxxx < 0 || xxxx > 9999)
    {
        mxf_log_error("Invalid last clip name '%s'" LOG_LOC_FORMAT, lastClipName, LOG_LOC_PARAMS);
        goto fail;
    }

    xxxx += 1;


    /* 1. Separate material number of UMID into eight 16-bit (unsigned short) numbers.
       The 16-bit numbers are arranged as little endian. */
    /* 2. Calculate the exclusive OR (XOR) of every bit of each 16-bit number. */
    globalClipIDPtr = (uint8_t*)&transfer->globalClipID;
    yy = 0x0000;
    for (i = 0; i < 16; i++)
    {
        yy ^= ((uint16_t)(globalClipIDPtr[2*i + 1]) << 8) |
            (uint16_t)(globalClipIDPtr[2*i]);
    }

    /* 3. Calculate the remainder given by dividing the result of step 2
       (XOR of every bits of each 16-bit number) by 1291 in decimal representation. */
    yy %= 1291;

    /* 4. Express the remainder in base of 36. If the value is more than 9,
       it is expressed as A-Z or a-z. */
    yy1 = yy / 36;
    yy2 = yy % 36;
    if (yy1 <= 9)
    {
        yy1 = '0' + yy1;
    }
    else
    {
        yy1 = 'A' + yy1 - 10;
    }
    if (yy2 <= 9)
    {
        yy2 = '0' + yy2;
    }
    else
    {
        yy2 = 'A' + yy2 - 10;
    }


    /* increment xxxx until there is no existing clip with the name */
    pastLimitAlready = 0;
    while (1)
    {
        FILE *file = NULL;

        mxf_snprintf(transfer->clipName, sizeof(transfer->clipName), "%04u%c%c", xxxx, yy1, yy2);

        mxf_snprintf(filename, sizeof(filename), "%s%s%s%s%s%s%s.XML",
                            transfer->p2path, g_fileSeparator,
                            g_contentsDirname, g_fileSeparator,
                            g_clipDirname, g_fileSeparator,
                            transfer->clipName);
        if ((file = fopen(filename, "r")) == NULL)
        {
            break;
        }
        fclose(file);

        xxxx++;

        if (xxxx > 9999)
        {
            if (pastLimitAlready)
            {
                mxf_log_error("No unique clip name could be created" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
                goto fail;
            }
            xxxx = 0;
            pastLimitAlready = 1;
        }
    }

    /* update last clip */
    if (fseek(lastClipFile, 0, SEEK_SET) != 0)
    {
        mxf_log_error("Failed to seek to start of last clip file" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
        goto fail;
    }
    if (fprintf(lastClipFile, "%s", transfer->clipName) < 0)
    {
        mxf_log_error("Failed to write last clip name" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
        goto fail;
    }

    fclose(lastClipFile);
    return 1;

fail:
    fclose(lastClipFile);
    return 0;
}

/* WARNING: this function must be kept in sync with the writing code */
static void estimate_total_storage_size(AvidMXFToP2Transfer *transfer)
{
    int i;
    int numVideo = 0;
    int numAudio = 0;

    transfer->totalStorageSizeEstimate = 0;

    /* audio and video files */
    for (i = 0; i < transfer->numInputs; i++)
    {
        P2MXFFile *output = &transfer->outputs[i];

        if (output->isPicture)
        {
            numVideo++;
        }
        else
        {
            numAudio++;
        }

        transfer->totalStorageSizeEstimate += g_p2_fixedStartByteOffset +
            output->essenceBytesLength +
            284; /* footer pp, index table segment, rip */
    }

    /* icon file */
    transfer->totalStorageSizeEstimate += sizeof(icon_avid_to_p2);

    /* clip file */
    transfer->totalStorageSizeEstimate += 929 + /* all but empty EssenceList */
        448 * numVideo + 295 * numAudio;
}



int prepare_transfer(char *inputFilenames[17], int numInputs,
    int64_t timecodeStart, int dropFrameFlag,
    progress_callback progress, insert_timecode_callback insertTimecode,
    AvidMXFToP2Transfer **transfer)
{
    AvidMXFToP2Transfer *newTransfer;
    uint32_t audioTrackNumber;
    int i;

    CHK_MALLOC_ORET(newTransfer, AvidMXFToP2Transfer);
    memset(newTransfer, 0, sizeof(AvidMXFToP2Transfer));

    newTransfer->insertTimecode = insertTimecode;
    newTransfer->progress = progress;
    newTransfer->timecodeStart = timecodeStart;
    newTransfer->dropFrameFlag = dropFrameFlag;
    newTransfer->pictureOutputIndex = -1;

    mxf_generate_umid(&newTransfer->globalClipID);

    /* process the input files extracting information */
    audioTrackNumber = 1;
    for (i = 0; i < numInputs; i++)
    {
        CHK_OFAIL(initialise_input_file(&newTransfer->inputs[i]));
        CHK_OFAIL(initialise_output_file(&newTransfer->outputs[i]));
        newTransfer->numInputs++;

        CHK_OFAIL(open_input_file(inputFilenames[i], &newTransfer->inputs[i]));

        CHK_OFAIL(preprocess_avid_input(newTransfer, i, i));
        if (newTransfer->outputs[i].isPicture)
        {
            newTransfer->outputs[i].materialTrackNumber = 1;
            newTransfer->outputs[i].materialTrackID = g_p2_timecodeTrackID + 1;
        }
        else
        {
            newTransfer->outputs[i].materialTrackNumber = audioTrackNumber;
            newTransfer->outputs[i].materialTrackID = g_p2_timecodeTrackID + 2 + audioTrackNumber - 1;
            audioTrackNumber++;
        }

        close_input_file(&newTransfer->inputs[i]);
    }

    /* set clip parameters */
    if (newTransfer->pictureOutputIndex >= 0)
    {
        newTransfer->duration = newTransfer->outputs[newTransfer->pictureOutputIndex].duration;
        newTransfer->editUnit.numerator = newTransfer->outputs[newTransfer->pictureOutputIndex].editRate.denominator;
        newTransfer->editUnit.denominator = newTransfer->outputs[newTransfer->pictureOutputIndex].editRate.numerator;
    }
    else
    {
        newTransfer->duration = newTransfer->outputs[0].duration;
        newTransfer->editUnit = newTransfer->outputs[0].samplingRate;
        newTransfer->editUnit.numerator = newTransfer->outputs[0].editRate.denominator;
        newTransfer->editUnit.denominator = newTransfer->outputs[0].editRate.numerator;
    }

    /* estimate the total size in bytes for storage */
    estimate_total_storage_size(newTransfer);

    *transfer = newTransfer;
    return 1;

fail:
    free_transfer(&newTransfer);
    return 0;
}

int transfer_avid_mxf_to_p2(const char *p2path, AvidMXFToP2Transfer *transfer, int *isComplete)
{
    int i;
    char outputFilename[FILENAME_MAX];
    uint64_t totalBytes;

    transfer->percentCompleted = 0.0f;
    transfer->lastCallPercentCompleted = -6.0f;
    CALL_PROGRESS();

    totalBytes = 0;
    for (i = 0; i < transfer->numInputs; i++)
    {
        totalBytes += transfer->outputs[i].essenceBytesLength;
    }
    for (i = 0; i < transfer->numInputs; i++)
    {
        /* 95% is essence data, 5% is other stuff */
        transfer->outputs[i].percentCompletedContribution = (float)
            (95 * (double)transfer->outputs[i].essenceBytesLength / totalBytes);
    }


    if (transfer->numInputs == 0)
    {
        *isComplete = 1;
        return 1;
    }

    transfer->p2path = p2path;
    CHK_ORET(create_p2_clipname(transfer));
    mxf_get_timestamp_now(&transfer->now);


    /* create the output file and transfer the essence */
    for (i = 0; i < transfer->numInputs; i++)
    {
        if (transfer->outputs[i].isPicture)
        {
            mxf_snprintf(outputFilename, sizeof(outputFilename), "%s%s%s%s%s%s%s.MXF",
                transfer->p2path, g_fileSeparator,
                g_contentsDirname, g_fileSeparator,
                g_videoDirname, g_fileSeparator,
                transfer->clipName);
        }
        else
        {
            mxf_snprintf(outputFilename, sizeof(outputFilename), "%s%s%s%s%s%s%s%02u.MXF",
                transfer->p2path, g_fileSeparator,
                g_contentsDirname, g_fileSeparator,
                g_audioDirname, g_fileSeparator,
                transfer->clipName, transfer->outputs[i].materialTrackNumber - 1);
        }

        CHK_OFAIL(open_input_file(NULL, &transfer->inputs[i]));
        CHK_OFAIL(open_output_file(outputFilename, &transfer->outputs[i]));

        CHK_OFAIL(transfer_to_p2(transfer, i, i));
        CALL_PROGRESS();
    }

    /* write the BMP icon */
    CHK_OFAIL(write_icon_bmp(transfer));

    /* write the clip xml document */
    CHK_OFAIL(write_clip_document(transfer));

    for (i = 0; i < transfer->numInputs; i++)
    {
        close_output_file(&transfer->outputs[i]);
        close_input_file(&transfer->inputs[i]);
    }

    transfer->percentCompleted = 100.0f;
    CALL_PROGRESS();

    *isComplete = 1;
    return 1;

fail:
    for (i = 0; i < transfer->numInputs; i++)
    {
        close_output_file(&transfer->outputs[i]);
        close_input_file(&transfer->inputs[i]);
    }
    return 0;
}

void free_transfer(AvidMXFToP2Transfer **transfer)
{
    int i;

    if (*transfer == NULL)
    {
        return;
    }

    for (i = 0; i < (*transfer)->numInputs; i++)
    {
        clear_input_file(&(*transfer)->inputs[i]);
        clear_output_file(&(*transfer)->outputs[i]);
    }

    SAFE_FREE(*transfer);
}

