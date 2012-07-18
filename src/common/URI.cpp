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

#include <uriparser/Uri.h>

#define BMXURIInternal      UriUriA

#include <bmx/URI.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;



URIStr::URIStr()
{
    mStr = 0;
}

URIStr::~URIStr()
{
    delete [] mStr;
}

void URIStr::Copy(const char *uri_str)
{
    Clear();

    if (uri_str) {
        mStr = new char[strlen(uri_str) + 1];
        strcpy(mStr, uri_str);
    }
}

void URIStr::Copy(const URIStr &uri_str)
{
    Copy(uri_str.mStr);
}

void URIStr::Allocate(size_t size)
{
    Clear();

    if (size > 0) {
        mStr = new char[size];
        mStr[0] = '\0';
    }
}

void URIStr::Clear()
{
    delete [] mStr;
    mStr = 0;
}



URI::URI()
{
#if defined(_WIN32)
    mWindowsNameConvert = true;
#else
    mWindowsNameConvert = false;
#endif
    mUriUri = new UriUriA;
    memset(mUriUri, 0, sizeof(*mUriUri));
}

URI::URI(const char *uri_str)
{
#if defined(_WIN32)
    mWindowsNameConvert = true;
#else
    mWindowsNameConvert = false;
#endif
    mUriUri = new UriUriA;
    memset(mUriUri, 0, sizeof(*mUriUri));
    BMX_CHECK(Parse(uri_str));
}

URI::URI(string uri_str)
{
#if defined(_WIN32)
    mWindowsNameConvert = true;
#else
    mWindowsNameConvert = false;
#endif
    mUriUri = new UriUriA;
    memset(mUriUri, 0, sizeof(*mUriUri));
    BMX_CHECK(Parse(uri_str.c_str()));
}

URI::URI(const URI &uri)
{
    mUriUri = new UriUriA;
    memset(mUriUri, 0, sizeof(*mUriUri));
    Copy(uri);
}

URI::~URI()
{
    Clear();
    delete mUriUri;
}

void URI::SetWindowsNameConvert(bool windows_name_convert)
{
    mWindowsNameConvert = windows_name_convert;
}

bool URI::Parse(const char *uri_str)
{
    Clear();

    if (!uri_str)
        return false;

    mSourceStr.Copy(uri_str);

    UriParserStateA state;
    state.uri = mUriUri;
    int result = uriParseUriA(&state, mSourceStr.GetCStr());
    if (result) {
        Clear();
        return false;
    }

    result = uriNormalizeSyntaxA(mUriUri);
    if (result) {
        Clear();
        return false;
    }

    return true;
}

bool URI::Parse(string uri_str)
{
    return Parse(uri_str.c_str());
}

bool URI::ParseFilename(string filename)
{
    Clear();

    URIStr uri_str;
    size_t uri_str_size;

    uri_str_size = 7 + 3 * filename.size() + 1;
    if (mWindowsNameConvert)
        uri_str_size += 1;

    uri_str.Allocate(uri_str_size);

    int result;
    if (mWindowsNameConvert)
        result = uriWindowsFilenameToUriStringA(filename.c_str(), uri_str.GetStr());
    else
        result = uriUnixFilenameToUriStringA(filename.c_str(), uri_str.GetStr());
    if (result)
        return false;

    if (!Parse(uri_str.GetCStr()))
        return false;

    return true;
}

bool URI::ParseDirectory(string dirname)
{
    if (mWindowsNameConvert) {
        if (dirname.empty() ||
            (dirname[dirname.size() - 1] != '\\' && dirname[dirname.size() - 1] != '/'))
        {
            return ParseFilename(dirname + "\\");
        }
    } else {
        if (dirname.empty() || dirname[dirname.size() - 1] != '/')
            return ParseFilename(dirname + "/");
    }

    return ParseFilename(dirname);
}

void URI::Copy(const URI &uri)
{
    mWindowsNameConvert = uri.mWindowsNameConvert;
    Parse(uri.ToString());
}

