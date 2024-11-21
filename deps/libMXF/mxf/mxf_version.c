/*
 * libMXF version information
 *
 * Copyright (C) 2006, British Broadcasting Corporation
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

#include <string.h>
#include <stdio.h>
#include <limits.h>

#include <mxf/mxf.h>
#include <mxf/mxf_utils.h>
#include <mxf/mxf_macros.h>

#include "git.h"
#include "fallback_git_version.h"


#define LPREF(s)    L ## s
#define WSTR(s)     LPREF(s)



static mxfProductVersion g_regtestVersion = {1,   /* major */
                                             0,   /* minor */
                                             0,   /* patch */
                                             0,   /* build */
                                             0};  /* release (unknown) */

#if defined (__linux__) && defined(__i386__)
static const char *g_libmxfPlatformString           =     LIBMXF_LIBRARY_NAME   " (Linux 32-bit)";
static const mxfUTF16Char *g_libmxfPlatformWString  = L"" LIBMXF_LIBRARY_WNAME L" (Linux 32-bit)";
#elif defined (__linux__) && defined(__x86_64__)
static const char *g_libmxfPlatformString           =     LIBMXF_LIBRARY_NAME   " (Linux 64-bit)";
static const mxfUTF16Char *g_libmxfPlatformWString  = L"" LIBMXF_LIBRARY_WNAME L" (Linux 64-bit)";
#elif defined(_WIN64) /* check first because _WIN32 also defined for Win64 */
static const char *g_libmxfPlatformString           =     LIBMXF_LIBRARY_NAME   " (Win64)";
static const mxfUTF16Char *g_libmxfPlatformWString  = L"" LIBMXF_LIBRARY_WNAME L" (Win64)";
#elif defined(_WIN32)
static const char *g_libmxfPlatformString           =     LIBMXF_LIBRARY_NAME   " (Win32)";
static const mxfUTF16Char *g_libmxfPlatformWString  = L"" LIBMXF_LIBRARY_WNAME L" (Win32)";
#elif defined(__APPLE__) && defined(__i386__)
static const char *g_libmxfPlatformString           =     LIBMXF_LIBRARY_NAME   " (Intel Darwin 32-bit)";
static const mxfUTF16Char *g_libmxfPlatformWString  = L"" LIBMXF_LIBRARY_WNAME L" (Intel Darwin 32-bit)";
#elif defined(__APPLE__) && defined(__x86_64__)
static const char *g_libmxfPlatformString           =     LIBMXF_LIBRARY_NAME   " (Intel Darwin 64-bit)";
static const mxfUTF16Char *g_libmxfPlatformWString  = L"" LIBMXF_LIBRARY_WNAME L" (Intel Darwin 64-bit)";
#elif defined(__APPLE__) && defined(__ppc__)
static const char *g_libmxfPlatformString           =     LIBMXF_LIBRARY_NAME   " (PowerPC Darwin 32-bit)";
static const mxfUTF16Char *g_libmxfPlatformWString  = L"" LIBMXF_LIBRARY_WNAME L" (PowerPC Darwin 32-bit)";
#elif defined(__APPLE__) && defined(__ppc64__)
static const char *g_libmxfPlatformString           =     LIBMXF_LIBRARY_NAME   " (PowerPC Darwin 64-bit)";
static const mxfUTF16Char *g_libmxfPlatformWString  = L"" LIBMXF_LIBRARY_WNAME L" (PowerPC Darwin 64-bit)";
#else
static const char *g_libmxfPlatformString           =     LIBMXF_LIBRARY_NAME   " (" LIBMXF_DEFAULT_PLATFORM_NAME ")";
static const mxfUTF16Char *g_libmxfPlatformWString  = L"" LIBMXF_LIBRARY_WNAME L" (" LIBMXF_DEFAULT_PLATFORM_WNAME L")";
#endif

static const char *g_regtestPlatformString              =  "libMXF (Linux)";
static const mxfUTF16Char *g_regtestPlatformWString     = L"libMXF (Linux)";
static const char *g_regtestSCMVersionString            =  "regtest-head";
static const mxfUTF16Char *g_regtestSCMVersionWString   = L"regtest-head";


