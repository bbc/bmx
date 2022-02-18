/*
 * Copyright (C) 2015, British Broadcasting Corporation
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

#define __STDC_LIMIT_MACROS

#include <string.h>

#include <bmx/writer_helper/AVCWriterHelper.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;


// MaxDpbFrames is limited to a maximum of 16
#define MAX_DPB_FRAMES  16


AVCWriterHelper::IndexedFrame::IndexedFrame()
{
    is_complete = false;
    is_decoded = false;
    position = 0;
    frame_type = UNKNOWN_FRAME_TYPE;
    pic_order_cnt = 0;
    decoded_frame_offset = 0;
    key_frame_offset = 0;
    temporal_offset = 0;
    flags = 0;
}


AVCWriterHelper::AVCWriterHelper()
{
    mDescriptorHelper = 0;
    mPosition = 0;
    mNextIndexedDecodedPos = 0;
    mNextIndexedPos = 0;
    mKeyFramePosition = -1;
    mDecodingDelay = 0;
    mBPictureCount = 0;
    mMaxBPictureCount = 0;
    mConstantBFrames = true;
    mPictureType = FRAME_PICTURE;
    mCodingType = FRAME_CODING;
    mClosedGOP = true;
    mGOPStartPosition = 0;
    mUnlimitedGOPSize = false;
    mMaxGOP = 0;
    mStartsWithIFrame = true;
    mIdenticalGOP = true;
    mFirstGOP = true;
    mMaxBitRate = 0;
    mMaxNumRefFrames = 0;
    mSPSConstant = true;
    mSPSFirstAUOnly = false;
    mSPSEveryAU = false;
    mSPSGOPStart = false;
    mPPSConstant = true;
    mPPSFirstAUOnly = false;
    mPPSEveryAU = false;
    mPPSGOPStart = false;
}

AVCWriterHelper::~AVCWriterHelper()
{
}

void AVCWriterHelper::SetDescriptorHelper(AVCMXFDescriptorHelper *descriptor_helper)
{
    mDescriptorHelper = descriptor_helper;
}

void AVCWriterHelper::SetHeader(const unsigned char *data, uint32_t size)
{
    vector<NALReference> nals;
    mEssenceParser.ParseNALUnits(data, size, &nals);

    size_t i;
    for (i = 0; i < nals.size(); i++) {
        if (nals[i].type == SEQUENCE_PARAMETER_SET)
            SetSPS(nals[i].data, nals[i].size);
        else if (nals[i].type == PICTURE_PARAMETER_SET)
            SetPPS(nals[i].data, nals[i].size);
    }
}

void AVCWriterHelper::SetSPS(const unsigned char *data, uint32_t size)
{
    mEssenceParser.SetSPS(data, size);
}

void AVCWriterHelper::SetPPS(const unsigned char *data, uint32_t size)
{
    mEssenceParser.SetPPS(data, size);
}

void AVCWriterHelper::ProcessFrame(const unsigned char *data, uint32_t size)
{
    mEssenceParser.ParseFrameInfo(data, size);
    int32_t pic_order_cnt;
    mEssenceParser.DecodePOC(&mPOCState, &pic_order_cnt);

    MPEGFrameType frame_type = mEssenceParser.GetFrameType();
    BMX_CHECK(frame_type != UNKNOWN_FRAME_TYPE);

    bool gop_start = (frame_type == I_FRAME);

    if (frame_type == B_FRAME) {
        mBPictureCount++;
        if (mBPictureCount > mMaxBPictureCount)
            mMaxBPictureCount = mBPictureCount;
    } else if (mBPictureCount > 0) {
        if (mBPictureCount != mMaxBPictureCount)
            mConstantBFrames = false;
        mBPictureCount = 0;
    }

    if (mPosition == 0) {
        if (mEssenceParser.IsFieldPicture())
            mPictureType = FIELD_PICTURE;
        else
            mPictureType = FRAME_PICTURE;
        if (mEssenceParser.IsFrameMBSOnly())
            mCodingType = FRAME_CODING;
        else if (mEssenceParser.IsFieldPicture())
            mCodingType = FIELD_CODING;
        else if (mEssenceParser.IsMBAdaptiveFFEncoding())
            mCodingType = MB_ADAPTIVE_FRAME_FIELD_CODING;
    } else {
        if (mPictureType == FRAME_PICTURE && mEssenceParser.IsFieldPicture())
            mPictureType = FRAME_OR_FIELD_PICTURE;
        else if (mPictureType == FIELD_PICTURE && !mEssenceParser.IsFieldPicture())
            mPictureType = FRAME_OR_FIELD_PICTURE;
        if (mCodingType == FRAME_CODING && mEssenceParser.IsFieldPicture())
            mCodingType = FRAME_FIELD_ENCODING;
        else if (mCodingType == FIELD_CODING && !mEssenceParser.IsFieldPicture())
            mCodingType = FRAME_FIELD_ENCODING;
        else if (mCodingType == MB_ADAPTIVE_FRAME_FIELD_CODING && mEssenceParser.IsFieldPicture())
            mCodingType = FRAME_FIELD_ENCODING;
        else if (mCodingType == FRAME_CODING && mEssenceParser.IsMBAdaptiveFFEncoding())
            mCodingType = MB_ADAPTIVE_FRAME_FIELD_CODING;
    }

    if (gop_start && !mEssenceParser.IsIDRFrame())
        mClosedGOP = false;

    if (gop_start && !mUnlimitedGOPSize) {
        if (mPosition - mGOPStartPosition > UINT16_MAX) {
            mUnlimitedGOPSize = true;
            mMaxGOP = 0;
        } else {
            uint16_t gop_size = (uint16_t)(mPosition - mGOPStartPosition);
            if (gop_size > mMaxGOP)
                mMaxGOP = gop_size;
        }
    }

    if (mPosition == 0 && !gop_start)
        mStartsWithIFrame = false;
    if (!mStartsWithIFrame && gop_start && mIdenticalGOP)
        mIdenticalGOP = false;

    if (mIdenticalGOP) {
        if (mFirstGOP && gop_start && mPosition > 0)
            mFirstGOP = false;

        if (mFirstGOP) {
            mGOPStructure.push_back(frame_type);
            if (mGOPStructure.size() >= 4096) {
                // GOP is so large that we don't care about identical GOP information
                mIdenticalGOP = false;
            }
        } else {
            size_t pos_in_gop = (gop_start ? 0 : (size_t)(mPosition - mGOPStartPosition));
            if (pos_in_gop >= mGOPStructure.size() || mGOPStructure[pos_in_gop] != frame_type)
                mIdenticalGOP = false;
        }
    }

    if (mEssenceParser.GetMaxNumRefFrames() > mMaxNumRefFrames)
        mMaxNumRefFrames = mEssenceParser.GetMaxNumRefFrames();
    if (mEssenceParser.GetMaxBitRate() > mMaxBitRate)
        mMaxBitRate = mEssenceParser.GetMaxBitRate();

    if (mPosition == 0) {
        if (mEssenceParser.FrameHasActiveSPS()) {
            mSPSFirstAUOnly = true;
            mSPSEveryAU     = true;
            mSPSGOPStart    = gop_start;
        }
        if (mEssenceParser.FrameHasActivePPS()) {
            mPPSFirstAUOnly = true;
            mPPSEveryAU     = true;
            mPPSGOPStart    = gop_start;
        }
    } else {
        if (mEssenceParser.FrameHasActiveSPS()) {
            if (mSPSConstant && !mEssenceParser.IsActiveSPSDataConstant())
                mSPSConstant = false;
            mSPSFirstAUOnly = false;
        } else {
            mSPSEveryAU = false;
            if (gop_start)
                mSPSGOPStart = false;
        }
        if (mEssenceParser.FrameHasActivePPS()) {
            if (mPPSConstant && !mEssenceParser.IsActivePPSDataConstant())
                mPPSConstant = false;
            mPPSFirstAUOnly = false;
        } else {
            mPPSEveryAU = false;
            if (gop_start)
                mPPSGOPStart = false;
        }
    }

    uint8_t flags = 0x00;
    // TODO: check if an open-GOP can be a random access point
    if (mEssenceParser.IsIDRFrame())
        flags |= 1 << 7; // random access bit
    if (mEssenceParser.FrameHasActiveSPS())
        flags |= 1 << 6; // sequence parameter set in stream
    // TODO: determine prediction directions (which can also effect the key frame offset...)
    if (frame_type == I_FRAME)
        flags |= 0 << 4;
    else if (frame_type == P_FRAME)
        flags |= 2 << 4; // naive setting - assume forward prediction
    else
        flags |= 3 << 4; // naive setting - assume forward and backward prediction
    // TODO: B and P picture reference/referenced cases
    if (mEssenceParser.IsIDRFrame())
        flags |= 1 << 2;
    if (frame_type == I_FRAME)
        flags |= 0;
    else if (frame_type == P_FRAME)
        flags |= 2;
    else
        flags |= 3;

    if (mEssenceParser.IsIDRFrame()) {
        PopAllDecodedFrames();
    } else {
        while (mDecodedFrames.size() > MAX_DPB_FRAMES)
            PopDecodedFrame();
    }

    IndexedFrame indexed_frame;
    indexed_frame.position      = mPosition;
    indexed_frame.pic_order_cnt = pic_order_cnt;
    indexed_frame.frame_type    = frame_type;
    indexed_frame.flags         = flags;
    mIndexedCodedFrames[mPosition] = indexed_frame;

    BMX_CHECK_M(mDecodedFrames.count(pic_order_cnt) == 0,
                ("Duplicate AVC pic order count value %d", pic_order_cnt));
    mDecodedFrames[pic_order_cnt] = mPosition;

    if (mPosition == 0)
        mDescriptorHelper->UpdateFileDescriptor(&mEssenceParser);

    if (frame_type == I_FRAME)
        mGOPStartPosition = mPosition;
    mPosition++;
}

void AVCWriterHelper::CompleteProcess()
{
    PopAllDecodedFrames();

    if (!mUnlimitedGOPSize) {
        if (mPosition - mGOPStartPosition > UINT16_MAX) {
            mUnlimitedGOPSize = true;
            mMaxGOP = 0;
        } else {
            uint16_t gop_size = (uint16_t)(mPosition - mGOPStartPosition);
            if (gop_size > mMaxGOP)
                mMaxGOP = gop_size;
        }
    }


    CDCIEssenceDescriptor *cdci_descriptor = dynamic_cast<CDCIEssenceDescriptor*>(mDescriptorHelper->GetFileDescriptor());
    BMX_ASSERT(cdci_descriptor);

    if (!cdci_descriptor->haveFrameLayout()) {
        // TODO: not sure progressive/interlaced can be deduced from the coding parameters
        if (mPictureType == FRAME_PICTURE && mCodingType == FRAME_CODING)
            cdci_descriptor->setFrameLayout(MXF_FULL_FRAME);
        else
            cdci_descriptor->setFrameLayout(MXF_SEPARATE_FIELDS);
    }
    if (cdci_descriptor->getFrameLayout() == MXF_SEPARATE_FIELDS) {
        if (cdci_descriptor->haveStoredHeight())
            cdci_descriptor->setStoredHeight(cdci_descriptor->getStoredHeight() / 2);
        if (cdci_descriptor->haveDisplayHeight())
            cdci_descriptor->setDisplayHeight(cdci_descriptor->getDisplayHeight() / 2);
        if (cdci_descriptor->haveSampledHeight())
            cdci_descriptor->setSampledHeight(cdci_descriptor->getSampledHeight() / 2);
    }
    if (!cdci_descriptor->haveVideoLineMap()) {
        // TODO: take signal standard into account
        if (cdci_descriptor->getFrameLayout() == MXF_FULL_FRAME) {
            cdci_descriptor->appendVideoLineMap(1);
            cdci_descriptor->appendVideoLineMap(0);
        } else {
            int32_t height_factor = 1;
            if (cdci_descriptor->getFrameLayout() == MXF_MIXED_FIELDS ||
                cdci_descriptor->getFrameLayout() == MXF_SEGMENTED_FRAME)
            {
                height_factor = 2;
            }
            cdci_descriptor->appendVideoLineMap(1);
            if (cdci_descriptor->haveDisplayHeight())
              cdci_descriptor->appendVideoLineMap((int32_t)(cdci_descriptor->getDisplayHeight() / height_factor + 1));
            else
              cdci_descriptor->appendVideoLineMap(0);
        }
    }


    AVCSubDescriptor *avc_sub_desc = dynamic_cast<AVCMXFDescriptorHelper*>(mDescriptorHelper)->GetAVCSubDescriptor();

    if (mMaxBPictureCount > 0) {
        avc_sub_desc->setAVCDecodingDelay(mDecodingDelay);
        avc_sub_desc->setAVCConstantBPictureFlag(mConstantBFrames);
    } else {
        avc_sub_desc->setAVCDecodingDelay(0);
    }
    if (mPictureType == FRAME_PICTURE &&
        mCodingType  == FRAME_CODING)
    {
        avc_sub_desc->setAVCCodedContentKind(MXF_AVC_PROGRESSIVE_FRAME_PICTURE);
    }
    else if (mPictureType == FIELD_PICTURE &&
             mCodingType  == FIELD_CODING)
    {
        avc_sub_desc->setAVCCodedContentKind(MXF_AVC_INTERLACED_FIELD_PICTURE);
    }
    else if (mPictureType == FRAME_PICTURE &&
             mCodingType  == MB_ADAPTIVE_FRAME_FIELD_CODING)
    {
        avc_sub_desc->setAVCCodedContentKind(MXF_AVC_INTERLACED_FRAME_PICTURE);
    }
    else if (mPictureType == FRAME_OR_FIELD_PICTURE &&
             (mCodingType == MB_ADAPTIVE_FRAME_FIELD_CODING ||
              mCodingType == FRAME_FIELD_ENCODING))
    {
        avc_sub_desc->setAVCCodedContentKind(MXF_AVC_INTERLACED_FRAME_AND_FIELD_PICTURE);
    }
    else
    {
        avc_sub_desc->setAVCCodedContentKind(MXF_AVC_UNKNOWN_CODED_CONTENT_TYPE);
    }
    avc_sub_desc->setAVCClosedGOPIndicator(mClosedGOP);
    avc_sub_desc->setAVCIdenticalGOPIndicator(mIdenticalGOP);
    avc_sub_desc->setAVCMaximumGOPSize(mMaxGOP);
    avc_sub_desc->setAVCMaximumBPictureCount(mMaxBPictureCount);
    if (mMaxBitRate > 0)
        avc_sub_desc->setAVCMaximumBitrate(mMaxBitRate);
    //avc_sub_desc->setAVCAverageBitrate(); // TODO
    avc_sub_desc->setAVCMaximumRefFrames(mMaxNumRefFrames);
    avc_sub_desc->setAVCSequenceParameterSetFlag(GetSPSFlag());
    avc_sub_desc->setAVCPictureParameterSetFlag(GetPPSFlag());
}

bool AVCWriterHelper::TakeCompleteIndexEntry(int64_t *position, int8_t *temporal_offset, int8_t *key_frame_offset,
                                             uint8_t *flags, MPEGFrameType *frame_type)
{
    if (mCompleteIndexedFrames.empty())
        return false;

    IndexedFrame complete_index = mCompleteIndexedFrames.front();
    mCompleteIndexedFrames.pop();

    SetIndexResult(complete_index, position, temporal_offset, key_frame_offset, flags, frame_type);

    return true;
}

void AVCWriterHelper::GetIncompleteIndexEntry(int64_t *position, int8_t *temporal_offset, int8_t *key_frame_offset,
                                              uint8_t *flags, MPEGFrameType *frame_type)
{
    int64_t current_pos = GetFramePosition();
    IndexedFrame incomplete_index;
    if (mIndexedCodedFrames.count(current_pos))
        incomplete_index = mIndexedCodedFrames[current_pos];
    else if (mIndexedDecodedFrames.count(current_pos))
        incomplete_index = mIndexedDecodedFrames[current_pos];
    else
        incomplete_index = mIncompleteIndexedFrames[current_pos];

    SetIndexResult(incomplete_index, position, temporal_offset, key_frame_offset, flags, frame_type);
}

void AVCWriterHelper::SetIndexResult(const IndexedFrame &indexed_frame, int64_t *position, int8_t *temporal_offset,
                                     int8_t *key_frame_offset, uint8_t *flags, MPEGFrameType *frame_type)
{
    *position = indexed_frame.position;
    *flags = indexed_frame.flags;
    if (indexed_frame.temporal_offset < INT8_MIN || indexed_frame.temporal_offset > INT8_MAX) {
        if (indexed_frame.temporal_offset < INT8_MIN)
            *temporal_offset = INT8_MIN;
        else
            *temporal_offset = INT8_MAX;
        *flags |= 1 << 3;
    } else {
        *temporal_offset = (int8_t)indexed_frame.temporal_offset;
    }
    if (indexed_frame.key_frame_offset < INT8_MIN) {
        *key_frame_offset = INT8_MIN;
        *flags |= 1 << 3;
    } else {
        *key_frame_offset = (int8_t)indexed_frame.key_frame_offset;
    }
    *frame_type = indexed_frame.frame_type;
}

void AVCWriterHelper::PopAllDecodedFrames()
{
    while (!mDecodedFrames.empty())
        PopDecodedFrame();
}

void AVCWriterHelper::PopDecodedFrame()
{
    int64_t decoded_pos = mPosition - mDecodedFrames.size();
    int64_t coded_pos   = mDecodedFrames.begin()->second;

    int64_t decoding_delay = coded_pos - decoded_pos;
    if (decoding_delay > (int64_t)mDecodingDelay)
        mDecodingDelay = (uint8_t)decoding_delay;

    mIndexedDecodedFrames[coded_pos] = mIndexedCodedFrames[coded_pos];
    mIndexedCodedFrames.erase(coded_pos);
    IndexedFrame &indexed_dec_frame = mIndexedDecodedFrames[coded_pos];
    if (indexed_dec_frame.frame_type == I_FRAME) {
        indexed_dec_frame.key_frame_offset = 0;
        mKeyFramePosition = coded_pos;
    } else if (mKeyFramePosition >= 0) {
        indexed_dec_frame.key_frame_offset = mKeyFramePosition - coded_pos;
    } else {
        indexed_dec_frame.key_frame_offset = (int64_t)INT32_MIN - 1;
    }
    indexed_dec_frame.decoded_frame_offset = decoded_pos - coded_pos;
    if (indexed_dec_frame.decoded_frame_offset == 0)
        indexed_dec_frame.is_complete = true;
    indexed_dec_frame.is_decoded = true;

    while (!mIndexedDecodedFrames.empty() &&
            mIndexedDecodedFrames.begin()->first == mNextIndexedDecodedPos &&
            mIndexedDecodedFrames.begin()->second.is_decoded)
    {
        if (mIncompleteIndexedFrames.count(mNextIndexedDecodedPos)) {
            IndexedFrame &forward_frame = mIncompleteIndexedFrames[mNextIndexedDecodedPos];
            mIndexedDecodedFrames[mNextIndexedDecodedPos].temporal_offset = forward_frame.temporal_offset;
            mIndexedDecodedFrames[mNextIndexedDecodedPos].is_complete     = true;
        }
        mIncompleteIndexedFrames[mNextIndexedDecodedPos] = mIndexedDecodedFrames[mNextIndexedDecodedPos];
        mIndexedDecodedFrames.erase(mIndexedDecodedFrames.begin());

        IndexedFrame &indexed_frame = mIncompleteIndexedFrames[mNextIndexedDecodedPos];
        if (indexed_frame.decoded_frame_offset != 0) {
            int64_t decode_pos = mNextIndexedDecodedPos + indexed_frame.decoded_frame_offset;
            if (mIncompleteIndexedFrames.count(decode_pos)) {
                mIncompleteIndexedFrames[decode_pos].temporal_offset = - indexed_frame.decoded_frame_offset;
                mIncompleteIndexedFrames[decode_pos].is_complete     = true;
            } else {
                IndexedFrame forward_frame;
                forward_frame.temporal_offset = - indexed_frame.decoded_frame_offset;
                mIncompleteIndexedFrames[decode_pos] = forward_frame;
            }
        }

        mNextIndexedDecodedPos++;
    }

    while (!mIncompleteIndexedFrames.empty() &&
            mIncompleteIndexedFrames.begin()->first == mNextIndexedPos &&
            mIncompleteIndexedFrames.begin()->second.is_complete)
    {
        IndexedFrame &indexed_frame = mIncompleteIndexedFrames.begin()->second;
        if (indexed_frame.frame_type == I_FRAME && indexed_frame.temporal_offset == 0)
            indexed_frame.flags |= 1 << 7; // random access bit

        mCompleteIndexedFrames.push(indexed_frame);
        mIncompleteIndexedFrames.erase(mIncompleteIndexedFrames.begin());

        mNextIndexedPos++;
    }

    mDecodedFrames.erase(mDecodedFrames.begin());
}

uint8_t AVCWriterHelper::GetSPSFlag() const
{
    uint8_t flag = 0;
    if (mSPSConstant)
        flag |= 1 << 7;
    if (mSPSFirstAUOnly)
        flag |= 1 << 4;
    else if (mSPSEveryAU)
        flag |= 2 << 4;
    else if (mSPSGOPStart)
        flag |= 3 << 4;

    return flag;
}

uint8_t AVCWriterHelper::GetPPSFlag() const
{
    uint8_t flag = 0;
    if (mPPSConstant)
        flag |= 1 << 7;
    if (mPPSFirstAUOnly)
        flag |= 1 << 4;
    else if (mPPSEveryAU)
        flag |= 2 << 4;
    else if (mPPSGOPStart)
        flag |= 3 << 4;

    return flag;
}
