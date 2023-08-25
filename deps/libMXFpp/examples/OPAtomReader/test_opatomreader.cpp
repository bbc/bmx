/*
 * Test the OP-Atom reader
 *
 * Copyright (C) 2009, British Broadcasting Corporation
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

#define __STDC_FORMAT_MACROS    1

#include <cstdio>

#include <libMXF++/MXF.h>

#include "OPAtomClipReader.h"

using namespace std;
using namespace mxfpp;



int main(int argc, const char **argv)
{
    vector<string> filenames;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <mxf filename>\n", argv[0]);
        return 1;
    }

    filenames.push_back(argv[1]);

    OPAtomClipReader *clip_reader;
    pair<OPAtomOpenResult, string> result = OPAtomClipReader::Open(filenames, &clip_reader);
    if (result.first != OP_ATOM_SUCCESS) {
        fprintf(stderr, "Failed to open file '%s': %s\n", result.second.c_str(),
                OPAtomClipReader::ErrorToString(result.first).c_str());
        return 1;
    }

    printf("Duration = %" PRId64 "\n", clip_reader->GetDuration());

    const OPAtomContentPackage *content_package;

    FILE *output = fopen("/tmp/test.raw", "wb");
    while (true) {
        content_package = clip_reader->Read();
        if (!content_package) {
            if (!clip_reader->IsEOF()) {
                fprintf(stderr, "Failed to read content package\n");
            }
            break;
        }

        printf("writing %d bytes from essence offset 0x%" PRIx64 "\n", content_package->GetEssenceDataISize(0),
               content_package->GetEssenceDataI(0)->GetEssenceOffset());
        fwrite(content_package->GetEssenceDataIBytes(0), content_package->GetEssenceDataISize(0), 1, output);
    }

    printf("Duration = %" PRId64 "\n", clip_reader->GetDuration());

    fclose(output);

    clip_reader->Seek(0);
    content_package = clip_reader->Read();
    if (!content_package) {
        if (!clip_reader->IsEOF()) {
            fprintf(stderr, "Failed to read content package\n");
        }
    }


    delete clip_reader;

    return 0;
}
