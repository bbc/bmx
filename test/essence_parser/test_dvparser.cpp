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

#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cerrno>

#include <bmx/essence_parser/DVEssenceParser.h>

using namespace bmx;


typedef struct
{
    unsigned char *data;
    uint32_t buffer_size;
    uint32_t data_size;
} Buffer;



static uint32_t fill_buffer(FILE *file, Buffer *buffer, uint32_t size)
{
    if (buffer->data_size + size > buffer->buffer_size) {
        unsigned char *new_data = new unsigned char[buffer->data_size + size];
        memcpy(new_data, buffer->data, buffer->data_size);
        delete [] buffer->data;
        buffer->data = new_data;
        buffer->buffer_size = buffer->data_size + size;
    }

    uint32_t num_read = fread(buffer->data + buffer->data_size, 1, size, file);
    if (num_read == 0 && ferror(file)) {
        fprintf(stderr, "Failed to read from file: %s\n", strerror(errno));
        exit(1);
    }
    buffer->data_size += num_read;

    return size;
}

static void print_frame_info(DVEssenceParser *parser)
{
    printf("Frame:\n");
    printf("  Type: ");
    switch (parser->GetEssenceType())
    {
        case DVEssenceParser::IEC_DV25:
            printf("IEC_DV25\n");
            break;
        case DVEssenceParser::DVBASED_DV25:
            printf("DVBASED_DV25\n");
            break;
        case DVEssenceParser::DV50:
            printf("DV50\n");
            break;
        case DVEssenceParser::DV100_1080I:
            printf("DV100_1080I\n");
            break;
        case DVEssenceParser::DV100_720P:
            printf("DV100_720P\n");
            break;
        case DVEssenceParser::UNKNOWN_DV:
            printf("UNKNOWN DV\n");
            break;
    }
    printf("  Rate: %s\n", parser->Is50Hz() ? "50Hz" : "60Hz");
    printf("  Size: %u\n", parser->GetFrameSize());
    printf("  Aspect ratio: %d/%d\n", parser->GetAspectRatio().numerator, parser->GetAspectRatio().denominator);
}


int main(int argc, const char **argv)
{
    const char *filename = 0;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <dv filename>\n", argv[0]);
        return 1;
    }
    filename = argv[1];

    FILE *file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Failed to open '%s': %s\n", filename, strerror(errno));
        return 1;
    }

    Buffer buffer;
    buffer.data = 0;
    buffer.buffer_size = 0;
    buffer.data_size = 0;

    DVEssenceParser parser;
    uint32_t frame_size = ESSENCE_PARSER_NULL_OFFSET;
    while (frame_size == ESSENCE_PARSER_NULL_OFFSET && buffer.data_size < 576000) {
        fill_buffer(file, &buffer, 8192);
        frame_size = parser.ParseFrameSize(buffer.data, buffer.data_size);
    }
    if (frame_size == ESSENCE_PARSER_NULL_FRAME_SIZE || frame_size == ESSENCE_PARSER_NULL_OFFSET) {
        fprintf(stderr, "Failed to parse frame size within %u bytes\n", buffer.data_size);
        return 1;
    }


    while (true) {
        parser.ParseFrameInfo(buffer.data, buffer.data_size);
        print_frame_info(&parser);

        if (buffer.data_size > frame_size)
            memmove(buffer.data, buffer.data + frame_size, buffer.data_size - frame_size);
        buffer.data_size -= frame_size;

        fill_buffer(file, &buffer, frame_size - buffer.data_size);
        if (buffer.data_size != frame_size)
            break;
    }


    fclose(file);

    return 0;
}

