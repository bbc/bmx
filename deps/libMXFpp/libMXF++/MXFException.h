/*
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

#ifndef MXFPP_MXFEXCEPTION_H_
#define MXFPP_MXFEXCEPTION_H_

#include <string>
#include <cassert>

#include <mxf/mxf_logging.h>


namespace mxfpp
{


#define MXFPP_CHECK(cmd)                                                                \
    do {                                                                                \
        if (!(cmd)) {                                                                   \
            mxf_log_error("'%s' failed, at %s:%d\n", #cmd, __FILENAME__, __LINE__);     \
            throw MXFException("'%s' failed, at %s:%d", #cmd, __FILENAME__, __LINE__);  \
        }                                                                               \
    } while (0)

#if defined(NDEBUG)
#define MXFPP_ASSERT(cmd)   MXFPP_CHECK(cmd);
#else
#define MXFPP_ASSERT(cond)  assert(cond)
#endif


class MXFException
{
public:
    MXFException();
    MXFException(const char *format, ...);
    virtual ~MXFException();

    std::string getMessage() const;

protected:
    std::string _message;

};


};



#endif

