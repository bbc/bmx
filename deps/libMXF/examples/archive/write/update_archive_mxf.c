/*
 * Update an archive MXF file with new filename and LTO Infax data
 *
 * Copyright (C) 2008, British Broadcasting Corporation
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

/*
    Example: update LTA00000501.mxf which will be the first file on LTO tape 'LTA000005'

        ./update_archive_mxf --file LTA00000501.mxf \
            --infax "LTO|D3 preservation programme||2006-02-02||LME1306H|71|T||D3 PRESERVATION COPY||1732|LTA000005||LONPROG|1" \
            LTA00000501.mxf

    Check parse_infax_data() in write_archive_mxf.c for the expected Infax
    string.
*/


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "write_archive_mxf.h"


static void usage(const char *cmd)
{
    fprintf(stderr, "Usage: %s [options] <MXF filename>\n", cmd);
    fprintf(stderr, "\n");
    fprintf(stderr, "Options: (options marked with * are required)\n");
    fprintf(stderr, "  -h, --help                 display this usage message\n");
    fprintf(stderr, "* --infax <string>           infax data string for the LTO\n");
    fprintf(stderr, "* --file <name>              filename of the MXF file stored on the LTO\n");
    fprintf(stderr, "\n");
}

int main(int argc, const char *argv[])
{
    const char *infaxString = NULL;
    const char *ltoMXFFilename = NULL;
    const char *mxfFilename = NULL;
    int cmdlnIndex = 1;
    InfaxData infaxData;


    while (cmdlnIndex + 1 < argc)
    {
        if (strcmp(argv[cmdlnIndex], "-h") == 0 ||
            strcmp(argv[cmdlnIndex], "--help") == 0)
        {
            usage(argv[0]);
            return 0;
        }
        else if (strcmp(argv[cmdlnIndex], "--infax") == 0)
        {
            infaxString = argv[cmdlnIndex + 1];
            cmdlnIndex += 2;
        }
        else if (strcmp(argv[cmdlnIndex], "--file") == 0)
        {
            ltoMXFFilename = argv[cmdlnIndex + 1];
            cmdlnIndex += 2;
        }
        else
        {
            if (cmdlnIndex + 1 != argc)
            {
                fprintf(stderr, "Unknown argument '%s'\n", argv[cmdlnIndex]);
                usage(argv[0]);
                return 1;
            }

            break;
        }
    }

    if (cmdlnIndex + 1 != argc)
    {
        fprintf(stderr, "Missing MXF filename\n");
        usage(argv[0]);
        return 1;
    }
    if (infaxString == NULL)
    {
        fprintf(stderr, "--infax is required\n");
        usage(argv[0]);
        return 1;
    }
    if (ltoMXFFilename == NULL)
    {
        fprintf(stderr, "--file is required\n");
        usage(argv[0]);
        return 1;
    }

    mxfFilename = argv[cmdlnIndex];
    cmdlnIndex++;


    if (!parse_infax_data(infaxString, &infaxData))
    {
        fprintf(stderr, "ERROR: Failed to parse the Infax data string '%s'\n", infaxString);
        exit(1);
    }

    if (!update_archive_mxf_file(mxfFilename, ltoMXFFilename, &infaxData))
    {
        fprintf(stderr, "ERROR: Failed to update MXF file '%s'\n", mxfFilename);
        exit(1);
    }


    return 0;
}

