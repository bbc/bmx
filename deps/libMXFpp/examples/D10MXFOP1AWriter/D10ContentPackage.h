/*
 * D10 MXF OP-1A content package
 *
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

#ifndef __D10_CONTENT_PACKAGE_H__
#define __D10_CONTENT_PACKAGE_H__

#include "../Common/CommonTypes.h"
#include "../Common/DynamicByteArray.h"

#define MAX_CP_AUDIO_TRACKS     8



class D10ContentPackage
{
public:
    virtual ~D10ContentPackage() {}

    virtual Timecode GetUserTimecode() const = 0;

    virtual const unsigned char* GetVideo() const = 0;
    virtual unsigned int GetVideoSize() const = 0;

    virtual uint32_t GetNumAudioTracks() const = 0;
    virtual const unsigned char* GetAudio(int index) const = 0;
    virtual unsigned int GetAudioSize() const = 0;
};


class D10ContentPackageInt : public D10ContentPackage
{
public:
    D10ContentPackageInt();
    D10ContentPackageInt(const D10ContentPackage *from);
    ~D10ContentPackageInt();

    void Reset();
    bool IsComplete(uint32_t num_audio_tracks) const;

public:
    // from D10ContentPackage
    virtual Timecode GetUserTimecode() const;
    virtual const unsigned char* GetVideo() const;
    virtual unsigned int GetVideoSize() const;
    virtual uint32_t GetNumAudioTracks() const;
    virtual const unsigned char* GetAudio(int index) const;
    virtual unsigned int GetAudioSize() const;

public:
    Timecode mUserTimecode;
    DynamicByteArray mVideoBytes;
    DynamicByteArray mAudioBytes[MAX_CP_AUDIO_TRACKS];
};


#endif

