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

#ifndef BMX_JPEGXS_MXF_DESCRIPTOR_HELPER_H_
#define BMX_JPEGXS_MXF_DESCRIPTOR_HELPER_H_


#include <bmx/mxf_helper/PictureMXFDescriptorHelper.h>
#include <bmx/essence_parser/JXSEssenceParser.h>


namespace bmx
{
    enum Profile {
        Profile_Unrestricted = 0x01,
        Profile_Main422 = 0x02,
        Profile_Main444 = 0x03,
        Profile_Main4444 = 0x04,
        Profile_Light422 = 0x05,
        Profile_Light444 = 0x06,
        Profile_LightSubline = 0x07,
        Profile_High444 = 0x08,
        Profile_High4444 = 0x09,
    };


    class JPEGXSMXFDescriptorHelper : public PictureMXFDescriptorHelper
    {
    public:
        static EssenceType IsSupported(mxfpp::FileDescriptor *file_descriptor, mxfUL alternative_ec_label);
        static bool IsSupported(EssenceType essence_type);

    public:
        JPEGXSMXFDescriptorHelper();
        virtual ~JPEGXSMXFDescriptorHelper();

    public:
        // initialize from existing descriptor
        virtual void Initialize(mxfpp::FileDescriptor *file_descriptor, uint16_t mxf_version, mxfUL alternative_ec_label);

    public:
        // configure and create new descriptor
        virtual void SetEssenceType(EssenceType essence_type);

        virtual mxfpp::FileDescriptor* CreateFileDescriptor(mxfpp::HeaderMetadata *header_metadata);

        // The desc values can be set using either the essence parser or the without this/ directly
        virtual void UpdateFileDescriptor();
        void UpdateFileDescriptor(JXSEssenceParser *essence_parser);

        virtual void UpdateFileDescriptor(mxfpp::FileDescriptor *file_desc_in);

        mxfpp::JPEGXSSubDescriptor* GetJPEGXSSubDescriptor() const { return mJPEGXSSubDescriptor; }

        // Set to false if codestream does not have to be parsed, default is true
        void SetParseStatus(bool value) { m_parse = value; }
        bool GetParseStatus() { return m_parse; }
        
        // To be able to directly set the values in the generic-desciptor without an essence parser/ user input (external)
        void SetJXSProfile(uint32_t profile) { mProfile = profile; }
        void SetHorizontalSubsampling(uint32_t hor_value) { mHorizontalSubSampling = hor_value; }
        void SetVerticalSubsampling(uint32_t ver_value) { mVerticalSubSampling = ver_value; }
        void SetStoredWidth(uint32_t value) { mStoredWidth = value; }
        void SetStoredHeight(uint32_t value) { mStoredHeight = value; }

        // To be able to directly set the values in the sub-desciptor without an essence parser
         void SetJPEGXSPpih(uint16_t value) { mJPEGXSSubDescriptor->setJPEGXSPpih(value); }
         void SetJPEGXSPlev(uint16_t value) { mJPEGXSSubDescriptor->setJPEGXSPlev(value); }
         void SetJPEGXSWf(uint16_t value) { mJPEGXSSubDescriptor->setJPEGXSWf(value); }
         void SetJPEGXSHf(uint16_t value) { mJPEGXSSubDescriptor->setJPEGXSHf(value); }
         void SetJPEGXSNc(uint8_t value) { mJPEGXSSubDescriptor->setJPEGXSNc(value); }
         void SetJPEGXSComponentTable(std::vector<uint8_t> value);
         void SetJPEGXSCw(uint16_t value) { mJPEGXSSubDescriptor->setJPEGXSCw(value); }
         void SetJPEGXSHsl(uint16_t value) { mJPEGXSSubDescriptor->setJPEGXSHsl(value); }
         void SetJPEGXSMaximumBitRate(uint32_t value) { mJPEGXSSubDescriptor->setJPEGXSMaximumBitRate(value); }

    protected:
        virtual mxfUL ChooseEssenceContainerUL() const;

    private:
        mxfpp::JPEGXSSubDescriptor *mJPEGXSSubDescriptor;

        bool m_parse;

        // Need to be set from an external application using the setters since these values have to be 
        // processed further before the pict desc setters are called
        mxfUL pc_label;
        uint32_t mProfile;
        
        std::vector<uint8_t> mComponentTable;
        uint32_t mHorizontalSubSampling;
        uint32_t mVerticalSubSampling;

        uint32_t mStoredWidth;
        uint32_t mStoredHeight;

    };


};

#endif
