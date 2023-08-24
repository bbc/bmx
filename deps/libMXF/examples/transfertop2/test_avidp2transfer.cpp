/*
 * Tests transfer of MXF files referenced in an Avid AAF composition to P2
 *
 * Copyright (C) 2006, British Broadcasting Corporation
 * All Rights Reserved.
 *
 * Author: Philip de Nier, Stuart Cunningham
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "avidp2transfer.h"

using namespace std;


void test_progress(float percentCompleted)
{
    printf("\r%6.2f%% completed", percentCompleted);
    fflush(stdout);
}


void usage(const char *cmd)
{
    fprintf(stderr, "%s -p prefix [-c] -r p2path -i filename\n", cmd);
    fprintf(stderr, "\n");
    fprintf(stderr, "    -p prefix        add the prefix to the MXF filepath\n");
    fprintf(stderr, "    -c               omit the colon after the MXF filepath drive letter\n");
    fprintf(stderr, "    -r p2path        directory path to the P2 card root directory\n");
    fprintf(stderr, "    -i filename      Avid AAF filename containing a sequence referencing the MXF file\n");
    fprintf(stderr, "\n");
}

int main(int argc, const char *argv[])
{
    const char *p2path = NULL;
    const char *filename = NULL;
    const char *prefix = NULL;
    bool omitDriveColon = false;
    int cmdlIndex;

    cmdlIndex = 1;
    while (cmdlIndex < argc)
    {
        if (!strcmp(argv[cmdlIndex], "-r"))
        {
            if (cmdlIndex >= argc-1)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing -r argument\n");
                return 1;
            }
            p2path = argv[cmdlIndex + 1];
            cmdlIndex += 2;
        }
        else if (!strcmp(argv[cmdlIndex], "-i"))
        {
            if (cmdlIndex >= argc-1)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing -i argument\n");
                return 1;
            }
            if (filename != NULL)
            {
                fprintf(stderr, "Too many inputs\n");
                return 0;
            }
            filename = argv[cmdlIndex + 1];
            cmdlIndex += 2;
        }
        else if (!strcmp(argv[cmdlIndex], "-p"))
        {
            if (cmdlIndex >= argc-1)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing -p argument\n");
                return 1;
            }
            prefix = argv[cmdlIndex + 1];
            cmdlIndex += 2;
        }
        else if (!strcmp(argv[cmdlIndex], "-c"))
        {
            omitDriveColon = true;
            cmdlIndex += 1;
        }
        else
        {
            usage(argv[0]);
            fprintf(stderr, "Unknown argument '%s'\n", argv[cmdlIndex]);
            return 1;
        }
    }

    if (p2path == NULL)
    {
        usage(argv[0]);
        fprintf(stderr, "Missing p2 path\n");
        return 1;
    }
    if (filename == NULL)
    {
        usage(argv[0]);
        fprintf(stderr, "Missing input AAF filename\n");
        return 1;
    }

    try
    {
        AvidP2Transfer transfer(filename, test_progress, NULL, prefix, omitDriveColon);

        printf("Estimated total storage size = %"PRIu64" bytes\n", transfer.totalStorageSizeEstimate);

        vector<APTTrackInfo>::const_iterator iter;
        for (iter = transfer.trackInfo.begin(); iter != transfer.trackInfo.end(); iter++)
        {
            printf("Input MXF file: '%s'\n", (*iter).mxfFilename.c_str());
            if ((*iter).isPicture)
            {
                printf("    Type = Picture\n");
            }
            else
            {
                printf("    Type = Sound\n");
            }
            printf("    Name = '%s'\n", (*iter).name.c_str());
            printf("    CompositionMob track length = (%d/%d) %" PRId64 "\n",
                (*iter).compositionEditRate.numerator, (*iter).compositionEditRate.denominator,
                (*iter).compositionTrackLength);
            printf("    SourceMob track length = (%d/%d) %" PRId64 "\n",
                (*iter).sourceEditRate.numerator, (*iter).sourceEditRate.denominator,
                (*iter).sourceTrackLength);
        }

        if (transfer.trackInfo.size() > 0)
        {
            transfer.transferToP2(p2path);
            printf("\nCreated clip '%s'\n", transfer.clipName.c_str());
        }
        else
        {
            fprintf(stderr, "No tracks to transfer\n");
        }
    }
    catch (APTException& ex)
    {
        fprintf(stderr, "Exception: %s\n", ex.getMessage());
        return 1;
    }

    return 0;
}

