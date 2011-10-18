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

#ifndef __IM_EXCEPTION_H__
#define __IM_EXCEPTION_H__

#include <cassert>

#include <string>
#include <exception>


#define IM_EXCEPTION(err) \
    { \
        im::log_error_nl err; \
        im::log_error("    near %s:%d\n", __FILE__, __LINE__); \
        throw IMException err; \
    }

#define IM_CHECK(cond) \
    if (!(cond)) \
        IM_EXCEPTION(("'%s' check failed", #cond))

#define IM_CHECK_M(cond, err) \
    if (!(cond)) \
        IM_EXCEPTION(err)

#if defined(NDEBUG)
#define IM_ASSERT(cond) \
    if (!(cond)) \
        IM_EXCEPTION(("'%s' assertion failed", #cond))
#else
#define IM_ASSERT(cond) \
    assert(cond)
#endif



namespace im
{


class IMException : public std::exception
{
public:
    IMException();
    IMException(const char *format, ...);
    IMException(const std::string &message);
    virtual ~IMException() throw();

    const char* what() const throw();

protected:
    std::string mMessage;
};



};


#endif

