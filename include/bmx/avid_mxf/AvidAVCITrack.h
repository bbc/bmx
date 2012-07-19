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

#ifndef BMX_AVID_AVCI_TRACK_H_
#define BMX_AVID_AVCI_TRACK_H_

#include <bmx/mxf_helper/AVCIMXFDescriptorHelper.h>
#include <bmx/writer_helper/AVCIWriterHelper.h>
#include <bmx/avid_mxf/AvidPictureTrack.h>



namespace bmx
{


typedef enum
{
    AVID_AVCI_NO_OR_ALL_FRAME_HEADER_MODE = 0,
    AVID_AVCI_NO_FRAME_HEADER_MODE,
    AVID_AVCI_ALL_FRAME_HEADER_MODE,
} AvidAVCIMode;


class AvidAVCITrack : public AvidPictureTrack
{
public:
    AvidAVCITrack(AvidClip *clip, uint32_t track_index, EssenceType essence_type, mxfpp::File *file);
    virtual ~AvidAVCITrack();

    void SetMode(AvidAVCIMode mode);                            // default AVID_AVCI_NO_OR_ALL_FRAME_HEADER_MODE
    void SetHeader(const unsigned char *data, uint32_t size);

public:
    uint32_t GetSampleWithoutHeaderSize();

public:
    virtual void WriteSamples(const unsigned char *data, uint32_t size, uint32_t num_samples);

protected:
    virtual void PostSampleWriting(mxfpp::Partition *partition);

private:
    AVCIMXFDescriptorHelper *mAVCIDescriptorHelper;
    AVCIWriterHelper mWriterHelper;
};


};



#endif

