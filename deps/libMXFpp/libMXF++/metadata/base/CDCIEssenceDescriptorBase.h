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

#ifndef MXFPP_CDCIESSENCEDESCRIPTOR_BASE_H_
#define MXFPP_CDCIESSENCEDESCRIPTOR_BASE_H_



#include <libMXF++/metadata/GenericPictureEssenceDescriptor.h>


namespace mxfpp
{


class CDCIEssenceDescriptorBase : public GenericPictureEssenceDescriptor
{
public:
    friend class MetadataSetFactory<CDCIEssenceDescriptorBase>;
    static const mxfKey setKey;

public:
    CDCIEssenceDescriptorBase(HeaderMetadata *headerMetadata);
    virtual ~CDCIEssenceDescriptorBase();


   // getters

   bool haveComponentDepth() const;
   uint32_t getComponentDepth() const;
   bool haveHorizontalSubsampling() const;
   uint32_t getHorizontalSubsampling() const;
   bool haveVerticalSubsampling() const;
   uint32_t getVerticalSubsampling() const;
   bool haveColorSiting() const;
   uint8_t getColorSiting() const;
   bool haveReversedByteOrder() const;
   bool getReversedByteOrder() const;
   bool havePaddingBits() const;
   int16_t getPaddingBits() const;
   bool haveAlphaSampleDepth() const;
   uint32_t getAlphaSampleDepth() const;
   bool haveBlackRefLevel() const;
   uint32_t getBlackRefLevel() const;
   bool haveWhiteReflevel() const;
   uint32_t getWhiteReflevel() const;
   bool haveColorRange() const;
   uint32_t getColorRange() const;


   // setters

   void setComponentDepth(uint32_t value);
   void setHorizontalSubsampling(uint32_t value);
   void setVerticalSubsampling(uint32_t value);
   void setColorSiting(uint8_t value);
   void setReversedByteOrder(bool value);
   void setPaddingBits(int16_t value);
   void setAlphaSampleDepth(uint32_t value);
   void setBlackRefLevel(uint32_t value);
   void setWhiteReflevel(uint32_t value);
   void setColorRange(uint32_t value);


protected:
    CDCIEssenceDescriptorBase(HeaderMetadata *headerMetadata, ::MXFMetadataSet *cMetadataSet);
};


};


#endif
