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

#ifndef BMX_VC2_WRITER_HELPER_H_
#define BMX_VC2_WRITER_HELPER_H_

#include <vector>
#include <set>

#include <bmx/frame/DataBufferArray.h>
#include <bmx/essence_parser/VC2EssenceParser.h>
#include <bmx/mxf_helper/VC2MXFDescriptorHelper.h>


#define VC2_PASSTHROUGH            0x0000
#define VC2_PICTURE_ONLY           0x0001
#define VC2_COMPLETE_SEQUENCES     0x0002


namespace bmx
{


class VC2WriterHelper
{
public:
    VC2WriterHelper();
    ~VC2WriterHelper();

    void SetDescriptorHelper(VC2MXFDescriptorHelper *descriptor_helper);

    void SetModeFlags(int flags);
    void SetSequenceHeader(const unsigned char *data, uint32_t size);

public:
    uint32_t ProcessFrame(const unsigned char *data, uint32_t size,
                          const CDataBuffer **data_array, uint32_t *array_size);
    void CompleteProcess();

    int64_t GetProcessFrameCount() const { return mProcessCount; }

private:
    uint32_t WriteParseInfo();
    uint32_t GetWaveletTransformOffset(uint32_t picture_number);
    uint32_t WritePictureHeader(uint32_t picture_number);

private:
    VC2MXFDescriptorHelper *mDescriptorHelper;
    VC2EssenceParser mEssenceParser;
    int mModeFlags;
    int64_t mProcessCount;
    unsigned char *mSeqHeaderData;
    uint32_t mSeqHeaderDataAllocSize;
    uint32_t mSeqHeaderDataSize;
    bool mFirstFrame;
    uint32_t mLastPictureNumber;
    bool mWarnedPictureNumberUpdate;
    VC2EssenceParser::ParseInfo mParseInfo;
    VC2EssenceParser::SequenceHeader mFirstSequenceHeader;
    VC2EssenceParser::SequenceHeader mCurrentSequenceHeader;
    std::set<uint8_t> mWaveletFilters;
    bool mIdenticalSequence;
    bool mCompleteSequence;
    std::vector<CDataBuffer> mDataArray;
    unsigned char *mWorkspace;
    uint32_t mWorkspaceAllocSize;
    uint32_t mWorkspaceSize;
};



};


#endif
