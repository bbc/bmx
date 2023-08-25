/*
 * Write video and audio to MXF files supported by Avid editing software
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

#include <mxf/mxf.h>
#include <mxf/mxf_avid.h>
#include <mxf/mxf_macros.h>
#include "write_avid_mxf.h"


/* TODO: legacy switch (Note frameLayout and subsamplings) */

/* size of array used to hold stream offsets for the MJPEG index tables */
/* a new array is added to the list each time this number is exceeded */
/* Note: this number does not take into account the size limits (16-bit) imposed by the local tag
   encoding used in the index table. Avid index tables ignore this limit and instead
   use the array header (32-bit) to provide the length */
#define MJPEG_STREAM_OFFSETS_ARRAY_SIZE    65535

#define MAX_TRACKS      17

#define IS_PICTURE(essenceType)     (essenceType != PCM)



typedef struct
{
    uint64_t *offsets;
    uint32_t len;
} MJPEGOffsetsArray;

typedef struct
{
    MXFMetadataItem *item;
    mxfRational editRate;
    uint32_t materialTrackID;
} TrackDurationItem;

typedef struct
{
    char *filename;
    MXFFile *mxfFile;

    EssenceType essenceType;

    uint32_t materialTrackID;
    mxfUMID fileSourcePackageUID;


    /* used when using the start..., write..., end... functions to write a frame */
    uint32_t sampleDataSize;

    /* audio and video */
    mxfUL essenceContainerLabel;
    mxfKey essenceElementKey;
    uint8_t essenceElementLLen;
    mxfLength duration;
    mxfRational sampleRate;
    uint32_t editUnitByteCount;
    uint32_t sourceTrackNumber;
    uint64_t essenceLength;
    mxfUL pictureDataDef;
    mxfUL soundDataDef;
    mxfUL timecodeDataDef;
    mxfUL dmDataDef;


    /* video only */
    mxfUL cdciEssenceContainerLabel;
    uint32_t frameSize;
    int32_t resolutionID;
    mxfRational imageAspectRatio;
    mxfUL pictureEssenceCoding;
    uint32_t storedHeight;
    uint32_t storedWidth;
    uint32_t sampledHeight;
    uint32_t sampledWidth;
    uint32_t displayHeight;
    uint32_t displayWidth;
    uint32_t displayYOffset;
    uint32_t displayXOffset;
    int32_t videoLineMap[2];
    int videoLineMapLen;
    uint32_t horizSubsampling;
    uint32_t vertSubsampling;
    uint8_t frameLayout;
    uint8_t componentDepth;
    uint8_t colorSiting;
    uint32_t imageAlignmentOffset;
    uint32_t imageStartOffset;
    mxfUL codingEquationsLabel;


    /* audio only */
    mxfRational samplingRate;
    uint32_t bitsPerSample;
    uint16_t blockAlign;
    uint32_t avgBps;
    int locked;
    int audioRefLevel;
    int dialNorm;
    int sequenceOffset;


    int64_t headerMetadataFilePos;

    MXFDataModel *dataModel;
    MXFList *partitions;
    MXFHeaderMetadata *headerMetadata;
    MXFIndexTableSegment *indexSegment;
    MXFEssenceElement *essenceElement;

    /* Avid MJPEG index table frame offsets */
    MXFList mjpegFrameOffsets;
    MJPEGOffsetsArray *currentMJPEGOffsetsArray;
    uint64_t prevFrameOffset;

    /* Avid uncompressed static essence data */
    uint8_t *vbiData;
    int64_t vbiSize;
    uint8_t *startOffsetData;

    /* these are references and should not be free'd */
    MXFPartition *headerPartition;
    MXFPartition *bodyPartition;
    MXFPartition *footerPartition;
    MXFMetadataSet *prefaceSet;
    MXFMetadataSet *dictionarySet;
    MXFMetadataSet *identSet;
    MXFMetadataSet *contentStorageSet;
    MXFMetadataSet *materialPackageSet;
    MXFMetadataSet *sourcePackageSet;
    MXFMetadataSet *sourcePackageTrackSet;
    MXFMetadataSet *materialPackageTrackSet;
    MXFMetadataSet *sequenceSet;
    MXFMetadataSet *sourceClipSet;
    MXFMetadataSet *dmSet;
    MXFMetadataSet *dmFrameworkSet;
    MXFMetadataSet *timecodeComponentSet;
    MXFMetadataSet *essContainerDataSet;
    MXFMetadataSet *multipleDescriptorSet;
    MXFMetadataSet *cdciDescriptorSet;
    MXFMetadataSet *bwfDescriptorSet;
    MXFMetadataSet *videoMaterialPackageTrackSet;
    MXFMetadataSet *videoSequenceSet;
    MXFMetadataSet *taggedValueSet;
    MXFMetadataSet *tapeDescriptorSet;
    MXFMetadataSet *dmTrackSet;
    MXFMetadataSet *dmSegmentSet;


    /* duration items that are updated when writing is completed */
    TrackDurationItem durationItems[MAX_TRACKS * 2 + 2];
    int numDurationItems;

    /* container duration item that are created when writing is completed */
    MXFMetadataSet *descriptorSet;
} TrackWriter;

struct AvidClipWriter
{
    TrackWriter *tracks[MAX_TRACKS];
    int numTracks;

    mxfUTF16Char *wProjectName;
    ProjectFormat projectFormat;
    int dropFrameFlag;
    int useLegacy;
    mxfRational projectEditRate;

    mxfTimestamp now;

    /* used to temporarily hold strings */
    mxfUTF16Char *wTmpString;
    mxfUTF16Char *wTmpString2;
};


/* AVID RGB values matching color names defined in enum AvidRGBColor */
static const RGBColor g_rgbColors[] =
{
    {65534, 65535, 65535}, /* white */
    {41471, 12134, 6564 }, /* red */
    {58981, 58981, 6553 }, /* yellow */
    {13107, 52428, 13107}, /* green */
    {13107, 52428, 52428}, /* cyan */
    {13107, 13107, 52428}, /* blue */
    {52428, 13107, 52428}, /* magenta */
    {0    , 0    , 0    }  /* black */
};




static const mxfUUID g_mxfIdentProductUID =
    {0x05, 0x7c, 0xd8, 0x49, 0x17, 0x8a, 0x4b, 0x88, 0xb4, 0xc7, 0x82, 0x5a, 0xf8, 0x76, 0x1b, 0x23};
static const mxfUTF16Char *g_mxfIdentCompanyName   = L"BBC Research";
static const mxfUTF16Char *g_mxfIdentProductName   = L"Avid MXF Writer";
static const mxfUTF16Char *g_mxfIdentVersionString = L"Beta version";


static uint32_t g_dmTrackID     = 1000;
static uint32_t g_dmTrackNumber = 1;


/* uncompressed UYVY frame size = width * height * 2 */
/* aligned frame size = frame size + (8192 - (frame size % 8192)) */
/* start offset = aligned frame size - frame size */

static const uint32_t g_uncImageAlignmentOffset  = 8192;

//static const uint32_t g_uncPALFrameSize          = 852480; /* 720*592*2 */
static const uint32_t g_uncAlignedPALFrameSize   = 860160;
static const uint32_t g_uncPALStartOffsetSize    = 7680;
static const uint32_t g_uncPALVBISize            = 720 * 16 * 2;

//static const uint32_t g_uncNTSCFrameSize         = 714240; /* 720*496*2 */
static const uint32_t g_uncAlignedNTSCFrameSize  = 720896;
static const uint32_t g_uncNTSCStartOffsetSize   = 6656;
static const uint32_t g_uncNTSCVBISize           = 720 * 10 * 2;

//static const uint32_t g_unc1080FrameSize         = 4147200; /* 1920*1080*2 */
static const uint32_t g_uncAligned1080FrameSize  = 4153344;
static const uint32_t g_unc1080StartOffsetSize   = 6144;

static const uint32_t g_unc720pFrameSize         = 1843200; /* 1280*720*2 */


static const uint32_t g_bodySID  = 1;
static const uint32_t g_indexSID = 2;

static const uint64_t g_fixedBodyPPOffset = 0x40020;



static void free_offsets_array_in_list(void *data)
{
    MJPEGOffsetsArray *offsetsArray;

    if (data == NULL)
    {
        return;
    }

    offsetsArray = (MJPEGOffsetsArray*)data;
    SAFE_FREE(offsetsArray->offsets);
    SAFE_FREE(offsetsArray);
}

static int create_avid_mjpeg_offsets_array(MXFList *mjpegFrameOffsets, MJPEGOffsetsArray **offsetsArray)
{
    MJPEGOffsetsArray *newOffsetsArray = NULL;

    CHK_MALLOC_ORET(newOffsetsArray, MJPEGOffsetsArray);
    memset(newOffsetsArray, 0, sizeof(MJPEGOffsetsArray));
    CHK_MALLOC_ARRAY_OFAIL(newOffsetsArray->offsets, uint64_t, MJPEG_STREAM_OFFSETS_ARRAY_SIZE);

    CHK_OFAIL(mxf_append_list_element(mjpegFrameOffsets, newOffsetsArray));

    *offsetsArray = newOffsetsArray;
    return 1;

fail:
    if (newOffsetsArray != NULL)
    {
        SAFE_FREE(newOffsetsArray->offsets);
    }
    SAFE_FREE(newOffsetsArray);
    return 0;
}

static int add_avid_mjpeg_offset(MXFList *mjpegFrameOffsets, uint64_t offset, MJPEGOffsetsArray **offsetsArray)
{
    if ((*offsetsArray) == NULL || (*offsetsArray)->len + 1 == MJPEG_STREAM_OFFSETS_ARRAY_SIZE)
    {
        CHK_ORET(create_avid_mjpeg_offsets_array(mjpegFrameOffsets, offsetsArray));
    }

    (*offsetsArray)->offsets[(*offsetsArray)->len] = offset;
    (*offsetsArray)->len++;

    return 1;
}

static uint32_t get_num_offsets(MXFList *mjpegFrameOffsets)
{
    MXFListIterator iter;
    uint32_t numOffsets = 0;
    MJPEGOffsetsArray *offsetsArray;

    mxf_initialise_list_iter(&iter, mjpegFrameOffsets);
    while (mxf_next_list_iter_element(&iter))
    {
        offsetsArray = (MJPEGOffsetsArray*)mxf_get_iter_element(&iter);
        numOffsets += offsetsArray->len;
    }

    return numOffsets;
}


static void free_track_writer(TrackWriter **writer)
{
    if (*writer == NULL)
    {
        return;
    }

    SAFE_FREE((*writer)->filename);

    mxf_free_index_table_segment(&(*writer)->indexSegment);
    mxf_free_file_partitions(&(*writer)->partitions);
    mxf_free_header_metadata(&(*writer)->headerMetadata);
    mxf_free_data_model(&(*writer)->dataModel);
    mxf_close_essence_element(&(*writer)->essenceElement);

    mxf_clear_list(&(*writer)->mjpegFrameOffsets);

    SAFE_FREE((*writer)->vbiData);
    SAFE_FREE((*writer)->startOffsetData);

    mxf_file_close(&(*writer)->mxfFile);

    SAFE_FREE(*writer);
}

static void free_avid_clip_writer(AvidClipWriter **clipWriter)
{
    int i;

    if (*clipWriter == NULL)
    {
        return;
    }

    SAFE_FREE((*clipWriter)->wProjectName);

    for (i = 0; i < (*clipWriter)->numTracks; i++)
    {
        free_track_writer(&(*clipWriter)->tracks[i]);
    }

    SAFE_FREE((*clipWriter)->wTmpString);
    SAFE_FREE((*clipWriter)->wTmpString2);

    SAFE_FREE(*clipWriter);
}

/* Take a char *string, convert to mxfUTF16Char *in wTmpString member */
static int convert_string(AvidClipWriter *clipWriter, const char *input)
{
    mxfUTF16Char *newOutput = NULL;

    size_t len = mxf_utf8_to_utf16(NULL, input, 0);
    CHK_OFAIL(len != (size_t)(-1));

    CHK_MALLOC_ARRAY_ORET(newOutput, mxfUTF16Char, len + 1);
    memset(newOutput, 0, sizeof(mxfUTF16Char) * (len + 1));

    mxf_utf8_to_utf16(newOutput, input, len + 1);

    SAFE_FREE(clipWriter->wTmpString);
    clipWriter->wTmpString = newOutput;
    return 1;

fail:
    SAFE_FREE(newOutput);
    return 0;
}

static int get_track_writer(AvidClipWriter *clipWriter, uint32_t materialTrackID, TrackWriter **writer)
{
    int i;

    for (i = 0; i < clipWriter->numTracks; i++)
    {
        if (clipWriter->tracks[i]->materialTrackID == materialTrackID)
        {
            *writer = clipWriter->tracks[i];
            return 1;
        }
    }

    return 0;
}

static int get_file_package(TrackWriter *trackWriter, PackageDefinitions *packageDefinitions, Package **filePackage)
{
    MXFListIterator iter;
    Package *package;

    mxf_initialise_list_iter(&iter, &packageDefinitions->fileSourcePackages);
    while (mxf_next_list_iter_element(&iter))
    {
        package = mxf_get_iter_element(&iter);

        if (mxf_equals_umid(&trackWriter->fileSourcePackageUID, &package->uid))
        {
            *filePackage = package;
            return 1;
        }
    }

    return 0;
}