mxf_get_version_func mxf_get_version                            = mxf_default_get_version;
mxf_get_platform_string_func mxf_get_platform_string            = mxf_default_get_platform_string;
mxf_get_platform_wstring_func mxf_get_platform_wstring          = mxf_default_get_platform_wstring;
mxf_get_scm_version_string_func mxf_get_scm_version_string      = mxf_default_get_scm_version_string;
mxf_get_scm_version_wstring_func mxf_get_scm_version_wstring    = mxf_default_get_scm_version_wstring;



const mxfProductVersion* mxf_default_get_version(void)
{
    static mxfProductVersion g_libmxfVersion = {0};

    if (g_libmxfVersion.major == 0 && g_libmxfVersion.minor == 0) {
        g_libmxfVersion.major = LIBMXF_VERSION_MAJOR;
        g_libmxfVersion.minor = LIBMXF_VERSION_MINOR;

        // Set the patch version value to the commit offset from the release tag.
        // The commit offset is part of the git describe tag string which has the
        // format "<tag>-<offset>-g<commit id>"
        const char *describe = libmxf_git_DescribeTag();
#ifdef PACKAGE_GIT_VERSION_STRING
        if (!describe[0] || strcmp(describe, "unknown") == 0)
            describe = PACKAGE_GIT_VERSION_STRING;
#endif
        if (describe[0] && strcmp(describe, "unknown") != 0) {
            int offset;
            int dash_count = 0;
            const char *offset_str = &describe[strlen(describe) - 1];

            // position offset_str after the second '-' in reverse order
            while (offset_str != describe) {
                if (*offset_str == '-') {
                    if (dash_count >= 1) {
                        offset_str++;
                        break;
                    }
                    dash_count++;
                }
                offset_str--;
            }
            if (offset_str == describe)
                offset_str = 0;

            if (offset_str && sscanf(offset_str, "%d", &offset) == 1 && offset >= 0 && offset <= UINT16_MAX) {
                g_libmxfVersion.patch = (uint16_t)offset;
                if (libmxf_git_AnyUncommittedChanges())
                    g_libmxfVersion.release = 0;  /* Unknown version */
                else if (offset == 0)
                    g_libmxfVersion.release = 1;  /* Released version */
                else
                    g_libmxfVersion.release = 2;  /* Post release, development version */
            }
        }
    }

    return &g_libmxfVersion;
}

const char* mxf_default_get_platform_string(void)
{
    return g_libmxfPlatformString;
}

const mxfUTF16Char* mxf_default_get_platform_wstring(void)
{
    return g_libmxfPlatformWString;
}

const char* mxf_default_get_scm_version_string(void)
{
    static char version_string[64] = {0};
    if (version_string[0] == 0) {
        const char *describe = libmxf_git_DescribeTag();
        if (!describe[0] || strcmp(describe, "unknown") == 0)
            describe = libmxf_git_Describe();

#ifdef PACKAGE_GIT_VERSION_STRING
        if (strcmp(describe, "unknown") == 0) {
            mxf_snprintf(version_string, ARRAY_SIZE(version_string), "%s", PACKAGE_GIT_VERSION_STRING);
        }
        else
#endif
        {
            if (libmxf_git_AnyUncommittedChanges())
                mxf_snprintf(version_string, ARRAY_SIZE(version_string), "%s-dirty", describe);
            else
                mxf_snprintf(version_string, ARRAY_SIZE(version_string), "%s", describe);
        }
    }

    return version_string;
}

const mxfUTF16Char* mxf_default_get_scm_version_wstring(void)
{
    static mxfUTF16Char version_wstring[64] = {0};
    if (version_wstring[0] == 0)
        mxf_utf8_to_utf16(version_wstring, mxf_default_get_scm_version_string(), ARRAY_SIZE(version_wstring));

    return version_wstring;
}


const mxfProductVersion* mxf_regtest_get_version(void)
{
    return &g_regtestVersion;
}

const char* mxf_regtest_get_platform_string(void)
{
    return g_regtestPlatformString;
}

const mxfUTF16Char* mxf_regtest_get_platform_wstring(void)
{
    return g_regtestPlatformWString;
}

const char* mxf_regtest_get_scm_version_string(void)
{
    return g_regtestSCMVersionString;
}

const mxfUTF16Char* mxf_regtest_get_scm_version_wstring(void)
{
    return g_regtestSCMVersionWString;
}

