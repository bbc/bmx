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

#ifndef BMX_AS02_MPEG2LG_TRACK_H_
#define BMX_AS02_MPEG2LG_TRACK_H_

#include <bmx/as02/AS02PictureTrack.h>
#include <bmx/writer_helper/MPEG2LGWriterHelper.h>
#include <bmx/ByteArray.h>



namespace bmx
{


class AS02MPEG2LGTrack : public AS02PictureTrack
{
public:
    AS02MPEG2LGTrack(AS02Clip *clip, uint32_t track_index, EssenceType essence_type, mxfpp::File *file,
                     std::string rel_uri);
    virtual ~AS02MPEG2LGTrack();

public:
    virtual void WriteSamples(const unsigned char *data, uint32_t size, uint32_t num_samples);

protected:
    virtual bool HaveCBEIndexTable() { return false; }
    virtual bool HaveVBEIndexEntries() { return mIndexSegments.size() != 0; }
    virtual void WriteVBEIndexTable(mxfpp::Partition *partition);
    virtual void PostSampleWriting(mxfpp::Partition *partition);

private:
    MPEG2LGWriterHelper mWriterHelper;

    std::vector<ByteArray*> mIndexSegments;
    int64_t mIndexStartPosition;
};


};



#endif

