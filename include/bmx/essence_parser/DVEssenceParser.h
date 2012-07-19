/*
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

#ifndef BMX_DV_ESSENCE_PARSER_H_
#define BMX_DV_ESSENCE_PARSER_H_


#include <bmx/essence_parser/EssenceParser.h>


#define DV_PARSER_MIN_DATA_SIZE     12000



namespace bmx
{


class DVEssenceParser : public EssenceParser
{
public:
    typedef enum
    {
        UNKNOWN_DV = 0,
        IEC_DV25,
        DVBASED_DV25,
        DV50,
        DV100_1080I,
        DV100_720P,
    } EssenceType;

public:
    DVEssenceParser();
    virtual ~DVEssenceParser();

    virtual uint32_t ParseFrameStart(const unsigned char *data, uint32_t data_size);
    virtual uint32_t ParseFrameSize(const unsigned char *data, uint32_t data_size);

    virtual void ParseFrameInfo(const unsigned char *data, uint32_t data_size);

public:
    EssenceType GetEssenceType() const { return mEssenceType; }
    bool Is50Hz() const { return mIs50Hz; }
    uint32_t GetFrameSize() const { return mFrameSize; }
    Rational GetAspectRatio() const { return mAspectRatio; }

private:
    uint32_t ParseFrameSizeInt(const unsigned char *data, uint32_t data_size);

private:
    EssenceType mEssenceType;
    bool mIs50Hz;
    uint32_t mFrameSize;
    Rational mAspectRatio;
};


};



#endif

