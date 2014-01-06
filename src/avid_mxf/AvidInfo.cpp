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

#include <cstdlib>

#include <libMXF++/MXF.h>

#include <mxf/mxf_avid.h>

#include <bmx/avid_mxf/AvidInfo.h>
#include "AvidRGBColors.h"
#include <bmx/MXFUtils.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;



static Color convert_rgb_color(const RGBColor *color)
{
    float diff[BMX_ARRAY_SIZE(AVID_RGB_COLORS)];
    float min_diff = -1;
    size_t min_diff_index = 0;
    size_t i;

    // choose the color that has minimum difference to a known color

    for (i = 0; i < BMX_ARRAY_SIZE(AVID_RGB_COLORS); i++) {
        diff[i] = ((float)(color->red   - AVID_RGB_COLORS[i].red)   * (float)(color->red   - AVID_RGB_COLORS[i].red) +
                   (float)(color->green - AVID_RGB_COLORS[i].green) * (float)(color->green - AVID_RGB_COLORS[i].green) +
                   (float)(color->blue  - AVID_RGB_COLORS[i].blue)  * (float)(color->blue  - AVID_RGB_COLORS[i].blue));
    }

    for (i = 0; i < BMX_ARRAY_SIZE(AVID_RGB_COLORS); i++) {
        if (min_diff < 0 || diff[i] < min_diff) {
            min_diff = diff[i];
            min_diff_index = i;
        }
    }

    return (Color)(min_diff_index + COLOR_WHITE);
}



void AvidInfo::RegisterExtensions(DataModel *data_model)
{
    BMX_CHECK(mxf_avid_load_extensions(data_model->getCDataModel()));
    data_model->finalise();
}

AvidInfo::AvidInfo()
{
    Reset();
}

AvidInfo::~AvidInfo()
{
    Reset();
}

void AvidInfo::ReadInfo(HeaderMetadata *header_metadata)
{
    Reset();

    MaterialPackage *mp = header_metadata->getPreface()->findMaterialPackage();
    if (!mp) {
        log_warn("No material package found\n");
        return;
    }

    GetMaterialPackageAttrs(mp);
    GetUserComments(mp);
    GetLocators(mp);

    GetPhysicalPackageInfo(header_metadata);

    Preface *preface = header_metadata->getPreface();
    if (preface->haveItem(&MXF_ITEM_K(Preface, ProjectName))) {
        project_name = preface->getStringItem(&MXF_ITEM_K(Preface, ProjectName));
    } else {
        size_t i;
        for (i = 0; i < material_package_attrs.size(); i++) {
            if (material_package_attrs[i]->name == "_PJ") {
                project_name = material_package_attrs[i]->value;
                break;
            }
        }
    }

    vector<SourcePackage*> file_source_packages = header_metadata->getPreface()->findFileSourcePackages();
    if (file_source_packages.size() == 1) {
        FileDescriptor *file_desc = dynamic_cast<FileDescriptor*>(file_source_packages[0]->getDescriptorLight());
        if (file_desc && file_desc->haveItem(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ResolutionID)))
            resolution_id = file_desc->getInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ResolutionID));
    }
}

void AvidInfo::Reset()
{
    project_name.clear();
    ClearAvidTaggedValues(material_package_attrs);
    ClearAvidTaggedValues(user_comments);
    locators.clear();
    locators_edit_rate = ZERO_RATIONAL;
    have_phys_package = false;
    phys_package_type = UNKNOWN_PACKAGE_TYPE;
    phys_package_name.clear();
    phys_package_locator.clear();
    phys_package_uid = g_Null_UMID;
    resolution_id = 0;
}

void AvidInfo::GetMaterialPackageAttrs(MaterialPackage *mp)
{
    GetAvidTaggedValues(mp, &MXF_ITEM_K(GenericPackage, MobAttributeList), &material_package_attrs);
}

void AvidInfo::GetUserComments(MaterialPackage *mp)
{
    GetAvidTaggedValues(mp, &MXF_ITEM_K(GenericPackage, UserComments), &user_comments);
}

