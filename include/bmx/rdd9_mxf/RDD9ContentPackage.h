/*
 * Copyright (C) 2012, British Broadcasting Corporation
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

#ifndef BMX_RDD9_CONTENT_PACKAGE_MANAGER_H_
#define BMX_RDD9_CONTENT_PACKAGE_MANAGER_H_

#include <vector>
#include <map>
#include <deque>

#include <libMXF++/MXF.h>

#include <bmx/ByteArray.h>
#include <bmx/rdd9_mxf/RDD9IndexTable.h>



namespace bmx
{


class RDD9File;


class RDD9ContentPackageElement
{
public:
  typedef enum
  {
    PICTURE_ELEMENT,
    SOUND_ELEMENT,
    DATA_ELEMENT,
  } ElementType;

public:
    RDD9ContentPackageElement(uint32_t track_index, mxfKey element_key);
    RDD9ContentPackageElement(uint32_t track_index, mxfKey element_key,
                              std::vector<uint32_t> sample_sequence, uint32_t sample_size);
    RDD9ContentPackageElement(uint32_t track_index, mxfKey element_key,
                              uint32_t constant_essence_len, uint32_t max_essence_len);

    void SetSoundSequenceOffset(uint8_t offset);

    bool CheckValidSampleCount(uint32_t num_samples) const;

    uint32_t GetNumSamples(int64_t position) const;

    uint32_t GetElementSize(uint32_t data_size) const;

    void Write(mxfpp::File *mxf_file, unsigned char *data, uint32_t size);

    uint32_t GetTrackIndex() const                         { return mTrackIndex; }
    ElementType GetElementType() const                     { return mElementType; }
    const std::vector<uint32_t>& GetSampleSequence() const { return mSampleSequence; }
    uint32_t GetSampleSize() const                         { return mSampleSize; }

private:
    uint32_t GetKAGAlignedSize(uint32_t klv_size) const;
    uint32_t GetKAGFillSize(int64_t klv_size) const;

private:
    uint32_t mTrackIndex;
    mxfKey mElementKey;
    ElementType mElementType;

    std::vector<uint32_t> mSampleSequence;
    uint32_t mSampleSize;
    uint32_t mFixedElementSize;

    bool mSoundSequenceOffsetSet;
    uint8_t mSoundSequenceOffset;
};


class RDD9ContentPackageElementData
{
public:
    RDD9ContentPackageElementData(mxfpp::File *mxf_file, RDD9IndexTable *index_table,
                                  RDD9ContentPackageElement *element, int64_t position);

    uint32_t WriteSamples(const unsigned char *data, uint32_t size, uint32_t num_samples);

    RDD9ContentPackageElement::ElementType GetElementType() const { return mElement->GetElementType(); }
    bool IsComplete() const;

    uint32_t GetElementSize() const;
    uint32_t GetNumSamplesWritten() const { return mNumSamplesWritten; }
    void Write();

    void Reset(int64_t new_position);

private:
    mxfpp::File *mMXFFile;
    RDD9IndexTable *mIndexTable;
    RDD9ContentPackageElement *mElement;
    int64_t mPosition;
    ByteArray mData;
    uint32_t mNumSamples;
    uint32_t mNumSamplesWritten;
};


class RDD9ContentPackage
{
public:
    RDD9ContentPackage(mxfpp::File *mxf_file, RDD9IndexTable *index_table, bool have_user_timecode,
                       Rational frame_rate, uint8_t sys_meta_item_flags,
                       std::vector<RDD9ContentPackageElement*> elements, int64_t position, Timecode start_timecode);
    ~RDD9ContentPackage();

    void Reset(int64_t new_position);

public:
    bool IsComplete(uint32_t track_index);

    void WriteUserTimecode(Timecode user_timecode);
    uint32_t WriteSamples(uint32_t track_index, const unsigned char *data, uint32_t size, uint32_t num_samples);

    uint32_t GetSoundSampleCount() const;

public:
    bool IsComplete();
    void UpdateIndexTable();
    void Write();
    void WriteSystemItem();

private:
    mxfpp::File *mMXFFile;
    RDD9IndexTable *mIndexTable;
    uint8_t mSysMetaItemFlags;
    bool mHaveInputUserTimecode;
    Timecode mStartTimecode;
    Rational mFrameRate;
    std::vector<RDD9ContentPackageElementData*> mElementData;
    std::map<uint32_t, RDD9ContentPackageElementData*> mElementTrackIndexMap;
    int64_t mPosition;
    bool mHaveUpdatedIndexTable;
    Timecode mUserTimecode;
    bool mUserTimecodeSet;
};


class RDD9ContentPackageManager
{
public:
    RDD9ContentPackageManager(mxfpp::File *mxf_file, RDD9IndexTable *index_table, Rational frame_rate);
    ~RDD9ContentPackageManager();

    void SetHaveInputUserTimecode(bool enable);
    void SetStartTimecode(Timecode start_timecode);
    void SetSoundSequenceOffset(uint8_t offset);

    void RegisterPictureTrackElement(uint32_t track_index, mxfKey element_key);
    void RegisterSoundTrackElement(uint32_t track_index, mxfKey element_key,
                                   std::vector<uint32_t> sample_sequence, uint32_t sample_size);
    void RegisterDataTrackElement(uint32_t track_index, mxfKey element_key,
                                  uint32_t constant_essence_len, uint32_t max_essence_len);

    void PrepareWrite();

public:
    void WriteUserTimecode(Timecode user_timecode);
    void WriteSamples(uint32_t track_index, const unsigned char *data, uint32_t size, uint32_t num_samples);

public:
    int64_t GetPosition() const             { return mPosition; }
    uint8_t GetSoundSequenceOffset() const  { return mSoundSequenceOffset; }

    bool HaveContentPackage(bool final_write);
    void WriteNextContentPackage();

private:
    size_t GetCurrentContentPackage(uint32_t track_index);
    size_t CreateContentPackage();
    void CalcSoundSequenceOffset(bool final_write);

private:
    mxfpp::File *mMXFFile;
    RDD9IndexTable *mIndexTable;
    Rational mFrameRate;

    bool mHaveInputUserTimecode;
    Timecode mStartTimecode;
    uint8_t mSysMetaItemFlags;

    std::vector<uint32_t> mSoundSequence;
    bool mSoundSequenceOffsetSet;
    uint8_t mSoundSequenceOffset;

    std::vector<RDD9ContentPackageElement*> mElements;
    std::map<uint32_t, RDD9ContentPackageElement*> mElementTrackIndexMap;

    std::deque<RDD9ContentPackage*> mContentPackages;
    std::vector<RDD9ContentPackage*> mFreeContentPackages;
    int64_t mPosition;
};


};



#endif