static int create_header_metadata(AvidClipWriter *clipWriter, PackageDefinitions *packageDefinitions,
                                  Package *filePackage, TrackWriter *writer)
{
    uint8_t *arrayElement;
    mxfUUID thisGeneration;
    uint16_t roundedTimecodeBase;
    Package *materialPackage = packageDefinitions->materialPackage;
    Package *tapePackage = packageDefinitions->tapeSourcePackage;
    MXFListIterator iter;
    Track *track;
    Locator *locator;
    int i;
    uint32_t maxTrackID;
    int64_t tapeLen;
    UserComment *userComment;
    MXFMetadataSet *metaDictSet = NULL;
    int32_t describedTrackID = -1;
    int haveDescribedTrackID = 0;


    mxf_generate_uuid(&thisGeneration);
    mxf_get_timestamp_now(&clipWriter->now);

    roundedTimecodeBase = (uint16_t)((float)clipWriter->projectEditRate.numerator /
                                            clipWriter->projectEditRate.denominator + 0.5);


    /* load the data model, plus AVID extensions */

    if (writer->dataModel == NULL)
    {
        CHK_ORET(mxf_load_data_model(&writer->dataModel));
        CHK_ORET(mxf_avid_load_extensions(writer->dataModel));
        CHK_ORET(mxf_finalise_data_model(writer->dataModel));
    }


    /* delete existing header metadata */

    if (writer->headerMetadata != NULL)
    {
        mxf_free_header_metadata(&writer->headerMetadata);
    }


    /* create the header metadata and associated primer pack */

    CHK_ORET(mxf_create_header_metadata(&writer->headerMetadata, writer->dataModel));


    /* create the Avid meta-dictionary */

    CHK_ORET(mxf_avid_create_default_metadictionary(writer->headerMetadata, &metaDictSet));


    /* Preface */
    CHK_ORET(mxf_create_set(writer->headerMetadata, &MXF_SET_K(Preface), &writer->prefaceSet));
    CHK_ORET(mxf_set_int16_item(writer->prefaceSet, &MXF_ITEM_K(Preface, ByteOrder), 0x4949)); /* little-endian */
    CHK_ORET(mxf_set_uint32_item(writer->prefaceSet, &MXF_ITEM_K(Preface, ObjectModelVersion), 0x00000001));
    CHK_ORET(mxf_set_version_type_item(writer->prefaceSet, &MXF_ITEM_K(Preface, Version), MXF_PREFACE_VER(1, 1))); /* AAF SDK version */
    CHK_ORET(mxf_set_timestamp_item(writer->prefaceSet, &MXF_ITEM_K(Preface, LastModifiedDate), &clipWriter->now));
    CHK_ORET(mxf_set_ul_item(writer->prefaceSet, &MXF_ITEM_K(Preface, OperationalPattern), &MXF_OP_L(atom, NTracks_NSourceClips)));
    CHK_ORET(mxf_alloc_array_item_elements(writer->prefaceSet, &MXF_ITEM_K(Preface, EssenceContainers), mxfUL_extlen, 1, &arrayElement));
    mxf_set_ul(&writer->essenceContainerLabel, arrayElement);
    if (clipWriter->wProjectName != NULL)
    {
        CHK_ORET(mxf_set_utf16string_item(writer->prefaceSet, &MXF_ITEM_K(Preface, ProjectName), clipWriter->wProjectName));
    }
    CHK_ORET(mxf_set_rational_item(writer->prefaceSet, &MXF_ITEM_K(Preface, ProjectEditRate), &clipWriter->projectEditRate));
    CHK_ORET(mxf_set_umid_item(writer->prefaceSet, &MXF_ITEM_K(Preface, MasterMobID), &materialPackage->uid));
    CHK_ORET(mxf_set_umid_item(writer->prefaceSet, &MXF_ITEM_K(Preface, EssenceFileMobID), &filePackage->uid));
    CHK_ORET(mxf_alloc_array_item_elements(writer->prefaceSet, &MXF_ITEM_K(Preface, DMSchemes), mxfUL_extlen, 1, &arrayElement));
    mxf_set_ul(&MXF_DM_L(LegacyDMS1), arrayElement);


    /* Preface - Dictionary */
    CHK_ORET(mxf_avid_create_default_dictionary(writer->headerMetadata, &writer->dictionarySet));
    CHK_ORET(mxf_set_strongref_item(writer->prefaceSet, &MXF_ITEM_K(Preface, Dictionary), writer->dictionarySet));


    /* Preface - ContentStorage */
    CHK_ORET(mxf_create_set(writer->headerMetadata, &MXF_SET_K(ContentStorage), &writer->contentStorageSet));
    CHK_ORET(mxf_set_strongref_item(writer->prefaceSet, &MXF_ITEM_K(Preface, ContentStorage), writer->contentStorageSet));


    /* Preface - ContentStorage - MaterialPackage */
    CHK_ORET(mxf_create_set(writer->headerMetadata, &MXF_SET_K(MaterialPackage), &writer->materialPackageSet));
    CHK_ORET(mxf_add_array_item_strongref(writer->contentStorageSet, &MXF_ITEM_K(ContentStorage, Packages), writer->materialPackageSet));
    CHK_ORET(mxf_set_umid_item(writer->materialPackageSet, &MXF_ITEM_K(GenericPackage, PackageUID), &materialPackage->uid));
    CHK_ORET(mxf_set_timestamp_item(writer->materialPackageSet, &MXF_ITEM_K(GenericPackage, PackageCreationDate), &materialPackage->creationDate));
    CHK_ORET(mxf_set_timestamp_item(writer->materialPackageSet, &MXF_ITEM_K(GenericPackage, PackageModifiedDate), &materialPackage->creationDate));
    if (materialPackage->name != NULL)
    {
        CHK_ORET(convert_string(clipWriter, materialPackage->name));
        CHK_ORET(mxf_set_utf16string_item(writer->materialPackageSet, &MXF_ITEM_K(GenericPackage, Name), clipWriter->wTmpString));
    }
    /* TODO: what are ConvertFrameRate and AppCode for? */
    CHK_ORET(mxf_set_boolean_item(writer->materialPackageSet, &MXF_ITEM_K(GenericPackage, ConvertFrameRate), 0));
    CHK_ORET(mxf_set_int32_item(writer->materialPackageSet, &MXF_ITEM_K(GenericPackage, AppCode), 0x00000007));

    mxf_initialise_list_iter(&iter, &materialPackage->tracks);
    while (mxf_next_list_iter_element(&iter))
    {
        track = (Track*)mxf_get_iter_element(&iter);

        /* get picture track id or first audio track id for locators below */
        if (track->isPicture && !haveDescribedTrackID)
        {
            describedTrackID = track->id;
            haveDescribedTrackID = 1;
        }
        else if (describedTrackID < 0)
        {
            describedTrackID = track->id;
        }


        /* Preface - ContentStorage - MaterialPackage - Timeline Track */
        CHK_ORET(mxf_create_set(writer->headerMetadata, &MXF_SET_K(Track), &writer->materialPackageTrackSet));
        CHK_ORET(mxf_add_array_item_strongref(writer->materialPackageSet, &MXF_ITEM_K(GenericPackage, Tracks), writer->materialPackageTrackSet));
        CHK_ORET(mxf_set_uint32_item(writer->materialPackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackID), track->id));
        if (track->name != NULL)
        {
            CHK_ORET(convert_string(clipWriter, track->name));
            CHK_ORET(mxf_set_utf16string_item(writer->materialPackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackName), clipWriter->wTmpString));
        }
        CHK_ORET(mxf_set_uint32_item(writer->materialPackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackNumber), track->number));
        CHK_ORET(mxf_set_rational_item(writer->materialPackageTrackSet, &MXF_ITEM_K(Track, EditRate), &track->editRate));
        CHK_ORET(mxf_set_position_item(writer->materialPackageTrackSet, &MXF_ITEM_K(Track, Origin), 0));

        /* Preface - ContentStorage - MaterialPackage - Timeline Track - Sequence */
        CHK_ORET(mxf_create_set(writer->headerMetadata, &MXF_SET_K(Sequence), &writer->sequenceSet));
        CHK_ORET(mxf_set_strongref_item(writer->materialPackageTrackSet, &MXF_ITEM_K(GenericTrack, Sequence), writer->sequenceSet));
        if (track->isPicture)
        {
            CHK_ORET(mxf_set_ul_item(writer->sequenceSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &writer->pictureDataDef));
        }
        else
        {
            CHK_ORET(mxf_set_ul_item(writer->sequenceSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &writer->soundDataDef));
        }
        CHK_ORET(mxf_set_length_item(writer->sequenceSet, &MXF_ITEM_K(StructuralComponent, Duration), -1)); /* to be filled in */

        CHK_ORET(mxf_get_item(writer->sequenceSet, &MXF_ITEM_K(StructuralComponent, Duration), &writer->durationItems[writer->numDurationItems].item));
        writer->durationItems[writer->numDurationItems].editRate = track->editRate;
        writer->durationItems[writer->numDurationItems].materialTrackID = track->id;
        writer->numDurationItems++;


        /* Preface - ContentStorage - MaterialPackage - Timeline Track - Sequence - SourceClip */
        CHK_ORET(mxf_create_set(writer->headerMetadata, &MXF_SET_K(SourceClip), &writer->sourceClipSet));
        CHK_ORET(mxf_add_array_item_strongref(writer->sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), writer->sourceClipSet));
        if (track->isPicture)
        {
            CHK_ORET(mxf_set_ul_item(writer->sourceClipSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &writer->pictureDataDef));
        }
        else
        {
            CHK_ORET(mxf_set_ul_item(writer->sourceClipSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &writer->soundDataDef));
        }
        CHK_ORET(mxf_set_length_item(writer->sourceClipSet, &MXF_ITEM_K(StructuralComponent, Duration), -1)); /* to be filled in */
        CHK_ORET(mxf_set_position_item(writer->sourceClipSet, &MXF_ITEM_K(SourceClip, StartPosition), track->startPosition));
        CHK_ORET(mxf_set_umid_item(writer->sourceClipSet, &MXF_ITEM_K(SourceClip, SourcePackageID), &track->sourcePackageUID));
        CHK_ORET(mxf_set_uint32_item(writer->sourceClipSet, &MXF_ITEM_K(SourceClip, SourceTrackID), track->sourceTrackID));

        CHK_ORET(mxf_get_item(writer->sourceClipSet, &MXF_ITEM_K(StructuralComponent, Duration), &writer->durationItems[writer->numDurationItems].item));
        writer->durationItems[writer->numDurationItems].editRate = track->editRate;
        writer->durationItems[writer->numDurationItems].materialTrackID = track->id;
        writer->numDurationItems++;

    }

    /* add a DMTrack if there are timed comments */
    if (mxf_get_list_length(&packageDefinitions->locators) > 0)
    {
        /* Preface - ContentStorage - MaterialPackage - (DM) Event Track */
        CHK_ORET(mxf_create_set(writer->headerMetadata, &MXF_SET_K(EventTrack), &writer->dmTrackSet));
        CHK_ORET(mxf_add_array_item_strongref(writer->materialPackageSet, &MXF_ITEM_K(GenericPackage, Tracks), writer->dmTrackSet));
        CHK_ORET(mxf_set_uint32_item(writer->dmTrackSet, &MXF_ITEM_K(GenericTrack, TrackID), g_dmTrackID));
        /* EventMobSlot in Avid AAF file has no name */
        CHK_ORET(mxf_set_uint32_item(writer->dmTrackSet, &MXF_ITEM_K(GenericTrack, TrackNumber), g_dmTrackNumber));
        CHK_ORET(mxf_set_rational_item(writer->dmTrackSet, &MXF_ITEM_K(EventTrack, EventEditRate), &packageDefinitions->locatorEditRate));
        /* not setting EventOrigin because this results in an error in Avid MediaComposer 3.0 */

        /* Preface - ContentStorage - MaterialPackage - (DM) Event Track - (DM) Sequence */
        CHK_ORET(mxf_create_set(writer->headerMetadata, &MXF_SET_K(Sequence), &writer->sequenceSet));
        CHK_ORET(mxf_set_strongref_item(writer->dmTrackSet, &MXF_ITEM_K(GenericTrack, Sequence), writer->sequenceSet));
        CHK_ORET(mxf_set_ul_item(writer->sequenceSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &writer->dmDataDef));

        mxf_initialise_list_iter(&iter, &packageDefinitions->locators);
        while (mxf_next_list_iter_element(&iter))
        {
            locator = (Locator*)mxf_get_iter_element(&iter);

            /* Preface - ContentStorage - MaterialPackage - (DM) Event Track - (DM) Sequence - DMSegment */
            CHK_ORET(mxf_create_set(writer->headerMetadata, &MXF_SET_K(DMSegment), &writer->dmSegmentSet));
            CHK_ORET(mxf_add_array_item_strongref(writer->sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), writer->dmSegmentSet));
            CHK_ORET(mxf_set_ul_item(writer->dmSegmentSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &writer->dmDataDef));
            /* no duration set in Avid AAF file */
            CHK_ORET(mxf_set_position_item(writer->dmSegmentSet, &MXF_ITEM_K(DMSegment, EventStartPosition), locator->position));
            if (locator->comment != NULL)
            {
                CHK_ORET(convert_string(clipWriter, locator->comment));
                CHK_ORET(mxf_set_utf16string_item(writer->dmSegmentSet, &MXF_ITEM_K(DMSegment, EventComment), clipWriter->wTmpString));
            }
            CHK_ORET(mxf_avid_set_rgb_color_item(writer->dmSegmentSet, &MXF_ITEM_K(DMSegment, CommentMarkerColor), &g_rgbColors[locator->color - AVID_WHITE]));

            if (describedTrackID >= 0)
            {
                CHK_ORET(mxf_alloc_array_item_elements(writer->dmSegmentSet, &MXF_ITEM_K(DMSegment, TrackIDs), 4, 1, &arrayElement));
                mxf_set_uint32(describedTrackID, arrayElement);
            }
        }
    }

    /* attach a project name attribute to the material package */
    if (clipWriter->wProjectName != NULL)
    {
        CHK_ORET(mxf_avid_attach_mob_attribute(writer->headerMetadata, writer->materialPackageSet, L"_PJ", clipWriter->wProjectName));
    }

    /* attach user comments to the material package*/
    mxf_initialise_list_iter(&iter, &packageDefinitions->userComments);
    while (mxf_next_list_iter_element(&iter))
    {
        userComment = (UserComment*)mxf_get_iter_element(&iter);

        if (userComment->name != NULL && userComment->value != NULL)
        {
            CHK_ORET(convert_string(clipWriter, userComment->name));
            SAFE_FREE(clipWriter->wTmpString2);
            clipWriter->wTmpString2 = clipWriter->wTmpString;
            clipWriter->wTmpString = NULL;
            CHK_ORET(convert_string(clipWriter, userComment->value));
            CHK_ORET(mxf_avid_attach_user_comment(writer->headerMetadata, writer->materialPackageSet,
                                                  clipWriter->wTmpString2, clipWriter->wTmpString));
        }
    }


    CHK_ORET(mxf_get_list_length(&filePackage->tracks) == 1);
    track = (Track*)mxf_get_list_element(&filePackage->tracks, 0);

    /* Preface - ContentStorage - SourcePackage */
    CHK_ORET(mxf_create_set(writer->headerMetadata, &MXF_SET_K(SourcePackage), &writer->sourcePackageSet));
    CHK_ORET(mxf_add_array_item_strongref(writer->contentStorageSet, &MXF_ITEM_K(ContentStorage, Packages), writer->sourcePackageSet));
    CHK_ORET(mxf_set_umid_item(writer->sourcePackageSet, &MXF_ITEM_K(GenericPackage, PackageUID), &filePackage->uid));
    CHK_ORET(mxf_set_timestamp_item(writer->sourcePackageSet, &MXF_ITEM_K(GenericPackage, PackageCreationDate), &filePackage->creationDate));
    CHK_ORET(mxf_set_timestamp_item(writer->sourcePackageSet, &MXF_ITEM_K(GenericPackage, PackageModifiedDate), &filePackage->creationDate));
    if (filePackage->name != NULL)
    {
        CHK_ORET(convert_string(clipWriter, filePackage->name));
        CHK_ORET(mxf_set_utf16string_item(writer->sourcePackageSet, &MXF_ITEM_K(GenericPackage, Name), clipWriter->wTmpString));
    }

    /* Preface - ContentStorage - SourcePackage - Timeline Track */
    CHK_ORET(mxf_create_set(writer->headerMetadata, &MXF_SET_K(Track), &writer->sourcePackageTrackSet));
    CHK_ORET(mxf_add_array_item_strongref(writer->sourcePackageSet, &MXF_ITEM_K(GenericPackage, Tracks), writer->sourcePackageTrackSet));
    CHK_ORET(mxf_set_uint32_item(writer->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackID), track->id));
    if (track->name != NULL)
    {
        CHK_ORET(convert_string(clipWriter, track->name));
        CHK_ORET(mxf_set_utf16string_item(writer->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackName), clipWriter->wTmpString));
    }
    CHK_ORET(mxf_set_uint32_item(writer->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackNumber), writer->sourceTrackNumber));
    CHK_ORET(mxf_set_rational_item(writer->sourcePackageTrackSet, &MXF_ITEM_K(Track, EditRate), &track->editRate));
    CHK_ORET(mxf_set_position_item(writer->sourcePackageTrackSet, &MXF_ITEM_K(Track, Origin), 0));

    /* Preface - ContentStorage - SourcePackage - Timeline Track - Sequence */
    CHK_ORET(mxf_create_set(writer->headerMetadata, &MXF_SET_K(Sequence), &writer->sequenceSet));
    CHK_ORET(mxf_set_strongref_item(writer->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, Sequence), writer->sequenceSet));
    if (track->isPicture)
    {
        CHK_ORET(mxf_set_ul_item(writer->sequenceSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &writer->pictureDataDef));
    }
    else
    {
        CHK_ORET(mxf_set_ul_item(writer->sequenceSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &writer->soundDataDef));
    }
    CHK_ORET(mxf_set_length_item(writer->sequenceSet, &MXF_ITEM_K(StructuralComponent, Duration), -1));  /* to be filled in */

    CHK_ORET(mxf_get_item(writer->sequenceSet, &MXF_ITEM_K(StructuralComponent, Duration), &writer->durationItems[writer->numDurationItems].item));
    writer->durationItems[writer->numDurationItems].editRate = track->editRate;
    writer->durationItems[writer->numDurationItems].materialTrackID = writer->materialTrackID;
    writer->numDurationItems++;


    /* Preface - ContentStorage - SourcePackage - Timeline Track - Sequence - SourceClip */
    CHK_ORET(mxf_create_set(writer->headerMetadata, &MXF_SET_K(SourceClip), &writer->sourceClipSet));
    CHK_ORET(mxf_add_array_item_strongref(writer->sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), writer->sourceClipSet));
    if (track->isPicture)
    {
        CHK_ORET(mxf_set_ul_item(writer->sourceClipSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &writer->pictureDataDef));
    }
    else
    {
        CHK_ORET(mxf_set_ul_item(writer->sourceClipSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &writer->soundDataDef));
    }
    CHK_ORET(mxf_set_length_item(writer->sourceClipSet, &MXF_ITEM_K(StructuralComponent, Duration), -1)); /* to be filled in */
    CHK_ORET(mxf_set_position_item(writer->sourceClipSet, &MXF_ITEM_K(SourceClip, StartPosition), track->startPosition));
    CHK_ORET(mxf_set_umid_item(writer->sourceClipSet, &MXF_ITEM_K(SourceClip, SourcePackageID), &track->sourcePackageUID));
    CHK_ORET(mxf_set_uint32_item(writer->sourceClipSet, &MXF_ITEM_K(SourceClip, SourceTrackID), track->sourceTrackID));

    CHK_ORET(mxf_get_item(writer->sourceClipSet, &MXF_ITEM_K(StructuralComponent, Duration), &writer->durationItems[writer->numDurationItems].item));
    writer->durationItems[writer->numDurationItems].editRate = track->editRate;
    writer->durationItems[writer->numDurationItems].materialTrackID = writer->materialTrackID;
    writer->numDurationItems++;


    if (track->isPicture)
    {
        /* Preface - ContentStorage - SourcePackage - CDCIEssenceDescriptor */
        CHK_ORET(mxf_create_set(writer->headerMetadata, &MXF_SET_K(CDCIEssenceDescriptor), &writer->cdciDescriptorSet));
        CHK_ORET(mxf_set_strongref_item(writer->sourcePackageSet, &MXF_ITEM_K(SourcePackage, Descriptor), writer->cdciDescriptorSet));
        CHK_ORET(mxf_set_rational_item(writer->cdciDescriptorSet, &MXF_ITEM_K(FileDescriptor, SampleRate), &writer->sampleRate));
        CHK_ORET(mxf_set_length_item(writer->cdciDescriptorSet, &MXF_ITEM_K(FileDescriptor, ContainerDuration), 0));
        CHK_ORET(mxf_set_ul_item(writer->cdciDescriptorSet, &MXF_ITEM_K(FileDescriptor, EssenceContainer), &MXF_EC_L(AvidAAFKLVEssenceContainer)));
        if (!mxf_equals_ul(&writer->pictureEssenceCoding, &g_Null_UL))
        {
            CHK_ORET(mxf_set_ul_item(writer->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, PictureEssenceCoding), &writer->pictureEssenceCoding));
        }
        CHK_ORET(mxf_set_uint32_item(writer->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, StoredHeight), writer->storedHeight));
        CHK_ORET(mxf_set_uint32_item(writer->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, StoredWidth), writer->storedWidth));
        if (writer->sampledHeight != 0 || writer->sampledWidth != 0)
        {
            CHK_ORET(mxf_set_uint32_item(writer->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, SampledHeight), writer->sampledHeight));
            CHK_ORET(mxf_set_uint32_item(writer->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, SampledWidth), writer->sampledWidth));
        }
        else
        {
            /* copy StoredWidth/Height into SampledWidth/Height, following Avid practice */
            CHK_ORET(mxf_set_uint32_item(writer->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, SampledHeight), writer->storedHeight));
            CHK_ORET(mxf_set_uint32_item(writer->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, SampledWidth), writer->storedWidth));
        }
        CHK_ORET(mxf_set_uint32_item(writer->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, SampledYOffset), 0));
        CHK_ORET(mxf_set_uint32_item(writer->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, SampledXOffset), 0));
        if (writer->displayHeight != 0 || writer->displayWidth != 0)
        {
            CHK_ORET(mxf_set_uint32_item(writer->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, DisplayHeight), writer->displayHeight));
            CHK_ORET(mxf_set_uint32_item(writer->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, DisplayWidth), writer->displayWidth));
        }
        else
        {
            /* copy StoredWidth/Height intoDisplayWidth/Height, following Avid practice */
            CHK_ORET(mxf_set_uint32_item(writer->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, DisplayHeight), writer->storedHeight));
            CHK_ORET(mxf_set_uint32_item(writer->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, DisplayWidth), writer->storedWidth));
        }
        CHK_ORET(mxf_set_uint32_item(writer->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, DisplayYOffset), writer->displayYOffset));
        CHK_ORET(mxf_set_uint32_item(writer->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, DisplayXOffset), writer->displayXOffset));
        CHK_ORET(mxf_alloc_array_item_elements(writer->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, VideoLineMap), 4, writer->videoLineMapLen, &arrayElement));
        for (i = 0; i < writer->videoLineMapLen; i++)
        {
            mxf_set_int32(writer->videoLineMap[i], &arrayElement[i * 4]);
        }
        CHK_ORET(mxf_set_uint32_item(writer->cdciDescriptorSet, &MXF_ITEM_K(CDCIEssenceDescriptor, HorizontalSubsampling), writer->horizSubsampling));
        CHK_ORET(mxf_set_uint32_item(writer->cdciDescriptorSet, &MXF_ITEM_K(CDCIEssenceDescriptor, VerticalSubsampling), writer->vertSubsampling));
        CHK_ORET(mxf_set_uint8_item(writer->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, FrameLayout), writer->frameLayout));
        CHK_ORET(mxf_set_rational_item(writer->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, AspectRatio), &writer->imageAspectRatio));
        CHK_ORET(mxf_set_uint32_item(writer->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, ImageAlignmentOffset), 1));
        if (!mxf_equals_ul(&writer->codingEquationsLabel, &g_Null_UL))
        {
            /* Avid stores this property as an AUID, where a UUID is stored as-is and a UL is half-swapped */
            mxfAUID auid;
            mxf_avid_set_auid(&writer->codingEquationsLabel, &auid);
            CHK_ORET(mxf_set_auid_item(writer->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, CodingEquations), &auid));
        }
        CHK_ORET(mxf_set_uint32_item(writer->cdciDescriptorSet, &MXF_ITEM_K(CDCIEssenceDescriptor, ComponentDepth), writer->componentDepth));
        CHK_ORET(mxf_set_uint8_item(writer->cdciDescriptorSet, &MXF_ITEM_K(CDCIEssenceDescriptor, ColorSiting), writer->colorSiting));
        if (writer->componentDepth == 10)
        {
            CHK_ORET(mxf_set_uint32_item(writer->cdciDescriptorSet, &MXF_ITEM_K(CDCIEssenceDescriptor, BlackRefLevel), 64));
            CHK_ORET(mxf_set_uint32_item(writer->cdciDescriptorSet, &MXF_ITEM_K(CDCIEssenceDescriptor, WhiteReflevel), 940));
            CHK_ORET(mxf_set_uint32_item(writer->cdciDescriptorSet, &MXF_ITEM_K(CDCIEssenceDescriptor, ColorRange), 897));
        }
        else
        {
            CHK_ORET(mxf_set_uint32_item(writer->cdciDescriptorSet, &MXF_ITEM_K(CDCIEssenceDescriptor, BlackRefLevel), 16));
            CHK_ORET(mxf_set_uint32_item(writer->cdciDescriptorSet, &MXF_ITEM_K(CDCIEssenceDescriptor, WhiteReflevel), 235));
            CHK_ORET(mxf_set_uint32_item(writer->cdciDescriptorSet, &MXF_ITEM_K(CDCIEssenceDescriptor, ColorRange), 225));
        }
        if (writer->imageAlignmentOffset != 0)
        {
            CHK_ORET(mxf_set_uint32_item(writer->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, ImageAlignmentOffset), writer->imageAlignmentOffset));
        }
        if (writer->imageStartOffset != 0)
        {
            CHK_ORET(mxf_set_uint32_item(writer->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, ImageStartOffset), writer->imageStartOffset));
        }
        if (writer->resolutionID != 0)
        {
            CHK_ORET(mxf_set_int32_item(writer->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, ResolutionID), writer->resolutionID));
        }
        CHK_ORET(mxf_set_int32_item(writer->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, FrameSampleSize), writer->frameSize));
        CHK_ORET(mxf_set_int32_item(writer->cdciDescriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, ImageSize), (int32_t)writer->essenceLength));

        writer->descriptorSet = writer->cdciDescriptorSet;  /* ContainerDuration and ImageSize updated when writing completed */
    }
    else
    {
        /* Preface - ContentStorage - SourcePackage - WaveEssenceDescriptor */
        CHK_ORET(mxf_create_set(writer->headerMetadata, &MXF_SET_K(WaveAudioDescriptor), &writer->bwfDescriptorSet));
        CHK_ORET(mxf_set_strongref_item(writer->sourcePackageSet, &MXF_ITEM_K(SourcePackage, Descriptor), writer->bwfDescriptorSet));
        CHK_ORET(mxf_set_rational_item(writer->bwfDescriptorSet, &MXF_ITEM_K(FileDescriptor, SampleRate), &writer->sampleRate));
        CHK_ORET(mxf_set_length_item(writer->bwfDescriptorSet, &MXF_ITEM_K(FileDescriptor, ContainerDuration), 0));
        CHK_ORET(mxf_set_ul_item(writer->bwfDescriptorSet, &MXF_ITEM_K(FileDescriptor, EssenceContainer), &MXF_EC_L(AvidAAFKLVEssenceContainer)));
        CHK_ORET(mxf_set_rational_item(writer->bwfDescriptorSet, &MXF_ITEM_K(GenericSoundEssenceDescriptor, AudioSamplingRate), &writer->samplingRate));
        CHK_ORET(mxf_set_uint32_item(writer->bwfDescriptorSet, &MXF_ITEM_K(GenericSoundEssenceDescriptor, ChannelCount), 1));
        CHK_ORET(mxf_set_uint32_item(writer->bwfDescriptorSet, &MXF_ITEM_K(GenericSoundEssenceDescriptor, QuantizationBits), writer->bitsPerSample));
        if (writer->locked == 1 || writer->locked == 0)
        {
            CHK_ORET(mxf_set_boolean_item(writer->bwfDescriptorSet, &MXF_ITEM_K(GenericSoundEssenceDescriptor, Locked), writer->locked));
        }
        if (writer->audioRefLevel >= -128 && writer->audioRefLevel <= 127)
        {
            CHK_ORET(mxf_set_int8_item(writer->bwfDescriptorSet, &MXF_ITEM_K(GenericSoundEssenceDescriptor, AudioRefLevel), writer->audioRefLevel));
        }
        CHK_ORET(mxf_set_uint8_item(writer->bwfDescriptorSet, &MXF_ITEM_K(GenericSoundEssenceDescriptor, ElectroSpatialFormulation), 2 /*single channel mode */));
        if (writer->dialNorm >= -128 && writer->dialNorm <= 127)
        {
            CHK_ORET(mxf_set_int8_item(writer->bwfDescriptorSet, &MXF_ITEM_K(GenericSoundEssenceDescriptor, DialNorm), writer->dialNorm));
        }
        if (writer->sequenceOffset >= 0 && writer->sequenceOffset < 5)
        {
            CHK_ORET(mxf_set_uint8_item(writer->bwfDescriptorSet, &MXF_ITEM_K(WaveAudioDescriptor, SequenceOffset), writer->sequenceOffset));
        }
        CHK_ORET(mxf_set_uint16_item(writer->bwfDescriptorSet, &MXF_ITEM_K(WaveAudioDescriptor, BlockAlign), writer->blockAlign));
        CHK_ORET(mxf_set_uint32_item(writer->bwfDescriptorSet, &MXF_ITEM_K(WaveAudioDescriptor, AvgBps), writer->avgBps));
        writer->descriptorSet = writer->bwfDescriptorSet;  /* ContainerDuration updated when writing completed */
    }

    /* attach a project name attribute to the file source package */
    if (clipWriter->wProjectName != NULL)
    {
        CHK_ORET(mxf_avid_attach_mob_attribute(writer->headerMetadata, writer->sourcePackageSet, L"_PJ", clipWriter->wProjectName));
    }


    /* Preface - ContentStorage - EssenceContainerData */
    CHK_ORET(mxf_create_set(writer->headerMetadata, &MXF_SET_K(EssenceContainerData), &writer->essContainerDataSet));
    CHK_ORET(mxf_add_array_item_strongref(writer->contentStorageSet, &MXF_ITEM_K(ContentStorage, EssenceContainerData), writer->essContainerDataSet));
    CHK_ORET(mxf_set_umid_item(writer->essContainerDataSet, &MXF_ITEM_K(EssenceContainerData, LinkedPackageUID), &filePackage->uid));
    CHK_ORET(mxf_set_uint32_item(writer->essContainerDataSet, &MXF_ITEM_K(EssenceContainerData, IndexSID), g_indexSID));
    CHK_ORET(mxf_set_uint32_item(writer->essContainerDataSet, &MXF_ITEM_K(EssenceContainerData, BodySID), g_bodySID));



    if (tapePackage != NULL)
    {
        /* Preface - ContentStorage - tape SourcePackage */
        CHK_ORET(mxf_create_set(writer->headerMetadata, &MXF_SET_K(SourcePackage), &writer->sourcePackageSet));
        CHK_ORET(mxf_add_array_item_strongref(writer->contentStorageSet, &MXF_ITEM_K(ContentStorage, Packages), writer->sourcePackageSet));
        CHK_ORET(mxf_set_umid_item(writer->sourcePackageSet, &MXF_ITEM_K(GenericPackage, PackageUID), &tapePackage->uid));
        CHK_ORET(mxf_set_timestamp_item(writer->sourcePackageSet, &MXF_ITEM_K(GenericPackage, PackageCreationDate), &tapePackage->creationDate));
        CHK_ORET(mxf_set_timestamp_item(writer->sourcePackageSet, &MXF_ITEM_K(GenericPackage, PackageModifiedDate), &tapePackage->creationDate));
        if (tapePackage->name != NULL)
        {
            CHK_ORET(convert_string(clipWriter, tapePackage->name));
            CHK_ORET(mxf_set_utf16string_item(writer->sourcePackageSet, &MXF_ITEM_K(GenericPackage, Name), clipWriter->wTmpString));
        }

        maxTrackID = 0;
        tapeLen = 0;
        mxf_initialise_list_iter(&iter, &tapePackage->tracks);
        while (mxf_next_list_iter_element(&iter))
        {
            track = (Track*)mxf_get_iter_element(&iter);

            /* Preface - ContentStorage - SourcePackage - Timeline Track */
            CHK_ORET(mxf_create_set(writer->headerMetadata, &MXF_SET_K(Track), &writer->sourcePackageTrackSet));
            CHK_ORET(mxf_add_array_item_strongref(writer->sourcePackageSet, &MXF_ITEM_K(GenericPackage, Tracks), writer->sourcePackageTrackSet));
            CHK_ORET(mxf_set_uint32_item(writer->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackID), track->id));
            if (track->name != NULL)
            {
                CHK_ORET(convert_string(clipWriter, track->name));
                CHK_ORET(mxf_set_utf16string_item(writer->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackName), clipWriter->wTmpString));
            }
            CHK_ORET(mxf_set_uint32_item(writer->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackNumber), track->number));
            CHK_ORET(mxf_set_rational_item(writer->sourcePackageTrackSet, &MXF_ITEM_K(Track, EditRate), &track->editRate));
            CHK_ORET(mxf_set_position_item(writer->sourcePackageTrackSet, &MXF_ITEM_K(Track, Origin), track->origin));

            /* Preface - ContentStorage - SourcePackage - Timeline Track - Sequence */
            CHK_ORET(mxf_create_set(writer->headerMetadata, &MXF_SET_K(Sequence), &writer->sequenceSet));
            CHK_ORET(mxf_set_strongref_item(writer->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, Sequence), writer->sequenceSet));
            if (track->isPicture)
            {
                CHK_ORET(mxf_set_ul_item(writer->sequenceSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &writer->pictureDataDef));
            }
            else
            {
                CHK_ORET(mxf_set_ul_item(writer->sequenceSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &writer->soundDataDef));
            }
            CHK_ORET(mxf_set_length_item(writer->sequenceSet, &MXF_ITEM_K(StructuralComponent, Duration), track->length));

            //fprintf(stderr, "Track name %2s, length %" PRId64 ", origin %" PRId64 ", editrate numerator %d\n", track->name, track->length, track->origin, track->editRate.numerator);

            /* Preface - ContentStorage - SourcePackage - Timeline Track - Sequence - SourceClip */
            CHK_ORET(mxf_create_set(writer->headerMetadata, &MXF_SET_K(SourceClip), &writer->sourceClipSet));
            CHK_ORET(mxf_add_array_item_strongref(writer->sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), writer->sourceClipSet));
            if (track->isPicture)
            {
                CHK_ORET(mxf_set_ul_item(writer->sourceClipSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &writer->pictureDataDef));
            }
            else
            {
                CHK_ORET(mxf_set_ul_item(writer->sourceClipSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &writer->soundDataDef));
            }
            CHK_ORET(mxf_set_length_item(writer->sourceClipSet, &MXF_ITEM_K(StructuralComponent, Duration), track->length));
            CHK_ORET(mxf_set_position_item(writer->sourceClipSet, &MXF_ITEM_K(SourceClip, StartPosition), track->startPosition));
            CHK_ORET(mxf_set_umid_item(writer->sourceClipSet, &MXF_ITEM_K(SourceClip, SourcePackageID), &track->sourcePackageUID));
            CHK_ORET(mxf_set_uint32_item(writer->sourceClipSet, &MXF_ITEM_K(SourceClip, SourceTrackID), track->sourceTrackID));

            if (track->id > maxTrackID)
            {
                maxTrackID = track->id;
            }
            tapeLen = track->length;
        }

        /* Preface - ContentStorage - SourcePackage - timecode Timeline Track */
        CHK_ORET(mxf_create_set(writer->headerMetadata, &MXF_SET_K(Track), &writer->sourcePackageTrackSet));
        CHK_ORET(mxf_add_array_item_strongref(writer->sourcePackageSet, &MXF_ITEM_K(GenericPackage, Tracks), writer->sourcePackageTrackSet));
        CHK_ORET(mxf_set_uint32_item(writer->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackID), maxTrackID + 1));
        CHK_ORET(mxf_set_utf16string_item(writer->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackName), L"TC1"));
        CHK_ORET(mxf_set_uint32_item(writer->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackNumber), 1));
        CHK_ORET(mxf_set_rational_item(writer->sourcePackageTrackSet, &MXF_ITEM_K(Track, EditRate), &clipWriter->projectEditRate));
        CHK_ORET(mxf_set_position_item(writer->sourcePackageTrackSet, &MXF_ITEM_K(Track, Origin), 0));

        /* Preface - ContentStorage - SourcePackage - timecode Timeline Track - Sequence */
        CHK_ORET(mxf_create_set(writer->headerMetadata, &MXF_SET_K(Sequence), &writer->sequenceSet));
        CHK_ORET(mxf_set_strongref_item(writer->sourcePackageTrackSet, &MXF_ITEM_K(GenericTrack, Sequence), writer->sequenceSet));
        CHK_ORET(mxf_set_ul_item(writer->sequenceSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &writer->timecodeDataDef));
        CHK_ORET(mxf_set_length_item(writer->sequenceSet, &MXF_ITEM_K(StructuralComponent, Duration), tapeLen));

        /* Preface - ContentStorage - SourcePackage - Timecode Track - Sequence - TimecodeComponent */
        CHK_ORET(mxf_create_set(writer->headerMetadata, &MXF_SET_K(TimecodeComponent), &writer->timecodeComponentSet));
        CHK_ORET(mxf_add_array_item_strongref(writer->sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), writer->timecodeComponentSet));
        CHK_ORET(mxf_set_ul_item(writer->timecodeComponentSet, &MXF_ITEM_K(StructuralComponent, DataDefinition), &writer->timecodeDataDef));
        CHK_ORET(mxf_set_length_item(writer->timecodeComponentSet, &MXF_ITEM_K(StructuralComponent, Duration), tapeLen));
        CHK_ORET(mxf_set_uint16_item(writer->timecodeComponentSet, &MXF_ITEM_K(TimecodeComponent, RoundedTimecodeBase), roundedTimecodeBase));
        CHK_ORET(mxf_set_boolean_item(writer->timecodeComponentSet, &MXF_ITEM_K(TimecodeComponent, DropFrame), clipWriter->dropFrameFlag));
        CHK_ORET(mxf_set_position_item(writer->timecodeComponentSet, &MXF_ITEM_K(TimecodeComponent, StartTimecode), 0));


        /* Preface - ContentStorage - tape SourcePackage - TapeDescriptor */
        CHK_ORET(mxf_create_set(writer->headerMetadata, &MXF_SET_K(TapeDescriptor), &writer->tapeDescriptorSet));
        CHK_ORET(mxf_set_strongref_item(writer->sourcePackageSet, &MXF_ITEM_K(SourcePackage, Descriptor), writer->tapeDescriptorSet));
        CHK_ORET(mxf_set_int32_item(writer->tapeDescriptorSet, &MXF_ITEM_K(TapeDescriptor, ColorFrame), 0));


        /* attach a project name attribute to the package */
        if (clipWriter->wProjectName != NULL)
        {
            CHK_ORET(mxf_avid_attach_mob_attribute(writer->headerMetadata, writer->sourcePackageSet, L"_PJ", clipWriter->wProjectName));
        }

    }

    /* Preface - Identification */
    CHK_ORET(mxf_create_set(writer->headerMetadata, &MXF_SET_K(Identification), &writer->identSet));
    CHK_ORET(mxf_add_array_item_strongref(writer->prefaceSet, &MXF_ITEM_K(Preface, Identifications), writer->identSet));
    CHK_ORET(mxf_set_uuid_item(writer->identSet, &MXF_ITEM_K(Identification, ThisGenerationUID), &thisGeneration));
    CHK_ORET(mxf_set_utf16string_item(writer->identSet, &MXF_ITEM_K(Identification, CompanyName), g_mxfIdentCompanyName));
    CHK_ORET(mxf_set_utf16string_item(writer->identSet, &MXF_ITEM_K(Identification, ProductName), g_mxfIdentProductName));
    CHK_ORET(mxf_set_utf16string_item(writer->identSet, &MXF_ITEM_K(Identification, VersionString), g_mxfIdentVersionString));
    CHK_ORET(mxf_set_uuid_item(writer->identSet, &MXF_ITEM_K(Identification, ProductUID), &g_mxfIdentProductUID));
    CHK_ORET(mxf_avid_set_product_version_item(writer->identSet, &MXF_ITEM_K(Identification, ProductVersion), mxf_get_version()));
    CHK_ORET(mxf_set_timestamp_item(writer->identSet, &MXF_ITEM_K(Identification, ModificationDate), &clipWriter->now));
    CHK_ORET(mxf_avid_set_product_version_item(writer->identSet, &MXF_ITEM_K(Identification, ToolkitVersion), mxf_get_version()));
    CHK_ORET(mxf_set_utf16string_item(writer->identSet, &MXF_ITEM_K(Identification, Platform), mxf_get_platform_wstring()));


    return 1;
}

