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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cstdio>

#include <im/Version.h>

using namespace std;


bool IM_REGRESSION_TEST = false;


static const char* IM_COMPANY_NAME = "BBC";
static const im::UUID IM_PRODUCT_UID =
    {0xb8, 0x60, 0x4d, 0x31, 0x2e, 0x15, 0x47, 0x99, 0xa3, 0xc6, 0x04, 0x7e, 0xd0, 0xe6, 0xf9, 0xa1};
static const mxfProductVersion IM_MXF_PRODUCT_VERSION = {IM_VERSION_MAJOR,
                                                         IM_VERSION_MINOR,
                                                         IM_VERSION_MICRO,
                                                         0,
                                                         IM_MXF_VERSION_RELEASE};

static const im::UUID REGTEST_PRODUCT_UID =
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const mxfProductVersion REGTEST_MXF_PRODUCT_VERSION = {1, 0, 0, 0, 0};



string im::get_im_library_name()
{
    if (IM_REGRESSION_TEST)
        return "RegLib";
    else
        return IM_LIBRARY_NAME;
}

string im::get_im_version_string()
{
    if (IM_REGRESSION_TEST) {
        return "0.0.0";
    } else {
        char buffer[32];
        sprintf(buffer, "%d.%d.%d", IM_VERSION_MAJOR, IM_VERSION_MINOR, IM_VERSION_MICRO);
        return buffer;
    }
}

string im::get_im_build_string()
{
    if (IM_REGRESSION_TEST)
        return "1970-01-01 00:00:00";
    else
        return __DATE__ " " __TIME__;
}

string im::get_im_company_name()
{
    if (IM_REGRESSION_TEST)
        return "RegCo";
    else
        return IM_COMPANY_NAME;
}

im::UUID im::get_im_product_uid()
{
    if (IM_REGRESSION_TEST)
        return REGTEST_PRODUCT_UID;
    else
        return IM_PRODUCT_UID;
}

mxfProductVersion im::get_im_mxf_product_version()
{
    if (IM_REGRESSION_TEST)
        return REGTEST_MXF_PRODUCT_VERSION;
    else
        return IM_MXF_PRODUCT_VERSION;
}

