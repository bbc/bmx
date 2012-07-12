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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <algorithm>

#include <bmx/mxf_op1a/OP1AContentPackage.h>
#include <bmx/MXFUtils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;


#define MAX_CONTENT_PACKAGES    250

#define ESS_ELEMENT_LLEN        4
#define LLEN                    4



static bool compare_element(const OP1AContentPackageElement *left, const OP1AContentPackageElement *right)
{
    return left->is_picture && !right->is_picture;
}



OP1AContentPackageElement::OP1AContentPackageElement(uint32_t track_index_, mxfKey element_key_,
                                                     uint32_t kag_size_, uint8_t min_llen_,
                                                     bool is_cbe_)
{
    track_index = track_index_;
    element_key = element_key_;
    kag_size = kag_size_;
    min_llen = min_llen_;
    is_picture = true;
    is_cbe = is_cbe_;
    sample_size = 0;
    fixed_element_size = 0;
    first_sample_size = 0;
    nonfirst_sample_size = 0;
}

OP1AContentPackageElement::OP1AContentPackageElement(uint32_t track_index_, mxfKey element_key_,
                                                     uint32_t kag_size_, uint8_t min_llen_,
                                                     uint32_t first_sample_size_, uint32_t nonfirst_sample_size_)
{
    track_index = track_index_;
    element_key = element_key_;
    kag_size = kag_size_;
    min_llen = min_llen_;
    is_picture = true;
    is_cbe = true;
    sample_size = 0;
    fixed_element_size = 0;
    first_sample_size = first_sample_size_;
    nonfirst_sample_size = nonfirst_sample_size_;
}

OP1AContentPackageElement::OP1AContentPackageElement(uint32_t track_index_, mxfKey element_key_,
                                                     uint32_t kag_size_, uint8_t min_llen_,
                                                     vector<uint32_t> sample_sequence_, uint32_t sample_size_)
{
    track_index = track_index_;
    element_key = element_key_;
    kag_size = kag_size_;
    min_llen = min_llen_;
    is_picture = false;
    is_cbe = true;
    sample_sequence = sample_sequence_;
    sample_size = sample_size_;
    first_sample_size = 0;
    nonfirst_sample_size = 0;

    uint32_t max_sample_sequence = 0;
    uint32_t prev_max_sample_sequence = 0;
    size_t i;
    for (i = 0; i < sample_sequence.size(); i++) {
        if (sample_sequence[i] > max_sample_sequence)
            max_sample_sequence = sample_sequence[i];
        if (sample_sequence[i] > prev_max_sample_sequence &&
            sample_sequence[i] < max_sample_sequence)
        {
            prev_max_sample_sequence = sample_sequence[i];
        }
    }

    fixed_element_size = GetKAGAlignedSize(mxfKey_extlen + ESS_ELEMENT_LLEN + max_sample_sequence * sample_size);
    if (prev_max_sample_sequence > 0 && prev_max_sample_sequence < max_sample_sequence) {
        uint32_t prev_max_element_size = GetKAGAlignedSize(mxfKey_extlen + ESS_ELEMENT_LLEN +
                                                            max_sample_sequence * sample_size +
                                                            mxfKey_extlen + min_llen);
        if (prev_max_element_size > fixed_element_size) {
            // max sample sequence element also requires filler
            fixed_element_size = GetKAGAlignedSize(mxfKey_extlen + ESS_ELEMENT_LLEN +
                                                      max_sample_sequence * sample_size +
                                                      mxfKey_extlen + min_llen);
        }
    }
}

uint32_t OP1AContentPackageElement::GetNumSamples(int64_t position) const
{
    if (sample_sequence.empty())
        return 1;
    else
        return sample_sequence[(size_t)(position % sample_sequence.size())];
}

uint32_t OP1AContentPackageElement::GetKAGAlignedSize(uint32_t data_size)
{
    // assuming the partition pack is aligned to the kag working from the first byte of the file

    uint32_t fill_size = 0;
    uint32_t data_in_kag_size = data_size % kag_size;
    if (data_in_kag_size > 0) {
        fill_size = kag_size - data_in_kag_size;
        while (fill_size < (uint32_t)min_llen + mxfKey_extlen)
            fill_size += kag_size;
    }

    return data_size + fill_size;
}