void AvidInfo::GetLocators(MaterialPackage *mp)
{
    // expect to find (DM) Event Track - (DM) Sequence - DMSegment
    vector<GenericTrack*> tracks = mp->getTracks();
    size_t i;
    for (i = 0; i < tracks.size(); i++) {
        EventTrack *et = dynamic_cast<EventTrack*>(tracks[i]);
        if (!et)
            continue;

        StructuralComponent *sc = et->getSequence();
        if (!sc || sc->getDataDefinition() != MXF_DDEF_L(DescriptiveMetadata))
            continue;

        Sequence *seq = dynamic_cast<Sequence*>(sc);
        if (!seq)
            continue;

        vector<StructuralComponent*> dm_segs = seq->getStructuralComponents();
        if (dm_segs.empty())
            continue;

        size_t j;
        for (j = 0; j < dm_segs.size(); j++) {
            DMSegment *seg = dynamic_cast<DMSegment*>(dm_segs[j]);
            if (!seg)
                break;

            AvidLocator locator;
            locator.position = seg->getEventStartPosition();
            if (seg->haveEventComment())
                locator.comment = seg->getEventComment();
            if (seg->haveItem(&MXF_ITEM_K(DMSegment, CommentMarkerColor))) {
                RGBColor rgb_color;
                BMX_CHECK(mxf_avid_get_rgb_color_item(seg->getCMetadataSet(),
                                                      &MXF_ITEM_K(DMSegment, CommentMarkerColor),
                                                      &rgb_color));
                locator.color = convert_rgb_color(&rgb_color);
            } else {
                locator.color = COLOR_BLACK;
            }
            locators.push_back(locator);
        }
        if (j < dm_segs.size())
            locators.clear();

        locators_edit_rate = et->getEventEditRate();
        break;
    }
}

void AvidInfo::GetPhysicalPackageInfo(HeaderMetadata *header_metadata)
{
    vector<GenericPackage*> packages = header_metadata->getPreface()->getContentStorage()->getPackages();
    size_t i;
    for (i = 0; i < packages.size(); i++) {
        SourcePackage *sp = dynamic_cast<SourcePackage*>(packages[i]);
        if (!sp || !sp->haveDescriptor())
            continue;

        GenericDescriptor *descriptor = sp->getDescriptorLight();
        if (!descriptor || !mxf_set_is_subclass_of(descriptor->getCMetadataSet(), &MXF_SET_K(PhysicalDescriptor)))
            continue;
        have_phys_package = true;

        if (mxf_set_is_subclass_of(descriptor->getCMetadataSet(), &MXF_SET_K(TapeDescriptor)))
            phys_package_type = TAPE_PACKAGE_TYPE;
        else if (mxf_set_is_subclass_of(descriptor->getCMetadataSet(), &MXF_SET_K(ImportDescriptor)))
            phys_package_type = IMPORT_PACKAGE_TYPE;
        else if (mxf_set_is_subclass_of(descriptor->getCMetadataSet(), &MXF_SET_K(RecordingDescriptor)))
            phys_package_type = RECORDING_PACKAGE_TYPE;
        else
            phys_package_type = UNKNOWN_PACKAGE_TYPE;

        phys_package_uid = sp->getPackageUID();
        if (sp->haveName())
            phys_package_name = sp->getName();

        if (descriptor->haveLocators()) {
            vector<Locator*> locators = descriptor->getLocators();
            size_t j;
            for (j = 0; j < locators.size(); j++) {
                NetworkLocator *network_locator = dynamic_cast<NetworkLocator*>(locators[j]);
                if (network_locator) {
                    phys_package_locator = network_locator->getURLString();
                    break;
                }
            }
        }
    }
}

void AvidInfo::GetAvidTaggedValues(MetadataSet *parent_set, const mxfKey *item_key,
                                   vector<AvidTaggedValue*> *tags)
{
    if (!parent_set->haveItem(item_key))
        return;

    ObjectIterator *iter = parent_set->getStrongRefArrayItem(item_key);
    if (!iter)
        return;

    while (iter->next()) {
        AvidTaggedValue *tag = GetAvidTaggedValue(iter->get());
        if (tag)
            tags->push_back(tag);
    }
    delete iter;
}

AvidTaggedValue* AvidInfo::GetAvidTaggedValue(MetadataSet *tag_set)
{
    mxfUTF16Char *name, *value;
    if (!mxf_avid_read_string_tagged_value(tag_set->getCMetadataSet(), &name, &value))
        return 0;

    AvidTaggedValue *tag = new AvidTaggedValue;
    tag->name  = convert_utf16_string(name);
    tag->value = convert_utf16_string(value);
    free(name);
    free(value);

    if (tag_set->haveItem(&MXF_ITEM_K(TaggedValue, TaggedValueAttributeList)))
        GetAvidTaggedValues(tag_set, &MXF_ITEM_K(TaggedValue, TaggedValueAttributeList), &tag->attributes);

    return tag;
}

void AvidInfo::ClearAvidTaggedValues(vector<AvidTaggedValue*> &tags)
{
    size_t i;
    for (i = 0; i < tags.size(); i++) {
        ClearAvidTaggedValues(tags[i]->attributes);
        delete tags[i];
    }
    tags.clear();
}

