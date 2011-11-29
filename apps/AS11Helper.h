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

#ifndef __AS11_HELPER_H__
#define __AS11_HELPER_H__

#include <string>

#include <libMXF++/MXF.h>

#include <im/as11/AS11Clip.h>



namespace im
{


typedef enum
{
    AS11_CORE_FRAMEWORK_TYPE,
    DPP_FRAMEWORK_TYPE,
} FrameworkType;


typedef struct
{
    const char *name;
    mxfKey item_key;
} PropertyInfo;

typedef struct
{
    const char *name;
    mxfKey set_key;
    const PropertyInfo *property_info;
} FrameworkInfo;

typedef struct
{
    FrameworkType type;
    std::string name;
    std::string value;
} FrameworkProperty;



class FrameworkHelper
{
public:
    FrameworkHelper(mxfpp::DataModel *data_model, mxfpp::DMFramework *framework);
    ~FrameworkHelper();

    bool SetProperty(std::string name, std::string value);

    mxfpp::DMFramework* GetFramework() const { return mFramework; }

private:
    mxfpp::DMFramework *mFramework;
    mxfpp::SetDef *mSetDef;
    const FrameworkInfo *mFrameworkInfo;
};



bool parse_framework_type(const char *fwork_str, FrameworkType *type);
bool parse_framework_file(const char *filename, FrameworkType type, std::vector<FrameworkProperty> *properties);
bool parse_segmentation_file(const char *filename, Rational frame_rate, std::vector<AS11TCSegment> *segments,
                             bool *filler_complete_segments);


};


#endif