OP1AContentPackageElementData::OP1AContentPackageElementData(OP1AContentPackageElement *element, int64_t position)
{
    mElement = element;
    mNumSamplesWritten = 0;
    mNumSamples = element->GetNumSamples(position);
}

uint32_t OP1AContentPackageElementData::WriteSamples(const unsigned char *data, uint32_t size, uint32_t num_samples)
{
    BMX_ASSERT(size % num_samples == 0);

    uint32_t write_num_samples = mNumSamples - mNumSamplesWritten;
    if (write_num_samples > num_samples)
        write_num_samples = num_samples;
    uint32_t write_size = (size / num_samples) * write_num_samples;

    mData.Append(data, write_size);
    mNumSamplesWritten += write_num_samples;

    return write_num_samples;
}

void OP1AContentPackageElementData::WriteSample(const CDataBuffer *data_array, uint32_t array_size)
{
    uint32_t size = dba_get_total_size(data_array, array_size);
    mData.Grow(size);
    dba_copy_data(mData.GetBytesAvailable(), mData.GetSizeAvailable(), data_array, array_size);
    mData.IncrementSize(size);

    mNumSamplesWritten++;
}

bool OP1AContentPackageElementData::IsComplete() const
{
    return mNumSamplesWritten >= mNumSamples;
}

uint32_t OP1AContentPackageElementData::GetWriteSize() const
{
    if (mElement->fixed_element_size)
        return mElement->fixed_element_size;
    else
        return mElement->GetKAGAlignedSize(mxfKey_extlen + ESS_ELEMENT_LLEN + mData.GetSize());
}

void OP1AContentPackageElementData::Write(File *mxf_file)
{
    mxf_file->writeFixedKL(&mElement->element_key, ESS_ELEMENT_LLEN, mData.GetSize());
    BMX_CHECK(mxf_file->write(mData.GetBytes(), mData.GetSize()) == mData.GetSize());

    uint32_t write_size = GetWriteSize();
    if (write_size != mxfKey_extlen + ESS_ELEMENT_LLEN + mData.GetSize())
        mxf_file->writeFill(write_size - (mxfKey_extlen + ESS_ELEMENT_LLEN + mData.GetSize()));
}

void OP1AContentPackageElementData::Reset(int64_t new_position)
{
    mData.SetSize(0);
    mNumSamplesWritten = 0;
    mNumSamples = mElement->GetNumSamples(new_position);
}



OP1AContentPackage::OP1AContentPackage(vector<OP1AContentPackageElement*> elements, int64_t position)
{
    size_t i;
    for (i = 0; i < elements.size(); i++) {
        mElementData.push_back(new OP1AContentPackageElementData(elements[i], position));
        mElementTrackIndexMap[elements[i]->track_index] = mElementData.back();
    }

    mHaveUpdatedIndexTable = false;
}

OP1AContentPackage::~OP1AContentPackage()
{
    size_t i;
    for (i = 0; i < mElementData.size(); i++)
        delete mElementData[i];
}

void OP1AContentPackage::Reset(int64_t new_position)
{
    size_t i;
    for (i = 0; i < mElementData.size(); i++)
        mElementData[i]->Reset(new_position);
    mHaveUpdatedIndexTable = false;
}

bool OP1AContentPackage::IsComplete(uint32_t track_index)
{
    BMX_ASSERT(mElementTrackIndexMap.find(track_index) != mElementTrackIndexMap.end());

    return mElementTrackIndexMap[track_index]->IsComplete();
}

uint32_t OP1AContentPackage::WriteSamples(uint32_t track_index, const unsigned char *data, uint32_t size,
                                          uint32_t num_samples)
{
    BMX_ASSERT(mElementTrackIndexMap.find(track_index) != mElementTrackIndexMap.end());

    return mElementTrackIndexMap[track_index]->WriteSamples(data, size, num_samples);
}

