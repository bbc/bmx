/*
 * Parse metadata from an Avid MXF file and print to screen
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "avid_mxf_info.h"



static void usage(const char *cmd)
{
    fprintf(stderr, "Usage: %s [options] <input>+\n", cmd);
    fprintf(stderr, "\n");
    fprintf(stderr, "Options: (options marked with * are required)\n");
    fprintf(stderr, "  -h, --help                 display this usage message\n");
}


int main(int argc, const char *argv[])
{
    int cmdlnIndex = 1;
    int inputFilenamesIndex;
    AvidMXFInfo info;
    int result;
    int i;
    const char *inputFilename;
    int test = 0;

    while (cmdlnIndex < argc)
    {
        if (strcmp(argv[cmdlnIndex], "-h") == 0 ||
            strcmp(argv[cmdlnIndex], "--help") == 0)
        {
            usage(argv[0]);
            return 0;
        }
        else if (strcmp(argv[cmdlnIndex], "--test") == 0)
        {
            test = 1;
            cmdlnIndex++;
        }
        else
        {
            break;
        }
    }

    if (cmdlnIndex >= argc)
    {
        usage(argv[0]);
        fprintf(stderr, "Missing <input> filename\n");
        return 1;
    }

    inputFilenamesIndex = cmdlnIndex;



    for (i = inputFilenamesIndex; i < argc; i++)
    {
        inputFilename = argv[i];

        printf("\nFilename = %s\n", inputFilename);

        result = ami_read_info(inputFilename, &info, 1);
        if (result != 0)
        {
            switch (result)
            {
                case -2:
                    fprintf(stderr, "Failed to open file (%s)\n", inputFilename);
                    break;
                case -3:
                    fprintf(stderr, "Failed to read header partition (%s)\n", inputFilename);
                    break;
                case -4:
                    fprintf(stderr, "File is not OP-Atom (%s)\n", inputFilename);
                    break;
                case -1:
                default:
                    fprintf(stderr, "Failed to read info (%s)\n", inputFilename);
                    break;
            }

            if (test)
                return result;

            continue;
        }

        ami_print_info(&info);

        ami_free_info(&info);
    }


    return 0;
}