static int complete_track(AvidClipWriter *clipWriter, TrackWriter *writer, PackageDefinitions *packageDefinitions,
                          Package *filePackage)
{
    MXFListIterator iter;
    MJPEGOffsetsArray *offsetsArray;
    int i;
    uint32_t j;
    MXFIndexEntry indexEntry;
    uint32_t numIndexEntries;
    int64_t filePos;


    /* finalise and close the essence element */

    writer->essenceLength = writer->essenceElement->totalLen;
    CHK_ORET(mxf_finalize_essence_element_write(writer->mxfFile, writer->essenceElement));
    mxf_close_essence_element(&writer->essenceElement);

    CHK_ORET(mxf_fill_to_kag(writer->mxfFile, writer->bodyPartition));


    /* write the footer partition pack */

    CHK_ORET(mxf_append_new_from_partition(writer->partitions, writer->headerPartition, &writer->footerPartition));
    writer->footerPartition->key = MXF_PP_K(ClosedComplete, Footer);
    writer->footerPartition->kagSize = 0x100;
    writer->footerPartition->indexSID = g_indexSID;

    CHK_ORET((filePos = mxf_file_tell(writer->mxfFile)) >= 0);
    CHK_ORET(mxf_write_partition(writer->mxfFile, writer->footerPartition));
    CHK_ORET(mxf_fill_to_position(writer->mxfFile, filePos + 199));


    /* write the index table segment */

    CHK_ORET(mxf_mark_index_start(writer->mxfFile, writer->footerPartition));


    if (writer->essenceType == AvidMJPEG)
    {
        /* Avid extension: last entry provides the length of the essence */
        CHK_ORET(add_avid_mjpeg_offset(&writer->mjpegFrameOffsets, writer->prevFrameOffset,
                                       &writer->currentMJPEGOffsetsArray));

        CHK_ORET(mxf_create_index_table_segment(&writer->indexSegment));
        mxf_generate_uuid(&writer->indexSegment->instanceUID);
        writer->indexSegment->indexEditRate = writer->sampleRate;
        writer->indexSegment->indexDuration = writer->duration;
        writer->indexSegment->indexSID = g_indexSID;
        writer->indexSegment->bodySID = g_bodySID;

        numIndexEntries = get_num_offsets(&writer->mjpegFrameOffsets);
        CHK_ORET(mxf_write_index_table_segment_header(writer->mxfFile, writer->indexSegment, 0, numIndexEntries));

        if (numIndexEntries > 0)
        {
            CHK_ORET(mxf_avid_write_index_entry_array_header(writer->mxfFile, 0, 0, numIndexEntries));

            memset(&indexEntry, 0, sizeof(MXFIndexEntry));
            indexEntry.flags = 0x80; /* random access */

            mxf_initialise_list_iter(&iter, &writer->mjpegFrameOffsets);
            while (mxf_next_list_iter_element(&iter))
            {
                offsetsArray = (MJPEGOffsetsArray*)mxf_get_iter_element(&iter);
                for (j = 0; j < offsetsArray->len; j++)
                {
                    indexEntry.streamOffset = offsetsArray->offsets[j];
                    CHK_ORET(mxf_write_index_entry(writer->mxfFile, 0, 0, &indexEntry));
                }
            }
        }
    }
    else
    {
        CHK_ORET(mxf_create_index_table_segment(&writer->indexSegment));
        mxf_generate_uuid(&writer->indexSegment->instanceUID);
        writer->indexSegment->indexEditRate = writer->sampleRate;
        writer->indexSegment->indexDuration = writer->duration;
        writer->indexSegment->editUnitByteCount = writer->editUnitByteCount;
        writer->indexSegment->indexSID = g_indexSID;
        writer->indexSegment->bodySID = g_bodySID;

        CHK_ORET(mxf_write_index_table_segment(writer->mxfFile, writer->indexSegment));
    }

    CHK_ORET(mxf_fill_to_kag(writer->mxfFile, writer->footerPartition));

    CHK_ORET(mxf_mark_index_end(writer->mxfFile, writer->footerPartition));


    /* write the random index pack */
    CHK_ORET(mxf_write_rip(writer->mxfFile, writer->partitions));


    /* update the header metadata if the package definitions have changed */

    if (packageDefinitions != NULL)
    {
        writer->numDurationItems = 0;
        writer->descriptorSet = NULL;

        CHK_ORET(create_header_metadata(clipWriter, packageDefinitions, filePackage, writer));
    }


    /* update the header metadata with durations */

    for (i = 0; i < writer->numDurationItems; i++)
    {
        TrackWriter *trackWriter;
        if (!get_track_writer(clipWriter, writer->durationItems[i].materialTrackID, &trackWriter))
        {
            /* the material package track doesn't have a corresponding track writer */
            continue;
        }

        if (memcmp(&writer->durationItems[i].editRate, &trackWriter->sampleRate, sizeof(mxfRational)) == 0)
        {
            CHK_ORET(mxf_set_length_item(writer->durationItems[i].item->set, &writer->durationItems[i].item->key,
                                         trackWriter->duration));
        }
        else
        {
            /* the duration is the number of samples at the file package track edit rate
            If the duration item is for a different track with a different edit rate (eg a material
            package track), then the duration must be converted */
            double factor = writer->durationItems[i].editRate.numerator * trackWriter->sampleRate.denominator /
                   (double)(writer->durationItems[i].editRate.denominator * trackWriter->sampleRate.numerator);
            CHK_ORET(mxf_set_length_item(writer->durationItems[i].item->set, &writer->durationItems[i].item->key,
                                         (int64_t)(trackWriter->duration * factor + 0.5)));
        }
    }
    CHK_ORET(mxf_set_length_item(writer->descriptorSet, &MXF_ITEM_K(FileDescriptor, ContainerDuration), writer->duration));
    if (mxf_have_item(writer->descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, ImageSize)))
    {
        CHK_ORET(mxf_set_int32_item(writer->descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, ImageSize), (int32_t)writer->essenceLength));
    }



    /* re-write header metadata with avid extensions */

    CHK_ORET(mxf_file_seek(writer->mxfFile, writer->headerMetadataFilePos, SEEK_SET));

    CHK_ORET(mxf_mark_header_start(writer->mxfFile, writer->headerPartition));
    CHK_ORET(mxf_avid_write_header_metadata(writer->mxfFile, writer->headerMetadata, writer->headerPartition));
    CHK_ORET(mxf_fill_to_position(writer->mxfFile, g_fixedBodyPPOffset));
    CHK_ORET(mxf_mark_header_end(writer->mxfFile, writer->headerPartition));


    /* update the partitions */
    writer->headerPartition->key = MXF_PP_K(ClosedComplete, Header);
    CHK_ORET(mxf_update_partitions(writer->mxfFile, writer->partitions));


    return 1;
}