void OP1AContentPackage::WriteSample(uint32_t track_index, const CDataBuffer *data_array, uint32_t array_size)
{
    BMX_ASSERT(mElementTrackIndexMap.find(track_index) != mElementTrackIndexMap.end());

    mElementTrackIndexMap[track_index]->WriteSample(data_array, array_size);
}

bool OP1AContentPackage::IsComplete()
{
    size_t i;
    for (i = 0; i < mElementData.size(); i++) {
        if (!mElementData[i]->IsComplete())
            return false;
    }

    return true;
}

void OP1AContentPackage::UpdateIndexTable(OP1AIndexTable *index_table)
{
    if (mHaveUpdatedIndexTable)
        return;

    uint32_t size = 0;
    vector<uint32_t> element_sizes;

    size_t i;
    for (i = 0; i < mElementData.size(); i++) {
        element_sizes.push_back(mElementData[i]->GetWriteSize());
        size += element_sizes.back();
    }

    index_table->UpdateIndex(size, element_sizes);
    mHaveUpdatedIndexTable = true;
}

void OP1AContentPackage::Write(File *mxf_file)
{
    BMX_ASSERT(mHaveUpdatedIndexTable);

    size_t i;
    for (i = 0; i < mElementData.size(); i++)
        mElementData[i]->Write(mxf_file);
}



OP1AContentPackageManager::OP1AContentPackageManager(uint32_t kag_size, uint8_t min_llen)
{
    // check assumption that filler will have llen == min_llen
    BMX_ASSERT(min_llen >= mxf_get_llen(0, kag_size + mxfKey_extlen + min_llen));

    mKAGSize = kag_size;
    mMinLLen = min_llen;
    mPosition = 0;
}

OP1AContentPackageManager::~OP1AContentPackageManager()
{
    size_t i;
    for (i = 0; i < mElements.size(); i++)
        delete mElements[i];

    for (i = 0; i < mContentPackages.size(); i++)
        delete mContentPackages[i];
    for (i = 0; i < mFreeContentPackages.size(); i++)
        delete mFreeContentPackages[i];
}

void OP1AContentPackageManager::RegisterPictureTrackElement(uint32_t track_index, mxfKey element_key, bool is_cbe)
{
    mElements.push_back(new OP1AContentPackageElement(track_index, element_key,
                                                      mKAGSize, mMinLLen,
                                                      is_cbe));
    mElementTrackIndexMap[track_index] = mElements.back();
}

void OP1AContentPackageManager::RegisterAVCITrackElement(uint32_t track_index, mxfKey element_key,
                                                         uint32_t first_sample_size, uint32_t nonfirst_sample_size)
{
    mElements.push_back(new OP1AContentPackageElement(track_index, element_key,
                                                      mKAGSize, mMinLLen,
                                                      first_sample_size, nonfirst_sample_size));
    mElementTrackIndexMap[track_index] = mElements.back();
}

void OP1AContentPackageManager::RegisterSoundTrackElement(uint32_t track_index, mxfKey element_key,
                                                          vector<uint32_t> sample_sequence, uint32_t sample_size)
{
    mElements.push_back(new OP1AContentPackageElement(track_index, element_key, mKAGSize, mMinLLen,
                                                      sample_sequence, sample_size));
    mElementTrackIndexMap[track_index] = mElements.back();
}

void OP1AContentPackageManager::PrepareWrite()
{
    // order elements: picture elements followed by sound elements
    stable_sort(mElements.begin(), mElements.end(), compare_element);

    // check sound elements have identical sequences
    bool valid_sequences = true;
    size_t i;
    for (i = 0; i < mElements.size(); i++) {
        if (!mElements[i]->is_picture)
            break;
    }
    if (i + 1 < mElements.size()) {
        vector<uint32_t> first_sample_sequence = mElements[i]->sample_sequence;
        for (i = i + 1; i < mElements.size() && valid_sequences; i++) {
            if (first_sample_sequence.size() != mElements[i]->sample_sequence.size()) {
                valid_sequences = false;
                break;
            }

            size_t j;
            for (j = 0; j < first_sample_sequence.size(); j++) {
                if (first_sample_sequence[j] != mElements[i]->sample_sequence[j]) {
                    valid_sequences = false;
                    break;
                }
            }
        }
    }
    BMX_CHECK_M(valid_sequences, ("Sound tracks have different number of samples per frame"));
}

