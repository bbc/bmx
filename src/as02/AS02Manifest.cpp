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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cstring>
#include <cerrno>
#include <sys/stat.h>

#include <algorithm>

#include <bmx/as02/AS02Manifest.h>
#include <bmx/as02/AS02Bundle.h>
#include <bmx/Checksum.h>
#include <bmx/XMLUtils.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;


static const char AS02_V10_NAMESPACE[] = "http://www.amwa.tv/as-02/1.0/manifest";


typedef struct
{
    FileRole role;
    const char *name;
} FileRoleNameMap;

static const FileRoleNameMap FILE_ROLE_NAME_MAP[] =
{
    {FILE_FILE_ROLE,                "file"},
    {FOLDER_FILE_ROLE,              "folder"},
    {VERSION_FILE_ROLE,             "version"},
    {ESSENCE_COMPONENT_FILE_ROLE,   "essencecomponent"},
    {SHIM_FILE_ROLE,                "shim"},
    {MANIFEST_FILE_ROLE,            "manifest"},
    {QC_FILE_ROLE,                  "qc"},
    {GRAPHIC_FILE_ROLE,             "graphic"},
    {CAPTION_FILE_ROLE,             "caption"},
};


typedef struct
{
    MICType type;
    const char *name;
} MICTypeNameMap;

static const MICTypeNameMap MIC_TYPE_NAME_MAP[] =
{
    {CRC16_MIC_TYPE,        "crc16"},
    {CRC32_MIC_TYPE,        "crc32"},
    {MD5_MIC_TYPE,          "md5"},
    {HMAC_SHA1_MIC_TYPE,    "hmac-sha1"},
};


typedef struct
{
    MICScope scope;
    const char *name;
} MICScopeNameMap;

static const MICScopeNameMap MIC_SCOPE_NAME_MAP[] =
{
    {ESSENCE_ONLY_MIC_SCOPE,   "essence_only"},
    {ENTIRE_FILE_MIC_SCOPE,    "entire_file"},
};



static string get_xml_file_role_name(FileRole role)
{
    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(FILE_ROLE_NAME_MAP); i++) {
        if (FILE_ROLE_NAME_MAP[i].role == role)
            return FILE_ROLE_NAME_MAP[i].name;
    }

    BMX_ASSERT(false);
    return "";
}

static string get_xml_mic_type_name(MICType type)
{
    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(MIC_TYPE_NAME_MAP); i++) {
        if (MIC_TYPE_NAME_MAP[i].type == type)
            return MIC_TYPE_NAME_MAP[i].name;
    }

    BMX_ASSERT(false);
    return "";
}

static string get_xml_mic_scope_name(MICScope scope)
{
    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(MIC_SCOPE_NAME_MAP); i++) {
        if (MIC_SCOPE_NAME_MAP[i].scope == scope)
            return MIC_SCOPE_NAME_MAP[i].name;
    }

    BMX_ASSERT(false);
    return "";
}



AS02ManifestFile::AS02ManifestFile()
{
    mIndex = 0;
    mRole = FILE_FILE_ROLE;
    mSize = 0;
    mId = get_xml_uuid_str(generate_uuid());
    mMICType = MD5_MIC_TYPE;
    mMICScope = ESSENCE_ONLY_MIC_SCOPE;
}

AS02ManifestFile::AS02ManifestFile(size_t index, string path, FileRole role)
{
    mIndex = index;
    mPath = path;
    mRole = role;
    mSize = 0;
    mId = get_xml_uuid_str(generate_uuid());
    mMICType = MD5_MIC_TYPE;
    mMICTypeSet = false;
    mMICScope = ESSENCE_ONLY_MIC_SCOPE;
    mMICScopeSet = false;
}

AS02ManifestFile::~AS02ManifestFile()
{
}