/* TODO: check which essence container labels and essence element keys older Avids support */
static int create_track_writer(AvidClipWriter *clipWriter, PackageDefinitions *packageDefinitions, Package *filePackage)
{
    TrackWriter *newTrackWriter = NULL;
    Track *track;
    MXFListIterator iter;
    int haveMaterialTrackRef;
    uint32_t i;
    int64_t filePos;

    CHK_ORET(filePackage->filename != NULL);
    CHK_ORET(clipWriter->projectFormat == PAL_25i || clipWriter->projectFormat == NTSC_30i);


    CHK_MALLOC_ORET(newTrackWriter, TrackWriter);
    memset(newTrackWriter, 0, sizeof(TrackWriter));
    mxf_initialise_list(&newTrackWriter->mjpegFrameOffsets, free_offsets_array_in_list);

    CHK_OFAIL((newTrackWriter->filename = strdup(filePackage->filename)) != NULL);

    newTrackWriter->essenceType = filePackage->essenceType;
    newTrackWriter->fileSourcePackageUID = filePackage->uid;


    /* get the material track id that references this file package */

    haveMaterialTrackRef = 0;
    mxf_initialise_list_iter(&iter, &packageDefinitions->materialPackage->tracks);
    while (mxf_next_list_iter_element(&iter))
    {
        track = (Track*)mxf_get_iter_element(&iter);
        if (mxf_equals_umid(&track->sourcePackageUID, &filePackage->uid))
        {
            newTrackWriter->materialTrackID = track->id;
            haveMaterialTrackRef = 1;
            break;
        }
    }
    CHK_OFAIL(haveMaterialTrackRef);


    /* set essence descriptor data */

    CHK_ORET(mxf_get_list_length(&filePackage->tracks) == 1);
    track = (Track*)mxf_get_list_element(&filePackage->tracks, 0);

    newTrackWriter->componentDepth = 8;

    switch (filePackage->essenceType)
    {
        case AvidMJPEG:
            newTrackWriter->cdciEssenceContainerLabel = MXF_EC_L(AvidAAFKLVEssenceContainer);
            newTrackWriter->essenceContainerLabel = MXF_EC_L(AvidMJPEGClipWrapped);
            newTrackWriter->frameSize = 0; /* variable */
            if (clipWriter->projectFormat == PAL_25i)
            {
                switch (filePackage->essenceInfo.mjpegResolution)
                {
                    case Res21:
                        newTrackWriter->resolutionID = g_AvidMJPEG21_ResolutionID;
                        newTrackWriter->pictureEssenceCoding = MXF_CMDEF_L(AvidMJPEG21_PAL);
                        newTrackWriter->videoLineMap[0] = 15;
                        newTrackWriter->videoLineMap[1] = 328;
                        newTrackWriter->videoLineMapLen = 2;
                        newTrackWriter->storedHeight = 296;
                        newTrackWriter->storedWidth = 720;
                        newTrackWriter->sampledHeight = 296;
                        newTrackWriter->sampledWidth = 720;
                        newTrackWriter->displayHeight = 288;
                        newTrackWriter->displayWidth = 720;
                        newTrackWriter->displayYOffset = 8;
                        newTrackWriter->displayXOffset = 0;
                        newTrackWriter->frameLayout = MXF_SEPARATE_FIELDS;
                        newTrackWriter->colorSiting = MXF_COLOR_SITING_REC601;
                        break;
                    case Res31:
                        newTrackWriter->resolutionID = g_AvidMJPEG31_ResolutionID;
                        newTrackWriter->pictureEssenceCoding = MXF_CMDEF_L(AvidMJPEG31_PAL);
                        newTrackWriter->videoLineMap[0] = 15;
                        newTrackWriter->videoLineMap[1] = 328;
                        newTrackWriter->videoLineMapLen = 2;
                        newTrackWriter->storedHeight = 296;
                        newTrackWriter->storedWidth = 720;
                        newTrackWriter->sampledHeight = 296;
                        newTrackWriter->sampledWidth = 720;
                        newTrackWriter->displayHeight = 288;
                        newTrackWriter->displayWidth = 720;
                        newTrackWriter->displayYOffset = 8;
                        newTrackWriter->displayXOffset = 0;
                        newTrackWriter->frameLayout = MXF_SEPARATE_FIELDS;
                        newTrackWriter->colorSiting = MXF_COLOR_SITING_REC601;
                        break;
                    case Res101:
                        newTrackWriter->resolutionID = g_AvidMJPEG101_ResolutionID;
                        newTrackWriter->pictureEssenceCoding = MXF_CMDEF_L(AvidMJPEG101_PAL);
                        newTrackWriter->videoLineMap[0] = 15;
                        newTrackWriter->videoLineMap[1] = 328;
                        newTrackWriter->videoLineMapLen = 2;
                        newTrackWriter->storedHeight = 296;
                        newTrackWriter->storedWidth = 720;
                        newTrackWriter->sampledHeight = 296;
                        newTrackWriter->sampledWidth = 720;
                        newTrackWriter->displayHeight = 288;
                        newTrackWriter->displayWidth = 720;
                        newTrackWriter->displayYOffset = 8;
                        newTrackWriter->displayXOffset = 0;
                        newTrackWriter->frameLayout = MXF_SEPARATE_FIELDS;
                        newTrackWriter->colorSiting = MXF_COLOR_SITING_REC601;
                        break;
                    case Res201:
                        newTrackWriter->resolutionID = g_AvidMJPEG201_ResolutionID;
                        newTrackWriter->pictureEssenceCoding = MXF_CMDEF_L(AvidMJPEG201_PAL);
                        newTrackWriter->videoLineMap[0] = 15;
                        newTrackWriter->videoLineMap[1] = 328;
                        newTrackWriter->videoLineMapLen = 2;
                        newTrackWriter->storedHeight = 296;
                        newTrackWriter->storedWidth = 720;
                        newTrackWriter->sampledHeight = 296;
                        newTrackWriter->sampledWidth = 720;
                        newTrackWriter->displayHeight = 288;
                        newTrackWriter->displayWidth = 720;
                        newTrackWriter->displayYOffset = 8;
                        newTrackWriter->displayXOffset = 0;
                        newTrackWriter->frameLayout = MXF_SEPARATE_FIELDS;
                        newTrackWriter->colorSiting = MXF_COLOR_SITING_REC601;
                        break;
                    case Res41m:
                        newTrackWriter->resolutionID = g_AvidMJPEG41m_ResolutionID;
                        newTrackWriter->pictureEssenceCoding = MXF_CMDEF_L(AvidMJPEG41m_PAL);
                        newTrackWriter->videoLineMap[0] = 15;
                        newTrackWriter->videoLineMapLen = 1;
                        newTrackWriter->storedHeight = 296;
                        newTrackWriter->storedWidth = 288;
                        newTrackWriter->sampledHeight = 296;
                        newTrackWriter->sampledWidth = 288;
                        newTrackWriter->displayHeight = 288;
                        newTrackWriter->displayWidth = 288;
                        newTrackWriter->displayYOffset = 8;
                        newTrackWriter->displayXOffset = 0;
                        newTrackWriter->frameLayout = MXF_SINGLE_FIELD;
                        newTrackWriter->colorSiting = MXF_COLOR_SITING_REC601;
                        break;
                    case Res101m:
                        newTrackWriter->resolutionID = g_AvidMJPEG101m_ResolutionID;
                        newTrackWriter->pictureEssenceCoding = MXF_CMDEF_L(AvidMJPEG101m_PAL);
                        newTrackWriter->videoLineMap[0] = 15;
                        newTrackWriter->videoLineMapLen = 1;
                        newTrackWriter->storedHeight = 296;
                        newTrackWriter->storedWidth = 288;
                        newTrackWriter->sampledHeight = 296;
                        newTrackWriter->sampledWidth = 288;
                        newTrackWriter->displayHeight = 288;
                        newTrackWriter->displayWidth = 288;
                        newTrackWriter->displayYOffset = 8;
                        newTrackWriter->displayXOffset = 0;
                        newTrackWriter->frameLayout = MXF_SINGLE_FIELD;
                        newTrackWriter->colorSiting = MXF_COLOR_SITING_REC601;
                        break;
                    case Res151s:
                        newTrackWriter->resolutionID = g_AvidMJPEG151s_ResolutionID;
                        newTrackWriter->pictureEssenceCoding = MXF_CMDEF_L(AvidMJPEG151s_PAL);
                        newTrackWriter->videoLineMap[0] = 15;
                        newTrackWriter->videoLineMapLen = 1;
                        newTrackWriter->storedHeight = 296;
                        newTrackWriter->storedWidth = 352;
                        newTrackWriter->sampledHeight = 296;
                        newTrackWriter->sampledWidth = 352;
                        newTrackWriter->displayHeight = 288;
                        newTrackWriter->displayWidth = 352;
                        newTrackWriter->displayYOffset = 8;
                        newTrackWriter->displayXOffset = 0;
                        newTrackWriter->frameLayout = MXF_SINGLE_FIELD;
                        newTrackWriter->colorSiting = MXF_COLOR_SITING_REC601;
                        break;
                    default:
                        assert(0);
                        return 0;
                }
            }
            else
            {
                switch (filePackage->essenceInfo.mjpegResolution)
                {
                    case Res21:
                        newTrackWriter->resolutionID = g_AvidMJPEG21_ResolutionID;
                        newTrackWriter->pictureEssenceCoding = MXF_CMDEF_L(AvidMJPEG21_NTSC);
                        newTrackWriter->videoLineMap[0] = 16;
                        newTrackWriter->videoLineMap[1] = 278;
                        newTrackWriter->videoLineMapLen = 2;
                        newTrackWriter->storedHeight = 248;
                        newTrackWriter->storedWidth = 720;
                        newTrackWriter->sampledHeight = 248;
                        newTrackWriter->sampledWidth = 720;
                        newTrackWriter->displayHeight = 243;
                        newTrackWriter->displayWidth = 720;
                        newTrackWriter->displayYOffset = 5;
                        newTrackWriter->displayXOffset = 0;
                        newTrackWriter->frameLayout = MXF_SEPARATE_FIELDS;
                        newTrackWriter->colorSiting = MXF_COLOR_SITING_REC601;
                        break;
                    case Res31:
                        newTrackWriter->resolutionID = g_AvidMJPEG31_ResolutionID;
                        newTrackWriter->pictureEssenceCoding = MXF_CMDEF_L(AvidMJPEG31_NTSC);
                        newTrackWriter->videoLineMap[0] = 16;
                        newTrackWriter->videoLineMap[1] = 278;
                        newTrackWriter->videoLineMapLen = 2;
                        newTrackWriter->storedHeight = 248;
                        newTrackWriter->storedWidth = 720;
                        newTrackWriter->sampledHeight = 248;
                        newTrackWriter->sampledWidth = 720;
                        newTrackWriter->displayHeight = 243;
                        newTrackWriter->displayWidth = 720;
                        newTrackWriter->displayYOffset = 5;
                        newTrackWriter->displayXOffset = 0;
                        newTrackWriter->frameLayout = MXF_SEPARATE_FIELDS;
                        newTrackWriter->colorSiting = MXF_COLOR_SITING_REC601;
                        break;
                    case Res101:
                        newTrackWriter->resolutionID = g_AvidMJPEG101_ResolutionID;
                        newTrackWriter->pictureEssenceCoding = MXF_CMDEF_L(AvidMJPEG101_NTSC);
                        newTrackWriter->videoLineMap[0] = 16;
                        newTrackWriter->videoLineMap[1] = 278;
                        newTrackWriter->videoLineMapLen = 2;
                        newTrackWriter->storedHeight = 248;
                        newTrackWriter->storedWidth = 720;
                        newTrackWriter->sampledHeight = 248;
                        newTrackWriter->sampledWidth = 720;
                        newTrackWriter->displayHeight = 243;
                        newTrackWriter->displayWidth = 720;
                        newTrackWriter->displayYOffset = 5;
                        newTrackWriter->displayXOffset = 0;
                        newTrackWriter->frameLayout = MXF_SEPARATE_FIELDS;
                        newTrackWriter->colorSiting = MXF_COLOR_SITING_REC601;
                        break;
                    case Res201:
                        newTrackWriter->resolutionID = g_AvidMJPEG201_ResolutionID;
                        newTrackWriter->pictureEssenceCoding = MXF_CMDEF_L(AvidMJPEG201_NTSC);
                        newTrackWriter->videoLineMap[0] = 16;
                        newTrackWriter->videoLineMap[1] = 278;
                        newTrackWriter->videoLineMapLen = 2;
                        newTrackWriter->storedHeight = 248;
                        newTrackWriter->storedWidth = 720;
                        newTrackWriter->sampledHeight = 248;
                        newTrackWriter->sampledWidth = 720;
                        newTrackWriter->displayHeight = 243;
                        newTrackWriter->displayWidth = 720;
                        newTrackWriter->displayYOffset = 5;
                        newTrackWriter->displayXOffset = 0;
                        newTrackWriter->frameLayout = MXF_SEPARATE_FIELDS;
                        newTrackWriter->colorSiting = MXF_COLOR_SITING_REC601;
                        break;
                    case Res41m:
                        newTrackWriter->resolutionID = g_AvidMJPEG41m_ResolutionID;
                        newTrackWriter->pictureEssenceCoding = MXF_CMDEF_L(AvidMJPEG41m_NTSC);
                        newTrackWriter->videoLineMap[0] = 16;
                        newTrackWriter->videoLineMapLen = 1;
                        newTrackWriter->storedHeight = 248;
                        newTrackWriter->storedWidth = 288;
                        newTrackWriter->sampledHeight = 248;
                        newTrackWriter->sampledWidth = 288;
                        newTrackWriter->displayHeight = 243;
                        newTrackWriter->displayWidth = 288;
                        newTrackWriter->displayYOffset = 5;
                        newTrackWriter->displayXOffset = 0;
                        newTrackWriter->frameLayout = MXF_SINGLE_FIELD;
                        newTrackWriter->colorSiting = MXF_COLOR_SITING_REC601;
                        break;
                    case Res101m:
                        newTrackWriter->resolutionID = g_AvidMJPEG101m_ResolutionID;
                        newTrackWriter->pictureEssenceCoding = MXF_CMDEF_L(AvidMJPEG101m_NTSC);
                        newTrackWriter->videoLineMap[0] = 16;
                        newTrackWriter->videoLineMapLen = 1;
                        newTrackWriter->storedHeight = 248;
                        newTrackWriter->storedWidth = 288;
                        newTrackWriter->sampledHeight = 248;
                        newTrackWriter->sampledWidth = 288;
                        newTrackWriter->displayHeight = 243;
                        newTrackWriter->displayWidth = 288;
                        newTrackWriter->displayYOffset = 5;
                        newTrackWriter->displayXOffset = 0;
                        newTrackWriter->frameLayout = MXF_SINGLE_FIELD;
                        newTrackWriter->colorSiting = MXF_COLOR_SITING_REC601;
                        break;
                    case Res151s:
                        newTrackWriter->resolutionID = g_AvidMJPEG151s_ResolutionID;
                        newTrackWriter->pictureEssenceCoding = MXF_CMDEF_L(AvidMJPEG151s_NTSC);
                        newTrackWriter->videoLineMap[0] = 16;
                        newTrackWriter->videoLineMapLen = 1;
                        newTrackWriter->storedHeight = 248;
                        newTrackWriter->storedWidth = 352;
                        newTrackWriter->sampledHeight = 248;
                        newTrackWriter->sampledWidth = 352;
                        newTrackWriter->displayHeight = 243;
                        newTrackWriter->displayWidth = 352;
                        newTrackWriter->displayYOffset = 5;
                        newTrackWriter->displayXOffset = 0;
                        newTrackWriter->frameLayout = MXF_SINGLE_FIELD;
                        newTrackWriter->colorSiting = MXF_COLOR_SITING_REC601;
                        break;
                    default:
                        assert(0);
                        return 0;
                }
            }
            newTrackWriter->imageAspectRatio = filePackage->essenceInfo.imageAspectRatio;
            newTrackWriter->horizSubsampling = 2;        /* All JPEG formats are 4:2:2 */
            newTrackWriter->vertSubsampling = 1;
            newTrackWriter->essenceElementKey = MXF_EE_K(AvidMJPEGClipWrapped);
            newTrackWriter->sourceTrackNumber = MXF_AVID_MJPEG_PICT_TRACK_NUM;
            newTrackWriter->essenceElementLLen = 9;
            CHK_ORET(memcmp(&track->editRate, &clipWriter->projectEditRate, sizeof(mxfRational)) == 0); /* why would it be different? */
            newTrackWriter->sampleRate = track->editRate;
            newTrackWriter->editUnitByteCount = newTrackWriter->frameSize;
            break;

        case IECDV25:
            newTrackWriter->cdciEssenceContainerLabel = g_Null_UL;
            if (clipWriter->projectFormat == PAL_25i)
            {
                /* TODO: this Avid label fails in Xpress HD 5.2.1, but what about earlier versions?
                  newTrackWriter->essenceContainerLabel = g_avid_DV25ClipWrappedEssenceContainer_label; */
                newTrackWriter->essenceContainerLabel = MXF_EC_L(IECDV_25_625_50_ClipWrapped);
                newTrackWriter->frameSize = 144000;
                newTrackWriter->pictureEssenceCoding = MXF_CMDEF_L(IECDV_25_625_50);
                newTrackWriter->storedHeight = 288;
                newTrackWriter->storedWidth = 720;
                newTrackWriter->videoLineMap[0] = 23;
                newTrackWriter->videoLineMap[1] = 335;
                newTrackWriter->videoLineMapLen = 2;
                newTrackWriter->colorSiting = MXF_COLOR_SITING_REC601;
                if (clipWriter->useLegacy)
                {
                    newTrackWriter->resolutionID = 0x8d;
                    newTrackWriter->horizSubsampling = 2;
                    newTrackWriter->vertSubsampling = 2;
                    newTrackWriter->frameLayout = MXF_MIXED_FIELDS;
                }
                else
                {
                    newTrackWriter->resolutionID = 0x8d;
                    newTrackWriter->horizSubsampling = 2;
                    newTrackWriter->vertSubsampling = 2;
                    newTrackWriter->frameLayout = MXF_SEPARATE_FIELDS;
                }
            }
            else
            {
                newTrackWriter->essenceContainerLabel = MXF_EC_L(IECDV_25_525_60_ClipWrapped);
                newTrackWriter->frameSize = 120000;
                newTrackWriter->pictureEssenceCoding = MXF_CMDEF_L(IECDV_25_525_60);
                newTrackWriter->storedHeight = 240;
                newTrackWriter->storedWidth = 720;
                newTrackWriter->videoLineMap[0] = 23;
                newTrackWriter->videoLineMap[1] = 285;
                newTrackWriter->videoLineMapLen = 2;
                newTrackWriter->colorSiting = MXF_COLOR_SITING_REC601;
                if (clipWriter->useLegacy)
                {
                    newTrackWriter->resolutionID = 0x8c;
                    newTrackWriter->horizSubsampling = 1;
                    newTrackWriter->vertSubsampling = 1;
                    newTrackWriter->frameLayout = MXF_MIXED_FIELDS;
                }
                else
                {
                    newTrackWriter->resolutionID = 0x8c;
                    newTrackWriter->horizSubsampling = 4;
                    newTrackWriter->vertSubsampling = 1;
                    newTrackWriter->frameLayout = MXF_SEPARATE_FIELDS;
                }
            }
            newTrackWriter->imageAspectRatio = filePackage->essenceInfo.imageAspectRatio;
            newTrackWriter->displayHeight = 0;
            newTrackWriter->displayWidth = 0;
            newTrackWriter->essenceElementKey = MXF_EE_K(DVClipWrapped);
            newTrackWriter->sourceTrackNumber = MXF_DV_TRACK_NUM(0x01, MXF_DV_CLIP_WRAPPED_EE_TYPE, 0x01);
            newTrackWriter->essenceElementLLen = 9;
            CHK_ORET(memcmp(&track->editRate, &clipWriter->projectEditRate, sizeof(mxfRational)) == 0); /* why would it be different? */
            newTrackWriter->sampleRate = track->editRate;
            newTrackWriter->editUnitByteCount = newTrackWriter->frameSize;
            break;

        case DVBased25:
            newTrackWriter->cdciEssenceContainerLabel = g_Null_UL;
            if (clipWriter->projectFormat == PAL_25i)
            {
                /* TODO: this Avid label fails in Xpress HD 5.2.1, but what about earlier versions?
                  newTrackWriter->essenceContainerLabel = g_avid_DV25ClipWrappedEssenceContainer_label; */
                newTrackWriter->essenceContainerLabel = MXF_EC_L(DVBased_25_625_50_ClipWrapped);
                newTrackWriter->frameSize = 144000;
                newTrackWriter->pictureEssenceCoding = MXF_CMDEF_L(DVBased_25_625_50);
                newTrackWriter->storedHeight = 288;
                newTrackWriter->storedWidth = 720;
                newTrackWriter->videoLineMap[0] = 23;
                newTrackWriter->videoLineMap[1] = 335;
                newTrackWriter->videoLineMapLen = 2;
                newTrackWriter->colorSiting = MXF_COLOR_SITING_REC601;
                if (clipWriter->useLegacy)
                {
                    newTrackWriter->resolutionID = 0x8c;
                    newTrackWriter->horizSubsampling = 2;
                    newTrackWriter->vertSubsampling = 2;
                    newTrackWriter->frameLayout = MXF_MIXED_FIELDS;
                }
                else
                {
                    newTrackWriter->resolutionID = 0x8c;
                    newTrackWriter->horizSubsampling = 4;
                    newTrackWriter->vertSubsampling = 1;
                    newTrackWriter->frameLayout = MXF_SEPARATE_FIELDS;
                }
            }
            else
            {
                newTrackWriter->essenceContainerLabel = MXF_EC_L(DVBased_25_525_60_ClipWrapped);
                newTrackWriter->frameSize = 120000;
                newTrackWriter->pictureEssenceCoding = MXF_CMDEF_L(DVBased_25_525_60);
                newTrackWriter->storedHeight = 240;
                newTrackWriter->storedWidth = 720;
                newTrackWriter->videoLineMap[0] = 23;
                newTrackWriter->videoLineMap[1] = 285;
                newTrackWriter->videoLineMapLen = 2;
                newTrackWriter->colorSiting = MXF_COLOR_SITING_REC601;
                if (clipWriter->useLegacy)
                {
                    newTrackWriter->resolutionID = 0x8c;
                    newTrackWriter->horizSubsampling = 1;
                    newTrackWriter->vertSubsampling = 1;
                    newTrackWriter->frameLayout = MXF_MIXED_FIELDS;
                }
                else
                {
                    newTrackWriter->resolutionID = 0x8c;
                    newTrackWriter->horizSubsampling = 4;
                    newTrackWriter->vertSubsampling = 1;
                    newTrackWriter->frameLayout = MXF_SEPARATE_FIELDS;
                }
            }
            newTrackWriter->imageAspectRatio = filePackage->essenceInfo.imageAspectRatio;
            newTrackWriter->displayHeight = 0;
            newTrackWriter->displayWidth = 0;
            newTrackWriter->essenceElementKey = MXF_EE_K(DVClipWrapped);
            newTrackWriter->sourceTrackNumber = MXF_DV_TRACK_NUM(0x01, MXF_DV_CLIP_WRAPPED_EE_TYPE, 0x01);
            newTrackWriter->essenceElementLLen = 9;
            CHK_ORET(memcmp(&track->editRate, &clipWriter->projectEditRate, sizeof(mxfRational)) == 0); /* why would it be different? */
            newTrackWriter->sampleRate = track->editRate;
            newTrackWriter->editUnitByteCount = newTrackWriter->frameSize;
            break;

        case DVBased50:
            newTrackWriter->cdciEssenceContainerLabel = g_Null_UL;
            if (clipWriter->projectFormat == PAL_25i)
            {
                newTrackWriter->essenceContainerLabel = MXF_EC_L(DVBased_50_625_50_ClipWrapped);
                newTrackWriter->frameSize = 288000;
                newTrackWriter->pictureEssenceCoding = MXF_CMDEF_L(DVBased_50_625_50);
                newTrackWriter->storedHeight = 288;
                newTrackWriter->storedWidth = 720;
                newTrackWriter->videoLineMap[0] = 23;
                newTrackWriter->videoLineMap[1] = 335;
                newTrackWriter->videoLineMapLen = 2;
                newTrackWriter->horizSubsampling = 2;
                newTrackWriter->vertSubsampling = 1;
                newTrackWriter->colorSiting = MXF_COLOR_SITING_REC601;
                if (clipWriter->useLegacy)
                {
                    newTrackWriter->frameLayout = MXF_MIXED_FIELDS;
                }
                else
                {
                    newTrackWriter->frameLayout = MXF_SEPARATE_FIELDS;
                }
            }
            else
            {
                newTrackWriter->essenceContainerLabel = MXF_EC_L(DVBased_50_525_60_ClipWrapped);
                newTrackWriter->frameSize = 240000;
                newTrackWriter->pictureEssenceCoding = MXF_CMDEF_L(DVBased_50_525_60);
                newTrackWriter->storedHeight = 240;
                newTrackWriter->storedWidth = 720;
                newTrackWriter->videoLineMap[0] = 23;
                newTrackWriter->videoLineMap[1] = 285;
                newTrackWriter->videoLineMapLen = 2;
                newTrackWriter->horizSubsampling = 2;
                newTrackWriter->vertSubsampling = 1;
                newTrackWriter->frameLayout = MXF_SEPARATE_FIELDS;
                newTrackWriter->colorSiting = MXF_COLOR_SITING_REC601;
            }
            newTrackWriter->imageAspectRatio = filePackage->essenceInfo.imageAspectRatio;
            newTrackWriter->displayHeight = 0;
            newTrackWriter->displayWidth = 0;
            newTrackWriter->essenceElementKey = MXF_EE_K(DVClipWrapped);
            newTrackWriter->sourceTrackNumber = MXF_DV_TRACK_NUM(0x01, MXF_DV_CLIP_WRAPPED_EE_TYPE, 0x01);
            newTrackWriter->essenceElementLLen = 9;
            CHK_ORET(memcmp(&track->editRate, &clipWriter->projectEditRate, sizeof(mxfRational)) == 0); /* why would it be different? */
            newTrackWriter->sampleRate = track->editRate;
            newTrackWriter->resolutionID = 0x8e;
            newTrackWriter->editUnitByteCount = newTrackWriter->frameSize;
            break;

        case DV1080i:
        case DV720p:
            newTrackWriter->cdciEssenceContainerLabel = MXF_EC_L(AvidAAFKLVEssenceContainer);
            newTrackWriter->displayYOffset = 0;
            newTrackWriter->displayXOffset = 0;
            newTrackWriter->colorSiting = MXF_COLOR_SITING_REC601;
            newTrackWriter->horizSubsampling = 2;
            newTrackWriter->vertSubsampling = 1;
            newTrackWriter->imageAspectRatio = filePackage->essenceInfo.imageAspectRatio;

            switch (filePackage->essenceType)
            {
                case DV1080i:        /* SMPTE 370M */
                    newTrackWriter->videoLineMapLen = 2;
                    newTrackWriter->videoLineMap[0] = 21;
                    newTrackWriter->videoLineMap[1] = 584;
                    newTrackWriter->displayHeight = 540;
                    newTrackWriter->displayWidth = 1920;
                    newTrackWriter->sampledHeight = 540;
                    newTrackWriter->sampledWidth = 1920;
                    newTrackWriter->frameLayout = MXF_SEPARATE_FIELDS;
                    newTrackWriter->essenceElementKey = MXF_EE_K(DVClipWrapped);
                    if (clipWriter->projectFormat == PAL_25i)
                    {
                        newTrackWriter->storedHeight = 540;
                        newTrackWriter->storedWidth = 1440;
                        newTrackWriter->frameSize = 576000;
                        newTrackWriter->essenceContainerLabel = MXF_EC_L(DVBased_100_1080_50_I_ClipWrapped);
                        newTrackWriter->pictureEssenceCoding = MXF_CMDEF_L(DVBased_100_1080_50_I);
                    }
                    else
                    {
                        newTrackWriter->storedHeight = 540;
                        newTrackWriter->storedWidth = 1280;
                        newTrackWriter->frameSize = 480000;
                        newTrackWriter->essenceContainerLabel = MXF_EC_L(DVBased_100_1080_60_I_ClipWrapped);
                        newTrackWriter->pictureEssenceCoding = MXF_CMDEF_L(DVBased_100_1080_60_I);
                    }
                    break;
                case DV720p:        /* Standardised in later version of SMPTE 370M */
                    newTrackWriter->videoLineMapLen = 2;
                    newTrackWriter->videoLineMap[0] = 26;
                    newTrackWriter->videoLineMap[1] = 0;
                    newTrackWriter->storedHeight = 720;
                    newTrackWriter->storedWidth = 960;
                    newTrackWriter->displayHeight = 720;
                    newTrackWriter->displayWidth = 1280;
                    newTrackWriter->sampledHeight = 720;
                    newTrackWriter->sampledWidth = 1280;
                    newTrackWriter->frameLayout = MXF_FULL_FRAME;
                    newTrackWriter->essenceElementKey = MXF_EE_K(DVClipWrapped);
                    if (clipWriter->projectFormat == PAL_25i)
                    {
                        newTrackWriter->frameSize = 288000;
                        newTrackWriter->essenceContainerLabel = MXF_EC_L(DVBased_100_720_50_P_ClipWrapped);
                        newTrackWriter->pictureEssenceCoding = MXF_CMDEF_L(DVBased_100_720_50_P);
                    }
                    else
                    {
                        newTrackWriter->frameSize = 240000;
                        newTrackWriter->essenceContainerLabel = MXF_EC_L(DVBased_100_720_60_P_ClipWrapped);
                        newTrackWriter->pictureEssenceCoding = MXF_CMDEF_L(DVBased_100_720_60_P);
                    }
                    break;
                default:
                    assert(0);
                    return 0;
            }
            /* Note: no ResolutionID is set in DV100 files */

            newTrackWriter->sourceTrackNumber = MXF_DV_TRACK_NUM(0x01, MXF_DV_CLIP_WRAPPED_EE_TYPE, 0x01);
            newTrackWriter->essenceElementLLen = 9;
            CHK_ORET(memcmp(&track->editRate, &clipWriter->projectEditRate, sizeof(mxfRational)) == 0); /* why would it be different? */
            newTrackWriter->sampleRate = track->editRate;
            newTrackWriter->editUnitByteCount = newTrackWriter->frameSize;
            break;

        case IMX30:
        case IMX40:
        case IMX50:
            newTrackWriter->cdciEssenceContainerLabel = g_Null_UL;
            if (clipWriter->projectFormat == PAL_25i)
            {
                switch (filePackage->essenceType)
                {
                    case IMX30:
                        newTrackWriter->frameSize = filePackage->essenceInfo.imxFrameSize;
                        newTrackWriter->essenceContainerLabel = MXF_EC_L(AvidIMX30_625_50);
                        newTrackWriter->pictureEssenceCoding = MXF_CMDEF_L(D10_30_625_50);
                        newTrackWriter->resolutionID = 162;
                        break;
                    case IMX40:
                        newTrackWriter->frameSize = filePackage->essenceInfo.imxFrameSize;
                        newTrackWriter->essenceContainerLabel = MXF_EC_L(AvidIMX40_625_50);
                        newTrackWriter->pictureEssenceCoding = MXF_CMDEF_L(D10_40_625_50);
                        newTrackWriter->resolutionID = 161;
                        break;
                    case IMX50:
                        newTrackWriter->frameSize = filePackage->essenceInfo.imxFrameSize;
                        newTrackWriter->essenceContainerLabel = MXF_EC_L(AvidIMX50_625_50);
                        newTrackWriter->pictureEssenceCoding = MXF_CMDEF_L(D10_50_625_50);
                        newTrackWriter->resolutionID = 160;
                        break;
                    default:
                        assert(0);
                        return 0;
                }
                newTrackWriter->displayHeight = 288;
                newTrackWriter->displayWidth = 720;
                newTrackWriter->displayYOffset = 16;
                newTrackWriter->displayXOffset = 0;
                newTrackWriter->storedHeight = 304;
                newTrackWriter->storedWidth = 720;
                newTrackWriter->videoLineMap[0] = 7;
                newTrackWriter->videoLineMap[1] = 320;
                newTrackWriter->videoLineMapLen = 2;
                newTrackWriter->horizSubsampling = 2;
                newTrackWriter->vertSubsampling = 1;
                newTrackWriter->colorSiting = MXF_COLOR_SITING_REC601;
                newTrackWriter->frameLayout = MXF_SEPARATE_FIELDS;
            }
            else
            {
                switch (filePackage->essenceType)
                {
                    case IMX30:
                        newTrackWriter->frameSize = filePackage->essenceInfo.imxFrameSize;
                        newTrackWriter->essenceContainerLabel = MXF_EC_L(AvidIMX30_525_60);
                        newTrackWriter->pictureEssenceCoding = MXF_CMDEF_L(D10_30_525_60);
                        newTrackWriter->resolutionID = 162;
                        break;
                    case IMX40:
                        newTrackWriter->frameSize = filePackage->essenceInfo.imxFrameSize;
                        newTrackWriter->essenceContainerLabel = MXF_EC_L(AvidIMX40_525_60);
                        newTrackWriter->pictureEssenceCoding = MXF_CMDEF_L(D10_30_525_60);
                        newTrackWriter->resolutionID = 161;
                        break;
                    case IMX50:
                        newTrackWriter->frameSize = filePackage->essenceInfo.imxFrameSize;
                        newTrackWriter->essenceContainerLabel = MXF_EC_L(AvidIMX50_525_60);
                        newTrackWriter->pictureEssenceCoding = MXF_CMDEF_L(D10_30_525_60);
                        newTrackWriter->resolutionID = 160;
                        break;
                    default:
                        assert(0);
                        return 0;
                }
                newTrackWriter->displayHeight = 243;
                newTrackWriter->displayWidth = 720;
                newTrackWriter->displayYOffset = 13;
                newTrackWriter->displayXOffset = 0;
                newTrackWriter->storedHeight = 256;
                newTrackWriter->storedWidth = 720;
                newTrackWriter->videoLineMap[0] = 7;
                newTrackWriter->videoLineMap[1] = 270;
                newTrackWriter->videoLineMapLen = 2;
                newTrackWriter->horizSubsampling = 2;
                newTrackWriter->vertSubsampling = 1;
                newTrackWriter->colorSiting = MXF_COLOR_SITING_REC601;
                newTrackWriter->frameLayout = MXF_SEPARATE_FIELDS;
            }
            newTrackWriter->imageAspectRatio = filePackage->essenceInfo.imageAspectRatio;
            newTrackWriter->essenceElementKey = MXF_EE_K(IMX);
            newTrackWriter->sourceTrackNumber = MXF_D10_PICTURE_TRACK_NUM(0x01);
            newTrackWriter->essenceElementLLen = 9;
            CHK_ORET(memcmp(&track->editRate, &clipWriter->projectEditRate, sizeof(mxfRational)) == 0);
            newTrackWriter->sampleRate = track->editRate;
            newTrackWriter->editUnitByteCount = newTrackWriter->frameSize;
            break;

        case DNxHD1080p1235:
        case DNxHD1080p1237:
        case DNxHD1080p1238:
        case DNxHD1080i1241:
        case DNxHD1080i1242:
        case DNxHD1080i1243:
        case DNxHD720p1250:
        case DNxHD720p1251:
        case DNxHD720p1252:
        case DNxHD1080p1253:
            newTrackWriter->cdciEssenceContainerLabel = MXF_EC_L(AvidAAFKLVEssenceContainer);
            newTrackWriter->displayYOffset = 0;
            newTrackWriter->displayXOffset = 0;
            newTrackWriter->colorSiting = MXF_COLOR_SITING_REC601;
            newTrackWriter->horizSubsampling = 2;
            newTrackWriter->vertSubsampling = 1;
            newTrackWriter->imageAlignmentOffset = 8192;
            newTrackWriter->essenceElementKey = MXF_EE_K(DNxHD);
            newTrackWriter->pictureEssenceCoding = MXF_CMDEF_L(DNxHD);

            switch (filePackage->essenceType)
            {
                case DNxHD1080p1235:
                    newTrackWriter->componentDepth = 10;
                    newTrackWriter->videoLineMap[0] = 42;
                    newTrackWriter->videoLineMap[1] = 0;
                    newTrackWriter->videoLineMapLen = 2;
                    newTrackWriter->storedHeight = 1080;
                    newTrackWriter->storedWidth = 1920;
                    newTrackWriter->displayHeight = 1080;
                    newTrackWriter->displayWidth = 1920;
                    newTrackWriter->frameLayout = MXF_FULL_FRAME;
                    newTrackWriter->essenceContainerLabel = MXF_EC_L(DNxHD1080p1235ClipWrapped);
                    newTrackWriter->resolutionID = 1235;
                    newTrackWriter->frameSize = 917504;
                    break;
                case DNxHD1080p1237:
                    newTrackWriter->componentDepth = 8;
                    newTrackWriter->videoLineMap[0] = 42;
                    newTrackWriter->videoLineMap[1] = 0;
                    newTrackWriter->videoLineMapLen = 2;
                    newTrackWriter->storedHeight = 1080;
                    newTrackWriter->storedWidth = 1920;
                    newTrackWriter->displayHeight = 1080;
                    newTrackWriter->displayWidth = 1920;
                    newTrackWriter->frameLayout = MXF_FULL_FRAME;
                    newTrackWriter->essenceContainerLabel = MXF_EC_L(DNxHD1080p1237ClipWrapped);
                    newTrackWriter->resolutionID = 1237;
                    newTrackWriter->frameSize = 606208;
                    break;
                case DNxHD1080p1238:
                    newTrackWriter->componentDepth = 8;
                    newTrackWriter->videoLineMap[0] = 42;
                    newTrackWriter->videoLineMap[1] = 0;
                    newTrackWriter->videoLineMapLen = 2;
                    newTrackWriter->storedHeight = 1080;
                    newTrackWriter->storedWidth = 1920;
                    newTrackWriter->displayHeight = 1080;
                    newTrackWriter->displayWidth = 1920;
                    newTrackWriter->frameLayout = MXF_FULL_FRAME;
                    newTrackWriter->essenceContainerLabel = MXF_EC_L(DNxHD1080p1238ClipWrapped);
                    newTrackWriter->resolutionID = 1238;
                    newTrackWriter->frameSize = 917504;
                    break;
                case DNxHD1080i1241:
                    newTrackWriter->componentDepth = 10;
                    newTrackWriter->videoLineMap[0] = 21;
                    newTrackWriter->videoLineMap[1] = 584;
                    newTrackWriter->videoLineMapLen = 2;
                    newTrackWriter->storedHeight = 540;
                    newTrackWriter->storedWidth = 1920;
                    newTrackWriter->displayHeight = 540;
                    newTrackWriter->displayWidth = 1920;
                    newTrackWriter->frameLayout = MXF_SEPARATE_FIELDS;
                    newTrackWriter->essenceContainerLabel = MXF_EC_L(DNxHD1080i1241ClipWrapped);
                    newTrackWriter->resolutionID = 1241;
                    newTrackWriter->frameSize = 917504;
                    break;
                case DNxHD1080i1242:
                    newTrackWriter->componentDepth = 8;
                    newTrackWriter->videoLineMap[0] = 21;
                    newTrackWriter->videoLineMap[1] = 584;
                    newTrackWriter->videoLineMapLen = 2;
                    newTrackWriter->storedHeight = 540;
                    newTrackWriter->storedWidth = 1920;
                    newTrackWriter->displayHeight = 540;
                    newTrackWriter->displayWidth = 1920;
                    newTrackWriter->frameLayout = MXF_SEPARATE_FIELDS;
                    newTrackWriter->essenceContainerLabel = MXF_EC_L(DNxHD1080i1242ClipWrapped);
                    newTrackWriter->resolutionID = 1242;
                    newTrackWriter->frameSize = 606208;
                    break;
                case DNxHD1080i1243:
                    newTrackWriter->componentDepth = 8;
                    newTrackWriter->videoLineMap[0] = 21;
                    newTrackWriter->videoLineMap[1] = 584;
                    newTrackWriter->videoLineMapLen = 2;
                    newTrackWriter->storedHeight = 540;
                    newTrackWriter->storedWidth = 1920;
                    newTrackWriter->displayHeight = 540;
                    newTrackWriter->displayWidth = 1920;
                    newTrackWriter->frameLayout = MXF_SEPARATE_FIELDS;
                    newTrackWriter->essenceContainerLabel = MXF_EC_L(DNxHD1080i1243ClipWrapped);
                    newTrackWriter->resolutionID = 1243;
                    newTrackWriter->frameSize = 917504;
                    break;
                case DNxHD720p1250:
                    newTrackWriter->componentDepth = 10;
                    newTrackWriter->videoLineMap[0] = 26;
                    newTrackWriter->videoLineMap[1] = 0;
                    newTrackWriter->videoLineMapLen = 2;
                    newTrackWriter->storedWidth = 1280;
                    newTrackWriter->storedHeight = 720;
                    newTrackWriter->displayWidth = 1280;
                    newTrackWriter->displayHeight = 720;
                    newTrackWriter->frameLayout = MXF_FULL_FRAME;
                    newTrackWriter->essenceContainerLabel = MXF_EC_L(DNxHD720p1250ClipWrapped);
                    newTrackWriter->resolutionID = 1250;
                    newTrackWriter->frameSize = 458752;
                    break;
                case DNxHD720p1251:
                    newTrackWriter->componentDepth = 8;
                    newTrackWriter->videoLineMap[0] = 26;
                    newTrackWriter->videoLineMap[1] = 0;
                    newTrackWriter->videoLineMapLen = 2;
                    newTrackWriter->storedWidth = 1280;
                    newTrackWriter->storedHeight = 720;
                    newTrackWriter->displayWidth = 1280;
                    newTrackWriter->displayHeight = 720;
                    newTrackWriter->frameLayout = MXF_FULL_FRAME;
                    newTrackWriter->essenceContainerLabel = MXF_EC_L(DNxHD720p1251ClipWrapped);
                    newTrackWriter->resolutionID = 1251;
                    newTrackWriter->frameSize = 458752;
                    break;
                case DNxHD720p1252:
                    newTrackWriter->componentDepth = 8;
                    newTrackWriter->videoLineMap[0] = 26;
                    newTrackWriter->videoLineMap[1] = 0;
                    newTrackWriter->videoLineMapLen = 2;
                    newTrackWriter->storedWidth = 1280;
                    newTrackWriter->storedHeight = 720;
                    newTrackWriter->displayWidth = 1280;
                    newTrackWriter->displayHeight = 720;
                    newTrackWriter->frameLayout = MXF_FULL_FRAME;
                    newTrackWriter->essenceContainerLabel = MXF_EC_L(DNxHD720p1252ClipWrapped);
                    newTrackWriter->resolutionID = 1252;
                    newTrackWriter->frameSize = 303104;
                    break;
                case DNxHD1080p1253:
                    newTrackWriter->componentDepth = 8;
                    newTrackWriter->videoLineMap[0] = 42;
                    newTrackWriter->videoLineMap[1] = 0;
                    newTrackWriter->videoLineMapLen = 2;
                    newTrackWriter->storedHeight = 1080;
                    newTrackWriter->storedWidth = 1920;
                    newTrackWriter->displayHeight = 1080;
                    newTrackWriter->displayWidth = 1920;
                    newTrackWriter->frameLayout = MXF_FULL_FRAME;
                    newTrackWriter->essenceContainerLabel = MXF_EC_L(DNxHD1080p1253ClipWrapped);
                    newTrackWriter->resolutionID = 1253;
                    newTrackWriter->frameSize = 188416;
                    break;
                default:
                    assert(0);
                    return 0;
            }
            newTrackWriter->imageAspectRatio = filePackage->essenceInfo.imageAspectRatio;
            newTrackWriter->sourceTrackNumber = MXF_DNXHD_PICT_TRACK_NUM;
            newTrackWriter->essenceElementLLen = 9;
            CHK_ORET(memcmp(&track->editRate, &clipWriter->projectEditRate, sizeof(mxfRational)) == 0); /* why would it be different? */
            newTrackWriter->sampleRate = track->editRate;
            newTrackWriter->editUnitByteCount = newTrackWriter->frameSize;
            break;

        case UncUYVY:
            /* TODO: check whether the same thing must be done as MJPEG regarding the ess container label
            in the descriptor */
            newTrackWriter->cdciEssenceContainerLabel = MXF_EC_L(AvidAAFKLVEssenceContainer);
            if (clipWriter->projectFormat == PAL_25i)
            {
                newTrackWriter->essenceContainerLabel = MXF_EC_L(SD_Unc_625_50i_422_135_ClipWrapped);
                newTrackWriter->frameSize = g_uncAlignedPALFrameSize;
                newTrackWriter->storedHeight = 592;
                newTrackWriter->storedWidth = 720;
                newTrackWriter->displayHeight = 576;
                newTrackWriter->displayWidth = 720;
                newTrackWriter->displayYOffset = 16;
                newTrackWriter->displayXOffset = 0;
                newTrackWriter->videoLineMap[0] = 15;
                newTrackWriter->videoLineMap[1] = 328;
                newTrackWriter->videoLineMapLen = 2;
                newTrackWriter->horizSubsampling = 2;
                newTrackWriter->vertSubsampling = 1;
                newTrackWriter->colorSiting = MXF_COLOR_SITING_REC601;
                newTrackWriter->frameLayout = MXF_MIXED_FIELDS;
                newTrackWriter->imageAlignmentOffset = g_uncImageAlignmentOffset;
                newTrackWriter->imageStartOffset = g_uncPALStartOffsetSize;

                /* vbiSize is modified below if input height > display height */
                newTrackWriter->vbiSize = g_uncPALVBISize;
            }
            else
            {
                newTrackWriter->essenceContainerLabel = MXF_EC_L(SD_Unc_525_5994i_422_135_ClipWrapped);
                newTrackWriter->frameSize = g_uncAlignedNTSCFrameSize;
                newTrackWriter->storedHeight = 496;
                newTrackWriter->storedWidth = 720;
                newTrackWriter->displayHeight = 486;
                newTrackWriter->displayWidth = 720;
                newTrackWriter->displayXOffset = 0;
                newTrackWriter->displayYOffset = 10;
                newTrackWriter->videoLineMap[0] = 16;
                newTrackWriter->videoLineMap[1] = 278;
                newTrackWriter->videoLineMapLen = 2;
                newTrackWriter->horizSubsampling = 2;
                newTrackWriter->vertSubsampling = 1;
                newTrackWriter->colorSiting = MXF_COLOR_SITING_REC601;
                newTrackWriter->frameLayout = MXF_MIXED_FIELDS;
                newTrackWriter->imageAlignmentOffset = g_uncImageAlignmentOffset;
                newTrackWriter->imageStartOffset = g_uncNTSCStartOffsetSize;

                /* vbiSize is modified below if input height > display height */
                newTrackWriter->vbiSize = g_uncNTSCVBISize;
            }

            CHK_ORET(filePackage->essenceInfo.inputHeight == 0 ||
                     filePackage->essenceInfo.inputHeight >= newTrackWriter->displayHeight);
            if (filePackage->essenceInfo.inputHeight > newTrackWriter->displayHeight)
            {
                newTrackWriter->vbiSize = newTrackWriter->storedWidth *
                    ((int64_t)newTrackWriter->storedHeight - filePackage->essenceInfo.inputHeight) * 2;
                /* note that vbiSize can be negative */
            }
            if (newTrackWriter->vbiSize > 0)
            {
                CHK_MALLOC_ARRAY_OFAIL(newTrackWriter->vbiData, uint8_t, (uint32_t)newTrackWriter->vbiSize);
                for (i = 0; i < (uint32_t)newTrackWriter->vbiSize / 4; i++)
                {
                    newTrackWriter->vbiData[i * 4    ] = 0x80; // U
                    newTrackWriter->vbiData[i * 4 + 1] = 0x10; // Y
                    newTrackWriter->vbiData[i * 4 + 2] = 0x80; // V
                    newTrackWriter->vbiData[i * 4 + 3] = 0x10; // Y 
                }
            }

            CHK_MALLOC_ARRAY_OFAIL(newTrackWriter->startOffsetData, uint8_t, newTrackWriter->imageStartOffset);
            memset(newTrackWriter->startOffsetData, 0, newTrackWriter->imageStartOffset);

            newTrackWriter->imageAspectRatio = filePackage->essenceInfo.imageAspectRatio;
            newTrackWriter->essenceElementKey = MXF_EE_K(UncClipWrapped);
            newTrackWriter->sourceTrackNumber = MXF_UNC_TRACK_NUM(0x01, MXF_UNC_CLIP_WRAPPED_EE_TYPE, 0x01);
            newTrackWriter->essenceElementLLen = 9;
            CHK_ORET(memcmp(&track->editRate, &clipWriter->projectEditRate, sizeof(mxfRational)) == 0); /* why would it be different? */
            newTrackWriter->sampleRate = track->editRate;
            newTrackWriter->resolutionID = 0xaa;
            newTrackWriter->editUnitByteCount = newTrackWriter->frameSize;
            break;

        case Unc1080iUYVY:
        case Unc1080pUYVY:
            newTrackWriter->cdciEssenceContainerLabel = MXF_EC_L(AvidAAFKLVEssenceContainer);
            newTrackWriter->codingEquationsLabel = ITUR_BT709_CODING_EQ;
            newTrackWriter->frameSize = g_uncAligned1080FrameSize;
            newTrackWriter->storedHeight = 1080;
            newTrackWriter->storedWidth = 1920;
            newTrackWriter->displayHeight = 1080;
            newTrackWriter->displayWidth = 1920;
            newTrackWriter->displayYOffset = 0;
            newTrackWriter->displayXOffset = 0;
            newTrackWriter->horizSubsampling = 2;
            newTrackWriter->vertSubsampling = 1;
            newTrackWriter->colorSiting = MXF_COLOR_SITING_REC601;
            newTrackWriter->imageAlignmentOffset = g_uncImageAlignmentOffset;
            newTrackWriter->imageStartOffset = g_unc1080StartOffsetSize;
            if (filePackage->essenceType == Unc1080iUYVY)
            {
                if (clipWriter->projectEditRate.numerator == 25 && clipWriter->projectEditRate.denominator == 1)
                {
                    newTrackWriter->essenceContainerLabel = MXF_EC_L(HD_Unc_1080_50i_422_ClipWrapped);
                }
                else if (clipWriter->projectEditRate.numerator == 30000 && clipWriter->projectEditRate.denominator == 1001)
                {
                    newTrackWriter->essenceContainerLabel = MXF_EC_L(HD_Unc_1080_5994i_422_ClipWrapped);
                }
                else
                {
                    mxf_log_error("Unsupported project edit rate %d/%d" LOG_LOC_FORMAT,
                                  clipWriter->projectEditRate.numerator, clipWriter->projectEditRate.denominator,
                                  LOG_LOC_PARAMS);
                    goto fail;
                }
                newTrackWriter->videoLineMapLen = 2;
                newTrackWriter->videoLineMap[0] = 21;
                newTrackWriter->videoLineMap[1] = 584;
                newTrackWriter->frameLayout = MXF_MIXED_FIELDS;
            }
            else
            {
                if (clipWriter->projectEditRate.numerator == 25 && clipWriter->projectEditRate.denominator == 1)
                {
                    newTrackWriter->essenceContainerLabel = MXF_EC_L(HD_Unc_1080_25p_422_ClipWrapped);
                }
                else if (clipWriter->projectEditRate.numerator == 30000 && clipWriter->projectEditRate.denominator == 1001)
                {
                    newTrackWriter->essenceContainerLabel = MXF_EC_L(HD_Unc_1080_2997p_422_ClipWrapped);
                }
                else if (clipWriter->projectEditRate.numerator == 30 && clipWriter->projectEditRate.denominator == 1)
                {
                    newTrackWriter->essenceContainerLabel = MXF_EC_L(HD_Unc_1080_30p_422_ClipWrapped);
                }
                else if (clipWriter->projectEditRate.numerator == 50 && clipWriter->projectEditRate.denominator == 1)
                {
                    newTrackWriter->essenceContainerLabel = MXF_EC_L(HD_Unc_1080_50p_422_ClipWrapped);
                }
                else if (clipWriter->projectEditRate.numerator == 60000 && clipWriter->projectEditRate.denominator == 1001)
                {
                    newTrackWriter->essenceContainerLabel = MXF_EC_L(HD_Unc_1080_5994p_422_ClipWrapped);
                }
                else if (clipWriter->projectEditRate.numerator == 60 && clipWriter->projectEditRate.denominator == 1)
                {
                    newTrackWriter->essenceContainerLabel = MXF_EC_L(HD_Unc_1080_60p_422_ClipWrapped);
                }
                else
                {
                    mxf_log_error("Unsupported project edit rate %d/%d" LOG_LOC_FORMAT,
                                  clipWriter->projectEditRate.numerator, clipWriter->projectEditRate.denominator,
                                  LOG_LOC_PARAMS);
                    goto fail;
                }
                newTrackWriter->videoLineMapLen = 2;
                newTrackWriter->videoLineMap[0] = 42;
                newTrackWriter->videoLineMap[1] = 0;
                newTrackWriter->frameLayout = MXF_FULL_FRAME;
            }

            CHK_MALLOC_ARRAY_OFAIL(newTrackWriter->startOffsetData, uint8_t, newTrackWriter->imageStartOffset);
            memset(newTrackWriter->startOffsetData, 0, newTrackWriter->imageStartOffset);

            newTrackWriter->imageAspectRatio = filePackage->essenceInfo.imageAspectRatio;
            newTrackWriter->essenceElementKey = MXF_EE_K(UncClipWrapped);
            newTrackWriter->sourceTrackNumber = MXF_UNC_TRACK_NUM(0x01, MXF_UNC_CLIP_WRAPPED_EE_TYPE, 0x01);
            newTrackWriter->essenceElementLLen = 9;
            CHK_ORET(memcmp(&track->editRate, &clipWriter->projectEditRate, sizeof(mxfRational)) == 0); /* why would it be different? */
            newTrackWriter->sampleRate = track->editRate;
            newTrackWriter->editUnitByteCount = newTrackWriter->frameSize;
            break;

        case Unc720pUYVY:
            newTrackWriter->cdciEssenceContainerLabel = MXF_EC_L(AvidAAFKLVEssenceContainer);
            newTrackWriter->codingEquationsLabel = ITUR_BT709_CODING_EQ;
            if (clipWriter->projectEditRate.numerator == 25 && clipWriter->projectEditRate.denominator == 1)
            {
                newTrackWriter->essenceContainerLabel = MXF_EC_L(HD_Unc_720_25p_422_ClipWrapped);
            }
            else if (clipWriter->projectEditRate.numerator == 30000 && clipWriter->projectEditRate.denominator == 1001)
            {
                newTrackWriter->essenceContainerLabel = MXF_EC_L(HD_Unc_720_2997p_422_ClipWrapped);
            }
            else if (clipWriter->projectEditRate.numerator == 30 && clipWriter->projectEditRate.denominator == 1)
            {
                newTrackWriter->essenceContainerLabel = MXF_EC_L(HD_Unc_720_30p_422_ClipWrapped);
            }
            else if (clipWriter->projectEditRate.numerator == 50 && clipWriter->projectEditRate.denominator == 1)
            {
                newTrackWriter->essenceContainerLabel = MXF_EC_L(HD_Unc_720_50p_422_ClipWrapped);
            }
            else if (clipWriter->projectEditRate.numerator == 60000 && clipWriter->projectEditRate.denominator == 1001)
            {
                newTrackWriter->essenceContainerLabel = MXF_EC_L(HD_Unc_720_5994p_422_ClipWrapped);
            }
            else if (clipWriter->projectEditRate.numerator == 60 && clipWriter->projectEditRate.denominator == 1)
            {
                newTrackWriter->essenceContainerLabel = MXF_EC_L(HD_Unc_720_60p_422_ClipWrapped);
            }
            else
            {
                mxf_log_error("Unsupported project edit rate %d/%d" LOG_LOC_FORMAT,
                              clipWriter->projectEditRate.numerator, clipWriter->projectEditRate.denominator,
                              LOG_LOC_PARAMS);
                goto fail;
            }
            newTrackWriter->frameSize = g_unc720pFrameSize;
            newTrackWriter->storedHeight = 720;
            newTrackWriter->storedWidth = 1280;
            newTrackWriter->displayHeight = 720;
            newTrackWriter->displayWidth = 1280;
            newTrackWriter->displayYOffset = 0;
            newTrackWriter->displayXOffset = 0;
            newTrackWriter->videoLineMap[0] = 26;
            newTrackWriter->videoLineMap[1] = 0;
            newTrackWriter->videoLineMapLen = 2;
            newTrackWriter->horizSubsampling = 2;
            newTrackWriter->vertSubsampling = 1;
            newTrackWriter->colorSiting = MXF_COLOR_SITING_REC601;
            newTrackWriter->frameLayout = MXF_FULL_FRAME;
            newTrackWriter->imageAlignmentOffset = g_uncImageAlignmentOffset;
            newTrackWriter->imageStartOffset = 0;
            newTrackWriter->imageAspectRatio = filePackage->essenceInfo.imageAspectRatio;
            newTrackWriter->essenceElementKey = MXF_EE_K(UncClipWrapped);
            newTrackWriter->sourceTrackNumber = MXF_UNC_TRACK_NUM(0x01, MXF_UNC_CLIP_WRAPPED_EE_TYPE, 0x01);
            newTrackWriter->essenceElementLLen = 9;
            CHK_ORET(memcmp(&track->editRate, &clipWriter->projectEditRate, sizeof(mxfRational)) == 0); /* why would it be different? */
            newTrackWriter->sampleRate = track->editRate;
            newTrackWriter->editUnitByteCount = newTrackWriter->frameSize;
            break;

        case PCM:
            newTrackWriter->samplingRate.numerator = 48000;
            newTrackWriter->samplingRate.denominator = 1;
            CHK_ORET(memcmp(&track->editRate, &clipWriter->projectEditRate, sizeof(mxfRational)) == 0 ||
                memcmp(&track->editRate, &newTrackWriter->samplingRate, sizeof(mxfRational)) == 0); /* why would it be different? */
            newTrackWriter->sampleRate = track->editRate;
            newTrackWriter->essenceContainerLabel = MXF_EC_L(BWFClipWrapped);
            newTrackWriter->essenceElementKey = MXF_EE_K(BWFClipWrapped);
            newTrackWriter->sourceTrackNumber = MXF_AES3BWF_TRACK_NUM(0x01, MXF_BWF_CLIP_WRAPPED_EE_TYPE, 0x01);
            newTrackWriter->essenceElementLLen = 9;
            newTrackWriter->bitsPerSample = filePackage->essenceInfo.pcmBitsPerSample;
            newTrackWriter->blockAlign = (uint8_t)((newTrackWriter->bitsPerSample + 7) / 8);
            newTrackWriter->avgBps = newTrackWriter->blockAlign * 48000;
            if (memcmp(&newTrackWriter->sampleRate, &newTrackWriter->samplingRate, sizeof(mxfRational)) == 0)
            {
                newTrackWriter->editUnitByteCount = newTrackWriter->blockAlign;
            }
            else
            {
                double factor = newTrackWriter->samplingRate.numerator * newTrackWriter->sampleRate.denominator /
                    (double)(newTrackWriter->samplingRate.denominator * newTrackWriter->sampleRate.numerator);
                newTrackWriter->editUnitByteCount = (uint32_t)(newTrackWriter->blockAlign * factor + 0.5);
            }
            newTrackWriter->locked = filePackage->essenceInfo.locked;
            newTrackWriter->audioRefLevel = filePackage->essenceInfo.audioRefLevel;
            newTrackWriter->dialNorm = filePackage->essenceInfo.dialNorm;
            newTrackWriter->sequenceOffset = filePackage->essenceInfo.sequenceOffset;
            break;
        default:
            assert(0);
    };

    if (clipWriter->useLegacy)
    {
        newTrackWriter->pictureDataDef = MXF_DDEF_L(LegacyPicture);
        newTrackWriter->soundDataDef = MXF_DDEF_L(LegacySound);
        newTrackWriter->timecodeDataDef = MXF_DDEF_L(LegacyTimecode);
    }
    else
    {
        newTrackWriter->pictureDataDef = MXF_DDEF_L(Picture);
        newTrackWriter->soundDataDef = MXF_DDEF_L(Sound);
        newTrackWriter->timecodeDataDef = MXF_DDEF_L(Timecode);
    }
    newTrackWriter->dmDataDef = MXF_DDEF_L(DescriptiveMetadata);


    /* create the header metadata */

    CHK_OFAIL(create_header_metadata(clipWriter, packageDefinitions, filePackage, newTrackWriter));


    /* open the file */

    CHK_OFAIL(mxf_create_file_partitions(&newTrackWriter->partitions));
    CHK_OFAIL(mxf_disk_file_open_new(newTrackWriter->filename, &newTrackWriter->mxfFile));


    /* set the minimum llen - Avid uses llen=9 everywhere */
    mxf_file_set_min_llen(newTrackWriter->mxfFile, 9);


    /* write the (incomplete) header partition pack */

    CHK_OFAIL(mxf_append_new_partition(newTrackWriter->partitions, &newTrackWriter->headerPartition));
    newTrackWriter->headerPartition->key = MXF_PP_K(ClosedIncomplete, Header);
    newTrackWriter->headerPartition->kagSize = 0x100;
    newTrackWriter->headerPartition->operationalPattern = MXF_OP_L(atom, NTracks_NSourceClips);
    CHK_OFAIL(mxf_append_partition_esscont_label(newTrackWriter->headerPartition, &newTrackWriter->essenceContainerLabel));

    CHK_OFAIL(mxf_write_partition(newTrackWriter->mxfFile, newTrackWriter->headerPartition));
    CHK_OFAIL(mxf_fill_to_kag(newTrackWriter->mxfFile, newTrackWriter->headerPartition));


    /* write header metadata with avid extensions */

    CHK_OFAIL((newTrackWriter->headerMetadataFilePos = mxf_file_tell(newTrackWriter->mxfFile)) >= 0);

    CHK_OFAIL(mxf_mark_header_start(newTrackWriter->mxfFile, newTrackWriter->headerPartition));
    CHK_OFAIL(mxf_avid_write_header_metadata(newTrackWriter->mxfFile, newTrackWriter->headerMetadata, newTrackWriter->headerPartition));
    CHK_ORET(mxf_fill_to_position(newTrackWriter->mxfFile, g_fixedBodyPPOffset));
    CHK_OFAIL(mxf_mark_header_end(newTrackWriter->mxfFile, newTrackWriter->headerPartition));


    /* write the body partition pack */

    CHK_OFAIL(mxf_append_new_from_partition(newTrackWriter->partitions, newTrackWriter->headerPartition, &newTrackWriter->bodyPartition));
    newTrackWriter->bodyPartition->key = MXF_PP_K(ClosedComplete, Body);
    if (newTrackWriter->essenceType == PCM) /* audio */
    {
        newTrackWriter->bodyPartition->kagSize = 0x1000;
    }
    else /* video */
    {
        newTrackWriter->bodyPartition->kagSize = 0x20000;
    }
    newTrackWriter->bodyPartition->bodySID = g_bodySID;

    CHK_OFAIL((filePos = mxf_file_tell(newTrackWriter->mxfFile)) >= 0);
    CHK_OFAIL(mxf_write_partition(newTrackWriter->mxfFile, newTrackWriter->bodyPartition));
    /* place the first byte of the essence data at the KAG boundary taking into account that the body
       partition offset (g_fixedBodyPPOffset) is 0x20 bytes (why?) beyond where it would be expected to be.
       57 = 0x20 + 16 + 9. This assumes a 9 byte llen */
    CHK_OFAIL(mxf_fill_to_position(newTrackWriter->mxfFile, filePos + newTrackWriter->bodyPartition->kagSize - 57));


    /* update the partitions */

    CHK_OFAIL((filePos = mxf_file_tell(newTrackWriter->mxfFile)) >= 0);
    CHK_OFAIL(mxf_update_partitions(newTrackWriter->mxfFile, newTrackWriter->partitions));
    CHK_OFAIL(mxf_file_seek(newTrackWriter->mxfFile, filePos, SEEK_SET));


    /* open the essence element, ready for writing */

    CHK_OFAIL(mxf_open_essence_element_write(newTrackWriter->mxfFile, &newTrackWriter->essenceElementKey,
                                             newTrackWriter->essenceElementLLen, 0, &newTrackWriter->essenceElement));


    /* set the new writer */

    clipWriter->tracks[clipWriter->numTracks] = newTrackWriter;
    clipWriter->numTracks++;


    return 1;

fail:
    free_track_writer(&newTrackWriter);
    return 0;
}




