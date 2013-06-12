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

#ifndef BMX_AVID_MPEG2LG_TRACK_H_
#define BMX_AVID_MPEG2LG_TRACK_H_

#include <bmx/avid_mxf/AvidPictureTrack.h>
#include <bmx/writer_helper/MPEG2LGWriterHelper.h>
#include <bmx/ByteArray.h>



namespace bmx
{


class AvidMPEG2LGTrack : public AvidPictureTrack
{
public:
    AvidMPEG2LGTrack(AvidClip *clip, uint32_t track_index, EssenceType essence_type, mxfpp::File *file);
    virtual ~AvidMPEG2LGTrack();

    virtual bool SupportOutputStartOffset() { return true; }

public:
    virtual void WriteSamples(const unsigned char *data, uint32_t size, uint32_t num_samples);

protected:
    virtual bool HaveCBEIndexTable() { return false; }
    virtual void WriteVBEIndexTable(mxfpp::Partition *partition);
    virtual void PostSampleWriting(mxfpp::Partition *partition);

private:
    MPEG2LGWriterHelper mWriterHelper;

    ByteArray mIndexSegment;
};


};



#endif

