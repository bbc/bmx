/*
 * Copyright (C) 2016, British Broadcasting Corporation
 * All Rights Reserved.
 *
 * AS10 contribution : Andrei Klotchkivski
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

/* Note: defines are undefined at the end of the file, unless KEEP_DATA_MODEL_DEFS defined */


#if !defined (MXF_BASIC_TYPE_DEF)
#define MXF_BASIC_TYPE_DEF(typeId, name, size)
#endif
#if !defined (MXF_ARRAY_TYPE_DEF)
#define MXF_ARRAY_TYPE_DEF(typeId, name, elementTypeId, fixedSize)
#endif
#if !defined (MXF_COMPOUND_TYPE_DEF)
#define MXF_COMPOUND_TYPE_DEF(typeId, name)
#endif
#if !defined (MXF_COMPOUND_TYPE_MEMBER)
#define MXF_COMPOUND_TYPE_MEMBER(memberName, memberTypeId)
#endif
#if !defined (MXF_INTERPRETED_TYPE_DEF)
#define MXF_INTERPRETED_TYPE_DEF(typeId, name, interpretedTypeId, fixedSize)
#endif

#if !defined (MXF_LABEL)
#define MXF_LABEL(d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, d12, d13, d14, d15)
#endif
#if !defined (MXF_SET_DEFINITION)
#define MXF_SET_DEFINITION(parentName, name, label)
#endif
#if !defined (MXF_ITEM_DEFINITION)
#define MXF_ITEM_DEFINITION(setName, name, label, localTag, typeId, isRequired)
#endif



MXF_SET_DEFINITION(DMFramework, AS10CoreFramework,
    MXF_LABEL(0x06, 0x0e, 0x2b, 0x34, 0x02, 0x53, 0x01, 0x01, 0x0d, 0x01, 0x07, 0x01, 0x0a, 0x01, 0x01, 0x00)
);

    MXF_ITEM_DEFINITION(AS10CoreFramework, AS10ShimName,
        MXF_LABEL(0x06, 0x0e, 0x2b, 0x34, 0x01, 0x01, 0x01, 0x01, 0x0d, 0x01, 0x07, 0x01, 0x0a, 0x01, 0x01, 0x01),
        0x0000,
        MXF_UTF16STRING_TYPE,
        1
    );

    MXF_ITEM_DEFINITION(AS10CoreFramework, AS10Type,
        MXF_LABEL(0x06, 0x0e, 0x2b, 0x34, 0x01, 0x01, 0x01, 0x01, 0x0d, 0x01, 0x07, 0x01, 0x0a, 0x01, 0x01, 0x02),
        0x0000,
        MXF_UTF16STRING_TYPE,
        0
    );

    MXF_ITEM_DEFINITION(AS10CoreFramework, AS10MainTitle,
        MXF_LABEL(0x06, 0x0e, 0x2b, 0x34, 0x01, 0x01, 0x01, 0x01, 0x0d, 0x01, 0x07, 0x01, 0x0a, 0x01, 0x01, 0x03),
        0x0000,
        MXF_UTF16STRING_TYPE,
        0
    );

    MXF_ITEM_DEFINITION(AS10CoreFramework, AS10SubTitle,
        MXF_LABEL(0x06, 0x0e, 0x2b, 0x34, 0x01, 0x01, 0x01, 0x01, 0x0d, 0x01, 0x07, 0x01, 0x0a, 0x01, 0x01, 0x04),
        0x0000,
        MXF_UTF16STRING_TYPE,
        0
    );

    MXF_ITEM_DEFINITION(AS10CoreFramework, AS10TitleDescription,
        MXF_LABEL(0x06, 0x0e, 0x2b, 0x34, 0x01, 0x01, 0x01, 0x01, 0x0d, 0x01, 0x07, 0x01, 0x0a, 0x01, 0x01, 0x05),
        0x0000,
        MXF_UTF16STRING_TYPE,
        0
    );

    MXF_ITEM_DEFINITION(AS10CoreFramework, AS10OrganizationName,
        MXF_LABEL(0x06, 0x0e, 0x2b, 0x34, 0x01, 0x01, 0x01, 0x01, 0x0d, 0x01, 0x07, 0x01, 0x0a, 0x01, 0x01, 0x06),
        0x0000,
        MXF_UTF16STRING_TYPE,
        0
    );

    MXF_ITEM_DEFINITION(AS10CoreFramework, AS10PersonName,
        MXF_LABEL(0x06, 0x0e, 0x2b, 0x34, 0x01, 0x01, 0x01, 0x01, 0x0d, 0x01, 0x07, 0x01, 0x0a, 0x01, 0x01, 0x07),
        0x0000,
        MXF_UTF16STRING_TYPE,
        0
    );

    MXF_ITEM_DEFINITION(AS10CoreFramework, AS10LocationDescription,
        MXF_LABEL(0x06, 0x0e, 0x2b, 0x34, 0x01, 0x01, 0x01, 0x01, 0x0d, 0x01, 0x07, 0x01, 0x0a, 0x01, 0x01, 0x08),
        0x0000,
        MXF_UTF16STRING_TYPE,
        0
    );

    MXF_ITEM_DEFINITION(AS10CoreFramework, AS10CommonSpanningID,
        MXF_LABEL(0x06, 0x0e, 0x2b, 0x34, 0x01, 0x01, 0x01, 0x01, 0x0d, 0x01, 0x07, 0x01, 0x0a, 0x01, 0x01, 0x09),
        0x0000,
        MXF_UMID_TYPE,
        0
    );

    MXF_ITEM_DEFINITION(AS10CoreFramework, AS10SpanningNumber,
        MXF_LABEL(0x06, 0x0e, 0x2b, 0x34, 0x01, 0x01, 0x01, 0x01, 0x0d, 0x01, 0x07, 0x01, 0x0a, 0x01, 0x01, 0x0a),
        0x0000,
        MXF_UINT16_TYPE,
        0
    );

    MXF_ITEM_DEFINITION(AS10CoreFramework, AS10CumulativeDuration,
        MXF_LABEL(0x06, 0x0e, 0x2b, 0x34, 0x01, 0x01, 0x01, 0x01, 0x0d, 0x01, 0x07, 0x01, 0x0a, 0x01, 0x01, 0x0b),
        0x0000,
        MXF_POSITION_TYPE,
        0
    );



#if !defined(KEEP_DATA_MODEL_DEFS)
#undef MXF_BASIC_TYPE_DEF
#undef MXF_ARRAY_TYPE_DEF
#undef MXF_COMPOUND_TYPE_DEF
#undef MXF_COMPOUND_TYPE_MEMBER
#undef MXF_INTERPRETED_TYPE_DEF
#undef MXF_LABEL
#undef MXF_SET_DEFINITION
#undef MXF_ITEM_DEFINITION
#endif
