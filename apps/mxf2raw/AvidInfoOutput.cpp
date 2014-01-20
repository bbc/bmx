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
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;


static const EnumInfo COLOR_EINFO[] =
{
    {COLOR_WHITE,       "White"},
    {COLOR_RED,         "Red"},
    {COLOR_YELLOW,      "Yellow"},
    {COLOR_GREEN,       "Green"},
    {COLOR_CYAN,        "Cyan"},
    {COLOR_BLUE,        "Blue"},
    {COLOR_MAGENTA,     "Magenta"},
    {COLOR_BLACK,       "Black"},
    {0, 0}
};

static const EnumInfo PACKAGE_TYPE_EINFO[] =
{
    {TAPE_PACKAGE_TYPE,         "Tape"},
    {IMPORT_PACKAGE_TYPE,       "Import"},
    {RECORDING_PACKAGE_TYPE,    "Recording"},
    {0, 0}
};


static void print_avid_tagged_values(AppInfoWriter *info_writer, const vector<AvidTaggedValue*> &tags,
                                     const string &section_name)
{
    size_t i;
    for (i = 0; i < tags.size(); i++) {
        info_writer->StartArrayElement(section_name, i);
        info_writer->WriteStringItem("name", tags[i]->name);
        info_writer->WriteStringItem("value", tags[i]->value);
        if (!tags[i]->attributes.empty()) {
            info_writer->StartArrayItem("attributes", tags[i]->attributes.size());
            print_avid_tagged_values(info_writer, tags[i]->attributes, "attribute");
            info_writer->EndArrayItem();
        }
        info_writer->EndArrayElement();
    }
}



void bmx::avid_register_extensions(MXFFileReader *file_reader)
{
    AvidInfo::RegisterExtensions(file_reader->GetDataModel());
}

void bmx::avid_write_info(AppInfoWriter *info_writer, MXFFileReader *file_reader)
{
    AvidInfo info;
    info.ReadInfo(file_reader->GetHeaderMetadata());

    info_writer->StartSection("avid");

    if (!info.project_name.empty())
        info_writer->WriteStringItem("project_name", info.project_name);
    if (info.resolution_id)
        info_writer->WriteIntegerItem("resolution_id", info.resolution_id);

    if (info.have_phys_package) {
        info_writer->StartSection("physical_package");
        info_writer->WriteEnumStringItem("type", PACKAGE_TYPE_EINFO, info.phys_package_type);
        if (!info.phys_package_name.empty())
            info_writer->WriteStringItem("name", info.phys_package_name);
        info_writer->WriteUMIDItem("package_uid", info.phys_package_uid);
        if (info.phys_package_type == IMPORT_PACKAGE_TYPE && !info.phys_package_locator.empty())
            info_writer->WriteStringItem("network_locator", info.phys_package_locator);
        info_writer->EndSection();
    }

    Rational frame_rate = file_reader->GetEditRate();
    size_t i;

    if (!info.user_comments.empty()) {
        info_writer->StartArrayItem("user_comments", info.user_comments.size());
        print_avid_tagged_values(info_writer, info.user_comments, "user_comment");
        info_writer->EndArrayItem();
    }

    if (!info.material_package_attrs.empty()) {
        info_writer->StartArrayItem("attributes", info.material_package_attrs.size());
        print_avid_tagged_values(info_writer, info.material_package_attrs, "attribute");
        info_writer->EndArrayItem();
    }

    if (!info.locators.empty()) {
        info_writer->StartArrayItem("locators", info.locators.size());
        for (i = 0; i < info.locators.size(); i++) {
            info_writer->StartArrayElement("locator", i);
            info_writer->WritePositionItem("position", info.locators[i].position, frame_rate);
            info_writer->WriteEnumStringItem("color", COLOR_EINFO, info.locators[i].color);
            if (!info.locators[i].comment.empty())
                info_writer->WriteStringItem("comment", info.locators[i].comment);
            info_writer->EndArrayElement();
        }
        info_writer->EndArrayItem();
    }

    info_writer->EndSection();
}

