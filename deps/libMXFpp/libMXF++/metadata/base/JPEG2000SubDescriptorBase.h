/*
 * Copyright (C) 2020, British Broadcasting Corporation
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

#ifndef MXFPP_JPEG2000SUBDESCRIPTOR_BASE_H_
#define MXFPP_JPEG2000SUBDESCRIPTOR_BASE_H_



#include <libMXF++/metadata/SubDescriptor.h>


namespace mxfpp
{


class JPEG2000SubDescriptorBase : public SubDescriptor
{
public:
    friend class MetadataSetFactory<JPEG2000SubDescriptorBase>;
    static const mxfKey setKey;

public:
    JPEG2000SubDescriptorBase(HeaderMetadata *headerMetadata);
    virtual ~JPEG2000SubDescriptorBase();


   // getters

   uint16_t getRsiz() const;
   uint32_t getXsiz() const;
   uint32_t getYsiz() const;
   uint32_t getXOsiz() const;
   uint32_t getYOsiz() const;
   uint32_t getXTsiz() const;
   uint32_t getYTsiz() const;
   uint32_t getXTOsiz() const;
   uint32_t getYTOsiz() const;
   uint16_t getCsiz() const;
   std::vector<mxfJ2KComponentSizing> getPictureComponentSizing() const;
   bool haveCodingStyleDefault() const;
   ByteArray getCodingStyleDefault() const;
   bool haveQuantizationDefault() const;
   ByteArray getQuantizationDefault() const;
   bool haveJ2CLayout() const;
   mxfRGBALayout getJ2CLayout() const;
   bool haveJ2KExtendedCapabilities() const;
   mxfJ2KExtendedCapabilities getJ2KExtendedCapabilities() const;
   bool haveJ2KProfile() const;
   std::vector<uint16_t> getJ2KProfile() const;
   bool haveJ2KCorrespondingProfile() const;
   std::vector<uint16_t> getJ2KCorrespondingProfile() const;


   // setters

   void setRsiz(uint16_t value);
   void setXsiz(uint32_t value);
   void setYsiz(uint32_t value);
   void setXOsiz(uint32_t value);
   void setYOsiz(uint32_t value);
   void setXTsiz(uint32_t value);
   void setYTsiz(uint32_t value);
   void setXTOsiz(uint32_t value);
   void setYTOsiz(uint32_t value);
   void setCsiz(uint16_t value);
   void setPictureComponentSizing(const std::vector<mxfJ2KComponentSizing> &value);
   void appendPictureComponentSizing(mxfJ2KComponentSizing value);
   void setCodingStyleDefault(ByteArray value);
   void setQuantizationDefault(ByteArray value);
   void setJ2CLayout(mxfRGBALayout value);
   void setJ2KExtendedCapabilities(mxfJ2KExtendedCapabilities value);
   void setJ2KProfile(const std::vector<uint16_t> &value);
   void appendJ2KProfile(uint16_t value);
   void setJ2KCorrespondingProfile(const std::vector<uint16_t> &value);
   void appendJ2KCorrespondingProfile(uint16_t value);


protected:
    JPEG2000SubDescriptorBase(HeaderMetadata *headerMetadata, ::MXFMetadataSet *cMetadataSet);
};


};


#endif
