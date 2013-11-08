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

#include <bmx/essence_parser/MJPEGEssenceParser.h>

using namespace bmx;


#define MAX_IMAGE_SIZE      50000000
#define BUFFER_INCREMENT    4096


typedef struct
{
    unsigned char *data;
    uint32_t buffer_size;
    uint32_t data_size;
} Buffer;



static uint32_t fill_buffer(FILE *file, Buffer *buffer)
{
    if (buffer->data_size + BUFFER_INCREMENT > buffer->buffer_size) {
        if (buffer->buffer_size + BUFFER_INCREMENT > MAX_IMAGE_SIZE) {
            fprintf(stderr, "Maximum buffer size %u exceeded\n", MAX_IMAGE_SIZE);
            exit(1);
        }

        unsigned char *new_data = new unsigned char[buffer->buffer_size + BUFFER_INCREMENT];
        memcpy(new_data, buffer->data, buffer->data_size);
        delete [] buffer->data;
        buffer->data = new_data;
        buffer->buffer_size += BUFFER_INCREMENT;
    }

    uint32_t num_read = fread(buffer->data + buffer->data_size, 1, BUFFER_INCREMENT, file);
    if (num_read == 0 && ferror(file)) {
        fprintf(stderr, "Failed to read from file: %s\n", strerror(errno));
        exit(1);
    }
    buffer->data_size += num_read;

    return num_read;
}

static void shift_buffer(Buffer *buffer, uint32_t start)
{
    memmove(buffer->data, buffer->data + start, buffer->data_size - start);
    buffer->data_size -= start;
}

static void print_image_info(MJPEGEssenceParser *parser, Buffer *buffer, uint32_t image_size)
{
    printf("Image size: %u\n", image_size);

    (void)parser;
    (void)buffer;
//    parser->ParseFrameInfo(buffer->data, image_size);
}


int main(int argc, const char **argv)
{
    const char *filename = 0;
    bool single_field = false;

    if (argc != 2 && argc != 3) {
        fprintf(stderr, "Usage: %s [--single-field] <mjpeg filename>\n", argv[0]);
        return 1;
    }
    if (argc == 3) {
        single_field = (strcmp(argv[1], "--single-field") == 0);
        filename = argv[2];
    } else {
        filename = argv[1];
    }

    FILE *file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Failed to open '%s': %s\n", filename, strerror(errno));
        return 1;
    }

    Buffer buffer;
    buffer.data = new unsigned char[BUFFER_INCREMENT];
    buffer.buffer_size = BUFFER_INCREMENT;
    buffer.data_size = 0;

    MJPEGEssenceParser parser(single_field);
    fill_buffer(file, &buffer);

    uint32_t image_start = 0;
    uint32_t image_size;
    while (true) {
        if (image_start > 0) {
            shift_buffer(&buffer, image_start);
            image_start = 0;
        }

        image_size = parser.ParseFrameSize(buffer.data, buffer.data_size);
        if (image_size == ESSENCE_PARSER_NULL_OFFSET) {
            if (fill_buffer(file, &buffer) == 0)
                break;
        } else if (image_size == ESSENCE_PARSER_NULL_FRAME_SIZE) {
            fprintf(stderr, "Invalid image data start\n");
            return 1;
        } else {
            print_image_info(&parser, &buffer, image_size);
            image_start = image_size;
        }
    }
    if (buffer.data_size > 0)
        print_image_info(&parser, &buffer, buffer.data_size);


    fclose(file);

    return 0;
}

