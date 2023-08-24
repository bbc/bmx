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

#ifndef MXFPP_RGBAESSENCEDESCRIPTOR_BASE_H_
#define MXFPP_RGBAESSENCEDESCRIPTOR_BASE_H_



#include <libMXF++/metadata/GenericPictureEssenceDescriptor.h>


namespace mxfpp
{


class RGBAEssenceDescriptorBase : public GenericPictureEssenceDescriptor
{
public:
    friend class MetadataSetFactory<RGBAEssenceDescriptorBase>;
    static const mxfKey setKey;

public:
    RGBAEssenceDescriptorBase(HeaderMetadata *headerMetadata);
    virtual ~RGBAEssenceDescriptorBase();


   // getters

   bool haveComponentMaxRef() const;
   uint32_t getComponentMaxRef() const;
   bool haveComponentMinRef() const;
   uint32_t getComponentMinRef() const;
   bool haveAlphaMaxRef() const;
   uint32_t getAlphaMaxRef() const;
   bool haveAlphaMinRef() const;
   uint32_t getAlphaMinRef() const;
   bool haveScanningDirection() const;
   uint8_t getScanningDirection() const;
   bool havePixelLayout() const;
   mxfRGBALayout getPixelLayout() const;
   bool havePalette() const;
   ByteArray getPalette() const;
   bool havePaletteLayout() const;
   mxfRGBALayout getPaletteLayout() const;


   // setters

   void setComponentMaxRef(uint32_t value);
   void setComponentMinRef(uint32_t value);
   void setAlphaMaxRef(uint32_t value);
   void setAlphaMinRef(uint32_t value);
   void setScanningDirection(uint8_t value);
   void setPixelLayout(mxfRGBALayout value);
   void setPalette(ByteArray value);
   void setPaletteLayout(mxfRGBALayout value);


protected:
    RGBAEssenceDescriptorBase(HeaderMetadata *headerMetadata, ::MXFMetadataSet *cMetadataSet);
};


};


#endif
