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

#ifndef BMX_EXCEPTION_H_
#define BMX_EXCEPTION_H_

#include <cassert>

#include <string>
#include <exception>


#define BMX_EXCEPTION(err)                                          \
    do {                                                            \
        bmx::log_error_nl err;                                      \
        bmx::log_error("    near %s:%d\n", __FILE__, __LINE__);     \
        throw BMXException err;                                     \
    } while (0)

#define BMX_CHECK(cond)                                     \
    do {                                                    \
        if (!(cond))                                        \
            BMX_EXCEPTION(("'%s' check failed", #cond));    \
    } while (0)

#define BMX_CHECK_M(cond, err)      \
    do {                            \
        if (!(cond))                \
            BMX_EXCEPTION(err);     \
    } while (0)

#if defined(NDEBUG)
#define BMX_ASSERT(cond)                                        \
    do {                                                        \
        if (!(cond))                                            \
            BMX_EXCEPTION(("'%s' assertion failed", #cond));    \
    } while (0)
#else
#define BMX_ASSERT(cond)    assert(cond)
#endif



namespace bmx
{


class BMXException : public std::exception
{
public:
    BMXException();
    BMXException(const char *format, ...);
    BMXException(const std::string &message);
    virtual ~BMXException() throw();

    const char* what() const throw();

protected:
    std::string mMessage;
};



};


#endif