void OP1AContentPackageManager::WriteSamples(uint32_t track_index, const unsigned char *data, uint32_t size,
                                             uint32_t num_samples)
{
    BMX_ASSERT(data && size && num_samples);
    BMX_ASSERT(mElementTrackIndexMap.find(track_index) != mElementTrackIndexMap.end());

    size_t cp_index = 0;
    while (cp_index < mContentPackages.size() && mContentPackages[cp_index]->IsComplete(track_index))
        cp_index++;

    uint32_t sample_size = mElementTrackIndexMap[track_index]->sample_size;
    if (sample_size == 0)
        sample_size = size / num_samples;
    BMX_CHECK(size >= sample_size * num_samples);

    const unsigned char *data_ptr = data;
    uint32_t rem_size = sample_size * num_samples;
    uint32_t rem_num_samples = num_samples;
    while (rem_num_samples > 0) {
        if (cp_index >= mContentPackages.size()) {
            if (mFreeContentPackages.empty()) {
                BMX_CHECK(mContentPackages.size() < MAX_CONTENT_PACKAGES);
                mContentPackages.push_back(new OP1AContentPackage(mElements, mPosition + cp_index));
            } else {
                mContentPackages.push_back(mFreeContentPackages.back());
                mFreeContentPackages.pop_back();
                mContentPackages.back()->Reset(mPosition + cp_index);
            }
        }

        uint32_t num_written = mContentPackages[cp_index]->WriteSamples(track_index, data_ptr, rem_size, rem_num_samples);
        rem_num_samples -= num_written;
        rem_size -= num_written * sample_size;
        data_ptr += num_written * sample_size;

        cp_index++;
    }
}

void OP1AContentPackageManager::WriteSample(uint32_t track_index, const CDataBuffer *data_array, uint32_t array_size)
{
    BMX_ASSERT(data_array && array_size);

    size_t cp_index = 0;
    while (cp_index < mContentPackages.size() && mContentPackages[cp_index]->IsComplete(track_index))
        cp_index++;

    if (cp_index >= mContentPackages.size()) {
        if (mFreeContentPackages.empty()) {
            BMX_CHECK(mContentPackages.size() < MAX_CONTENT_PACKAGES);
            mContentPackages.push_back(new OP1AContentPackage(mElements, mPosition + cp_index));
        } else {
            mContentPackages.push_back(mFreeContentPackages.back());
            mFreeContentPackages.pop_back();
            mContentPackages.back()->Reset(mPosition + cp_index);
        }
    }

    mContentPackages[cp_index]->WriteSample(track_index, data_array, array_size);
}

bool OP1AContentPackageManager::HaveContentPackage() const
{
    return !mContentPackages.empty() && mContentPackages.front()->IsComplete();
}

bool OP1AContentPackageManager::HaveContentPackages(size_t num) const
{
    size_t i;
    for (i = 0; i < mContentPackages.size() && i < num; i++) {
        if (!mContentPackages[i]->IsComplete())
            break;
    }

    return i >= num;
}

void OP1AContentPackageManager::UpdateIndexTable(OP1AIndexTable *index_table, size_t num_content_packages)
{
    size_t i;
    for (i = 0; i < mContentPackages.size() && i < num_content_packages; i++) {
        BMX_ASSERT(mContentPackages[i]->IsComplete());
        mContentPackages[i]->UpdateIndexTable(index_table);
    }
    BMX_ASSERT(i == num_content_packages);
}

void OP1AContentPackageManager::WriteNextContentPackage(File *mxf_file, OP1AIndexTable *index_table)
{
    BMX_ASSERT(HaveContentPackage());

    mContentPackages.front()->UpdateIndexTable(index_table);
    mContentPackages.front()->Write(mxf_file);

    mFreeContentPackages.push_back(mContentPackages.front());
    mContentPackages.pop_front();

    mPosition++;
}

