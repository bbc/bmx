/*
 * Copyright (C) 2013, British Broadcasting Corporation
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

#ifndef BMX_KLV_ESSENCE_SOURCE_H_
#define BMX_KLV_ESSENCE_SOURCE_H_


#include <bmx/essence_parser/EssenceSource.h>



namespace bmx
{


class KLVEssenceSource : public EssenceSource
{
public:
    KLVEssenceSource(EssenceSource *child_source);
    KLVEssenceSource(EssenceSource *child_source, const mxfKey *key);
    KLVEssenceSource(EssenceSource *child_source, uint32_t track_num);
    virtual ~KLVEssenceSource();

    virtual uint32_t Read(unsigned char *data, uint32_t size);
    virtual bool SeekStart();
    virtual bool Skip(int64_t offset);

    virtual bool HaveError() const;
    virtual int GetErrno() const;
    virtual std::string GetStrError() const;

private:
    typedef enum
    {
        READ_KL_STATE,
        READ_V_STATE,
        READ_END_STATE,
    } KLVState;

private:
    EssenceSource *mChildSource;
    mxfKey mKey;
    uint32_t mTrackNum;
    KLVState mState;
    uint64_t mValueLen;
};


};



#endif

