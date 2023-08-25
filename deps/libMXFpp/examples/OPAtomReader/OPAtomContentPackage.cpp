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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libMXF++/MXFException.h>

#include "OPAtomContentPackage.h"

using namespace std;
using namespace mxfpp;



OPAtomContentElement::OPAtomContentElement()
{
    mIsPicture = true;
    mMaterialTrackId = 0;
    mEssenceOffset = 0;
}




OPAtomContentPackage::OPAtomContentPackage()
{
    mPosition = 0;
}

OPAtomContentPackage::~OPAtomContentPackage()
{
    size_t i;
    for (i = 0; i < mEssenceDataVector.size(); i++)
        delete mEssenceDataVector[i];

    // mEssenceData contains copies of content elements
}

bool OPAtomContentPackage::HaveEssenceData(uint32_t mp_track_id) const
{
    return GetEssenceData(mp_track_id, false) != 0;
}

const OPAtomContentElement *OPAtomContentPackage::GetEssenceData(uint32_t mp_track_id) const
{
    return GetEssenceData(mp_track_id, true);
}

const unsigned char *OPAtomContentPackage::GetEssenceDataBytes(uint32_t mp_track_id) const
{
    const OPAtomContentElement *element = GetEssenceData(mp_track_id);

    return element->GetBytes();
}

uint32_t OPAtomContentPackage::GetEssenceDataSize(uint32_t mp_track_id) const
{
    const OPAtomContentElement *element = GetEssenceData(mp_track_id);

    return element->GetSize();
}

const OPAtomContentElement *OPAtomContentPackage::GetEssenceDataI(size_t index) const
{
    MXFPP_ASSERT(index < mEssenceDataVector.size());

    return mEssenceDataVector[index];
}

const unsigned char *OPAtomContentPackage::GetEssenceDataIBytes(size_t index) const
{
    const OPAtomContentElement *element = GetEssenceDataI(index);

    return element->GetBytes();
}

uint32_t OPAtomContentPackage::GetEssenceDataISize(size_t index) const
{
    const OPAtomContentElement *element = GetEssenceDataI(index);

    return element->GetSize();
}

OPAtomContentElement* OPAtomContentPackage::GetEssenceData(uint32_t mp_track_id, bool check) const
{
    map<uint32_t, OPAtomContentElement*>::const_iterator result = mEssenceData.find(mp_track_id);
    if (result == mEssenceData.end()) {
        if (check)
            MXFPP_ASSERT(result != mEssenceData.end());
        return 0;
    }

    return result->second;
}

OPAtomContentElement* OPAtomContentPackage::AddElement(uint32_t mp_track_id)
{
    MXFPP_ASSERT(!GetEssenceData(mp_track_id, false));

    mEssenceDataVector.push_back(new OPAtomContentElement());
    mEssenceData[mp_track_id] = mEssenceDataVector.back();

    return GetEssenceData(mp_track_id, true);
}

