/*
 * D10 MXF OP-1A writer
 *
 * Copyright (C) 2010, British Broadcasting Corporation
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

#include <cstring>
#include <cstdio>

#include <libMXF++/MXFException.h>

#include "D10MXFOP1AWriter.h"

using namespace std;
using namespace mxfpp;


#define UNKNOWN_SEQUENCE_OFFSET         255

#define LAST_AES3_BLOCK_SEQ_INDEX       (uint8_t)((mAudioSequenceIndex + mAES3Blocks.size() - 1) % mAudioSequenceCount)
#define AES3_BLOCK_SIZE(num_samples)    (4 + num_samples * 32)
#define AES3_BLOCK_NUM_SAMPLES(size)    ((size - 4) / 32)


static const char *DEFAULT_COMPANY_NAME   = "BBC";
static const char *DEFAULT_PRODUCT_NAME   = "Ingex";
static const char *DEFAULT_VERSION_STRING = "0.1";
static const mxfUUID DEFAULT_PRODUCT_UID  =
    {0x93, 0xa1, 0xba, 0xae, 0x41, 0xc5, 0x4a, 0x16, 0x8b, 0x2c, 0x42, 0xe7, 0x23, 0x0d, 0x0a, 0x4e};

static const uint8_t LLEN      = 4;
static const uint32_t KAG_SIZE = 0x200;

static const mxfRational AUDIO_SAMPLING_RATE = {48000, 1};

static const uint32_t INDEX_SID = 1;
static const uint32_t BODY_SID  = 2;

static const mxfKey VIDEO_ELEMENT_KEY = MXF_D10_PICTURE_EE_K(0x00);
static const mxfKey AUDIO_ELEMENT_KEY = MXF_D10_SOUND_EE_K(0x00);

static const mxfKey MXF_EE_K(EmptyPackageMetadataSet) = MXF_SDTI_CP_PACKAGE_METADATA_KEY(0x00);
static const uint32_t SYSTEM_ITEM_METADATA_PACK_SIZE  = 7 + 16 + 17 + 17;



static void convert_timecode_to_12m(Timecode tc, unsigned char *t12m)
{
    // the format follows section 8.2 of SMPTE 331M
    // The Binary Group Data is not used and is set to 0

    memset(t12m, 0, 8);
    if (tc.dropFrame)
        t12m[0] = 0x40; // set drop-frame flag
    t12m[0] |= ((tc.frame % 10) & 0x0f) | (((tc.frame / 10) & 0x3) << 4);
    t12m[1]  = ((tc.sec   % 10) & 0x0f) | (((tc.sec   / 10) & 0x7) << 4);
    t12m[2]  = ((tc.min   % 10) & 0x0f) | (((tc.min   / 10) & 0x7) << 4);
    t12m[3]  = ((tc.hour  % 10) & 0x0f) | (((tc.hour  / 10) & 0x3) << 4);

}

static string get_track_name(string prefix, int number)
{
    char buf[16];
    mxf_snprintf(buf, sizeof(buf), "%d", number);
    return prefix.append(buf);
}

static uint32_t get_kag_aligned_size(uint32_t data_size)
{
    // assuming the partition pack is aligned to the kag working from the first byte of the file

    uint32_t fill_size = KAG_SIZE - (data_size % KAG_SIZE);
    if (fill_size < mxfKey_extlen + LLEN)
        fill_size += KAG_SIZE;

    return data_size + fill_size;
}



class SetWithDuration
{
public:
    virtual ~SetWithDuration() {}

    virtual void UpdateDuration(int64_t duration) = 0;
};

class StructComponentSet : public SetWithDuration
{
public:
    StructComponentSet(StructuralComponent *component)
    {
        mComponent = component;
    }

    virtual ~StructComponentSet()
    {}

    virtual void UpdateDuration(int64_t duration)
    {
        mComponent->setDuration(duration);
    }

private:
    StructuralComponent *mComponent;
};

class FileDescriptorSet : public SetWithDuration
{
public:
    FileDescriptorSet(FileDescriptor *descriptor)
    {
        mDescriptor = descriptor;
    }

    virtual ~FileDescriptorSet()
    {}

    virtual void UpdateDuration(int64_t duration)
    {
        mDescriptor->setContainerDuration(duration);
    }

private:
    FileDescriptor *mDescriptor;
};




uint32_t D10MXFOP1AWriter::GetContentPackageSize(D10SampleRate sample_rate, uint32_t encoded_picture_size)
{
    uint32_t content_package_size;

    content_package_size = get_kag_aligned_size(mxfKey_extlen + LLEN + SYSTEM_ITEM_METADATA_PACK_SIZE + mxfKey_extlen + LLEN);
    content_package_size += get_kag_aligned_size(mxfKey_extlen + LLEN + encoded_picture_size);
    if (sample_rate == D10_SAMPLE_RATE_625_50I)
        content_package_size += get_kag_aligned_size(mxfKey_extlen + LLEN + 1920 * 4 * 8 + 4);
    else
        content_package_size += get_kag_aligned_size(mxfKey_extlen + LLEN + 1602 * 4 * 8 + 4);

    return content_package_size;
}


D10MXFOP1AWriter::D10MXFOP1AWriter()
{
    mxfRational default_aspect_ratio = {16, 9};

    SetSampleRate(D10_SAMPLE_RATE_625_50I);
    SetAudioChannelCount(4);
    SetAudioQuantizationBits(24);
    mAudioSequenceOffset = UNKNOWN_SEQUENCE_OFFSET;
    SetAspectRatio(default_aspect_ratio);
    SetStartTimecode(0, false);
    SetBitRate(D10_BIT_RATE_50, mMaxEncodedImageSize);
    mxf_generate_umid(&mFileSourcePackageUID);
    mxf_generate_umid(&mMaterialPackageUID);
    mReserveMinBytes = 0;

    mCompanyName = DEFAULT_COMPANY_NAME;
    mProductName = DEFAULT_PRODUCT_NAME;
    mVersionString = DEFAULT_VERSION_STRING;
    mProductUID = DEFAULT_PRODUCT_UID;

    mSystemItemSize = 0;
    mVideoItemSize = 0;
    mAudioItemSize = 0;
    memset(&mEssenceContainerUL, 0, sizeof(mEssenceContainerUL));

    mMXFFile = 0;
    mDataModel = 0;
    mHeaderMetadata = 0;
    mIndexSegment = 0;
    mHeaderPartition = 0;
    mHeaderMetadataStartPos = 0;
    mHeaderMetadataEndPos = 0;
    mMaterialPackageTC = 0;
    mFilePackageTC = 0;

    mAES3Blocks.push(new DynamicByteArray());

    mContentPackage = new D10ContentPackageInt();

    mDuration = 0;
}

D10MXFOP1AWriter::~D10MXFOP1AWriter()
{
    size_t i;
    for (i = 0; i < mBufferedContentPackages.size(); i++)
        delete mBufferedContentPackages[i];
    delete mContentPackage;

    while (!mAES3Blocks.empty()) {
        delete mAES3Blocks.front();
        mAES3Blocks.pop();
    }

    for (i = 0; i < mSetsWithDuration.size(); i++)
        delete mSetsWithDuration[i];

    delete mMXFFile;
    delete mDataModel;
    delete mHeaderMetadata;
    delete mIndexSegment;
    // mHeaderPartition is owned by mMXFFile
}

void D10MXFOP1AWriter::SetSampleRate(D10SampleRate sample_rate)
{
    mSampleRate = sample_rate;

    if (sample_rate == D10_SAMPLE_RATE_625_50I) {
        mMaxEncodedImageSize = 250000;
        mVideoSampleRate.numerator = 25;
        mVideoSampleRate.denominator = 1;
        mRoundedTimecodeBase = 25;
        mAudioSequenceCount = 1;
        mAudioSequenceIndex = 0;
        mAudioSequence[0] = 1920;
        mMaxAudioSamples = 1920;
        mMinAudioSamples = 1920;
        mMaxFinalPaddingSamples = 0;
    } else {
        mMaxEncodedImageSize = 208541;
        mVideoSampleRate.numerator = 30000;
        mVideoSampleRate.denominator = 1001;
        mRoundedTimecodeBase = 30;
        mAudioSequenceCount = 5;
        mAudioSequenceIndex = 0; // will be set later
        mAudioSequence[0] = 1602;
        mAudioSequence[1] = 1601;
        mAudioSequence[2] = 1602;
        mAudioSequence[3] = 1601;
        mAudioSequence[4] = 1602;
        mMaxAudioSamples = 1602;
        mMinAudioSamples = 1601;
        mMaxFinalPaddingSamples = 4; // if final frame is missing max this amount then pad with zero samples
                                     // value must be >= 2 to support the DV sequence 1600,1602,1602,1602,1602
    }
}

void D10MXFOP1AWriter::SetAudioChannelCount(uint32_t count)
{
    MXFPP_CHECK(count <= MAX_CP_AUDIO_TRACKS);
    mChannelCount = count;
}

void D10MXFOP1AWriter::SetAudioQuantizationBits(uint32_t bits)
{
    MXFPP_CHECK(bits >= 16 && bits <= 24);
    mAudioQuantizationBits = bits;
    mAudioBytesPerSample = (mAudioQuantizationBits + 7) / 8;
}

void D10MXFOP1AWriter::SetAudioSequenceOffset(uint8_t offset)
{
    mAudioSequenceOffset = offset;
}

void D10MXFOP1AWriter::SetAspectRatio(mxfRational aspect_ratio)
{
    MXFPP_CHECK((aspect_ratio.numerator == 4 && aspect_ratio.denominator == 3) ||
                (aspect_ratio.numerator == 16 && aspect_ratio.denominator == 9));
    mAspectRatio = aspect_ratio;
}

void D10MXFOP1AWriter::SetStartTimecode(int64_t count, bool drop_frame)
{
    MXFPP_CHECK(count >= 0);
    mStartTimecode = count;
    mDropFrameTimecode = drop_frame;
}

void D10MXFOP1AWriter::SetBitRate(D10BitRate rate, uint32_t encoded_image_size)
{
    MXFPP_CHECK(encoded_image_size <= mMaxEncodedImageSize);
    mD10BitRate = rate;
    mEncodedImageSize = encoded_image_size;
}

void D10MXFOP1AWriter::SetMaterialPackageUID(mxfUMID uid)
{
    mMaterialPackageUID = uid;
}

void D10MXFOP1AWriter::SetFileSourcePackageUID(mxfUMID uid)
{
    mFileSourcePackageUID = uid;
}

void D10MXFOP1AWriter::SetProductInfo(string company_name, string product_name, string version, mxfUUID product_uid)
{
    mCompanyName = company_name;
    mProductName = product_name;
    mVersionString = version;
    mProductUID = product_uid;
}

DataModel* D10MXFOP1AWriter::CreateDataModel()
{
    mDataModel = new DataModel();
    return mDataModel;
}

HeaderMetadata* D10MXFOP1AWriter::CreateHeaderMetadata()
{
    MXFPP_ASSERT(!mHeaderMetadata);

    mxfTimestamp now;
    mxfUL picture_essence_coding_ul = g_Null_UL;
    uint32_t i;


    // inits

    mxf_get_timestamp_now(&now);
    if (mSampleRate == D10_SAMPLE_RATE_625_50I) {
        switch (mD10BitRate)
        {
            case D10_BIT_RATE_30:
                mEssenceContainerUL = MXF_EC_L(D10_30_625_50_defined_template);
                picture_essence_coding_ul = MXF_CMDEF_L(D10_30_625_50);
                break;
            case D10_BIT_RATE_40:
                mEssenceContainerUL = MXF_EC_L(D10_40_625_50_defined_template);
                picture_essence_coding_ul = MXF_CMDEF_L(D10_40_625_50);
                break;
            case D10_BIT_RATE_50:
                mEssenceContainerUL = MXF_EC_L(D10_50_625_50_defined_template);
                picture_essence_coding_ul = MXF_CMDEF_L(D10_50_625_50);
                break;
        }
    } else {
        switch (mD10BitRate)
        {
            case D10_BIT_RATE_30:
                mEssenceContainerUL = MXF_EC_L(D10_30_525_60_defined_template);
                picture_essence_coding_ul = MXF_CMDEF_L(D10_30_525_60);
                break;
            case D10_BIT_RATE_40:
                mEssenceContainerUL = MXF_EC_L(D10_40_525_60_defined_template);
                picture_essence_coding_ul = MXF_CMDEF_L(D10_40_525_60);
                break;
            case D10_BIT_RATE_50:
                mEssenceContainerUL = MXF_EC_L(D10_50_525_60_defined_template);
                picture_essence_coding_ul = MXF_CMDEF_L(D10_50_525_60);
                break;
        }
    }

    // create the header metadata
    if (!mDataModel)
        CreateDataModel();
    mHeaderMetadata = new HeaderMetadata(mDataModel);

    // Preface
    Preface *preface = new Preface(mHeaderMetadata);
    preface->setLastModifiedDate(now);
    preface->setVersion(MXF_PREFACE_VER(1, 2));
    preface->setOperationalPattern(MXF_OP_L(1a, MultiTrack_Stream_Internal));
    preface->appendEssenceContainers(mEssenceContainerUL);
    preface->setDMSchemes(vector<mxfUL>());

    // Preface - Identification
    Identification *ident = new Identification(mHeaderMetadata);
    preface->appendIdentifications(ident);
    ident->initialise(mCompanyName, mProductName, mVersionString, mProductUID);
    ident->setModificationDate(now);

    // Preface - ContentStorage
    ContentStorage *content_storage = new ContentStorage(mHeaderMetadata);
    preface->setContentStorage(content_storage);

    // Preface - ContentStorage - EssenceContainerData
    EssenceContainerData *ess_container_data = new EssenceContainerData(mHeaderMetadata);
    content_storage->appendEssenceContainerData(ess_container_data);
    ess_container_data->setLinkedPackageUID(mFileSourcePackageUID);
    ess_container_data->setIndexSID(INDEX_SID);
    ess_container_data->setBodySID(BODY_SID);


    // Preface - ContentStorage - MaterialPackage
    MaterialPackage *material_package = new MaterialPackage(mHeaderMetadata);
    content_storage->appendPackages(material_package);
    material_package->setPackageUID(mMaterialPackageUID);
    material_package->setPackageCreationDate(now);
    material_package->setPackageModifiedDate(now);

    // Preface - ContentStorage - MaterialPackage - Timecode Track
    Track *timecode_track = new Track(mHeaderMetadata);
    material_package->appendTracks(timecode_track);
    timecode_track->setTrackName("TC1");
    timecode_track->setTrackID(1);
    timecode_track->setTrackNumber(0);
    timecode_track->setEditRate(mVideoSampleRate);
    timecode_track->setOrigin(0);

    // Preface - ContentStorage - MaterialPackage - Timecode Track - Sequence
    Sequence *sequence = new Sequence(mHeaderMetadata);
    timecode_track->setSequence(sequence);
    sequence->setDataDefinition(MXF_DDEF_L(Timecode));
    sequence->setDuration(-1); // updated when writing completed
    mSetsWithDuration.push_back(new StructComponentSet(sequence));

    // Preface - ContentStorage - MaterialPackage - Timecode Track - TimecodeComponent
    mMaterialPackageTC = new TimecodeComponent(mHeaderMetadata);
    sequence->appendStructuralComponents(mMaterialPackageTC);
    mMaterialPackageTC->setDataDefinition(MXF_DDEF_L(Timecode));
    mMaterialPackageTC->setDuration(-1); // updated when writing completed
    mMaterialPackageTC->setRoundedTimecodeBase(mRoundedTimecodeBase);
    if (mSampleRate == D10_SAMPLE_RATE_625_50I)
        mMaterialPackageTC->setDropFrame(false);
    else
        mMaterialPackageTC->setDropFrame(mDropFrameTimecode);
    mMaterialPackageTC->setStartTimecode(mStartTimecode);
    mSetsWithDuration.push_back(new StructComponentSet(mMaterialPackageTC));

    // Preface - ContentStorage - MaterialPackage - Timeline Tracks
    // video track and audio track
    for (i = 0; i < 2; i++)
    {
        bool is_picture = (i == 0);

        // Preface - ContentStorage - MaterialPackage - Timeline Track
        Track *track = new Track(mHeaderMetadata);
        material_package->appendTracks(track);
        track->setTrackName(is_picture ? "V1" : get_track_name("A", i));
        track->setTrackID(i + 2);
        track->setTrackNumber(0);
        track->setEditRate(mVideoSampleRate);
        track->setOrigin(0);

        // Preface - ContentStorage - MaterialPackage - Timeline Track - Sequence
        Sequence *sequence = new Sequence(mHeaderMetadata);
        track->setSequence(sequence);
        sequence->setDataDefinition(is_picture ? MXF_DDEF_L(Picture) : MXF_DDEF_L(Sound));
        sequence->setDuration(-1); // updated when writing completed
        mSetsWithDuration.push_back(new StructComponentSet(sequence));

        // Preface - ContentStorage - MaterialPackage - Timeline Track - Sequence - SourceClip
        SourceClip *source_clip = new SourceClip(mHeaderMetadata);
        sequence->appendStructuralComponents(source_clip);
        source_clip->setDataDefinition(is_picture ? MXF_DDEF_L(Picture) : MXF_DDEF_L(Sound));
        source_clip->setDuration(-1); // updated when writing completed
        source_clip->setStartPosition(0);
        source_clip->setSourceTrackID(i + 2);
        source_clip->setSourcePackageID(mFileSourcePackageUID);
        mSetsWithDuration.push_back(new StructComponentSet(source_clip));
    }


    // Preface - ContentStorage - SourcePackage
    SourcePackage *file_source_package = new SourcePackage(mHeaderMetadata);
    content_storage->appendPackages(file_source_package);
    file_source_package->setPackageUID(mFileSourcePackageUID);
    file_source_package->setPackageCreationDate(now);
    file_source_package->setPackageModifiedDate(now);

    // Preface - ContentStorage - SourcePackage - Timecode Track
    timecode_track = new Track(mHeaderMetadata);
    file_source_package->appendTracks(timecode_track);
    timecode_track->setTrackName("TC1");
    timecode_track->setTrackID(1);
    timecode_track->setTrackNumber(0);
    timecode_track->setEditRate(mVideoSampleRate);
    timecode_track->setOrigin(0);

    // Preface - ContentStorage - SourcePackage - Timecode Track - Sequence
    sequence = new Sequence(mHeaderMetadata);
    timecode_track->setSequence(sequence);
    sequence->setDataDefinition(MXF_DDEF_L(Timecode));
    sequence->setDuration(-1); // updated when writing completed
    mSetsWithDuration.push_back(new StructComponentSet(sequence));

    // Preface - ContentStorage - SourcePackage - Timecode Track - TimecodeComponent
    mFilePackageTC = new TimecodeComponent(mHeaderMetadata);
    sequence->appendStructuralComponents(mFilePackageTC);
    mFilePackageTC->setDataDefinition(MXF_DDEF_L(Timecode));
    mFilePackageTC->setDuration(-1); // updated when writing completed
    mFilePackageTC->setRoundedTimecodeBase(mRoundedTimecodeBase);
    if (mSampleRate == D10_SAMPLE_RATE_625_50I)
        mFilePackageTC->setDropFrame(false);
    else
        mFilePackageTC->setDropFrame(mDropFrameTimecode);
    mFilePackageTC->setStartTimecode(mStartTimecode);
    mSetsWithDuration.push_back(new StructComponentSet(mFilePackageTC));

    // Preface - ContentStorage - SourcePackage - Timeline Tracks
    // video track followed by audio track
    for (i = 0; i < 2; i++)
    {
        bool is_picture = (i == 0);

        // Preface - ContentStorage - SourcePackage - Timeline Track
        Track *track = new Track(mHeaderMetadata);
        file_source_package->appendTracks(track);
        track->setTrackName(is_picture ? "V1" : get_track_name("A", i));
        track->setTrackID(i + 2);
        if (is_picture)
            track->setTrackNumber(MXF_D10_PICTURE_TRACK_NUM(0x00));
        else
            track->setTrackNumber(MXF_D10_SOUND_TRACK_NUM(0x00));
        track->setEditRate(mVideoSampleRate);
        track->setOrigin(0);

        // Preface - ContentStorage - SourcePackage - Timeline Track - Sequence
        Sequence *sequence = new Sequence(mHeaderMetadata);
        track->setSequence(sequence);
        sequence->setDataDefinition(is_picture ? MXF_DDEF_L(Picture) : MXF_DDEF_L(Sound));
        sequence->setDuration(-1); // updated when writing completed
        mSetsWithDuration.push_back(new StructComponentSet(sequence));

        // Preface - ContentStorage - SourcePackage - Timeline Track - Sequence - SourceClip
        SourceClip *source_clip = new SourceClip(mHeaderMetadata);
        sequence->appendStructuralComponents(source_clip);
        source_clip->setDataDefinition(is_picture ? MXF_DDEF_L(Picture) : MXF_DDEF_L(Sound));
        source_clip->setDuration(-1); // updated when writing completed
        source_clip->setStartPosition(0);
        source_clip->setSourceTrackID(0);
        source_clip->setSourcePackageID(g_Null_UMID);
        mSetsWithDuration.push_back(new StructComponentSet(source_clip));
    }


    // Preface - ContentStorage - SourcePackage - MultipleDescriptor
    MultipleDescriptor *mult_descriptor = new MultipleDescriptor(mHeaderMetadata);
    file_source_package->setDescriptor(mult_descriptor);
    mult_descriptor->setSampleRate(mVideoSampleRate);
    mult_descriptor->setEssenceContainer(mEssenceContainerUL);
    mSetsWithDuration.push_back(new FileDescriptorSet(mult_descriptor));


    // Preface - ContentStorage - SourcePackage - MultipleDescriptor - CDCIEssenceDescriptor
    CDCIEssenceDescriptor *cdciDescriptor = new CDCIEssenceDescriptor(mHeaderMetadata);
    mult_descriptor->appendSubDescriptorUIDs(cdciDescriptor);
    cdciDescriptor->setLinkedTrackID(2);
    cdciDescriptor->setSampleRate(mVideoSampleRate);
    cdciDescriptor->setEssenceContainer(mEssenceContainerUL);
    cdciDescriptor->setPictureEssenceCoding(picture_essence_coding_ul);
    cdciDescriptor->setSignalStandard(MXF_SIGNAL_STANDARD_ITU601);
    cdciDescriptor->setFrameLayout(MXF_SEPARATE_FIELDS);
    cdciDescriptor->setStoredWidth(GetStoredWidth());
    cdciDescriptor->setStoredHeight(GetStoredHeight());
    if (mSampleRate == D10_SAMPLE_RATE_625_50I) {
        cdciDescriptor->setSampledWidth(720);
        cdciDescriptor->setSampledHeight(304);
        cdciDescriptor->setDisplayWidth(720);
        cdciDescriptor->setDisplayHeight(288);
        cdciDescriptor->setDisplayXOffset(0);
        cdciDescriptor->setDisplayYOffset(16);
        cdciDescriptor->appendVideoLineMap(7);
        cdciDescriptor->appendVideoLineMap(320);
    } else {
        cdciDescriptor->setSampledWidth(720);
        cdciDescriptor->setSampledHeight(256);
        cdciDescriptor->setDisplayWidth(720);
        cdciDescriptor->setDisplayHeight(243);
        cdciDescriptor->setDisplayXOffset(0);
        cdciDescriptor->setDisplayYOffset(13);
        cdciDescriptor->appendVideoLineMap(7);
        cdciDescriptor->appendVideoLineMap(270);
    }
    cdciDescriptor->setStoredF2Offset(0);
    cdciDescriptor->setSampledXOffset(0);
    cdciDescriptor->setSampledYOffset(0);
    cdciDescriptor->setDisplayF2Offset(0);
    cdciDescriptor->setAspectRatio(mAspectRatio);
    cdciDescriptor->setAlphaTransparency(false);
    cdciDescriptor->setCaptureGamma(ITUR_BT470_TRANSFER_CH);
    cdciDescriptor->setImageAlignmentOffset(0);
    cdciDescriptor->setFieldDominance(1); // field 1
    cdciDescriptor->setImageStartOffset(0);
    cdciDescriptor->setImageEndOffset(0);
    cdciDescriptor->setComponentDepth(8);
    cdciDescriptor->setHorizontalSubsampling(2);
    cdciDescriptor->setVerticalSubsampling(1);
    cdciDescriptor->setColorSiting(MXF_COLOR_SITING_REC601);
    cdciDescriptor->setReversedByteOrder(false);
    cdciDescriptor->setPaddingBits(0);
    cdciDescriptor->setBlackRefLevel(16);
    cdciDescriptor->setWhiteReflevel(235);
    cdciDescriptor->setColorRange(225);
    mSetsWithDuration.push_back(new FileDescriptorSet(cdciDescriptor));


    // Preface - ContentStorage - SourcePackage - MultipleDescriptor - GenericSoundEssenceDescriptor
    GenericSoundEssenceDescriptor *soundDescriptor = new GenericSoundEssenceDescriptor(mHeaderMetadata);
    mult_descriptor->appendSubDescriptorUIDs(soundDescriptor);
    soundDescriptor->setLinkedTrackID(3);
    soundDescriptor->setSampleRate(mVideoSampleRate);
    soundDescriptor->setEssenceContainer(mEssenceContainerUL);
    soundDescriptor->setAudioSamplingRate(AUDIO_SAMPLING_RATE);
    soundDescriptor->setLocked(true);
    soundDescriptor->setAudioRefLevel(0);
    soundDescriptor->setChannelCount(mChannelCount);
    soundDescriptor->setQuantizationBits(mAudioQuantizationBits);
    mSetsWithDuration.push_back(new FileDescriptorSet(soundDescriptor));

    return mHeaderMetadata;
}

void D10MXFOP1AWriter::ReserveHeaderMetadataSpace(uint32_t min_bytes)
{
    mReserveMinBytes = min_bytes;
}

bool D10MXFOP1AWriter::CreateFile(string filename)
{
    try
    {
        mMXFFile = File::openNew(filename);

        if (!mHeaderMetadata)
            CreateHeaderMetadata();

        CreateFile();
    }
    catch (...)
    {
        return false;
    }

    return true;
}

bool D10MXFOP1AWriter::CreateFile(File **file)
{
    try
    {
        mMXFFile = *file;

        if (!mHeaderMetadata)
            CreateHeaderMetadata();

        CreateFile();
    }
    catch (...)
    {
        mMXFFile = 0;
        return false;
    }

    *file = 0;
    return true;
}

void D10MXFOP1AWriter::SetUserTimecode(Timecode user_timecode)
{
    MXFPP_ASSERT(mMXFFile);

    mContentPackage->mUserTimecode = user_timecode;
}

Timecode D10MXFOP1AWriter::GenerateUserTimecode()
{
    MXFPP_ASSERT(mMXFFile);

    Timecode user_timecode;
    int64_t tc_count = mStartTimecode + mDuration;

    if (mDropFrameTimecode && mSampleRate == D10MXFOP1AWriter::D10_SAMPLE_RATE_525_60I) {
        // first 2 frame numbers shall be omitted at the start of each minute,
        //   except minutes 0, 10, 20, 30, 40 and 50

        int hour, min;
        int64_t prev_skipped_count = -1;
        int64_t skipped_count = 0;
        while (prev_skipped_count != skipped_count)
        {
            prev_skipped_count = skipped_count;

            hour = (int)(( tc_count + skipped_count) / (60 * 60 * mRoundedTimecodeBase));
            min  = (int)(((tc_count + skipped_count) % (60 * 60 * mRoundedTimecodeBase)) / (60 * mRoundedTimecodeBase));

            // add frames skipped
            skipped_count  = (60-6) * 2 * hour;      // every whole hour
            skipped_count += (min / 10) * 9 * 2;    // every whole 10 min
            skipped_count += (min % 10) * 2;        // every whole min, except min 0
        }

        tc_count += skipped_count;
    }

    user_timecode.hour  = (int)(tc_count   / (60 * 60 * mRoundedTimecodeBase));
    user_timecode.min   = (int)((tc_count  % (60 * 60 * mRoundedTimecodeBase)) / (60 * mRoundedTimecodeBase));
    user_timecode.sec   = (int)(((tc_count % (60 * 60 * mRoundedTimecodeBase)) % (60 * mRoundedTimecodeBase)) / mRoundedTimecodeBase);
    user_timecode.frame = (int)(((tc_count % (60 * 60 * mRoundedTimecodeBase)) % (60 * mRoundedTimecodeBase)) % mRoundedTimecodeBase);
    user_timecode.dropFrame = (mDropFrameTimecode && mSampleRate == D10MXFOP1AWriter::D10_SAMPLE_RATE_525_60I);

    return user_timecode;
}

void D10MXFOP1AWriter::SetVideo(const unsigned char *data, uint32_t size)
{
    MXFPP_ASSERT(mMXFFile);
    MXFPP_CHECK(size > 0 && size <= mEncodedImageSize);

    mContentPackage->mVideoBytes.setBytes(data, size);

    if (size < mEncodedImageSize)
        mContentPackage->mVideoBytes.appendZeros(mEncodedImageSize - size);
}

uint32_t D10MXFOP1AWriter::GetAudioSampleCount()
{
    if (mAudioSequenceCount > 1 && mAudioSequenceOffset == UNKNOWN_SEQUENCE_OFFSET)
        return mAudioSequence[mBufferedContentPackages.size() % mAudioSequenceCount]; // sample count for default sequence
    else if (mAudioSequenceCount > 1 && mAudioSequenceOffset != UNKNOWN_SEQUENCE_OFFSET && mDuration == 0)
        return mAudioSequence[mAudioSequenceOffset % mAudioSequenceCount]; // in case CreateFile() hasn't been called yet
    else
        return mAudioSequence[mAudioSequenceIndex];
}

void D10MXFOP1AWriter::SetAudio(uint32_t channel, const unsigned char *data, uint32_t size)
{
    MXFPP_ASSERT(mMXFFile);
    MXFPP_ASSERT(channel < mChannelCount);

    mContentPackage->mAudioBytes[channel].setBytes(data, size);
}

void D10MXFOP1AWriter::WriteContentPackage()
{
    MXFPP_CHECK(mContentPackage->IsComplete(mChannelCount));

    WriteContentPackage(mContentPackage);

    mContentPackage->Reset();
}

void D10MXFOP1AWriter::WriteContentPackage(const D10ContentPackage *content_package)
{
    MXFPP_ASSERT(mMXFFile);

    if (mChannelCount > 0) {
        if (mAudioSequenceCount > 1 && mAudioSequenceOffset == UNKNOWN_SEQUENCE_OFFSET) {
            // buffer content packages if sequence offset can't yet be determined
            if (content_package && mBufferedContentPackages.size() + 1 <= mAudioSequenceCount) {
                mBufferedContentPackages.push_back(new D10ContentPackageInt(content_package));
                return;
            }
            mAudioSequenceOffset = GetAudioSequenceOffset(content_package);
            mAudioSequenceIndex = mAudioSequenceOffset;

            size_t i;
            for (i = 0; i < mBufferedContentPackages.size(); i++)
                UpdateAES3Blocks(mBufferedContentPackages[i]);
        }

        if (content_package)
            UpdateAES3Blocks(content_package);
        else
            FinalUpdateAES3Blocks();
    } else {
        InitAES3Block(mAES3Blocks.front(), mAudioSequenceIndex);
        mAES3Blocks.front()->setSize(AES3_BLOCK_SIZE(mAudioSequence[mAudioSequenceIndex]));
    }


    // determine number of content packages that can be written

    MXFPP_ASSERT(!mAES3Blocks.empty());
    size_t num_complete_aes3_blocks = mAES3Blocks.size();
    if (mAES3Blocks.back()->getSize() < AES3_BLOCK_SIZE(mAudioSequence[LAST_AES3_BLOCK_SEQ_INDEX]))
        num_complete_aes3_blocks--; // last one is still incomplete

    size_t num_cp_write = mBufferedContentPackages.size();
    if (content_package)
        num_cp_write++;

    if (num_cp_write > num_complete_aes3_blocks) {
        if (content_package)
            mBufferedContentPackages.push_back(new D10ContentPackageInt(content_package)); // won't be written
        num_cp_write = num_complete_aes3_blocks;
    }

    if (num_cp_write == 0)
        return;


    // write content packages

    size_t i;
    for (i = 0; i < num_cp_write; i++) {
        const D10ContentPackage *cp_write;
        if (i >= mBufferedContentPackages.size())
            cp_write = content_package;
        else
            cp_write = mBufferedContentPackages[i];


        // write system item

        uint32_t element_size = WriteSystemItem(cp_write);
        mMXFFile->writeFill(mSystemItemSize - element_size);


        // write video item

        mMXFFile->writeFixedKL(&VIDEO_ELEMENT_KEY, LLEN, cp_write->GetVideoSize());
        MXFPP_CHECK(mMXFFile->write(cp_write->GetVideo(), cp_write->GetVideoSize()) == cp_write->GetVideoSize());
        mMXFFile->writeFill(mVideoItemSize - mxfKey_extlen - LLEN - cp_write->GetVideoSize());


        // write audio item

        mMXFFile->writeFixedKL(&AUDIO_ELEMENT_KEY, LLEN, mAES3Blocks.front()->getSize());
        MXFPP_CHECK(mMXFFile->write(mAES3Blocks.front()->getBytes(), mAES3Blocks.front()->getSize()) ==
                                        mAES3Blocks.front()->getSize());
        mMXFFile->writeFill(mAudioItemSize - (mxfKey_extlen + LLEN + mAES3Blocks.front()->getSize()));

        // delete aes3 block if have more than 1, else keep and reset (not clear) it
        if (mAES3Blocks.size() > 1) {
            delete mAES3Blocks.front();
            mAES3Blocks.pop();
        } else {
            mAES3Blocks.front()->setSize(0);
        }


        mDuration++;
        mAudioSequenceIndex = (mAudioSequenceIndex + 1) % mAudioSequenceCount;
    }

    for (i = 0; i < num_cp_write && i < mBufferedContentPackages.size(); i++)
        delete mBufferedContentPackages[i];
    if (i > 0) {
        mBufferedContentPackages.erase(mBufferedContentPackages.begin(),
                                       mBufferedContentPackages.begin() + i);
    }
}

int64_t D10MXFOP1AWriter::GetFileSize() const
{
    return mMXFFile->size();
}

uint32_t D10MXFOP1AWriter::GetStoredWidth() const
{
    return 720;
}

uint32_t D10MXFOP1AWriter::GetStoredHeight() const
{
    if (mSampleRate == D10_SAMPLE_RATE_625_50I)
        return 304;
    else
        return 256;
}

void D10MXFOP1AWriter::UpdateStartTimecode(int64_t count)
{
    MXFPP_ASSERT(mMXFFile);

    mMaterialPackageTC->setStartTimecode(count);
    mFilePackageTC->setStartTimecode(count);
}

void D10MXFOP1AWriter::CompleteFile()
{
    MXFPP_ASSERT(mMXFFile);

    // write buffered content packages
    if (!mBufferedContentPackages.empty())
        WriteContentPackage(0);


    // write the footer partition pack
    Partition &footer_partition = mMXFFile->createPartition();
    footer_partition.setKey(&MXF_PP_K(ClosedComplete, Footer));
    footer_partition.write(mMXFFile);


    // update metadata sets and index with duration
    size_t i;
    for (i = 0; i < mSetsWithDuration.size(); i++)
        mSetsWithDuration[i]->UpdateDuration(mDuration);
    mIndexSegment->setIndexDuration(mDuration);

    // re-write the header metadata
    mMXFFile->seek(mHeaderMetadataStartPos, SEEK_SET);
    PositionFillerWriter pos_filler_writer(mHeaderMetadataEndPos);
    mHeaderMetadata->write(mMXFFile, mHeaderPartition, &pos_filler_writer);


    // re-write the header index table segment (position and size hasn't changed)
    KAGFillerWriter kag_filler_writer(mHeaderPartition);
    mIndexSegment->write(mMXFFile, mHeaderPartition, &kag_filler_writer);


    // update the partition packs
    mMXFFile->updatePartitions();


    // done with the file
    delete mMXFFile;
    mMXFFile = 0;
}

void D10MXFOP1AWriter::CreateFile()
{
    mxfUUID uuid;


    // inits

    mSystemItemSize = get_kag_aligned_size(mxfKey_extlen + LLEN + SYSTEM_ITEM_METADATA_PACK_SIZE + mxfKey_extlen + LLEN);
    mVideoItemSize = get_kag_aligned_size(mxfKey_extlen + LLEN + mEncodedImageSize);
    if (mSampleRate == D10_SAMPLE_RATE_625_50I) {
        mAudioItemSize = get_kag_aligned_size(mxfKey_extlen + LLEN + 1920 * 4 * 8 + 4);
    } else {
        mAudioItemSize = get_kag_aligned_size(mxfKey_extlen + LLEN + 1602 * 4 * 8 + 4);
        // check the fill item fits with the smaller audio frame size
        MXFPP_ASSERT(1601 * 4 * 8 + 4 <= mAudioItemSize - mxfKey_extlen - LLEN);
    }
    if (mAudioSequenceCount > 1 && mAudioSequenceOffset != UNKNOWN_SEQUENCE_OFFSET)
        mAudioSequenceIndex = mAudioSequenceOffset % mAudioSequenceCount;


    // set minimum llen

    mMXFFile->setMinLLen(LLEN);


    // write the header partition pack

    mHeaderPartition = &(mMXFFile->createPartition());
    mHeaderPartition->setKey(&MXF_PP_K(ClosedComplete, Header));
    mHeaderPartition->setIndexSID(INDEX_SID);
    mHeaderPartition->setBodySID(BODY_SID);
    mHeaderPartition->setKagSize(KAG_SIZE);
    mHeaderPartition->setOperationalPattern(&MXF_OP_L(1a, MultiTrack_Stream_Internal));
    mHeaderPartition->addEssenceContainer(&mEssenceContainerUL);
    mHeaderPartition->write(mMXFFile);


    // write the header metadata

    mHeaderMetadataStartPos = mMXFFile->tell(); // need this position when we re-write the header metadata
    KAGFillerWriter reserve_filler_writer(mHeaderPartition, mReserveMinBytes);
    mHeaderMetadata->write(mMXFFile, mHeaderPartition, &reserve_filler_writer);
    mHeaderMetadataEndPos = mMXFFile->tell();  // need this position when we re-write the header metadata


    // write the index table segment

    mIndexSegment = new IndexTableSegment();
    mxf_generate_uuid(&uuid);
    mIndexSegment->setInstanceUID(uuid);
    mIndexSegment->setIndexEditRate(mVideoSampleRate);
    mIndexSegment->setIndexDuration(0); // will be updated when writing is completed
    mIndexSegment->setIndexSID(INDEX_SID);
    mIndexSegment->setBodySID(BODY_SID);
    uint32_t deltaOffset = 0;
    mIndexSegment->appendDeltaEntry(0, 0, deltaOffset);
    deltaOffset += mSystemItemSize;
    mIndexSegment->appendDeltaEntry(0, 0, deltaOffset);
    deltaOffset += mVideoItemSize;
    mIndexSegment->appendDeltaEntry(0, 0, deltaOffset);
    deltaOffset += mAudioItemSize;
    mIndexSegment->setEditUnitByteCount(deltaOffset);
    MXFPP_ASSERT(deltaOffset == GetContentPackageSize(mSampleRate, mEncodedImageSize));

    KAGFillerWriter kag_filler_writer(mHeaderPartition);
    mIndexSegment->write(mMXFFile, mHeaderPartition, &kag_filler_writer);


    // update the header partition

    mMXFFile->updatePartitions();
}

uint32_t D10MXFOP1AWriter::WriteSystemItem(const D10ContentPackage *content_package)
{
    // System Metadata Pack

    mMXFFile->writeFixedKL(&MXF_EE_K(SDTI_CP_System_Pack), LLEN, SYSTEM_ITEM_METADATA_PACK_SIZE);

    // core fields
    mMXFFile->writeUInt8(0x5c); // system bitmap - : 01011100b
    if (mSampleRate == D10_SAMPLE_RATE_625_50I)
        mMXFFile->writeUInt8(0x02 << 1); // 25 fps content package rate
    else
        mMXFFile->writeUInt8((0x03 << 1) | 1); // 30000/1001 fps content package rate
    mMXFFile->writeUInt8(0x00); // content package type
    mMXFFile->writeUInt16(0x0000); // channel handle
    mMXFFile->writeUInt16((uint16_t)(mDuration % 65536)); // continuity count

    // SMPTE Universal Label
    mMXFFile->writeUL(&mEssenceContainerUL);

    // TODO: what should go in here?
    // Package creation date / time stamp
    unsigned char bytes[17];
    memset(bytes, 0, sizeof(bytes));
    MXFPP_CHECK(mMXFFile->write(bytes, sizeof(bytes)) == sizeof(bytes));

    // User date / time stamp
    bytes[0] = 0x81; // SMPTE 12-M timecode
    convert_timecode_to_12m(content_package->GetUserTimecode(), &bytes[1]);
    MXFPP_CHECK(mMXFFile->write(bytes, sizeof(bytes)) == sizeof(bytes));


    // Package Metadata Set (empty)

    mMXFFile->writeFixedKL(&MXF_EE_K(EmptyPackageMetadataSet), LLEN, 0);

    return mxfKey_extlen + LLEN + SYSTEM_ITEM_METADATA_PACK_SIZE + mxfKey_extlen + LLEN;
}

void D10MXFOP1AWriter::InitAES3Block(DynamicByteArray *aes3_block, uint8_t sequence_index)
{
    aes3_block->minAllocate(AES3_BLOCK_SIZE(mMaxAudioSamples));
    aes3_block->setSize(0);

    unsigned char bytes[32];

    bytes[0] = 0; // element header (FVUCP Valid Flag == 0 (false)
    if (mSampleRate == D10_SAMPLE_RATE_525_60I)
        bytes[0] |= sequence_index & 0x07; // 5-sequence count
    bytes[1] = (unsigned char)( mAudioSequence[sequence_index]       & 0xff); // samples per frame (LSB)
    bytes[2] = (unsigned char)((mAudioSequence[sequence_index] >> 8) & 0xff); // samples per frame (MSB)
    bytes[3] = (1 << mChannelCount) - 1; // channel valid flags
    aes3_block->append(bytes, 4);

    if (mChannelCount == 0 && mDuration == 0) {
        // initialize null audio once
        uint32_t s, c;
        for (c = 0; c < 8; c++) {
            bytes[c * 4    ] = (unsigned char)c; // channel number
            bytes[c * 4 + 1] = 0;
            bytes[c * 4 + 2] = 0;
            bytes[c * 4 + 3] = 0;
        }
        for (s = 0; s < mMaxAudioSamples; s++)
            aes3_block->append(bytes, 32);
    }
}

void D10MXFOP1AWriter::UpdateAES3Blocks(const D10ContentPackage *content_package)
{
    MXFPP_ASSERT(!mAES3Blocks.empty());

    unsigned char bytes[4];
    uint32_t start_input_sample = 0;
    uint32_t rem_input_samples = content_package->GetAudioSize() / mAudioBytesPerSample;
    while (rem_input_samples > 0) {
        uint8_t sequence_index = LAST_AES3_BLOCK_SEQ_INDEX;

        DynamicByteArray *aes3_block = mAES3Blocks.back();
        if (aes3_block->getSize() == 0)
            InitAES3Block(aes3_block, sequence_index);

        // copy audio samples
        uint32_t rem_block_samples = mAudioSequence[sequence_index] - AES3_BLOCK_NUM_SAMPLES(aes3_block->getSize());
        if (rem_block_samples > 0) {
            uint32_t copy_num_samples = rem_block_samples;
            if (copy_num_samples > rem_input_samples)
                copy_num_samples = rem_input_samples;

            uint32_t end_input_sample = start_input_sample + copy_num_samples;
            uint32_t s, c;
            for (s = start_input_sample; s < end_input_sample; s++) {
                for (c = 0; c < mChannelCount; c++) {
                    bytes[0] = (unsigned char)c; // channel number

                    if (mAudioBytesPerSample == 3) { // 24-bit
                        bytes[0] |= (content_package->GetAudio(c)[s * mAudioBytesPerSample    ] << 4) & 0xf0;
                        bytes[1] = ((content_package->GetAudio(c)[s * mAudioBytesPerSample    ] >> 4) & 0x0f) |
                                   ((content_package->GetAudio(c)[s * mAudioBytesPerSample + 1] << 4) & 0xf0);
                        bytes[2] = ((content_package->GetAudio(c)[s * mAudioBytesPerSample + 1] >> 4) & 0x0f) |
                                   ((content_package->GetAudio(c)[s * mAudioBytesPerSample + 2] << 4) & 0xf0);
                        bytes[3] = ((content_package->GetAudio(c)[s * mAudioBytesPerSample + 2] >> 4) & 0x0f);
                    } else { // 16-bit
                        bytes[1] = ((content_package->GetAudio(c)[s * mAudioBytesPerSample    ] << 4) & 0xf0);
                        bytes[2] = ((content_package->GetAudio(c)[s * mAudioBytesPerSample    ] >> 4) & 0x0f) |
                                   ((content_package->GetAudio(c)[s * mAudioBytesPerSample + 1] << 4) & 0xf0);
                        bytes[3] = ((content_package->GetAudio(c)[s * mAudioBytesPerSample + 1] >> 4) & 0x0f);
                    }

                    aes3_block->append(bytes, 4);
                }

                memset(bytes, 0, sizeof(bytes));
                for (; c < 8; c++) {
                    bytes[0] = (unsigned char)c; // channel number
                    aes3_block->append(bytes, 4);
                }
            }


            rem_input_samples -= copy_num_samples;
            start_input_sample += copy_num_samples;
            rem_block_samples -= copy_num_samples;
        }

        if (rem_block_samples == 0 && rem_input_samples > 0)
            mAES3Blocks.push(new DynamicByteArray());
    }
}

void D10MXFOP1AWriter::FinalUpdateAES3Blocks()
{
    MXFPP_ASSERT(!mAES3Blocks.empty());

    unsigned char bytes[32];
    uint8_t sequence_index = LAST_AES3_BLOCK_SEQ_INDEX;

    DynamicByteArray *aes3_block = mAES3Blocks.back();
    if (aes3_block->getSize() == 0)
        InitAES3Block(aes3_block, sequence_index);

    uint32_t rem_block_samples = mAudioSequence[sequence_index] - AES3_BLOCK_NUM_SAMPLES(aes3_block->getSize());
    if (rem_block_samples <= mMaxFinalPaddingSamples) {
        uint32_t s, c;
        for (c = 0; c < 8; c++) {
            bytes[c * 4    ] = (unsigned char)c; // channel number
            bytes[c * 4 + 1] = 0;
            bytes[c * 4 + 2] = 0;
            bytes[c * 4 + 3] = 0;
        }
        for (s = 0; s < rem_block_samples; s++)
            aes3_block->append(bytes, 32);
    }
}

uint8_t D10MXFOP1AWriter::GetAudioSequenceOffset(const D10ContentPackage *next_content_package)
{
    uint8_t offset = 0;
    while (offset < mAudioSequenceCount) {
        size_t i;
        for (i = 0; i < mBufferedContentPackages.size(); i++) {
            if (mBufferedContentPackages[i]->GetAudioSize() / mAudioBytesPerSample !=
                    mAudioSequence[(i + offset) % mAudioSequenceCount])
            {
                break;
            }
        }
        if (i >= mBufferedContentPackages.size() &&
            (!next_content_package ||
                next_content_package->GetAudioSize() / mAudioBytesPerSample ==
                    mAudioSequence[(i + offset) % mAudioSequenceCount]))
        {
            break;
        }

        offset++;
    }

    if (offset >= mAudioSequenceCount)
        offset = 0; // default to offset 0 for other input sample sequences (e.g. DV sample sequence)

    return offset;
}

