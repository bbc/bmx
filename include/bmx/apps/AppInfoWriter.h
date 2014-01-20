/*
 * Copyright (C) 2013, British Broadcasting Corporation
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

#ifndef BMX_APP_INFO_WRITER_H_
#define BMX_APP_INFO_WRITER_H_

#include <string>
#include <vector>
#include <map>

#include <bmx/BMXTypes.h>



namespace bmx
{


typedef struct
{
    int value;
    const char *name;
} EnumInfo;


class AppInfoWriter
{
public:
    AppInfoWriter();
    virtual ~AppInfoWriter();

    virtual void SetClipEditRate(Rational rate);
    virtual void RegisterCCName(const std::string &name, const std::string &cc_name);

    virtual void Start(const std::string &name) = 0;
    virtual void End() = 0;

    virtual void StartSection(const std::string &name) = 0;
    virtual void EndSection() = 0;
    virtual void StartArrayItem(const std::string &name, size_t size) = 0;
    virtual void EndArrayItem() = 0;
    virtual void StartArrayElement(const std::string &name, size_t index) = 0;
    virtual void EndArrayElement() = 0;
    virtual void StartComplexItem(const std::string &name) = 0;
    virtual void EndComplexItem() = 0;

    virtual void StartAnnotations();
    virtual void EndAnnotations();

    virtual void WriteStringItem(const std::string &name, const std::string &value);
    virtual void WriteBoolItem(const std::string &name, bool value);
    virtual void WriteIntegerItem(const std::string &name, uint8_t value, bool hex = false);
    virtual void WriteIntegerItem(const std::string &name, uint16_t value, bool hex = false);
    virtual void WriteIntegerItem(const std::string &name, uint32_t value, bool hex = false);
    virtual void WriteIntegerItem(const std::string &name, uint64_t value, bool hex = false);
    virtual void WriteIntegerItem(const std::string &name, int8_t value);
    virtual void WriteIntegerItem(const std::string &name, int16_t value);
    virtual void WriteIntegerItem(const std::string &name, int32_t value);
    virtual void WriteIntegerItem(const std::string &name, int64_t value);
    virtual void WriteRationalItem(const std::string &name, Rational value);
    virtual void WriteTimecodeItem(const std::string &name, Timecode timecode);
    virtual void WriteAUIDItem(const std::string &name, UL value);
    virtual void WriteIDAUItem(const std::string &name, UUID value);
    virtual void WriteUMIDItem(const std::string &name, UMID value);
    virtual void WriteTimestampItem(const std::string &name, Timestamp value);
    virtual void WriteDateOnlyItem(const std::string &name, Timestamp value);
    virtual void WriteVersionTypeItem(const std::string &name, mxfVersionType value);
    virtual void WriteProductVersionItem(const std::string &name, mxfProductVersion value);
    virtual void WriteDurationItem(const std::string &name, int64_t duration, Rational rate);
    virtual void WritePositionItem(const std::string &name, int64_t position, Rational rate);
    virtual void WriteEnumItem(const std::string &name, const EnumInfo *enum_info, int enum_value);
    virtual void WriteEnumStringItem(const std::string &name, const EnumInfo *enum_info, int enum_value);

    virtual void WriteFormatItem(const std::string &name, const char *format, ...);

protected:
    virtual void WriteItem(const std::string &name, const std::string &value) = 0;

    void WriteULItem(const std::string &name, const UL *value);
    void WriteUUIDItem(const std::string &name, const UUID *value);

    std::string CamelCaseName(const std::string &name) const;

protected:
    Rational mClipEditRate;
    std::map<std::string, std::string> mCCNames;
    bool mIsAnnotation;
    std::vector<std::pair<std::string, std::string> > mAnnotations;
};


};


#endif