void AS02ManifestFile::SetId(string id)
{
    static const unsigned char uuid_umid_prefix[16] =
        {0x06, 0x0a, 0x2b, 0x34, 0x01, 0x01, 0x01, 0x05, 0x01, 0x01, 0x0f, 0x20, 0x13, 0x00, 0x00, 0x00};

    BMX_CHECK(id.find("urn:uuid:") == 0 ||
              id.find("urn:smpte:ul:") == 0 ||
              id.find("urn:smpte:umid:") == 0);

    UMID umid;
    if (id.find("urn:smpte:umid:") == 0 &&
        parse_xml_umid_str(id, &umid) &&
        memcmp(&umid.octet0, uuid_umid_prefix, 16) == 0)
    {
        // use the last 16 bytes as a UUID
        UUID uuid;
        memcpy(&uuid.octet0, &umid.octet16, 16);
        mId = get_xml_uuid_str(uuid);
    } else {
        mId = id;
    }
}

void AS02ManifestFile::SetId(UUID uuid)
{
    SetId(get_xml_uuid_str(uuid));
}

void AS02ManifestFile::SetId(UMID umid)
{
    SetId(get_xml_umid_str(umid));
}

void AS02ManifestFile::SetSize(uint64_t size)
{
    mSize = size;
}

void AS02ManifestFile::SetMIC(MICType type, MICScope scope, string mic)
{
    mMIC = mic;
    mMICType = type;
    mMICTypeSet = true;
    mMICScope = scope;
    mMICScopeSet = true;
}

void AS02ManifestFile::SetMICType(MICType type)
{
    mMICType = type;
    mMICTypeSet = true;
}

void AS02ManifestFile::SetMICScope(MICScope scope)
{
    mMICScope = scope;
    mMICScopeSet = true;
}

void AS02ManifestFile::SetMIC(string mic)
{
    mMIC = mic;
    mMICTypeSet = true;
    mMICScopeSet = true;
}

void AS02ManifestFile::AppendAnnotation(string annotation)
{
    mAnnotations.push_back(annotation);
}

bool AS02ManifestFile::operator<(const AS02ManifestFile &right) const
{
    return mIndex < right.mIndex;
}

void AS02ManifestFile::CompleteInfo(AS02Bundle *bundle, MICType default_mic_type, MICScope default_mic_scope)
{
    if (mRole != MANIFEST_FILE_ROLE) {
        string complete_path = bundle->CompleteFilepath(mPath);

        // get file sizes
        if (mRole != FOLDER_FILE_ROLE) {
#if defined(_WIN32)
            struct _stati64 stat_buf;
            if (_stati64(complete_path.c_str(), &stat_buf) == 0)
#else
            struct stat stat_buf;
            if (stat(complete_path.c_str(), &stat_buf) == 0)
#endif
                mSize = stat_buf.st_size;
            else
                log_warn("Failed to stat '%s' for size: %s\n", complete_path.c_str(), bmx_strerror(errno).c_str());
        }

        // calculate checksum for entire files if not done so already

        MICType mic_type   = (mMICTypeSet  ? mMICType  : default_mic_type);
        MICScope mic_scope = (mMICScopeSet ? mMICScope : default_mic_scope);

        if (mMIC.empty() && mic_scope == ENTIRE_FILE_MIC_SCOPE && mRole != FOLDER_FILE_ROLE) {
            if (mic_type == MD5_MIC_TYPE) {
                SetMIC(mic_type, mic_scope, Checksum::CalcFileChecksum(complete_path, MD5_CHECKSUM));
                if (mMIC.empty())
                    log_warn("Failed to calc MD5 for '%s'\n", complete_path.c_str());
            }
        }
    }
}

void AS02ManifestFile::Write(XMLWriter *xml_writer)
{
    xml_writer->WriteElement(AS02_V10_NAMESPACE, "FileID", mId);
    xml_writer->WriteElement(AS02_V10_NAMESPACE, "Role",   get_xml_file_role_name(mRole));
    xml_writer->WriteElement(AS02_V10_NAMESPACE, "Size",   get_xml_uint64_str(mSize));
    xml_writer->WriteElement(AS02_V10_NAMESPACE, "Path",   mPath);

    if (!mMIC.empty()) {
        xml_writer->WriteElementStart(AS02_V10_NAMESPACE, "MIC");
        xml_writer->WriteAttribute(AS02_V10_NAMESPACE, "type",  get_xml_mic_type_name(mMICType));
        xml_writer->WriteAttribute(AS02_V10_NAMESPACE, "scope", get_xml_mic_scope_name(mMICScope));
        xml_writer->WriteElementContent(mMIC);
        xml_writer->WriteElementEnd();
    }

    size_t i;
    for (i = 0; i < mAnnotations.size(); i++)
        xml_writer->WriteElement(AS02_V10_NAMESPACE, "AnnotationText", mAnnotations[i]);
}



