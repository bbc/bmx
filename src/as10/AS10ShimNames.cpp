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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <bmx/as10/AS10ShimNames.h>
#include <bmx/Utils.h>


using namespace std;
using namespace bmx;


typedef struct
{
    AS10Shim shim;
    const char *name;
} ShimNamesMap;


static const ShimNamesMap SHIM_NAMES_MAP[] =
{
    {AS10_HIGH_HD_2014,         "HIGH_HD_2014"},
    {AS10_CNN_HD_2012,          "CNN_HD_2012"},
    {AS10_NRK_HD_2012,          "NRK_HD_2012"},
    {AS10_JVC_HD_35_VBR_2012,   "JVC_HD_35_VBR_2012"},
    {AS10_JVC_HD_25_CBR_2012,   "JVC_HD_25_CBR_2012"},
};



const char* bmx::get_as10_shim_name(AS10Shim shim)
{
    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(SHIM_NAMES_MAP); i++) {
        if (shim == SHIM_NAMES_MAP[i].shim)
            return SHIM_NAMES_MAP[i].name;
    }

    return "";
}

AS10Shim bmx::get_as10_shim(const string &name)
{
    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(SHIM_NAMES_MAP); i++) {
        if (name == SHIM_NAMES_MAP[i].name)
            return SHIM_NAMES_MAP[i].shim;
    }

    return AS10_UNKNOWN_SHIM;
}

string bmx::get_as10_shim_names()
{
    string names;
    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(SHIM_NAMES_MAP); i++) {
        if (i > 0)
            names.append(", ");
        names.append(SHIM_NAMES_MAP[i].name);
    }

    return names;
}

