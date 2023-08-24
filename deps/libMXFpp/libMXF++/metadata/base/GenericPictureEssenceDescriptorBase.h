/*
 * Copyright (C) 2008, British Broadcasting Corporation
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

#ifndef MXFPP_GENERICPICTUREESSENCEDESCRIPTOR_BASE_H_
#define MXFPP_GENERICPICTUREESSENCEDESCRIPTOR_BASE_H_



#include <libMXF++/metadata/FileDescriptor.h>


namespace mxfpp
{


class GenericPictureEssenceDescriptorBase : public FileDescriptor
{
public:
    friend class MetadataSetFactory<GenericPictureEssenceDescriptorBase>;
    static const mxfKey setKey;

public:
    GenericPictureEssenceDescriptorBase(HeaderMetadata *headerMetadata);
    virtual ~GenericPictureEssenceDescriptorBase();


   // getters

   bool haveSignalStandard() const;
   uint8_t getSignalStandard() const;
   bool haveFrameLayout() const;
   uint8_t getFrameLayout() const;
   bool haveStoredWidth() const;
   uint32_t getStoredWidth() const;
   bool haveStoredHeight() const;
   uint32_t getStoredHeight() const;
   bool haveStoredF2Offset() const;
   int32_t getStoredF2Offset() const;
   bool haveSampledWidth() const;
   uint32_t getSampledWidth() const;
   bool haveSampledHeight() const;
   uint32_t getSampledHeight() const;
   bool haveSampledXOffset() const;
   int32_t getSampledXOffset() const;
   bool haveSampledYOffset() const;
   int32_t getSampledYOffset() const;
   bool haveDisplayHeight() const;
   uint32_t getDisplayHeight() const;
   bool haveDisplayWidth() const;
   uint32_t getDisplayWidth() const;
   bool haveDisplayXOffset() const;
   int32_t getDisplayXOffset() const;
   bool haveDisplayYOffset() const;
   int32_t getDisplayYOffset() const;
   bool haveDisplayF2Offset() const;
   int32_t getDisplayF2Offset() const;
   bool haveAspectRatio() const;
   mxfRational getAspectRatio() const;
   bool haveActiveFormatDescriptor() const;
   uint8_t getActiveFormatDescriptor() const;
   bool haveVideoLineMap() const;
   std::vector<int32_t> getVideoLineMap() const;
   mxfVideoLineMap getVideoLineMapStruct() const;
   bool haveAlphaTransparency() const;
   uint8_t getAlphaTransparency() const;
   bool haveCaptureGamma() const;
   mxfUL getCaptureGamma() const;
   bool haveImageAlignmentOffset() const;
   uint32_t getImageAlignmentOffset() const;
   bool haveImageStartOffset() const;
   uint32_t getImageStartOffset() const;
   bool haveImageEndOffset() const;
   uint32_t getImageEndOffset() const;
   bool haveFieldDominance() const;
   uint8_t getFieldDominance() const;
   bool havePictureEssenceCoding() const;
   mxfUL getPictureEssenceCoding() const;
   bool haveCodingEquations() const;
   mxfUL getCodingEquations() const;
   bool haveColorPrimaries() const;
   mxfUL getColorPrimaries() const;
   bool haveMasteringDisplayPrimaries() const;
   mxfThreeColorPrimaries getMasteringDisplayPrimaries() const;
   bool haveMasteringDisplayWhitePointChromaticity() const;
   mxfColorPrimary getMasteringDisplayWhitePointChromaticity() const;
   bool haveMasteringDisplayMaximumLuminance() const;
   uint32_t getMasteringDisplayMaximumLuminance() const;
   bool haveMasteringDisplayMinimumLuminance() const;
   uint32_t getMasteringDisplayMinimumLuminance() const;
   bool haveActiveHeight() const;
   uint32_t getActiveHeight() const;
   bool haveActiveWidth() const;
   uint32_t getActiveWidth() const;
   bool haveActiveXOffset() const;
   uint32_t getActiveXOffset() const;
   bool haveActiveYOffset() const;
   uint32_t getActiveYOffset() const;
   bool haveAlternativeCenterCuts() const;
   std::vector<mxfUL> getAlternativeCenterCuts() const;


   // setters

   void setSignalStandard(uint8_t value);
   void setFrameLayout(uint8_t value);
   void setStoredWidth(uint32_t value);
   void setStoredHeight(uint32_t value);
   void setStoredF2Offset(int32_t value);
   void setSampledWidth(uint32_t value);
   void setSampledHeight(uint32_t value);
   void setSampledXOffset(int32_t value);
   void setSampledYOffset(int32_t value);
   void setDisplayHeight(uint32_t value);
   void setDisplayWidth(uint32_t value);
   void setDisplayXOffset(int32_t value);
   void setDisplayYOffset(int32_t value);
   void setDisplayF2Offset(int32_t value);
   void setAspectRatio(mxfRational value);
   void setActiveFormatDescriptor(uint8_t value);
   void setVideoLineMap(const std::vector<int32_t> &value);
   void setVideoLineMap(int32_t first, int32_t second);
   void setVideoLineMap(mxfVideoLineMap value);
   void appendVideoLineMap(int32_t value);
   void setAlphaTransparency(uint8_t value);
   void setCaptureGamma(mxfUL value);
   void setImageAlignmentOffset(uint32_t value);
   void setImageStartOffset(uint32_t value);
   void setImageEndOffset(uint32_t value);
   void setFieldDominance(uint8_t value);
   void setPictureEssenceCoding(mxfUL value);
   void setCodingEquations(mxfUL value);
   void setColorPrimaries(mxfUL value);
   void setMasteringDisplayPrimaries(mxfThreeColorPrimaries value);
   void setMasteringDisplayWhitePointChromaticity(mxfColorPrimary value);
   void setMasteringDisplayMaximumLuminance(uint32_t value);
   void setMasteringDisplayMinimumLuminance(uint32_t value);
   void setActiveHeight(uint32_t value);
   void setActiveWidth(uint32_t value);
   void setActiveXOffset(uint32_t value);
   void setActiveYOffset(uint32_t value);
   void setAlternativeCenterCuts(const std::vector<mxfUL> &value);
   void appendAlternativeCenterCuts(mxfUL value);


protected:
    GenericPictureEssenceDescriptorBase(HeaderMetadata *headerMetadata, ::MXFMetadataSet *cMetadataSet);
};


};


#endif
