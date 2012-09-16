/*
 * Copyright (C) 2010, British Broadcasting Corporation
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

#define __STDC_FORMAT_MACROS

#include <cstdio>
#include <cstring>

#include <bmx/XMLUtils.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;



string bmx::get_xml_bool_str(bool value)
{
    if (value)
        return "true";
    else
        return "false";
}

string bmx::get_xml_uint64_str(uint64_t value)
{
    char buf[32];
    bmx_snprintf(buf, sizeof(buf), "%"PRIu64"", value);

    return buf;
}

string bmx::get_xml_timestamp_str(Timestamp timestamp)
{
    char buf[64];
    bmx_snprintf(buf, sizeof(buf), "%04d-%02u-%02uT%02u:%02u:%02uZ",
                 timestamp.year, timestamp.month, timestamp.day,
                 timestamp.hour, timestamp.min, timestamp.sec);

    return buf;
}

string bmx::get_xml_uuid_str(UUID value)
{
    char buf[64];
    bmx_snprintf(buf, sizeof(buf), "urn:uuid:%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                 value.octet0,  value.octet1,  value.octet2,  value.octet3,
                 value.octet4,  value.octet5,
                 value.octet6,  value.octet7,
                 value.octet8,  value.octet9,
                 value.octet10, value.octet11, value.octet12, value.octet13, value.octet14, value.octet15);

    return buf;
}

string bmx::get_xml_umid_str(UMID value)
{
    static const char hex_chars[] = "0123456789abcdef";
    char buf[128];
    size_t offset;

    bmx_snprintf(buf, sizeof(buf), "urn:smpte:umid:");
    offset = strlen(buf);

    size_t i, j;
    for (i = 0; i < 32; i += 4) {
        if (i != 0)
            buf[offset++] = '.';
        for (j = 0; j < 4; j++) {
            buf[offset++] = hex_chars[((&value.octet0)[i + j] >> 4) & 0x0f];
            buf[offset++] = hex_chars[ (&value.octet0)[i + j]       & 0x0f];
        }
    }
    buf[offset] = '\0';

    return buf;
}

bool bmx::parse_xml_umid_str(string umid_str, UMID *umid)
{
    unsigned int bytes[32];
    int result = sscanf(umid_str.c_str(),
                        "urn:smpte:umid:"
                        "%02x%02x%02x%02x.%02x%02x%02x%02x."
                        "%02x%02x%02x%02x.%02x%02x%02x%02x."
                        "%02x%02x%02x%02x.%02x%02x%02x%02x."
                        "%02x%02x%02x%02x.%02x%02x%02x%02x",
                        &bytes[0],  &bytes[1],  &bytes[2],  &bytes[3],  &bytes[4],  &bytes[5],  &bytes[6],  &bytes[7],
                        &bytes[8],  &bytes[9],  &bytes[10], &bytes[11], &bytes[12], &bytes[13], &bytes[14], &bytes[15],
                        &bytes[16], &bytes[17], &bytes[18], &bytes[19], &bytes[20], &bytes[21], &bytes[22], &bytes[23],
                        &bytes[24], &bytes[25], &bytes[26], &bytes[27], &bytes[28], &bytes[29], &bytes[30], &bytes[31]);
    if (result != 32)
        return false;

    int i;
    for (i = 0; i < 32; i++)
        (&umid->octet0)[i] = (unsigned char)bytes[i];

    return true;
}

