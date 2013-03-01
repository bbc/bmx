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

#ifndef BMX_AVCI_WRITER_HELPER_H_
#define BMX_AVCI_WRITER_HELPER_H_

#include <bmx/frame/DataBufferArray.h>
#include <bmx/essence_parser/AVCIEssenceParser.h>



namespace bmx
{


typedef enum
{
    AVCI_PASS_MODE = 0,
    AVCI_NO_FRAME_HEADER_MODE,
    AVCI_NO_OR_ALL_FRAME_HEADER_MODE,
    AVCI_FIRST_FRAME_HEADER_MODE,
    AVCI_FIRST_OR_ALL_FRAME_HEADER_MODE,
    AVCI_ALL_FRAME_HEADER_MODE,
} AVCIMode;


class AVCIWriterHelper
{
public:
    AVCIWriterHelper();
    ~AVCIWriterHelper();

    void SetMode(AVCIMode mode);
    void SetHeader(const unsigned char *data, uint32_t size);
    void SetReplaceHeader(bool enable);

public:
    uint32_t ProcessFrame(const unsigned char *data, uint32_t size,
                          const CDataBuffer **data_array, uint32_t *array_size);

private:
    uint32_t PassFrame(const unsigned char *data, uint32_t size, uint32_t *array_size);
    uint32_t StripFrameHeader(const unsigned char *data, uint32_t size, uint32_t *array_size);
    uint32_t PrependFrameHeader(const unsigned char *data, uint32_t size, uint32_t *array_size);

private:
    AVCIEssenceParser mEssenceParser;
    AVCIMode mMode;
    unsigned char *mHeader;
    bool mReplaceHeader;
    CDataBuffer mDataArray[3];
    int64_t mSampleCount;
    bool mFirstFrameHeader;
    bool mSecondFrameHeader;
};



};


#endif

