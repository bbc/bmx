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

#ifndef BMX_AS10_CORE_FRAMEWORK_H_
#define BMX_AS10_CORE_FRAMEWORK_H_


#include <libMXF++/metadata/DMFramework.h>



namespace bmx
{


class AS10CoreFramework : public mxfpp::DMFramework
{
public:
    static void RegisterObjectFactory(mxfpp::HeaderMetadata *header_metadata);

public:
    static const mxfKey set_key;

public:
    friend class mxfpp::MetadataSetFactory<AS10CoreFramework>;

public:
    AS10CoreFramework(mxfpp::HeaderMetadata *header_metadata);
    virtual ~AS10CoreFramework();

    std::string GetShimName();
    bool HaveType();
    std::string GetType();
    bool HaveMainTitle();
    std::string GetMainTitle();
    bool HaveSubTitle();
    std::string GetSubTitle();
    bool HaveTitleDescription();
    std::string GetTitleDescription();
    bool HavePersonName();
    std::string GetPersonName();
    bool HaveOrganizationName();
    std::string GetOrganizationName();
    bool HaveLocationDescription();
    std::string GetLocationDescription();
    bool HaveCommonSpanningID();
    mxfUMID GetCommonSpanningID();
    bool HaveSpanningNumber();
    uint16_t GetSpanningNumber();
    bool HaveCumulativeDuration();
    int64_t GetCumulativeDuration();

    void SetShimName(std::string value);
    void SetType(std::string value);
    void SetMainTitle(std::string value);
    void SetSubTitle(std::string value);
    void SetTitleDescription(std::string value);
    void SetOrganizationName(std::string value);
    void SetPersonName(std::string value);
    void SetLocationDescription(std::string value);
    void SetCommonSpanningID(mxfUMID value);
    void SetSpanningNumber(uint16_t value);
    void SetCumulativeDuration(int64_t value);

protected:
    AS10CoreFramework(mxfpp::HeaderMetadata *header_metadata, ::MXFMetadataSet *c_metadata_set);
};



};


#endif
