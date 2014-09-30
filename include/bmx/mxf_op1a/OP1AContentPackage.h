/*
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

#ifndef BMX_OP1A_CONTENT_PACKAGE_MANAGER_H_
#define BMX_OP1A_CONTENT_PACKAGE_MANAGER_H_

#include <vector>
#include <map>
#include <deque>

#include <libMXF++/MXF.h>

#include <bmx/ByteArray.h>
#include <bmx/frame/DataBufferArray.h>
#include <bmx/mxf_op1a/OP1AIndexTable.h>



namespace bmx
{


class OP1AFile;


class OP1AContentPackageElement
{
public:
  typedef enum
  {
    DATA_ELEMENT,
    PICTURE_ELEMENT,
    SOUND_ELEMENT,
  } ElementType;

public:
    OP1AContentPackageElement(uint32_t track_index_, ElementType element_type_,
                              mxfKey element_key_, uint32_t kag_size_, uint8_t min_llen_);

    void SetSampleSequence(const std::vector<uint32_t> &sample_sequence_, uint32_t sample_size_);
    void SetConstantEssenceLen(uint32_t constant_essence_len);
    void SetMaxEssenceLen(uint32_t max_essence_len);

    uint32_t GetNumSamples(int64_t position) const;

    uint32_t GetKAGAlignedSize(uint32_t klv_size);
    uint32_t GetKAGFillSize(int64_t klv_size);

    void WriteKL(mxfpp::File *mxf_file, int64_t essence_len);
    void WriteFill(mxfpp::File *mxf_file, int64_t essence_len);

public:
    uint32_t track_index;
    mxfKey element_key;
    uint32_t kag_size;
    uint8_t min_llen;
    uint8_t essence_llen;
    ElementType element_type;
    bool is_cbe;
    bool is_frame_wrapped;
    uint32_t fixed_element_size;

    // sound
    std::vector<uint32_t> sample_sequence;
    uint32_t sample_size;

    // AVCI
    uint32_t first_sample_size;
    uint32_t nonfirst_sample_size;
};


class OP1AContentPackageElementData
{
public:
    OP1AContentPackageElementData(mxfpp::File *mxf_file, OP1AIndexTable *index_table,
                                  OP1AContentPackageElement *element, int64_t position);

    uint32_t WriteSamples(const unsigned char *data, uint32_t size, uint32_t num_samples);
    void WriteSample(const CDataBuffer *data_array, uint32_t array_size);

    bool IsReady() const;

    uint32_t GetWriteSize() const;
    uint32_t GetNumSamplesWritten() const { return mNumSamplesWritten; }
    uint32_t Write();
    void CompleteWrite();

    void Reset(int64_t new_position);

private:
    mxfpp::File *mMXFFile;
    OP1AIndexTable *mIndexTable;
    OP1AContentPackageElement *mElement;
    ByteArray mData;
    uint32_t mNumSamples;
    uint32_t mNumSamplesWritten;
    int64_t mTotalWriteSize;
    int64_t mElementStartPos;
};


class OP1AContentPackage
{
public:
    OP1AContentPackage(mxfpp::File *mxf_file, OP1AIndexTable *index_table, uint32_t kag_size, uint8_t min_llen,
                       bool have_system_item, bool have_user_timecode, Rational frame_rate, uint8_t sys_meta_item_flags,
                       std::vector<OP1AContentPackageElement*> elements, int64_t position, Timecode start_timecode);
    ~OP1AContentPackage();

    void Reset(int64_t new_position);

public:
    bool IsReady(uint32_t track_index);

    void WriteUserTimecode(Timecode user_timecode);
    uint32_t WriteSamples(uint32_t track_index, const unsigned char *data, uint32_t size, uint32_t num_samples);
    void WriteSample(uint32_t track_index, const CDataBuffer *data_array, uint32_t array_size);

public:
    bool IsReady();
    void UpdateIndexTable();
    uint32_t Write();
    void WriteSystemItem();
    void CompleteWrite();

private:
    mxfpp::File *mMXFFile;
    OP1AIndexTable *mIndexTable;
    bool mFrameWrapped;
    bool mHaveSystemItem;
    uint32_t mSystemItemSize;
    uint8_t mSystemMetadataBitmap;
    bool mHaveInputUserTimecode;
    Rational mFrameRate;
    Timecode mStartTimecode;
    uint8_t mContentPackageRate;
    std::vector<OP1AContentPackageElementData*> mElementData;
    std::map<uint32_t, OP1AContentPackageElementData*> mElementTrackIndexMap;
    int64_t mPosition;
    bool mHaveUpdatedIndexTable;
    Timecode mUserTimecode;
    bool mUserTimecodeSet;
};


class OP1AContentPackageManager
{
public:
    OP1AContentPackageManager(mxfpp::File *mxf_file, OP1AIndexTable *index_table, Rational frame_rate,
                              uint32_t kag_size, uint8_t min_llen);
    ~OP1AContentPackageManager();

    void SetHaveInputUserTimecode(bool enable);
    void SetStartTimecode(Timecode start_timecode);
    void SetClipWrapped(bool enable);

    void RegisterSystemItem();
    void RegisterPictureTrackElement(uint32_t track_index, mxfKey element_key, bool is_cbe);
    void RegisterPictureTrackElement(uint32_t track_index, mxfKey element_key, bool is_cbe, uint8_t element_llen);
    void RegisterAVCITrackElement(uint32_t track_index, mxfKey element_key,
                                  uint32_t first_sample_size, uint32_t nonfirst_sample_size);
    void RegisterSoundTrackElement(uint32_t track_index, mxfKey element_key,
                                   std::vector<uint32_t> sample_sequence, uint32_t sample_size);
    void RegisterSoundTrackElement(uint32_t track_index, mxfKey element_key, uint8_t element_llen);
    void RegisterDataTrackElement(uint32_t track_index, mxfKey element_key, uint32_t constant_essence_len,
                                  uint32_t max_essence_len);

    void PrepareWrite();

public:
    void WriteUserTimecode(Timecode user_timecode);
    void WriteSamples(uint32_t track_index, const unsigned char *data, uint32_t size, uint32_t num_samples);
    void WriteSample(uint32_t track_index, const CDataBuffer *data_array, uint32_t array_size);

public:
    int64_t GetPosition() const { return mPosition; }

    bool HaveContentPackage() const;
    bool HaveContentPackages(size_t num) const;
    void UpdateIndexTable(size_t num_content_packages);
    void WriteNextContentPackage();

    void CompleteWrite();

private:
    size_t GetCurrentContentPackage(uint32_t track_index);
    size_t CreateContentPackage();

private:
    mxfpp::File *mMXFFile;
    OP1AIndexTable *mIndexTable;
    Rational mFrameRate;
    uint32_t mKAGSize;
    uint8_t mMinLLen;
    bool mFrameWrapped;
    bool mHaveSystemItem;
    bool mHaveInputUserTimecode;
    Timecode mStartTimecode;
    uint8_t mSysMetaItemFlags;

    std::vector<OP1AContentPackageElement*> mElements;
    std::map<uint32_t, OP1AContentPackageElement*> mElementTrackIndexMap;

    std::deque<OP1AContentPackage*> mContentPackages;
    std::vector<OP1AContentPackage*> mFreeContentPackages;
    int64_t mPosition;
};


};



#endif