bool URI::MakeAbsolute(const URI &base_uri)
{
    BMX_CHECK(base_uri.IsAbsolute());

    if (IsAbsolute())
        return true;

    URI abs_uri;
    int result = uriAddBaseUriA(abs_uri.mUriUri, mUriUri, base_uri.mUriUri);
    if (result)
        return false;

    Copy(abs_uri);

    return true;
}

bool URI::MakeRelative(const URI &base_uri)
{
    BMX_CHECK(base_uri.IsAbsolute());

    if (IsRelative())
        return true;

    URI rel_uri;
    int result = uriRemoveBaseUriA(rel_uri.mUriUri, mUriUri, base_uri.mUriUri, 0);
    if (result)
        return false;
    if (!rel_uri.IsRelative())
        return false;

    Copy(rel_uri);

    return true;
}

bool URI::IsRelative() const
{
    return !mUriUri->scheme.first;
}

bool URI::IsAbsolute() const
{
    return mUriUri->scheme.first;
}

bool URI::IsAbsFile() const
{
    return mUriUri->scheme.first && mUriUri->scheme.afterLast &&
           strncmp(mUriUri->scheme.first, "file", mUriUri->scheme.afterLast - mUriUri->scheme.first) == 0;
}

string URI::ToString() const
{
    int uri_str_len;
    int result = uriToStringCharsRequiredA(mUriUri, &uri_str_len);
    if (result)
        return "";

    URIStr uri_str;
    uri_str.Allocate(uri_str_len + 1);

    result = uriToStringA(uri_str.GetStr(), mUriUri, uri_str_len + 1, 0);
    if (result)
        return "";

    return uri_str.GetCStr();
}

string URI::ToFilename() const
{
    if (!IsAbsFile() && !IsRelative())
        return "";

    string uri_str = ToString();

    URIStr filename;
    filename.Allocate(uri_str.size() + 1);

    int result;
    if (mWindowsNameConvert)
        result = uriUriStringToWindowsFilenameA(uri_str.c_str(), filename.GetStr());
    else
        result = uriUriStringToUnixFilenameA(uri_str.c_str(), filename.GetStr());
    if (result)
        return 0;

    return filename.GetCStr();
}

size_t URI::GetNumSegments() const
{
    size_t size = 0;
    UriPathSegmentA *segment = mUriUri->pathHead;
    while (segment) {
        segment = segment->next;
        size++;
    }

    return size;
}

string URI::GetSegment(size_t index) const
{
    size_t count = 0;
    UriPathSegmentA *segment = mUriUri->pathHead;
    while (count < index && segment) {
        segment = segment->next;
        count++;
    }

    string segment_string;
    if (count == index && segment && segment->text.first && segment->text.afterLast)
        segment_string.assign(segment->text.first, segment->text.afterLast - segment->text.first);

    return segment_string;
}

string URI::GetLastSegment() const
{
    string segment_string;
    if (mUriUri->pathTail && mUriUri->pathTail->text.first && mUriUri->pathTail->text.afterLast) {
        segment_string.assign(mUriUri->pathTail->text.first,
                              mUriUri->pathTail->text.afterLast - mUriUri->pathTail->text.first);
    }

    return segment_string;
}

string URI::GetFirstSegment() const
{
    string segment_string;
    if (mUriUri->pathHead && mUriUri->pathHead->text.first && mUriUri->pathHead->text.afterLast) {
        segment_string.assign(mUriUri->pathHead->text.first,
                              mUriUri->pathHead->text.afterLast - mUriUri->pathHead->text.first);
    }

    return segment_string;
}

bool URI::operator==(const URI &right) const
{
    return uriEqualsUriA(mUriUri, right.mUriUri);
}

URI& URI::operator=(const URI &right)
{
    Copy(right);
    return *this;
}

void URI::Clear()
{
    uriFreeUriMembersA(mUriUri);
    memset(mUriUri, 0, sizeof(*mUriUri));

    mSourceStr.Clear();
}

