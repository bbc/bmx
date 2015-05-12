/*
 * Copyright (C) 2011, British Broadcasting Corporation
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

#ifndef BMX_VERSION_H_
#define BMX_VERSION_H_


#include <string>

#include <bmx/BMXTypes.h>


#define BMX_VERSION_MAJOR    0
#define BMX_VERSION_MINOR    1
#define BMX_VERSION_MICRO    3

#define BMX_MXF_VERSION_RELEASE  2   /* 0 = Unknown version
                                        1 = Released version
                                        2 = Development version
                                        3 = Released version with patches
                                        4 = Pre-release beta version
                                        5 = Private version not intended for general release */

#define BMX_VERSION          (BMX_VERSION_MAJOR << 16 | BMX_VERSION_MINOR << 8 | BMX_VERSION_MICRO)

#define BMX_LIBRARY_NAME     "bmx"



namespace bmx
{


std::string get_bmx_library_name();
std::string get_bmx_version_string();
std::string get_bmx_scm_version_string();
std::string get_bmx_build_string();
Timestamp get_bmx_build_timestamp();

std::string get_bmx_company_name();
UUID get_bmx_product_uid();
mxfProductVersion get_bmx_mxf_product_version();
std::string get_bmx_mxf_version_string();


};



#endif

