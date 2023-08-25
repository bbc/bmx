/*
 * General purpose linked list
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

#ifndef MXF_LIST_H_
#define MXF_LIST_H_


#ifdef __cplusplus
extern "C"
{
#endif


typedef void (*free_func_type)(void *data);
typedef int (*eq_func_type)(void *data, void *info);

typedef struct MXFListElement
{
    struct MXFListElement *next;
    void *data;
} MXFListElement;

typedef struct
{
    MXFListElement *elements;
    MXFListElement *lastElement;
    size_t len;
    free_func_type freeFunc;
} MXFList;

typedef struct
{
    MXFListElement *nextElement;
    void *data;
    size_t index;
} MXFListIterator;


static const size_t MXF_LIST_NPOS = (size_t)(-1);


int mxf_create_list(MXFList **list, free_func_type freeFunc);
void mxf_free_list(MXFList **list);
void mxf_initialise_list(MXFList *list, free_func_type freeFunc);
void mxf_clear_list(MXFList *list);

int mxf_append_list_element(MXFList *list, void *data);
int mxf_prepend_list_element(MXFList *list, void *data);
int mxf_insert_list_element(MXFList *list, size_t index, int before, void *data);
size_t mxf_get_list_length(MXFList *list);

void* mxf_find_list_element(const MXFList *list, void *info, eq_func_type eqFunc);
void* mxf_remove_list_element(MXFList *list, void *info, eq_func_type eqFunc);
void* mxf_remove_list_element_at_index(MXFList *list, size_t index);

void* mxf_get_list_element(MXFList *list, size_t index);

void* mxf_get_first_list_element(MXFList *list);
void* mxf_get_last_list_element(MXFList *list);

void mxf_free_list_element_data(MXFList *list, void *data);

void mxf_initialise_list_iter(MXFListIterator *iter, const MXFList *list);
void mxf_initialise_list_iter_at(MXFListIterator *iter, const MXFList *list, size_t index);
void mxf_copy_list_iter(const MXFListIterator *fromIter, MXFListIterator *toIter);
int mxf_next_list_iter_element(MXFListIterator *iter);
void* mxf_get_iter_element(MXFListIterator *iter);
size_t mxf_get_list_iter_index(MXFListIterator *iter);


#ifdef __cplusplus
}
#endif


#endif


