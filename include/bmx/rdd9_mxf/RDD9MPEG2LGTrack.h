/*
 * Copyright (C) 2012, British Broadcasting Corporation
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

#ifndef BMX_RDD9_MPEG2LG_TRACK_H_
#define BMX_RDD9_MPEG2LG_TRACK_H_

#include <bmx/rdd9_mxf/RDD9Track.h>
#include <bmx/mxf_helper/PictureMXFDescriptorHelper.h>
#include <bmx/writer_helper/MPEG2LGWriterHelper.h>
#include <bmx/ByteArray.h>



namespace bmx
{


class RDD9MPEG2LGTrack : public RDD9Track
{
public:
    RDD9MPEG2LGTrack(RDD9File *file, uint32_t track_index, uint32_t track_id, uint8_t track_type_number,
                     Rational frame_rate, EssenceType essence_type);
    virtual ~RDD9MPEG2LGTrack();

    void SetAspectRatio(Rational aspect_ratio);         // default 16/9
    void SetPartitionInterval(int64_t frame_count);     // default 0 (single partition)
    void SetAFD(uint8_t afd);                           // default not set

protected:
    virtual void PrepareWrite(uint8_t track_count);
    virtual void WriteSamplesInt(const unsigned char *data, uint32_t size, uint32_t num_samples);
    virtual void CompleteWrite();

private:
    PictureMXFDescriptorHelper *mPictureDescriptorHelper;
    MPEG2LGWriterHelper mWriterHelper;
};


};



#endif

