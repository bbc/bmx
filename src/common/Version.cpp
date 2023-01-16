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
#include <cstring>
#include <ctime>
#include <limits.h>

#include "git.h"
#include "fallback_git_version.h"

#include <bmx/Version.h>
#include <bmx/Utils.h>

using namespace std;
using namespace bmx;


namespace bmx
{
bool BMX_REGRESSION_TEST = false;
};


static const char* BMX_COMPANY_NAME = "BBC";
static const bmx::UUID BMX_PRODUCT_UID =
    {0xb8, 0x60, 0x4d, 0x31, 0x2e, 0x15, 0x47, 0x99, 0xa3, 0xc6, 0x04, 0x7e, 0xd0, 0xe6, 0xf9, 0xa1};
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
        mxfProductVersion product_version = get_bmx_mxf_product_version();
        char buffer[32];
        bmx_snprintf(buffer, sizeof(buffer), "%d.%d.%d",
                     product_version.major, product_version.minor, product_version.patch);
        return buffer;
    }
}

string bmx::get_bmx_scm_version_string()
{
    if (BMX_REGRESSION_TEST) {
        return "regtest-head";
    } else {
        static string version_string;
        if (version_string.empty()) {
            version_string = git::DescribeTag();
            if (version_string.empty() || version_string == "unknown")
                version_string = git::Describe();

#ifdef PACKAGE_GIT_VERSION_STRING
            if (version_string.empty() || version_string == "unknown") {
                version_string = PACKAGE_GIT_VERSION_STRING;
            }
            else
#endif
            {
                if (git::AnyUncommittedChanges())
                    version_string += "-dirty";
            }
        }

        return version_string.c_str();
    }
}

string bmx::get_bmx_build_string()
{
    if (BMX_REGRESSION_TEST)
        return "1970-01-01 00:00:00";
    else
        return __DATE__ " " __TIME__;
}

Timestamp bmx::get_bmx_build_timestamp()
{
    static const char* month_names[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

    Timestamp timestamp;
    memset(&timestamp, 0, sizeof(timestamp));

    if (BMX_REGRESSION_TEST) {
        timestamp.year  = 1970;
        timestamp.month = 1;
        timestamp.day   = 1;
        timestamp.hour  = 0;
        timestamp.min   = 0;
        timestamp.sec   = 0;
        timestamp.qmsec = 0;
        return timestamp;
    }

    const char *date_str = __DATE__;
    const char *time_str = __TIME__;
    if (strlen(date_str) < 11 || strlen(time_str) < 8)
        return timestamp;

    struct tm build_ltm;
    memset(&build_ltm, 0, sizeof(build_ltm));

    // date: Mmm dd yyyy
    int i;
    for (i = 0; i < 12; i++) {
        if (strncmp(month_names[i], date_str, 3) == 0)
            break;
    }
    if (i >= 12)
        return timestamp;
    build_ltm.tm_mon = i;
    if (sscanf(&date_str[4], "%d %d", &build_ltm.tm_mday, &build_ltm.tm_year) != 2)
        return timestamp;
    build_ltm.tm_year -= 1900;

    // time: hh:mm:ss
    if (sscanf(time_str, "%d:%d:%d", &build_ltm.tm_hour, &build_ltm.tm_min, &build_ltm.tm_sec) != 3)
        return timestamp;

    time_t build_ltt = mktime(&build_ltm);

    struct tm build_gmt;
#if HAVE_GMTIME_R
    if (!gmtime_r(&build_ltt, &build_gmt))
        return timestamp;
#elif defined(_MSC_VER)
    if (gmtime_s(&build_gmt, &build_ltt) != 0)
        return timestamp;
#else
    const struct tm *gmt_ptr = gmtime(&build_ltt);
    if (!gmt_ptr)
        return timestamp;
    build_gmt = *gmt_ptr;
#endif

    timestamp.year  = build_gmt.tm_year + 1900;
    timestamp.month = build_gmt.tm_mon + 1;
    timestamp.day   = build_gmt.tm_mday;
    timestamp.hour  = build_gmt.tm_hour;
    timestamp.min   = build_gmt.tm_min;
    timestamp.sec   = build_gmt.tm_sec;
    timestamp.qmsec = 0;

    return timestamp;
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

    static mxfProductVersion product_version = {0, 0, 0, 0, 0};
    if (product_version.major == 0 && product_version.minor == 0) {
        product_version.major = BMX_VERSION_MAJOR;
        product_version.minor = BMX_VERSION_MINOR;

        // Set the patch version value to the commit offset from the release tag.
        // The commit offset is part of the git describe tag string which has the
        // format "<tag>-<offset>-g<commit id>"
        string describe = git::DescribeTag();
#ifdef PACKAGE_GIT_VERSION_STRING
        if (describe.empty() || describe == "unknown")
            describe = PACKAGE_GIT_VERSION_STRING;
#endif
        if (!describe.empty() && describe != "unknown") {
            size_t dash_pos = describe.rfind("-", describe.size() - 1);
            if (dash_pos != string::npos) {
                dash_pos = describe.rfind("-", dash_pos - 1);
                if (dash_pos != string::npos)
                    dash_pos++;
            }

            int offset;
            if (dash_pos != string::npos && sscanf(&describe[dash_pos], "%d", &offset) == 1 && offset >= 0 && offset <= UINT16_MAX) {
                product_version.patch = (uint16_t)offset;
                if (git::AnyUncommittedChanges())
                    product_version.release = 0;  /* Unknown version */
                else if (offset == 0)
                    product_version.release = 1;  /* Released version */
                else
                    product_version.release = 2;  /* Post release, development version */
            }
        }
    }

    return product_version;
}

string bmx::get_bmx_mxf_version_string()
{
    if (BMX_REGRESSION_TEST) {
        return "0.0.0";
    } else {
        static string version_string;
        if (version_string.empty()) {
            mxfProductVersion product_version = get_bmx_mxf_product_version();
            char buffer[64];
            bmx_snprintf(buffer, sizeof(buffer), "%d.%d.%d",
                         product_version.major, product_version.minor, product_version.patch);
            version_string = buffer;
            version_string += " (scm " + get_bmx_scm_version_string() + ")";
        }

        return version_string;
    }
}

