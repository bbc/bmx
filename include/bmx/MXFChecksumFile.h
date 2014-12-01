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

#ifndef BMX_MXF_CHECKSUM_FILE_H_
#define BMX_MXF_CHECKSUM_FILE_H_


#include <string>

#include <mxf/mxf_file.h>

#include <bmx/Checksum.h>



namespace bmx
{


typedef struct MXFChecksumFile MXFChecksumFile;

MXFChecksumFile* mxf_checksum_file_open(MXFFile *target, ChecksumType type);
MXFFile* mxf_checksum_file_get_file(MXFChecksumFile *checksum_file);
void mxf_checksum_file_force_update(MXFChecksumFile *checksum_file);
bool mxf_checksum_file_final(MXFChecksumFile *checksum_file);
size_t mxf_checksum_file_digest_size(const MXFChecksumFile *checksum_file);
void mxf_checksum_file_digest(const MXFChecksumFile *checksum_file, unsigned char *digest, size_t size);
std::string mxf_checksum_file_digest_str(const MXFChecksumFile *checksum_file);


};



#endif
