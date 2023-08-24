/*
 * General purpose macros
 *
 * Copyright (C) 2006, British Broadcasting Corporation
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

#ifndef MXF_MACROS_H_
#define MXF_MACROS_H_


#ifdef __cplusplus
extern "C"
{
#endif


/*
* Command checking macros
*   ...ORET - check succeeds, otherwise return 0
*   ...OFAIL - check succeeds, otherwise goto fail
*/

#define CHK_ORET(cmd)                                                                \
    do {                                                                             \
        if (!(cmd)) {                                                                \
            mxf_log_error("'%s' failed, in %s:%d\n", #cmd, __FILENAME__, __LINE__);  \
            return 0;                                                                \
        }                                                                            \
    } while (0)

#define CHK_OFAIL(cmd)                                                               \
    do {                                                                             \
        if (!(cmd)) {                                                                \
            mxf_log_error("'%s' failed, in %s:%d\n", #cmd, __FILENAME__, __LINE__);  \
            goto fail;                                                               \
        }                                                                            \
    } while (0)

#define CHK_MALLOC_ORET(var, type) \
    CHK_ORET((var = (type*)malloc(sizeof(type))) != NULL)

#define CHK_MALLOC_OFAIL(var, type) \
    CHK_OFAIL((var = (type*)malloc(sizeof(type))) != NULL)

#define CHK_MALLOC_ARRAY_ORET(var, type, length) \
    CHK_ORET((var = (type*)malloc(sizeof(type) * (length))) != NULL)

#define CHK_MALLOC_ARRAY_OFAIL(var, type, length) \
    CHK_OFAIL((var = (type*)malloc(sizeof(type) * (length))) != NULL)


/*
* Free the memory and set the variable to NULL
*/

#define SAFE_FREE(var)  \
    do {                \
        free(var);      \
        var = NULL;     \
    } while (0)


/*
* Helpers for logging
*/

/* e.g. mxf_log_error("Some error %d" LOG_LOC_FORMAT, x, LOG_LOC_PARAMS); */
#define LOG_LOC_FORMAT      ", in %s:%d\n"
#define LOG_LOC_PARAMS      __FILENAME__, __LINE__



#define ARRAY_SIZE(array)   (sizeof(array) / sizeof((array)[0]))



#ifdef __cplusplus
}
#endif


#endif