AS02Manifest::AS02Manifest()
{
    mDefaultMICType = MD5_MIC_TYPE;
    mDefaultMICScope = ESSENCE_ONLY_MIC_SCOPE;
    memset(&mCreationDate, 0, sizeof(mCreationDate));
}

AS02Manifest::~AS02Manifest()
{
}

void AS02Manifest::SetDefaultMICType(MICType type)
{
    mDefaultMICType = type;
}

void AS02Manifest::SetDefaultMICScope(MICScope scope)
{
    mDefaultMICScope = scope;
}

void AS02Manifest::SetBundleName(string name)
{
    mBundleName = name;
}

void AS02Manifest::SetBundleId(string uuid_str)
{
    BMX_CHECK(uuid_str.find("urn:uuid:") == 0);

    mBundleId = uuid_str;
}

void AS02Manifest::SetBundleId(UUID uuid)
{
    SetBundleId(get_xml_uuid_str(uuid));
}

void AS02Manifest::SetCreator(string creator)
{
    mCreator = creator;
}

void AS02Manifest::AppendAnnotation(string annotation)
{
    mAnnotations.push_back(annotation);
}

AS02ManifestFile* AS02Manifest::RegisterFile(string path, FileRole role)
{
    if (mFiles.find(path) != mFiles.end())
        return &mFiles[path];


    AS02ManifestFile manifest_file(mFiles.size(), path, role);
    mFiles[path] = manifest_file;

    // recursively register folders in path
    string folder = strip_name(path);
    if (!folder.empty())
        RegisterFile(folder, FOLDER_FILE_ROLE);

    return &mFiles[path];
}

void AS02Manifest::Write(AS02Bundle *bundle, string filename)
{
    XMLWriter *xml_writer = XMLWriter::Open(filename);
    BMX_CHECK(xml_writer);

    try
    {
        vector<AS02ManifestFile> ordered_files;
        map<std::string, AS02ManifestFile>::const_iterator iter;
        for (iter = mFiles.begin(); iter != mFiles.end(); iter++)
            ordered_files.push_back(iter->second);
        sort(ordered_files.begin(), ordered_files.end());

        size_t i;
        for (i = 0; i < ordered_files.size(); i++)
            ordered_files[i].CompleteInfo(bundle, mDefaultMICType, mDefaultMICScope);

        mCreationDate = generate_timestamp_now();


        xml_writer->WriteDocumentStart();
        xml_writer->WriteElementStart(AS02_V10_NAMESPACE, "Manifest");
        xml_writer->DeclareNamespace(AS02_V10_NAMESPACE, "");

        xml_writer->WriteElement(AS02_V10_NAMESPACE, "BundleName",   mBundleName);
        xml_writer->WriteElement(AS02_V10_NAMESPACE, "BundleID",     mBundleId);
        xml_writer->WriteElement(AS02_V10_NAMESPACE, "Creator",      mCreator);
        xml_writer->WriteElement(AS02_V10_NAMESPACE, "CreationDate", get_xml_timestamp_str(mCreationDate));

        for (i = 0; i < mAnnotations.size(); i++)
            xml_writer->WriteElement(AS02_V10_NAMESPACE, "AnnotationText", mAnnotations[i]);

        xml_writer->WriteElementStart(AS02_V10_NAMESPACE, "FileList");
        for (i = 0; i < ordered_files.size(); i++) {
            xml_writer->WriteElementStart(AS02_V10_NAMESPACE, "File");
            ordered_files[i].Write(xml_writer);
            xml_writer->WriteElementEnd();
        }
        xml_writer->WriteElementEnd();

        xml_writer->WriteElementEnd();
        xml_writer->WriteDocumentEnd();

        delete xml_writer;
    }
    catch (...)
    {
        delete xml_writer;
        throw;
    }
}

