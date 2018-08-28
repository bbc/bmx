/*
 * Copyright (C) 2018, British Broadcasting Corporation
 * All Rights Reserved.
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

#include <bmx/apps/PropertyFileParser.h>

#include <errno.h>

#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;


PropertyFileParser::PropertyFileParser()
{
    mFile = 0;
    mNextChar = '1';
    mLineNumber = 1;
    mDoneParsing = false;
    mHaveError = false;
}

PropertyFileParser::~PropertyFileParser()
{
    if (mFile)
        fclose(mFile);
}

bool PropertyFileParser::Open(const string &filename)
{
    mFile = fopen(filename.c_str(), "rb");
    if (!mFile) {
        fprintf(stderr, "Failed to open property file: %s\n", bmx_strerror(errno).c_str());
        return false;
    }

    return true;
}

bool PropertyFileParser::ParseNext(string *name_out, string *value_out)
{
    BMX_ASSERT(mFile);

    if (mDoneParsing)
        return false;

    while (mNextChar != EOF) {
        // move past the newline characters
        while ((mNextChar = fgetc(mFile)) != EOF && (mNextChar == '\r' || mNextChar == '\n')) {
            if (mNextChar == '\n')
                mLineNumber++;
        }

        // skip leading space
        while (mNextChar != EOF && (mNextChar == ' ' || mNextChar == '\t'))
            mNextChar = fgetc(mFile);

        // skip line starting with '#'
        if (mNextChar == '#') {
            while ((mNextChar = fgetc(mFile)) != EOF && (mNextChar != '\r' && mNextChar != '\n'))
            { }
            if (mNextChar == '\n')
                mLineNumber++;
            continue;
        }

        if (mNextChar == EOF)
            break;

        // parse line into <name> + ':' + <value>
        string name;
        string value;
        bool parse_name = true;
        while (mNextChar != EOF && (mNextChar != '\r' && mNextChar != '\n')) {
            if (mNextChar == ':' && parse_name) {
                parse_name = false;
            } else {
                if (parse_name)
                    name += mNextChar;
                else
                    value += mNextChar;
            }

            mNextChar = fgetc(mFile);
        }

        if (!name.empty() && parse_name) {
            fprintf(stderr, "Failed to parse property line %d with name %s\n", mLineNumber, name.c_str());
            mHaveError = true;
            break;
        }

        if (mNextChar == '\n')
            mLineNumber++;

        if (!name.empty()) {
            *name_out = trim_string(name);
            *value_out = trim_string(value);
            return true;
        }
    }

    mDoneParsing = true;
    return false;
}
