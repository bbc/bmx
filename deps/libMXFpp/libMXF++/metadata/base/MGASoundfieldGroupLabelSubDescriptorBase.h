/*
 * Copyright (C) 2023, British Broadcasting Corporation
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

#ifndef MXFPP_MGASOUNDFIELDGROUPLABELSUBDESCRIPTOR_BASE_H_
#define MXFPP_MGASOUNDFIELDGROUPLABELSUBDESCRIPTOR_BASE_H_



#include <libMXF++/metadata/SoundfieldGroupLabelSubDescriptor.h>


namespace mxfpp
{


class MGASoundfieldGroupLabelSubDescriptorBase : public SoundfieldGroupLabelSubDescriptor
{
public:
    friend class MetadataSetFactory<MGASoundfieldGroupLabelSubDescriptorBase>;
    static const mxfKey setKey;

public:
    MGASoundfieldGroupLabelSubDescriptorBase(HeaderMetadata *headerMetadata);
    virtual ~MGASoundfieldGroupLabelSubDescriptorBase();


   // getters

   mxfUUID getMGAMetadataSectionLinkID() const;
   bool haveADMAudioProgrammeID() const;
   std::string getADMAudioProgrammeID() const;
   bool haveADMAudioContentID() const;
   std::string getADMAudioContentID() const;
   bool haveADMAudioObjectID() const;
   std::string getADMAudioObjectID() const;


   // setters

   void setMGAMetadataSectionLinkID(mxfUUID value);
   void setADMAudioProgrammeID(const std::string &value);
   void setADMAudioContentID(const std::string &value);
   void setADMAudioObjectID(const std::string &value);


protected:
    MGASoundfieldGroupLabelSubDescriptorBase(HeaderMetadata *headerMetadata, ::MXFMetadataSet *cMetadataSet);
};


};


#endif
