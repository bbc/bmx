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

#ifndef BMX_AVC_WRITER_HELPER_H_
#define BMX_AVC_WRITER_HELPER_H_

#include <vector>
#include <map>
#include <queue>

#include <bmx/essence_parser/AVCEssenceParser.h>
#include <bmx/mxf_helper/AVCMXFDescriptorHelper.h>


namespace bmx
{


class AVCWriterHelper
{
public:
    typedef enum
    {
        FRAME_PICTURE,
        FIELD_PICTURE,
        FRAME_OR_FIELD_PICTURE,
    } PictureType;

    typedef enum
    {
        FRAME_CODING,
        FIELD_CODING,
        MB_ADAPTIVE_FRAME_FIELD_CODING,
        FRAME_FIELD_ENCODING,
    } CodingType;

public:
    AVCWriterHelper();
    ~AVCWriterHelper();

    void SetDescriptorHelper(AVCMXFDescriptorHelper *descriptor_helper);

    void SetHeader(const unsigned char *data, uint32_t size);
    void SetSPS(const unsigned char *data, uint32_t size);
    void SetPPS(const unsigned char *data, uint32_t size);

    void ProcessFrame(const unsigned char *data, uint32_t size);
    bool CheckTemporalOffsetsComplete(int64_t end_offset);
    void CompleteProcess();

public:
    int64_t GetFramePosition() const { return mPosition - 1; }

    bool TakeCompleteIndexEntry(int64_t *position, int8_t *temporal_offset, int8_t *key_frame_offset, uint8_t *flags,
                                MPEGFrameType *frame_type);
    void GetIncompleteIndexEntry(int64_t *position, int8_t *temporal_offset, int8_t *key_frame_offset,
                                 uint8_t *flags, MPEGFrameType *frame_type);

private:
    class IndexedFrame
    {
    public:
        IndexedFrame();

    public:
        bool is_complete;
        bool is_decoded;
        int64_t position;
        MPEGFrameType frame_type;
        int32_t pic_order_cnt;
        int64_t decoded_frame_offset;
        int64_t key_frame_offset;
        int64_t temporal_offset;
        uint8_t flags;
    };

private:
    void SetIndexResult(const IndexedFrame &indexed_frame, int64_t *position, int8_t *temporal_offset,
                        int8_t *key_frame_offset, uint8_t *flags, MPEGFrameType *frame_type);

    void PopAllDecodedFrames();
    void PopDecodedFrame();

    uint8_t GetSPSFlag() const;
    uint8_t GetPPSFlag() const;

private:
    AVCMXFDescriptorHelper *mDescriptorHelper;
    AVCEssenceParser mEssenceParser;
    int64_t mPosition;

    POCState mPOCState;
    std::map<int64_t, IndexedFrame> mIndexedCodedFrames;
    std::map<int32_t, int64_t> mDecodedFrames;
    std::map<int64_t, IndexedFrame> mIndexedDecodedFrames;
    std::map<int64_t, IndexedFrame> mIncompleteIndexedFrames;
    std::queue<IndexedFrame> mCompleteIndexedFrames;
    int64_t mNextIndexedDecodedPos;
    int64_t mNextIndexedPos;
    int64_t mKeyFramePosition;

    uint8_t mDecodingDelay;
    uint16_t mBPictureCount;
    uint16_t mMaxBPictureCount;
    bool mConstantBFrames;
    PictureType mPictureType;
    CodingType mCodingType;
    bool mClosedGOP;
    int64_t mGOPStartPosition;
    bool mUnlimitedGOPSize;
    uint16_t mMaxGOP;
    bool mStartsWithIFrame;
    bool mIdenticalGOP;
    bool mFirstGOP;
    std::vector<int> mGOPStructure;
    uint32_t mMaxBitRate;
    uint8_t mMaxNumRefFrames;
    bool mSPSConstant;
    bool mSPSFirstAUOnly;
    bool mSPSEveryAU;
    bool mSPSGOPStart;
    bool mPPSConstant;
    bool mPPSFirstAUOnly;
    bool mPPSEveryAU;
    bool mPPSGOPStart;
};



};


#endif
