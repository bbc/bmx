/*
 * Extracts a list of JPEG 2000 Picture Essence Coding Labels.
 *
 * Copyright (C) 2021, British Broadcasting Corporation
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <mxf/mxf.h>
#include <mxf/mxf_labels_and_keys.h>
#include <mxf/mxf_utils.h>

#define MAX_PROFILE_NO      0x30
#define MAX_MAIN_LEVEL_NO   11
#define MAX_SUB_LEVEL_NO    10


int main()
{
    // Print the generic JPEG2000 label
    mxf_print_label(&MXF_CMDEF_L(JPEG2000_UNDEFINED));

    // Loop through the range of profiles, main levels and sub levels.
    // If the generic JPEG2000 label is returned then use that to skip to the next main level or profile.
    for (int profile = 0; profile <= MAX_PROFILE_NO; profile++) {

        for (int ml = -1; ml <= MAX_MAIN_LEVEL_NO; ml++) {
            uint8_t main_level = (uint8_t)ml;

            for (int sl = -1; sl <= MAX_SUB_LEVEL_NO; sl++) {
                mxfUL label;
                uint8_t sub_level = (uint8_t)sl;

                mxf_get_jpeg2000_coding_label(profile, main_level, sub_level, &label);

                if (mxf_equals_ul(&label, &MXF_CMDEF_L(JPEG2000_UNDEFINED))) {
                    if (sub_level != (uint8_t)-1)
                        break;
                } else {
                    mxf_print_label(&label);
                }
            }
        }
    }

    // Print the generic HTJ2K label
    mxf_print_label(&MXF_CMDEF_L(JPEG2000_HTJ2K_GENERIC));

    return 0;
}
