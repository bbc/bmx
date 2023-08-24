/*
 * Holds the essence data for each track in the clip
 *
 * Copyright (C) 2009, British Broadcasting Corporation
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

#ifndef __OPATOM_CONTENT_PACKAGE_H__
#define __OPATOM_CONTENT_PACKAGE_H__

#include <map>
#include <vector>

#include "../Common/DynamicByteArray.h"


class OPAtomClipReader;
class OPAtomTrackReader;


class OPAtomContentElement
{
public:
    friend class OPAtomClipReader;
    friend class OPAtomTrackReader;

public:
    OPAtomContentElement();

    bool IsPicture() const { return mIsPicture; }
    uint32_t GetMaterialTrackId() const { return mMaterialTrackId; }

    const DynamicByteArray* GetEssenceData() const { return &mEssenceData; }
    uint32_t GetSize() const { return mEssenceData.getSize(); }
    const unsigned char* GetBytes() const { return mEssenceData.getBytes(); }

    int64_t GetEssenceOffset() const { return mEssenceOffset; }

    uint32_t GetNumSamples() const { return mNumSamples; }

private:
    void Reset();

private:
    bool mIsPicture;
    uint32_t mMaterialTrackId;
    DynamicByteArray mEssenceData;
    int64_t mEssenceOffset;
    uint32_t mNumSamples;
};


class OPAtomContentPackage
{
public:
    friend class OPAtomClipReader;
    friend class OPAtomTrackReader;

public:
    OPAtomContentPackage();
    virtual ~OPAtomContentPackage();

    int64_t GetPosition() const { return mPosition; }

    bool HaveEssenceData(uint32_t mp_track_id) const;
    const OPAtomContentElement* GetEssenceData(uint32_t mp_track_id) const;
    const unsigned char* GetEssenceDataBytes(uint32_t mp_track_id) const;
    uint32_t GetEssenceDataSize(uint32_t mp_track_id) const;

    size_t NumEssenceData() const { return mEssenceDataVector.size(); }
    const OPAtomContentElement* GetEssenceDataI(size_t index) const;
    const unsigned char* GetEssenceDataIBytes(size_t index) const;
    uint32_t GetEssenceDataISize(size_t index) const;

private:
    OPAtomContentElement* GetEssenceData(uint32_t mp_track_id, bool check) const;

    OPAtomContentElement* AddElement(uint32_t mp_track_id);

private:
    int64_t mPosition;

    std::vector<OPAtomContentElement*> mEssenceDataVector;
    std::map<uint32_t, OPAtomContentElement*> mEssenceData;
};



#endif

