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

#ifndef FW_HELPER_H_
#define FW_HELPER_H_

#include <string>

#include <libMXF++/MXF.h>

#include <bmx/BMXTypes.h>


namespace bmx
{


typedef enum
{
    AS11_CORE_FRAMEWORK_TYPE,
    DPP_FRAMEWORK_TYPE,
    AS10_CORE_FRAMEWORK_TYPE
} FrameworkType;


typedef struct
{
    const char *name;
    mxfKey item_key;
    size_t max_unicode_len;
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
    FrameworkHelper(mxfpp::DMFramework *framework, const FrameworkInfo *info,
                    Timecode start_tc, Rational frame_rate);
    ~FrameworkHelper();

    void NormaliseStrings();

    bool SetProperty(const std::string &name, const std::string &value);

    mxfpp::DMFramework* GetFramework() const { return mFramework; }

private:
    mxfpp::DMFramework *mFramework;
    Timecode mStartTimecode;
    Rational mFrameRate;
    mxfpp::SetDef *mSetDef;
    const FrameworkInfo *mFrameworkInfo;
};


};



#endif
