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

#include "bmx_scm_version.h"

#include <cstdio>

#include <bmx/Version.h>

using namespace std;


namespace bmx
{
bool BMX_REGRESSION_TEST = false;
};


static const char* BMX_COMPANY_NAME = "BBC";
static const bmx::UUID BMX_PRODUCT_UID =
    {0xb8, 0x60, 0x4d, 0x31, 0x2e, 0x15, 0x47, 0x99, 0xa3, 0xc6, 0x04, 0x7e, 0xd0, 0xe6, 0xf9, 0xa1};
static const mxfProductVersion BMX_MXF_PRODUCT_VERSION = {BMX_VERSION_MAJOR,
                                                          BMX_VERSION_MINOR,
                                                          BMX_VERSION_MICRO,
                                                          0,
                                                          BMX_MXF_VERSION_RELEASE};

static const bmx::UUID REGTEST_PRODUCT_UID =
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const mxfProductVersion REGTEST_MXF_PRODUCT_VERSION = {1, 0, 0, 0, 0};



string bmx::get_bmx_library_name()
{
    if (BMX_REGRESSION_TEST)
        return "RegLib";
    else
        return BMX_LIBRARY_NAME;
}

string bmx::get_bmx_version_string()
{
    if (BMX_REGRESSION_TEST) {
        return "0.0.0";
    } else {
        char buffer[32];
        sprintf(buffer, "%d.%d.%d", BMX_VERSION_MAJOR, BMX_VERSION_MINOR, BMX_VERSION_MICRO);
        return buffer;
    }
}

string bmx::get_bmx_scm_version_string()
{
    if (BMX_REGRESSION_TEST)
        return "regtest-head";
    else
        return BMX_SCM_VERSION;
}

string bmx::get_bmx_build_string()
{
    if (BMX_REGRESSION_TEST)
        return "1970-01-01 00:00:00";
    else
        return __DATE__ " " __TIME__;
}

string bmx::get_bmx_company_name()
{
    if (BMX_REGRESSION_TEST)
        return "RegCo";
    else
        return BMX_COMPANY_NAME;
}

bmx::UUID bmx::get_bmx_product_uid()
{
    if (BMX_REGRESSION_TEST)
        return REGTEST_PRODUCT_UID;
    else
        return BMX_PRODUCT_UID;
}

mxfProductVersion bmx::get_bmx_mxf_product_version()
{
    if (BMX_REGRESSION_TEST)
        return REGTEST_MXF_PRODUCT_VERSION;
    else
        return BMX_MXF_PRODUCT_VERSION;
}

string bmx::get_bmx_mxf_version_string()
{
    if (BMX_REGRESSION_TEST) {
        return "0.0.0";
    } else {
        char buffer[64];
        sprintf(buffer, "%d.%d.%d (scm %s)",
                BMX_VERSION_MAJOR, BMX_VERSION_MINOR, BMX_VERSION_MICRO,
                BMX_SCM_VERSION);
        return buffer;
    }
}

