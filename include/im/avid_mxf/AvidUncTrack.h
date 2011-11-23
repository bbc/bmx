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

#ifndef __IM_AVID_UNC_TRACK_H__
#define __IM_AVID_UNC_TRACK_H__

#include <im/avid_mxf/AvidPictureTrack.h>
#include <im/mxf_helper/UncMXFDescriptorHelper.h>



namespace im
{


class AvidUncTrack : public AvidPictureTrack
{
public:
    AvidUncTrack(AvidClip *clip, uint32_t track_index, AvidEssenceType essence_type, mxfpp::File *file);
    virtual ~AvidUncTrack();

    void SetInputHeight(uint32_t height);   // default stored width

public:
    uint32_t GetInputSampleSize();

public:
    virtual void PrepareWrite();
    virtual void WriteSamples(const unsigned char *data, uint32_t size, uint32_t num_samples);

    virtual uint32_t GetImageStartOffset();

private:
    UncMXFDescriptorHelper *mUncDescriptorHelper;
    bool mIsAvid10Bit;
    uint32_t mInputHeight;
    uint32_t mInputSampleSize;
    unsigned char *mPadding;
    uint32_t mPaddingSize;
    uint32_t mSkipSize;
    uint32_t mMSBSampleSize;
    uint32_t mLSBSampleSize;
    uint32_t mLSBPaddingSize;
    uint32_t mLSBSkipSize;
};


};



#endif

