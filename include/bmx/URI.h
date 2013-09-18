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

#ifndef BMX_URI_H_
#define BMX_URI_H_


#include <string>


#if defined(_WIN32)
#define DIR_SEPARATOR_C     '\\'
#define DIR_SEPARATOR_S     "\\"
#else
#define DIR_SEPARATOR_C     '/'
#define DIR_SEPARATOR_S     "/"
#endif

#ifndef BMXURIInternal
#define BMXURIInternal      void
#endif



namespace bmx
{


class URIStr
{
public:
    URIStr();
    ~URIStr();

    void Copy(const char *uri_str);
    void Copy(const URIStr &uri_str);

    void Allocate(size_t size);

    void Clear();

    char* GetStr() { return mStr; }
    const char* GetCStr() const { return mStr; }

    bool IsEmpty() const { return (!mStr || !mStr[0]); }

private:
    char *mStr;
};



class URI
{
public:
    URI();
    URI(const char *uri_str);
    URI(std::string uri_str);
    URI(const URI &uri);
    ~URI();

    void SetWindowsNameConvert(bool windows_name_convert);

    bool Parse(const char *uri_str);
    bool Parse(std::string uri_str);
    bool ParseFilename(std::string filename);
    bool ParseDirectory(std::string dirname);

    void Copy(const URI &uri);

    bool MakeAbsolute(const URI &base_uri);
    bool MakeRelative(const URI &base_uri);

    bool IsRelative() const;
    bool IsAbsolute() const;
    bool IsAbsFile() const;

    std::string ToString() const;
    std::string ToFilename() const;

    size_t GetNumSegments() const;
    std::string GetSegment(size_t index) const;
    std::string GetLastSegment() const;
    std::string GetFirstSegment() const;

    bool operator==(const URI &right) const;
    URI& operator=(const URI &right);

    bool IsEmpty() const { return mSourceStr.IsEmpty(); }

private:
    void Clear();

private:
    BMXURIInternal *mUriUri;
    URIStr mSourceStr;
    bool mWindowsNameConvert;
};



};

#endif