int create_clip_writer(const char *projectName, ProjectFormat projectFormat, mxfRational projectEditRate,
                       int dropFrameFlag, int useLegacy, PackageDefinitions *packageDefinitions,
                       AvidClipWriter **clipWriter)
{
    AvidClipWriter *newClipWriter = NULL;
    MXFListIterator iter;

    CHK_ORET(mxf_get_list_length(&packageDefinitions->materialPackage->tracks) <= MAX_TRACKS);


    CHK_MALLOC_ORET(newClipWriter, AvidClipWriter);
    memset(newClipWriter, 0, sizeof(AvidClipWriter));

    if (projectName != NULL)
    {
        CHK_OFAIL(convert_string(newClipWriter, projectName));
        newClipWriter->wProjectName = newClipWriter->wTmpString;
        newClipWriter->wTmpString = NULL;
    }
    newClipWriter->projectFormat = projectFormat;
    newClipWriter->dropFrameFlag = dropFrameFlag;
    newClipWriter->useLegacy = useLegacy;

    newClipWriter->projectEditRate.numerator = projectEditRate.numerator;
    newClipWriter->projectEditRate.denominator = projectEditRate.denominator;

    /* create track writer for each file package */
    mxf_initialise_list_iter(&iter, &packageDefinitions->fileSourcePackages);
    while (mxf_next_list_iter_element(&iter))
    {
        CHK_OFAIL(create_track_writer(newClipWriter, packageDefinitions, (Package*)mxf_get_iter_element(&iter)));
    }

    *clipWriter = newClipWriter;
    return 1;

fail:
    free_avid_clip_writer(&newClipWriter);
    return 0;
}

