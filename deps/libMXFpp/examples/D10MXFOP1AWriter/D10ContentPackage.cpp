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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libMXF++/MXFException.h>

#include <mxf/mxf_logging.h>

#include "../Common/CommonTypes.h"
#include "D10ContentPackage.h"

using namespace std;
using namespace mxfpp;



D10ContentPackageInt::D10ContentPackageInt()
{
    Reset();
}

D10ContentPackageInt::D10ContentPackageInt(const D10ContentPackage *from)
{
    mUserTimecode = from->GetUserTimecode();
    mVideoBytes.setBytes(from->GetVideo(), from->GetVideoSize());

    uint32_t i;
    for (i = 0; i < from->GetNumAudioTracks(); i++)
        mAudioBytes[i].setBytes(from->GetAudio(i), from->GetAudioSize());
}

D10ContentPackageInt::~D10ContentPackageInt()
{
}

void D10ContentPackageInt::Reset()
{
    mUserTimecode.hour      = 0;
    mUserTimecode.min       = 0;
    mUserTimecode.sec       = 0;
    mUserTimecode.frame     = 0;
    mUserTimecode.dropFrame = false;

    mVideoBytes.setSize(0);
    uint32_t i;
    for (i = 0; i < MAX_CP_AUDIO_TRACKS; i++)
        mAudioBytes[i].setSize(0);
}

bool D10ContentPackageInt::IsComplete(uint32_t num_audio_tracks) const
{
    if (mVideoBytes.getSize() == 0)
        return false;

    return GetNumAudioTracks() >= num_audio_tracks;
}

Timecode D10ContentPackageInt::GetUserTimecode() const
{
    return mUserTimecode;
}

const unsigned char *D10ContentPackageInt::GetVideo() const
{
    return mVideoBytes.getBytes();
}

unsigned int D10ContentPackageInt::GetVideoSize() const
{
    return mVideoBytes.getSize();
}

uint32_t D10ContentPackageInt::GetNumAudioTracks() const
{
    uint32_t i;
    for (i = 0; i < MAX_CP_AUDIO_TRACKS; i++) {
        if (mAudioBytes[i].getSize() == 0)
            break;
    }

    return i;
}

const unsigned char *D10ContentPackageInt::GetAudio(int index) const
{
    MXFPP_ASSERT(index >= 0 && index < MAX_CP_AUDIO_TRACKS);

    return mAudioBytes[index].getBytes();
}

unsigned int D10ContentPackageInt::GetAudioSize() const
{
    return mAudioBytes[0].getSize();
}

