/*
 * Copyright (C) 2023, Fraunhofer IIS
 * All Rights Reserved.
 *
 * Author: Nisha Bhaskar
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

#include <mxf/mxf.h>
#include <mxf/mxf_labels_and_keys.h>

#include <bmx/mxf_helper/JPEGXSMXFDescriptorHelper.h>

#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;


EssenceType JPEGXSMXFDescriptorHelper::IsSupported(FileDescriptor *file_descriptor, mxfUL alternative_ec_label)
{
	mxfUL ec_label = file_descriptor->getEssenceContainer();
	if (mxf_is_jpegxs_ec(&ec_label) || mxf_is_jpegxs_ec(&alternative_ec_label)) {
		if (dynamic_cast<CDCIEssenceDescriptor*>(file_descriptor))
			return JPEGXS_CDCI;
		else if (dynamic_cast<RGBAEssenceDescriptor*>(file_descriptor))
			return JPEGXS_RGBA;
	}

	return UNKNOWN_ESSENCE_TYPE;
}

bool JPEGXSMXFDescriptorHelper::IsSupported(EssenceType essence_type)
{
	return essence_type == JPEGXS_CDCI || essence_type == JPEGXS_RGBA;
}

void JPEGXSMXFDescriptorHelper::SetJPEGXSComponentTable(std::vector<uint8_t> value)
{
	mJPEGXSSubDescriptor->setJPEGXSComponentTable(value);
	mComponentTable = value;
}

JPEGXSMXFDescriptorHelper::JPEGXSMXFDescriptorHelper()
	: PictureMXFDescriptorHelper()
{
	mEssenceType = JPEGXS_CDCI;
	mJPEGXSSubDescriptor = 0;
	mHorizontalSubSampling = 1;
	mVerticalSubSampling = 1;
	mProfile = Profile_Unrestricted;
	mStoredWidth = 0;
	mStoredHeight = 0;
	mFrameLayout = 0;
	m_parse = true;
}

JPEGXSMXFDescriptorHelper::~JPEGXSMXFDescriptorHelper()
{
}

void JPEGXSMXFDescriptorHelper::Initialize(FileDescriptor *file_descriptor, uint16_t mxf_version,
	mxfUL alternative_ec_label)
{
	BMX_ASSERT(IsSupported(file_descriptor, alternative_ec_label));

	if (dynamic_cast<CDCIEssenceDescriptor*>(file_descriptor))
		mEssenceType = JPEGXS_CDCI;
	else
		mEssenceType = JPEGXS_RGBA;

	mEssenceType = JPEGXS_RGBA;

	PictureMXFDescriptorHelper::Initialize(file_descriptor, mxf_version, alternative_ec_label);

	if (file_descriptor->haveSubDescriptors()) {
		vector<SubDescriptor*> sub_descriptors = file_descriptor->getSubDescriptors();
		for (size_t i = 0; i < sub_descriptors.size(); i++) {
			mJPEGXSSubDescriptor = dynamic_cast<JPEGXSSubDescriptor*>(sub_descriptors[i]);
			if (mJPEGXSSubDescriptor)
				break;
		}
	}
}

void JPEGXSMXFDescriptorHelper::SetEssenceType(EssenceType essence_type)
{
	BMX_ASSERT(!mFileDescriptor);

	BMX_CHECK(essence_type == JPEGXS_CDCI || essence_type == JPEGXS_RGBA);
	PictureMXFDescriptorHelper::SetEssenceType(essence_type);
}

FileDescriptor* JPEGXSMXFDescriptorHelper::CreateFileDescriptor(mxfpp::HeaderMetadata *header_metadata)
{
	if (mEssenceType == JPEGXS_CDCI)
		mFileDescriptor = new CDCIEssenceDescriptor(header_metadata);
	else
		mFileDescriptor = new RGBAEssenceDescriptor(header_metadata);
	mJPEGXSSubDescriptor = new JPEGXSSubDescriptor(header_metadata);
	mFileDescriptor->appendSubDescriptors(mJPEGXSSubDescriptor);

	GenericPictureEssenceDescriptor *pict_descriptor = dynamic_cast<GenericPictureEssenceDescriptor*>(mFileDescriptor);
	pict_descriptor->setFrameLayout(MXF_FULL_FRAME);

	PictureMXFDescriptorHelper::UpdateFileDescriptor();

	return mFileDescriptor;
}


void JPEGXSMXFDescriptorHelper::UpdateFileDescriptor()
{
	// Currently only support progressive video frame layout
	GenericPictureEssenceDescriptor *pict_descriptor = dynamic_cast<GenericPictureEssenceDescriptor*>(mFileDescriptor);

	pict_descriptor->setStoredWidth(mStoredWidth);
	pict_descriptor->setStoredHeight(mStoredHeight);

	mxfUL pc_label;
	switch (mProfile) {
	case Profile_Unrestricted:
		pc_label = MXF_CMDEF_L(JPEGXSUnrestrictedCodestream);
		break;
	case Profile_Light422:
		pc_label = MXF_CMDEF_L(JPEGXSLight422_10Profile);
		break;
	case Profile_Light444:
		pc_label = MXF_CMDEF_L(JPEGXSLight444_12Profile);
		break;
	case Profile_LightSubline:
		pc_label = MXF_CMDEF_L(JPEGXSLightSubline422_10Profile);
		break;
	case Profile_Main422:
		pc_label = MXF_CMDEF_L(JPEGXSMain422_10Profile);
		break;
	case Profile_Main444:
		pc_label = MXF_CMDEF_L(JPEGXSMain444_12Profile);
		break;
	case Profile_Main4444:
		pc_label = MXF_CMDEF_L(JPEGXSMain4444_12Profile);
		break;
	case Profile_High444:
		pc_label = MXF_CMDEF_L(JPEGXSHigh444_12Profile);
		break;
	case Profile_High4444:
		pc_label = MXF_CMDEF_L(JPEGXSHigh4444_12Profile);
		break;
	default:
		pc_label = MXF_CMDEF_L(JPEGXSUnrestrictedCodestream);
		break;
	}
	pict_descriptor->setPictureEssenceCoding(pc_label);

	pict_descriptor->setSampledWidth(pict_descriptor->getStoredWidth());
	pict_descriptor->setSampledHeight(pict_descriptor->getStoredHeight());
	pict_descriptor->setSampledXOffset(0);
	pict_descriptor->setSampledYOffset(0);

	//pict_descriptor->setSampleRate();

	CDCIEssenceDescriptor *cdci_descriptor = dynamic_cast<CDCIEssenceDescriptor*>(mFileDescriptor);
	if (cdci_descriptor) {
		cdci_descriptor->setPaddingBits(0);
		if (!mComponentTable.empty())
			cdci_descriptor->setComponentDepth(mComponentTable[4]);
		cdci_descriptor->setHorizontalSubsampling(mHorizontalSubSampling);
		cdci_descriptor->setVerticalSubsampling(mVerticalSubSampling);
	}

	RGBAEssenceDescriptor *rgba_descriptor = dynamic_cast<RGBAEssenceDescriptor*>(mFileDescriptor);
	if (rgba_descriptor) {
		// Include fill in the pixel layout
		if (mJPEGXSSubDescriptor)
		{
			mxfRGBALayout pixel_layout;
			for (int i = 0; i < 8; i++)
			{
				pixel_layout.components[i].code = 0;
				pixel_layout.components[i].depth = 0;
			}
				
			if (!mComponentTable.empty())
			{
				pixel_layout.components[0].code = 'R';
				pixel_layout.components[0].depth = mComponentTable[4];
				pixel_layout.components[1].code = 'G';
				pixel_layout.components[1].depth = mComponentTable[6];
				pixel_layout.components[2].code = 'B';
				pixel_layout.components[2].depth = mComponentTable[8];

				rgba_descriptor->setPixelLayout(pixel_layout);
			}

		}
	}

	PictureMXFDescriptorHelper::UpdateFileDescriptor();
	
}

void JPEGXSMXFDescriptorHelper::UpdateFileDescriptor(FileDescriptor *file_desc_in)
{
#define SET_PROPERTY(name)                                                \
    if (comp_desc_in->have##name() && !comp_desc->have##name())     \
        comp_desc->set##name(comp_desc_in->get##name());

	GenericPictureEssenceDescriptor *comp_desc = dynamic_cast<GenericPictureEssenceDescriptor*>(mFileDescriptor);
	GenericPictureEssenceDescriptor *comp_desc_in = dynamic_cast<GenericPictureEssenceDescriptor*>(file_desc_in);

	if (comp_desc_in->haveFrameLayout() && comp_desc_in->getFrameLayout() != MXF_FULL_FRAME)
		BMX_EXCEPTION(("Only progressive video frame layout is currently supported"));

	SET_PROPERTY(FrameLayout)
	SET_PROPERTY(SignalStandard)
	SET_PROPERTY(AspectRatio)
	SET_PROPERTY(ActiveFormatDescriptor)
	SET_PROPERTY(VideoLineMap)
	if ((mFlavour & MXFDESC_IMF_FLAVOUR)) {
		BMX_ASSERT(comp_desc->getFrameLayout() == MXF_FULL_FRAME);
		comp_desc->setDisplayF2Offset(0);
	}
	else {
		SET_PROPERTY(FieldDominance)
		SET_PROPERTY(DisplayF2Offset)
	}
	SET_PROPERTY(CaptureGamma)
	SET_PROPERTY(CodingEquations)
	SET_PROPERTY(ColorPrimaries)

	SET_PROPERTY(MasteringDisplayPrimaries)
	SET_PROPERTY(MasteringDisplayWhitePointChromaticity)
	SET_PROPERTY(MasteringDisplayMaximumLuminance)
	SET_PROPERTY(MasteringDisplayMinimumLuminance)

	SET_PROPERTY(ActiveWidth)
	SET_PROPERTY(ActiveHeight)
	SET_PROPERTY(ActiveXOffset)
	SET_PROPERTY(ActiveYOffset)

	SET_PROPERTY(AlternativeCenterCuts);

	if (dynamic_cast<CDCIEssenceDescriptor*>(mFileDescriptor)) {
		CDCIEssenceDescriptor *comp_desc = dynamic_cast<CDCIEssenceDescriptor*>(mFileDescriptor);
		CDCIEssenceDescriptor *comp_desc_in = dynamic_cast<CDCIEssenceDescriptor*>(file_desc_in);

		if (!comp_desc->haveColorSiting() && comp_desc_in->haveColorSiting())
			SetColorSitingMod(comp_desc_in->getColorSiting());
		SET_PROPERTY(BlackRefLevel)
		SET_PROPERTY(WhiteReflevel)
		SET_PROPERTY(ColorRange)
	}

	if (dynamic_cast<RGBAEssenceDescriptor*>(mFileDescriptor)) {
		RGBAEssenceDescriptor *comp_desc = dynamic_cast<RGBAEssenceDescriptor*>(mFileDescriptor);
		RGBAEssenceDescriptor *comp_desc_in = dynamic_cast<RGBAEssenceDescriptor*>(file_desc_in);

		SET_PROPERTY(ComponentMaxRef)
		SET_PROPERTY(ComponentMinRef)
		SET_PROPERTY(ScanningDirection)
	}
}

void JPEGXSMXFDescriptorHelper::UpdateFileDescriptor(JXSEssenceParser *essence_parser)
{
	
	mJPEGXSSubDescriptor->setJPEGXSPpih(essence_parser->GetJPEGXSPpih());
	mJPEGXSSubDescriptor->setJPEGXSPlev(essence_parser->GetJPEGXSPlev());
	mJPEGXSSubDescriptor->setJPEGXSWf(essence_parser->GetJPEGXSWf());
	mJPEGXSSubDescriptor->setJPEGXSHf(essence_parser->GetJPEGXSHf());
	mJPEGXSSubDescriptor->setJPEGXSNc(essence_parser->GetJPEGXSNc());
	mJPEGXSSubDescriptor->setJPEGXSHsl(essence_parser->GetJPEGXSHsl());

	int length = essence_parser->GetJPEGXSComponentTable().GetSize();
	std::vector<uint8_t> tmp(length);
	ByteArray tmpArr = essence_parser->GetJPEGXSComponentTable();
	unsigned char* data = (tmpArr).GetBytes();
	for (int i = 0; i < length; i++)
	{
		tmp[i] = (*data);
		data++;
	}
	
	// for loop each element in the data pointer to the array
	mJPEGXSSubDescriptor->setJPEGXSComponentTable(tmp);
	mJPEGXSSubDescriptor->setJPEGXSCw(essence_parser->GetJPEGXSCw());
	mJPEGXSSubDescriptor->setJPEGXSMaximumBitRate(essence_parser->GetJPEGXSMaximumBitRate());

	GenericPictureEssenceDescriptor *pict_descriptor = dynamic_cast<GenericPictureEssenceDescriptor*>(mFileDescriptor);
	pict_descriptor->setFrameLayout(MXF_FULL_FRAME);

	// These are also set during parsing
	pict_descriptor->setStoredWidth(essence_parser->GetStoredWidth());
	pict_descriptor->setStoredHeight(essence_parser->GetStoredHeight());
	pict_descriptor->setSampledXOffset(0);
	pict_descriptor->setSampledYOffset(0);
	

	CDCIEssenceDescriptor *cdci_descriptor = dynamic_cast<CDCIEssenceDescriptor*>(mFileDescriptor);
	if (cdci_descriptor) {
		cdci_descriptor->setPaddingBits(0);
		cdci_descriptor->setComponentDepth((essence_parser->GetJPEGXSComponentTable()).GetBytes()[4]);
	}

	RGBAEssenceDescriptor *rgba_descriptor = dynamic_cast<RGBAEssenceDescriptor*>(mFileDescriptor);
	if (rgba_descriptor) {
		// Include fill in the pixel layout
		mxfRGBALayout pixel_layout;

		for (int i = 0; i < 8; i++)
		{
			pixel_layout.components[i].code = 0;
			pixel_layout.components[i].depth = 0;
		}

		pixel_layout.components[0].code = 'R';
		pixel_layout.components[0].depth = essence_parser->GetJPEGXSComponentTable().GetBytes()[4];
		pixel_layout.components[1].code = 'G';
		pixel_layout.components[1].depth = essence_parser->GetJPEGXSComponentTable().GetBytes()[6];
		pixel_layout.components[2].code = 'B';
		pixel_layout.components[2].depth = essence_parser->GetJPEGXSComponentTable().GetBytes()[8];
		pixel_layout.components[3].code = 0;
		pixel_layout.components[3].depth = 0;
		rgba_descriptor->setPixelLayout(pixel_layout);
	}
	PictureMXFDescriptorHelper::UpdateFileDescriptor();
}

mxfUL JPEGXSMXFDescriptorHelper::ChooseEssenceContainerUL() const
{
	return MXF_EC_L(MXFGCFrameWrappedProgressiveJPEGXSPictures);
}

