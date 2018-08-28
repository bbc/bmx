/*
 * Copyright (C) 2016, British Broadcasting Corporation
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

#ifndef BMX_UNIQUE_ID_HELPER_H_
#define BMX_UNIQUE_ID_HELPER_H_

#include <map>
#include <set>
#include <string>

#include <bmx/BMXTypes.h>


namespace bmx
{


typedef enum
{
    PICTURE_TRACK_TYPE  = MXF_PICTURE_DDEF,
    SOUND_TRACK_TYPE    = MXF_SOUND_DDEF,
    TIMECODE_TRACK_TYPE = MXF_TIMECODE_DDEF,
    DATA_TRACK_TYPE     = MXF_DATA_DDEF,
    DM_TRACK_TYPE       = MXF_DM_DDEF,
    XML_TRACK_TYPE      = 256,
    INDEX_STREAM_TYPE   = 1024,
    BODY_STREAM_TYPE    = 1025,
    GENERIC_STREAM_TYPE = 1026,
    STREAM_TYPE         = 1027,
} UniqueIdType;


class UniqueIdHelper
{
public:
    UniqueIdHelper();
    ~UniqueIdHelper();

    uint32_t GetNextId();

    void SetStartId(int type, uint32_t start_id);
    uint32_t GetNextId(int type);

    uint32_t SetId(const std::string &str_id, uint32_t suggest_id = 0);
    uint32_t GetId(const std::string &str_id);

private:
    uint32_t GetNextId(uint32_t &next_id);

private:
    uint32_t mNextId;
    std::map<int, uint32_t> mNextIdByType;
    std::map<std::string, uint32_t> mIdByStr;
    std::set<uint32_t> mUsedIds;
};



};


#endif
