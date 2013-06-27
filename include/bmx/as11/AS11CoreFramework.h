/*
 * Copyright (C) 2011, British Broadcasting Corporation
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

#ifndef BMX_AS11_CORE_FRAMEWORK_H_
#define BMX_AS11_CORE_FRAMEWORK_H_


#include <libMXF++/metadata/DMFramework.h>



namespace bmx
{


class AS11CoreFramework : public mxfpp::DMFramework
{
public:
    static void RegisterObjectFactory(mxfpp::HeaderMetadata *header_metadata);

public:
    static const mxfKey set_key;

public:
    friend class mxfpp::MetadataSetFactory<AS11CoreFramework>;

public:
    AS11CoreFramework(mxfpp::HeaderMetadata *header_metadata);
    virtual ~AS11CoreFramework();

    std::string GetSeriesTitle();
    std::string GetProgrammeTitle();
    std::string GetEpisodeTitleNumber();
    std::string GetShimName();
    mxfVersionType GetShimVersion();
    uint8_t GetAudioTrackLayout();
    std::string GetPrimaryAudioLanguage();
    bool GetClosedCaptionsPresent();
    bool HaveClosedCaptionsType();
    uint8_t GetClosedCaptionsType();
    bool HaveClosedCaptionsLanguage();
    std::string GetClosedCaptionsLanguage();

    void SetSeriesTitle(std::string value);
    void SetProgrammeTitle(std::string value);
    void SetEpisodeTitleNumber(std::string value);
    void SetShimName(std::string value);
    void SetShimVersion(mxfVersionType value);
    void SetAudioTrackLayout(uint8_t value);
    void SetPrimaryAudioLanguage(std::string value);
    void SetClosedCaptionsPresent(bool value);
    void SetClosedCaptionsType(uint8_t value);
    void SetClosedCaptionsLanguage(std::string value);

protected:
    AS11CoreFramework(mxfpp::HeaderMetadata *header_metadata, ::MXFMetadataSet *c_metadata_set);
};



};


#endif

