/*
 * Simple XML writer
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "xml_writer.h"
#include <mxf/mxf_macros.h>
#include <mxf/mxf_logging.h>


typedef enum
{
    ELEMENT_START,
    ATTRIBUTE,
    ELEMENT_END,
    CHARACTER_DATA
} PreviousWrite;

struct XMLWriter
{
    FILE *file;
    int indent;
    PreviousWrite previousWrite;
};


static int write_indent(XMLWriter *writer)
{
    int i;

    CHK_ORET(fprintf(writer->file, "\r\n") > 0);
    for (i = 0; i < writer->indent; i++)
    {
        CHK_ORET(fprintf(writer->file, "  ") > 0);
    }

    return 1;
}

int xml_writer_open(const char *filename, XMLWriter **writer)
{
    XMLWriter *newWriter;

    CHK_MALLOC_ORET(newWriter, XMLWriter);
    memset(newWriter, 0, sizeof(XMLWriter));
    newWriter->previousWrite = ELEMENT_END;

    if ((newWriter->file = fopen(filename, "wb")) == NULL)
    {
        mxf_log_error("Failed to open xml file '%s'" LOG_LOC_FORMAT, filename, LOG_LOC_PARAMS);
        goto fail;
    }

    CHK_OFAIL(fprintf(newWriter->file, "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\" ?>") > 0);

    *writer = newWriter;
    return 1;

fail:
    SAFE_FREE(newWriter);
    return 0;
}

void xml_writer_close(XMLWriter **writer)
{
    if (*writer == NULL)
    {
        return;
    }

    if ((*writer)->file != NULL)
    {
        fprintf((*writer)->file, "\r\n");
        fclose((*writer)->file);
    }
    SAFE_FREE(*writer);
}


int xml_writer_element_start(XMLWriter *writer, const char *name)
{
    if (writer->previousWrite == ELEMENT_START || writer->previousWrite == ATTRIBUTE)
    {
        CHK_ORET(fprintf(writer->file, ">") > 0);
    }

    write_indent(writer);
    CHK_ORET(fprintf(writer->file, "<%s", name) > 0);

    writer->indent++;
    writer->previousWrite = ELEMENT_START;
    return 1;
}

int xml_writer_attribute(XMLWriter *writer, const char *name, const char *value)
{
    assert(writer->previousWrite == ELEMENT_START || writer->previousWrite == ATTRIBUTE);

    CHK_ORET(fprintf(writer->file, " %s=\"%s\"", name, value) > 0);

    writer->previousWrite = ATTRIBUTE;
    return 1;
}


int xml_writer_element_end(XMLWriter *writer, const char *name)
{
    writer->indent--;
    if (writer->previousWrite == ELEMENT_END)
    {
        write_indent(writer);
    }
    else if (writer->previousWrite == ELEMENT_START || writer->previousWrite == ATTRIBUTE)
    {
        CHK_ORET(fprintf(writer->file, ">") > 0);
    }
    CHK_ORET(fprintf(writer->file, "</%s>", name) > 0);

    writer->previousWrite = ELEMENT_END;
    return 1;
}

int xml_writer_character_data(XMLWriter *writer, const char *data)
{
    if (writer->previousWrite == ELEMENT_START || writer->previousWrite == ATTRIBUTE)
    {
        CHK_ORET(fprintf(writer->file, ">") > 0);
    }
    CHK_ORET(fprintf(writer->file, "%s", data) > 0);

    writer->previousWrite = CHARACTER_DATA;
    return 1;
}

