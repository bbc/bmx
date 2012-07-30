/*
 * Copyright (C) 2012, British Broadcasting Corporation
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

#ifndef BMX_AVID_INFO_H_
#define BMX_AVID_INFO_H_


#include <bmx/avid_mxf/AvidTypes.h>



namespace bmx
{


class AvidInfo
{
public:
    static void RegisterExtensions(mxfpp::DataModel *data_model);

public:
    AvidInfo();
    ~AvidInfo();

    void ReadInfo(mxfpp::HeaderMetadata *header_metadata);

private:
    void Reset();

    void GetMaterialPackageAttrs(mxfpp::MaterialPackage *mp);
    void GetUserComments(mxfpp::MaterialPackage *mp);
    void GetLocators(mxfpp::MaterialPackage *mp);
    void GetPhysicalPackageInfo(mxfpp::HeaderMetadata *header_metadata);

    void GetAvidTaggedValues(mxfpp::MetadataSet *parent_set, const mxfKey *item_key,
                             std::vector<AvidTaggedValue*> *tags);
    AvidTaggedValue* GetAvidTaggedValue(mxfpp::MetadataSet *tag_set);

    void ClearAvidTaggedValues(std::vector<AvidTaggedValue*> &tags);

public:
    std::string project_name;
    std::vector<AvidTaggedValue*> material_package_attrs;
    std::vector<AvidTaggedValue*> user_comments;
    std::vector<AvidLocator> locators;
    Rational locators_edit_rate;
    bool have_phys_package;
    AvidPhysicalPackageType phys_package_type;
    std::string phys_package_name;
    std::string phys_package_locator;
    UMID phys_package_uid;
    int32_t resolution_id;
};


};



#endif
