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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "AvidInfoOutput.h"
#include <bmx/avid_mxf/AvidInfo.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;


static const char *COLOR_STRINGS[] =
{
    "white",
    "red",
    "yellow",
    "green",
    "cyan",
    "blue",
    "magenta",
    "black"
};

static const char *PACKAGE_TYPE_STRINGS[] =
{
    "unknown",
    "tape",
    "import",
    "recording"
};


static char* get_umid_string(UMID umid, char *buf)
{
    sprintf(buf,
            "%02x%02x%02x%02x.%02x%02x%02x%02x.%02x%02x%02x%02x.%02x%02x%02x%02x."
            "%02x%02x%02x%02x.%02x%02x%02x%02x.%02x%02x%02x%02x.%02x%02x%02x%02x",
            umid.octet0,  umid.octet1,  umid.octet2,  umid.octet3,
            umid.octet4,  umid.octet5,  umid.octet6,  umid.octet7,
            umid.octet8,  umid.octet9,  umid.octet10, umid.octet11,
            umid.octet12, umid.octet13, umid.octet14, umid.octet15,
            umid.octet16, umid.octet17, umid.octet18, umid.octet19,
            umid.octet20, umid.octet21, umid.octet22, umid.octet23,
            umid.octet24, umid.octet25, umid.octet26, umid.octet27,
            umid.octet28, umid.octet29, umid.octet30, umid.octet31);

    return buf;
}

static void print_avid_tagged_values(vector<AvidTaggedValue*> &tags, int indent)
{
    size_t i;
    for (i = 0; i < tags.size(); i++) {
        printf("%*c%s: %s\n", indent, ' ', tags[i]->name.c_str(), tags[i]->value.c_str());
        if (!tags[i]->attributes.empty())
            print_avid_tagged_values(tags[i]->attributes, indent + 2);
    }
}



void bmx::avid_register_extensions(MXFFileReader *file_reader)
{
    AvidInfo::RegisterExtensions(file_reader->GetDataModel());
}

void bmx::avid_print_info(MXFFileReader *file_reader)
{
    AvidInfo info;
    info.ReadInfo(file_reader->GetHeaderMetadata());

    printf("Avid Information:\n");

    printf("  Project name      : %s\n", info.project_name.c_str());

    if (info.have_phys_package) {
        char string_buffer[128];
        printf("  Physical package  :\n");
        printf("      Type            : %s\n", PACKAGE_TYPE_STRINGS[info.phys_package_type]);
        printf("      Name            : %s\n", info.phys_package_name.c_str());
        printf("      Package uid     : %s\n", get_umid_string(info.phys_package_uid, string_buffer));
        printf("      Network locator : %s\n", info.phys_package_locator.c_str());
    }

    Rational frame_rate = file_reader->GetSampleRate();
    size_t i;

    if (!info.user_comments.empty()) {
        printf("  User comments     :\n");
        print_avid_tagged_values(info.user_comments, 6);
    }

    if (!info.material_package_attrs.empty()) {
        printf("  Attributes        :\n");
        print_avid_tagged_values(info.material_package_attrs, 6);
    }

    if (!info.locators.empty()) {
        printf("  Locators          :\n");
        printf("      %10s: %14s %10s    %s\n",
               "num", "pos", "color", "comment");
        for (i = 0; i < info.locators.size(); i++) {
            printf("      %10"PRIszt": %14s %10s    %s\n",
                   i, get_duration_string(info.locators[i].position, frame_rate).c_str(),
                   COLOR_STRINGS[info.locators[i].color], info.locators[i].comment.c_str());
        }
    }

    printf("\n");
}