int write_samples(AvidClipWriter *clipWriter, uint32_t materialTrackID, uint32_t numSamples,
                  const uint8_t *data, uint32_t size)
{
    TrackWriter *writer;
    CHK_ORET(get_track_writer(clipWriter, materialTrackID, &writer));

    switch (writer->essenceType)
    {
        case AvidMJPEG:
            CHK_ORET(numSamples == 1);
            /* update frame offsets array */
            CHK_ORET(add_avid_mjpeg_offset(&writer->mjpegFrameOffsets, writer->prevFrameOffset,
                                           &writer->currentMJPEGOffsetsArray));
            writer->prevFrameOffset += size;
            CHK_ORET(mxf_write_essence_element_data(writer->mxfFile, writer->essenceElement, data, size));
            writer->duration += numSamples;
            break;
        case IECDV25:
        case DVBased25:
        case DVBased50:
        case DV1080i:
        case DV720p:
        case IMX30:
        case IMX40:
        case IMX50:
        case DNxHD1080p1235:
        case DNxHD1080p1237:
        case DNxHD1080p1238:
        case DNxHD1080i1241:
        case DNxHD1080i1242:
        case DNxHD1080i1243:
        case DNxHD720p1250:
        case DNxHD720p1251:
        case DNxHD720p1252:
        case DNxHD1080p1253:
        case Unc720pUYVY:
        case PCM:
            CHK_ORET(size == numSamples * writer->editUnitByteCount);
            CHK_ORET(mxf_write_essence_element_data(writer->mxfFile, writer->essenceElement, data, size));
            writer->duration += numSamples;
            break;
        case UncUYVY:
        case Unc1080iUYVY:
        case Unc1080pUYVY:
            CHK_ORET(numSamples == 1);
            CHK_ORET(size + writer->imageStartOffset + writer->vbiSize == writer->editUnitByteCount);
            CHK_ORET(mxf_write_essence_element_data(writer->mxfFile, writer->essenceElement, writer->startOffsetData,
                                                    writer->imageStartOffset));
            if (writer->vbiSize >= 0)
            {
                if (writer->vbiSize > 0)
                {
                    /* black VBI lines */
                    CHK_ORET(mxf_write_essence_element_data(writer->mxfFile, writer->essenceElement, writer->vbiData,
                                                            (uint32_t)writer->vbiSize));
                }
                CHK_ORET(mxf_write_essence_element_data(writer->mxfFile, writer->essenceElement, data, size));
            }
            else
            {
                /* write input data, but skip extra (-writer->vbiSize) input VBI data */
                CHK_ORET(mxf_write_essence_element_data(writer->mxfFile, writer->essenceElement,
                                                        data - writer->vbiSize, (uint32_t)(size + writer->vbiSize)));
            }
            writer->duration++;
            break;
        default:
            assert(0);
    }

    return 1;
}


