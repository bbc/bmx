/*
 * Copyright (C) 2010, British Broadcasting Corporation
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

#ifndef BMX_AS02_MANIFEST_H_
#define BMX_AS02_MANIFEST_H_

#include <string>
#include <vector>
#include <map>

#include <bmx/BMXTypes.h>
#include <bmx/XMLWriter.h>



namespace bmx
{


typedef enum
{
    FILE_FILE_ROLE,
    FOLDER_FILE_ROLE,
    VERSION_FILE_ROLE,
    ESSENCE_COMPONENT_FILE_ROLE,
    SHIM_FILE_ROLE,
    MANIFEST_FILE_ROLE,
    QC_FILE_ROLE,
    GRAPHIC_FILE_ROLE,
    CAPTION_FILE_ROLE,
} FileRole;

typedef enum
{
    NONE_MIC_TYPE = 0,
    CRC16_MIC_TYPE,
    CRC32_MIC_TYPE,
    MD5_MIC_TYPE,
    HMAC_SHA1_MIC_TYPE,
} MICType;

typedef enum
{
    ESSENCE_ONLY_MIC_SCOPE,
    ENTIRE_FILE_MIC_SCOPE,
} MICScope;



class AS02Manifest;
class AS02Bundle;

class AS02ManifestFile
{
public:
    friend class AS02Manifest;

public:
    AS02ManifestFile();
    AS02ManifestFile(size_t index, std::string path, FileRole role);
    ~AS02ManifestFile();

    void SetId(std::string id);
    void SetId(UUID uuid);
    void SetId(UMID umid);
    void SetSize(uint64_t size);
    void SetMIC(MICType type, MICScope scope, std::string mic);
    void SetMICType(MICType type);
    void SetMICScope(MICScope scope);
    void SetMIC(std::string mic);
    void AppendAnnotation(std::string annotation);

    std::string GetId() const { return mId; }
    FileRole GetRole() const { return mRole; }
    uint64_t GetSize() const { return mSize; }
    std::string GetPath() const { return mPath; }
    std::string GetMIC() const { return mMIC; }
    MICType GetMICType() const { return mMICType; }
    MICScope GetMICScope() const { return mMICScope; }
    std::vector<std::string> GetAnnotations() const { return mAnnotations; }

public:
    bool operator<(const AS02ManifestFile &right) const;

public:
    void CompleteInfo(AS02Bundle *bundle, MICType default_mic_type, MICScope default_mic_scope);

    void Write(XMLWriter *xml_writer);

private:
    size_t mIndex;
    std::string mId;
    FileRole mRole;
    uint64_t mSize;
    std::string mPath;
    std::string mMIC;
    MICType mMICType;
    bool mMICTypeSet;
    MICScope mMICScope;
    bool mMICScopeSet;
    std::vector<std::string> mAnnotations;
};



class AS02Manifest
{
public:
    AS02Manifest();
    ~AS02Manifest();

    void SetDefaultMICType(MICType type);
    void SetDefaultMICScope(MICScope scope);

    void SetBundleName(std::string name);
    void SetBundleId(std::string uuid_str);
    void SetBundleId(UUID uuid);
    void SetCreator(std::string creator);
    void AppendAnnotation(std::string annotation);
    AS02ManifestFile* RegisterFile(std::string path, FileRole role);

    std::string GetBundleName() const { return mBundleName; }
    std::string GetBundleId() const { return mBundleId; }
    std::string GetCreator() const { return mCreator; }
    Timestamp GetCreationDate() const { return mCreationDate; }
    std::vector<std::string> GetAnnotations() const { return mAnnotations; }
    AS02ManifestFile* GetFile(std::string path);

public:
    void Write(AS02Bundle *bundle, std::string filename);

private:
    std::string mBundleName;
    std::string mBundleId;
    std::string mCreator;
    Timestamp mCreationDate;
    std::vector<std::string> mAnnotations;
    MICType mDefaultMICType;
    MICScope mDefaultMICScope;
    std::map<std::string, AS02ManifestFile> mFiles;
};


};



#endif

