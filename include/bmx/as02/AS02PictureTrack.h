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

#ifndef BMX_AS02_PICTURE_TRACK_H_
#define BMX_AS02_PICTURE_TRACK_H_

#include <bmx/as02/AS02Track.h>
#include <bmx/mxf_helper/PictureMXFDescriptorHelper.h>
#include <bmx/frame/DataBufferArray.h>



namespace bmx
{


class AS02PictureTrack : public AS02Track
{
public:
    AS02PictureTrack(AS02Clip *clip, uint32_t track_index, EssenceType essence_type, mxfpp::File *file,
                     std::string rel_uri);
    virtual ~AS02PictureTrack();

    void SetAspectRatio(mxfRational aspect_ratio);      // default 16/9
    void SetPartitionInterval(int64_t frame_count);     // default 0 (single partition)
    void SetAFD(uint8_t afd);                           // default not set

public:
    virtual void WriteSamples(const unsigned char *data, uint32_t size, uint32_t num_samples);
    virtual void WriteSample(const CDataBuffer *data_array, uint32_t array_size);

protected:
    void HandlePartitionInterval(bool can_start_partition);

protected:
    PictureMXFDescriptorHelper *mPictureDescriptorHelper;

    int64_t mPartitionInterval;
    int64_t mPartitionFrameCount;
};


};



#endif

