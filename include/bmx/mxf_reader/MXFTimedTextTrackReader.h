/*
 * Copyright (C) 2018, British Broadcasting Corporation
 * All Rights Reserved.
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

#ifndef BMX_MXF_TIMED_TEXT_TRACK_READER_H_
#define BMX_MXF_TIMED_TEXT_TRACK_READER_H_

#include <stdio.h>

#include <bmx/mxf_reader/MXFFileTrackReader.h>
#include <bmx/mxf_helper/TimedTextMXFResourceProvider.h>



namespace bmx
{


class MXFTimedTextTrackReader : public MXFFileTrackReader
{
public:
    MXFTimedTextTrackReader(MXFFileReader *file_reader, size_t track_index, MXFTrackInfo *track_info,
                            mxfpp::FileDescriptor *file_descriptor, mxfpp::SourcePackage *file_source_package);
    virtual ~MXFTimedTextTrackReader();

    virtual int64_t GetOrigin() const;

    virtual int16_t GetPrecharge(int64_t position, bool limit_to_available) const;
    virtual int64_t GetAvailablePrecharge(int64_t position) const;
    virtual int16_t GetRollout(int64_t position, bool limit_to_available) const;
    virtual int64_t GetAvailableRollout(int64_t position) const;

public:
    void SetBodySID(uint32_t body_sid);

    TimedTextManifest* GetManifest();

    void ReadTimedText(FILE *file_out, unsigned char **data_out, size_t *size_out);
    void ReadAncillaryResourceById(mxfUUID resource_id, FILE *file_out, unsigned char **data_out, size_t *size_out);
    void ReadAncillaryResourceByStreamId(uint32_t stream_id, FILE *file_out, unsigned char **data_out, size_t *size_out);

    TimedTextMXFResourceProvider* CreateResourceProvider();

private:
    void ReadStream(uint32_t stream_id, const mxfKey *stream_key,
                    FILE *file_out,
                    unsigned char **data_out, size_t *data_out_size,
                    std::vector<std::pair<int64_t, int64_t> > *ranges_out);

private:
    uint32_t mBodySID;
    mxfKey mEssenceElementKey;
};


};



#endif