int start_write_samples(AvidClipWriter *clipWriter, uint32_t materialTrackID)
{
    TrackWriter *writer;
    CHK_ORET(get_track_writer(clipWriter, materialTrackID, &writer));

    writer->sampleDataSize = 0;

    return 1;
}

int write_sample_data(AvidClipWriter *clipWriter, uint32_t materialTrackID, const uint8_t *data, uint32_t size)
{
    TrackWriter *writer;
    CHK_ORET(get_track_writer(clipWriter, materialTrackID, &writer));

    /* Uncompressed video data has a start offset and SD also has VBI data */
    if ((writer->essenceType == UncUYVY || writer->essenceType == Unc1080iUYVY || writer->essenceType == Unc1080pUYVY) &&
        writer->sampleDataSize == 0)
    {
        CHK_ORET(mxf_write_essence_element_data(writer->mxfFile, writer->essenceElement, writer->startOffsetData,
                                                writer->imageStartOffset));
        if (writer->vbiSize > 0)
        {
            CHK_ORET(mxf_write_essence_element_data(writer->mxfFile, writer->essenceElement, writer->vbiData,
                                                    (uint32_t)writer->vbiSize));
        }
    }

    if (writer->sampleDataSize + writer->vbiSize >= 0)
    {
        CHK_ORET(mxf_write_essence_element_data(writer->mxfFile, writer->essenceElement, data, size));
    }
    else if (writer->sampleDataSize + writer->vbiSize + size > 0)
    {
        /* write data, skipping extra VBI data */
        uint32_t writeSize = (uint32_t)(size - (writer->sampleDataSize + writer->vbiSize));
        CHK_ORET(mxf_write_essence_element_data(writer->mxfFile, writer->essenceElement,
                                                data + (size - writeSize), writeSize));
    }

    writer->sampleDataSize += size;
    return 1;
}

