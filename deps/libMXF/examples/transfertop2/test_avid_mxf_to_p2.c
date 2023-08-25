/*
 * Tests transfers of Avid MXF files to P2
 *
 * Copyright (C) 2006, British Broadcasting Corporation
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "avid_mxf_to_p2.h"


void usage(const char *cmd)
{
    fprintf(stderr, "%s -r p2path (-i filename)+\n", cmd);
    fprintf(stderr, "\n");
    fprintf(stderr, "    -r p2path        directory path to the P2 card root directory\n");
    fprintf(stderr, "    -i filename      input MXF filename\n");
    fprintf(stderr, "\n");
}

int main(int argc, const char *argv[])
{
    const char *p2path = NULL;
    char *inputFilenames[17];
    int inputIndex;
    int cmdlIndex;
    AvidMXFToP2Transfer *transfer = NULL;
    int isComplete;
    int i;

    inputIndex = 0;
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
            if (inputIndex >= 17)
            {
                fprintf(stderr, "Too many inputs\n");
                goto fail;
            }
            inputFilenames[inputIndex] = strdup(argv[cmdlIndex + 1]);
            if (inputFilenames[inputIndex] == NULL)
            {
                goto fail;
            }
            inputIndex++;
            cmdlIndex += 2;
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
        fprintf(stderr, "Missing -r p2path\n");
        goto fail;
    }
    if (inputIndex == 0)
    {
        usage(argv[0]);
        fprintf(stderr, "No -i filename inputs provided\n");
        goto fail;
    }

    if (!prepare_transfer(inputFilenames, inputIndex, 900000, 0,
        NULL, NULL, &transfer))
    {
        fprintf(stderr, "prepare_transfer failed\n");
        goto fail;
    }

    if (!transfer_avid_mxf_to_p2(p2path, transfer, &isComplete))
    {
        fprintf(stderr, "transfer_avid_mxf_to_p2 failed\n");
        goto fail;
    }

    free_transfer(&transfer);

    for (i = 0; i < inputIndex; i++)
    {
        free(inputFilenames[i]);
    }

    return 0;


fail:
    free_transfer(&transfer);
    for (i = 0; i < inputIndex; i++)
    {
        free(inputFilenames[i]);
    }
    return 1;
}

