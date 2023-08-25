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

#ifndef MXFPP_AES3AUDIODESCRIPTOR_BASE_H_
#define MXFPP_AES3AUDIODESCRIPTOR_BASE_H_



#include <libMXF++/metadata/WaveAudioDescriptor.h>


namespace mxfpp
{


class AES3AudioDescriptorBase : public WaveAudioDescriptor
{
public:
    friend class MetadataSetFactory<AES3AudioDescriptorBase>;
    static const mxfKey setKey;

public:
    AES3AudioDescriptorBase(HeaderMetadata *headerMetadata);
    virtual ~AES3AudioDescriptorBase();


    // getters

    bool haveEmphasis() const;
    uint8_t getEmphasis() const;
    bool haveBlockStartOffset() const;
    uint16_t getBlockStartOffset() const;
    bool haveAuxiliaryBitsMode() const;
    uint8_t getAuxiliaryBitsMode() const;
    bool haveChannelStatusMode() const;
    std::vector<uint8_t> getChannelStatusMode() const;
    bool haveFixedChannelStatusData() const;
    std::vector<mxfAES3FixedData> getFixedChannelStatusData() const;
    bool haveUserDataMode() const;
    std::vector<uint8_t> getUserDataMode() const;
    bool haveFixedUserData() const;
    std::vector<mxfAES3FixedData> getFixedUserData() const;
    bool haveLinkedTimecodeTrackID() const;
    uint32_t getLinkedTimecodeTrackID() const;
    bool haveSMPTE377MDataStreamNumber() const;
    uint8_t getSMPTE377MDataStreamNumber() const;


    // setters

    void setEmphasis(uint8_t value);
    void setBlockStartOffset(uint16_t value);
    void setAuxiliaryBitsMode(uint8_t value);
    void setChannelStatusMode(const std::vector<uint8_t> &value);
    void appendChannelStatusMode(uint8_t value);
    void setFixedChannelStatusData(const std::vector<mxfAES3FixedData> &value);
    void appendFixedChannelStatusData(const mxfAES3FixedData &value);
    void setUserDataMode(const std::vector<uint8_t> &value);
    void appendUserDataMode(uint8_t value);
    void setFixedUserData(const std::vector<mxfAES3FixedData> &value);
    void appendFixedUserData(const mxfAES3FixedData &value);
    void setLinkedTimecodeTrackID(uint32_t value);
    void setSMPTE377MDataStreamNumber(uint8_t value);


protected:
    AES3AudioDescriptorBase(HeaderMetadata *headerMetadata, ::MXFMetadataSet *cMetadataSet);

private:
    std::vector<mxfAES3FixedData> getAES3FixedDataArrayItem(const mxfKey *itemKey) const;
    void setAES3FixedArrayItem(const mxfKey *itemKey, const std::vector<mxfAES3FixedData> &value);
    void appendAES3FixedArrayItem(const mxfKey *itemKey, const mxfAES3FixedData &value);
};


};


#endif
