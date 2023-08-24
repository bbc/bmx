/*
 * Read a single OP-Atom file representing a clip track
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

#ifndef __OPATOM_TRACK_READER_H__
#define __OPATOM_TRACK_READER_H__

#include <string>

#include "OPAtomContentPackage.h"
#include "RawEssenceParser.h"
#include "FrameOffsetIndexTable.h"

namespace mxfpp
{
class HeaderMetadata;
class File;
};


typedef enum
{
    OP_ATOM_SUCCESS = 0,
    OP_ATOM_FAIL = -1,
    OP_ATOM_FILE_OPEN_READ_ERROR = -2,
    OP_ATOM_NO_HEADER_PARTITION = -3,
    OP_ATOM_NOT_OP_ATOM = -4,
    OP_ATOM_HEADER_ERROR = -5,
    OP_ATOM_NO_FILE_PACKAGE = -6,
    OP_ATOM_ESSENCE_DATA_NOT_FOUND = -7
} OPAtomOpenResult;


class OPAtomTrackReader
{
public:
    static OPAtomOpenResult Open(std::string filename, OPAtomTrackReader **track_reader);
    static std::string ErrorToString(OPAtomOpenResult result);

public:
    ~OPAtomTrackReader();

    std::string GetFilename() const { return mFilename; }
    mxfpp::HeaderMetadata* GetHeaderMetadata() const { return mHeaderMetadata; }
    mxfpp::DataModel* GetDataModel() const { return mDataModel; }
    uint32_t GetMaterialTrackId() const { return mTrackId; }

    mxfUL GetEssenceContainerLabel();
    int64_t GetDuration();
    int64_t DetermineDuration();
    int64_t GetPosition();

    bool Read(OPAtomContentPackage *content);

    bool Seek(int64_t position);

    bool IsEOF();

private:
    OPAtomTrackReader(std::string filename, mxfpp::File *mxf_file);

private:
    std::string mFilename;

    uint32_t mTrackId;
    int64_t mDurationInMetadata;
    bool mIsPicture;

    mxfpp::HeaderMetadata *mHeaderMetadata;
    mxfpp::DataModel *mDataModel;

    FrameOffsetIndexTableSegment *mIndexTable;

    RawEssenceParser *mEssenceParser;
};



#endif

