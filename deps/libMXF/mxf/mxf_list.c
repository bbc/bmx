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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include <mxf/mxf.h>
#include <mxf/mxf_macros.h>



static void* remove_list_element(MXFList *list, MXFListElement *element, MXFListElement *prevElement)
{
    void *data = element->data;

    if (!prevElement) {
        list->elements = element->next;
        if (!list->elements)
            list->lastElement = NULL;
    } else {
        prevElement->next = element->next;
        if (!prevElement->next)
            list->lastElement = prevElement;
    }

    SAFE_FREE(element); /* must free the wrapper element because we only return the data */
    list->len--;

    return data;
}



int mxf_create_list(MXFList **list, free_func_type freeFunc)
{
    MXFList *newList;

    CHK_MALLOC_ORET(newList, MXFList);
    mxf_initialise_list(newList, freeFunc);

    *list = newList;
    return 1;
}

void mxf_free_list(MXFList **list)
{
    if (!(*list))
        return;

    mxf_clear_list(*list);
    SAFE_FREE(*list);
}

void mxf_initialise_list(MXFList *list, free_func_type freeFunc)
{
    memset(list, 0, sizeof(MXFList));
    list->freeFunc = freeFunc;
}

void mxf_clear_list(MXFList *list)
{
    MXFListElement *element;
    MXFListElement *nextElement;

    if (!list)
        return;

    element = list->elements;
    while (element) {
        nextElement = element->next;

        if (list->freeFunc)
            list->freeFunc(element->data);
        SAFE_FREE(element);

        element = nextElement;
    }

    list->elements = NULL;
    list->lastElement = NULL;
    list->len = 0;
}

int mxf_append_list_element(MXFList *list, void *data)
{
    MXFListElement *newElement;

    CHK_ORET(list->len + 1 != MXF_LIST_NPOS);

    CHK_MALLOC_ORET(newElement, MXFListElement);
    memset(newElement, 0, sizeof(MXFListElement));
    newElement->data = data;

    if (!list->elements)
        list->elements = newElement;
    else
        list->lastElement->next = newElement;
    list->lastElement = newElement;

    list->len++;
    return 1;
}

int mxf_prepend_list_element(MXFList *list, void *data)
{
    MXFListElement *newElement;

    CHK_ORET(list->len + 1 != MXF_LIST_NPOS);

    CHK_MALLOC_ORET(newElement, MXFListElement);
    memset(newElement, 0, sizeof(MXFListElement));
    newElement->data = data;

    if (!list->elements) {
        list->elements = newElement;
        list->lastElement = newElement;
    } else {
        newElement->next = list->elements;
        list->elements = newElement;
    }

    list->len++;
    return 1;
}

int mxf_insert_list_element(MXFList *list, size_t index, int before, void *data)
{
    MXFListElement *newElement;
    MXFListElement *nextElement;
    MXFListElement *prevElement;
    size_t currentIndex;

    CHK_ORET(list->len + 1 != MXF_LIST_NPOS);
    if (index == MXF_LIST_NPOS)
        return 0;

    /* create new element */
    CHK_MALLOC_ORET(newElement, MXFListElement);
    memset(newElement, 0, sizeof(MXFListElement));
    newElement->data = data;

    /* special case when list is empty */
    if (!list->elements) {
        list->elements = newElement;
        list->lastElement = newElement;
        list->len++;
        return 1;
    }

    nextElement = list->elements;
    prevElement = NULL;
    currentIndex = 0;
    if (before) {
        /* move to index position */
        while (currentIndex < index && nextElement) {
            prevElement = nextElement;
            nextElement = nextElement->next;
            currentIndex++;
        }
        if (currentIndex != index)
            goto fail;
    } else {
        /* move to after index position */
        while (currentIndex <= index && nextElement) {
            prevElement = nextElement;
            nextElement = nextElement->next;
            currentIndex++;
        }
        if (currentIndex != index + 1)
            goto fail;
    }

    /* insert element */
    if (!prevElement)
        list->elements = newElement;
    else
        prevElement->next = newElement;
    newElement->next = nextElement;
    if (!newElement->next)
        list->lastElement = newElement;

    list->len++;
    return 1;

fail:
    SAFE_FREE(newElement);
    return 0;
}

size_t mxf_get_list_length(MXFList *list)
{
    return list->len;
}

void* mxf_find_list_element(const MXFList *list, void *info, eq_func_type eqFunc)
{
    MXFListElement *element = list->elements;

    while (element && !eqFunc(element->data, info))
        element = element->next;

    return (element ? element->data : NULL);
}

void* mxf_remove_list_element(MXFList *list, void *info, eq_func_type eqFunc)
{
    MXFListElement *element = list->elements;
    MXFListElement *prevElement = NULL;

    while (element && !eqFunc(element->data, info)) {
        prevElement = element;
        element = element->next;
    }
    if (!element)
        return NULL;

    return remove_list_element(list, element, prevElement);
}

void* mxf_remove_list_element_at_index(MXFList *list, size_t index)
{
    size_t currentIndex = 0;
    MXFListElement *element = list->elements;
    MXFListElement *prevElement = NULL;

    if (index == MXF_LIST_NPOS || index >= list->len)
        return NULL;

    while (currentIndex != index) {
        currentIndex++;
        prevElement = element;
        element = element->next;
    }

    return remove_list_element(list, element, prevElement);
}

void* mxf_get_list_element(MXFList *list, size_t index)
{
    size_t currentIndex = 0;
    MXFListElement *element = list->elements;

    if (index == MXF_LIST_NPOS || index >= list->len)
        return NULL;

    if (index == 0)
        return list->elements->data;
    if (index == list->len - 1)
        return list->lastElement->data;

    while (currentIndex != index) {
        currentIndex++;
        element = element->next;
    }

    return element->data;
}

void* mxf_get_first_list_element(MXFList *list)
{
    if (!list->elements)
        return NULL;
    return list->elements->data;
}

void* mxf_get_last_list_element(MXFList *list)
{
    if (!list->lastElement)
        return NULL;
    return list->lastElement->data;
}

void mxf_free_list_element_data(MXFList *list, void *data)
{
    if (list->freeFunc)
        list->freeFunc(data);
}

void mxf_initialise_list_iter(MXFListIterator *iter, const MXFList *list)
{
    iter->nextElement = list->elements;
    iter->data = NULL;
    iter->index = MXF_LIST_NPOS;
}

void mxf_initialise_list_iter_at(MXFListIterator *iter, const MXFList *list, size_t index)
{
    if (index == MXF_LIST_NPOS) {
        mxf_initialise_list_iter(iter, list);
    } else {
        iter->nextElement = list->elements;
        iter->data = NULL;
        iter->index = 0;

        while (iter->index != index && iter->nextElement) {
            iter->index++;
            iter->nextElement = iter->nextElement->next;
        }
    }
}

void mxf_copy_list_iter(const MXFListIterator *fromIter, MXFListIterator *toIter)
{
    toIter->nextElement = fromIter->nextElement;
    toIter->data = fromIter->data;
    toIter->index = fromIter->index;
}

int mxf_next_list_iter_element(MXFListIterator *iter)
{
    if (iter->nextElement) {
        iter->data = iter->nextElement->data;
        iter->nextElement = iter->nextElement->next;
    } else {
        iter->data = NULL;
    }

    if (iter->data)
        iter->index++;
    else
        iter->index = MXF_LIST_NPOS;

    return iter->data != NULL;
}

void* mxf_get_iter_element(MXFListIterator *iter)
{
    return iter->data;
}

size_t mxf_get_list_iter_index(MXFListIterator *iter)
{
    return iter->index;
}