int end_write_samples(AvidClipWriter *clipWriter, uint32_t materialTrackID, uint32_t numSamples)
{
    TrackWriter *writer;
    CHK_ORET(get_track_writer(clipWriter, materialTrackID, &writer));

    switch (writer->essenceType)
    {
        case AvidMJPEG:
            /* Avid MJPEG has variable size sample and indexing currently accepts only 1 sample at a time */
            CHK_ORET(numSamples == 1);
            /* update frame offsets array */
            CHK_ORET(add_avid_mjpeg_offset(&writer->mjpegFrameOffsets, writer->prevFrameOffset,
                                           &writer->currentMJPEGOffsetsArray));
            writer->prevFrameOffset += writer->sampleDataSize;
            writer->duration += numSamples;
            break;
        case IECDV25:
        case DVBased25:
        case DVBased50:
        case DV1080i:
        case DV720p:
        case IMX30:
        case IMX40:
        case IMX50:
        case DNxHD1080p1235:
        case DNxHD1080p1237:
        case DNxHD1080p1238:
        case DNxHD1080i1241:
        case DNxHD1080i1242:
        case DNxHD1080i1243:
        case DNxHD720p1250:
        case DNxHD720p1251:
        case DNxHD720p1252:
        case DNxHD1080p1253:
        case PCM:
            CHK_ORET(writer->sampleDataSize == numSamples * writer->editUnitByteCount);
            writer->duration += numSamples;
            break;
        case UncUYVY:
        case Unc1080iUYVY:
        case Unc1080pUYVY:
            /* Avid uncompressed video requires padding and VBI and currently only accepts 1 sample at a time */
            CHK_ORET(numSamples == 1);
            CHK_ORET((writer->sampleDataSize + writer->imageStartOffset + writer->vbiSize) == writer->editUnitByteCount);
            writer->duration++;
            break;
        default:
            assert(0);
            return 0;
    }

    return 1;
}


int get_num_samples(AvidClipWriter *clipWriter, uint32_t materialTrackID, int64_t *num_samples)
{
    TrackWriter *writer;
    CHK_ORET(get_track_writer(clipWriter, materialTrackID, &writer));

    *num_samples = writer->duration;
    return 1;
}

int get_stored_dimensions(AvidClipWriter *clipWriter, uint32_t materialTrackID, uint32_t *width, uint32_t *height)
{
    TrackWriter *writer;
    CHK_ORET(get_track_writer(clipWriter, materialTrackID, &writer));
    CHK_ORET(IS_PICTURE(writer->essenceType));

    *width = writer->storedWidth;
    *height = writer->storedHeight;
    return 1;
}

void abort_writing(AvidClipWriter **clipWriter, int deleteFile)
{
    int i;
    TrackWriter *trackWriter;

    for (i = 0; i < (*clipWriter)->numTracks; i++)
    {
        trackWriter = (*clipWriter)->tracks[i];

        mxf_file_close(&trackWriter->mxfFile);

        if (deleteFile)
        {
            if (remove(trackWriter->filename) != 0)
            {
                mxf_log_warn("Failed to delete MXF file '%s'" LOG_LOC_FORMAT, trackWriter->filename, LOG_LOC_PARAMS);
            }
        }
    }

    free_avid_clip_writer(clipWriter);
    return;
}

int complete_writing(AvidClipWriter **clipWriter)
{
    return update_and_complete_writing(clipWriter, NULL, NULL);
}

int update_and_complete_writing(AvidClipWriter **clipWriter, PackageDefinitions *packageDefinitions,
                                const char *projectName)
{
    int i;
    Package *filePackage = NULL;

    if (packageDefinitions != NULL)
    {
        if (projectName != NULL)
        {
            SAFE_FREE((*clipWriter)->wProjectName);

            CHK_ORET(convert_string((*clipWriter), projectName));
            (*clipWriter)->wProjectName = (*clipWriter)->wTmpString;
            (*clipWriter)->wTmpString = NULL;
        }
    }

    for (i = 0; i < (*clipWriter)->numTracks; i++)
    {
        if (packageDefinitions != NULL)
        {
            CHK_ORET(get_file_package((*clipWriter)->tracks[i], packageDefinitions, &filePackage));
            CHK_ORET(complete_track(*clipWriter, (*clipWriter)->tracks[i], packageDefinitions, filePackage));
        }
        else
        {
            CHK_ORET(complete_track(*clipWriter, (*clipWriter)->tracks[i], NULL, NULL));
        }
    }

    free_avid_clip_writer(clipWriter);

    return 1;
}

