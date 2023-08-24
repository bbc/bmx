/*
 * Implements an AVL binary search tree
 *
 * Copyright (C) 2012, British Broadcasting Corporation
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

#ifndef MXF_TREE_H_
#define MXF_TREE_H_


#ifdef __cplusplus
extern "C"
{
#endif


typedef void (*mxf_tree_free_node_f)(void *node_data);
typedef int (*mxf_tree_compare_node_f)(void *left, void *right);
typedef int (*mxf_tree_process_node_f)(void *node_data, void *process_data);

typedef struct MXFTreeNode
{
    struct MXFTreeNode *left;
    struct MXFTreeNode *right;
    int height;
    void *data;
} MXFTreeNode;

typedef struct
{
    MXFTreeNode *root;
    int allow_duplicates;
    mxf_tree_compare_node_f compare_f;
    mxf_tree_free_node_f free_f;
} MXFTree;



int mxf_tree_create(MXFTree **tree, int allow_duplicates, mxf_tree_compare_node_f compare_f,
                    mxf_tree_free_node_f free_f);
void mxf_tree_free(MXFTree **tree);
void mxf_tree_init(MXFTree *tree, int allow_duplicates, mxf_tree_compare_node_f compare_f,
                   mxf_tree_free_node_f free_f);
void mxf_tree_clear(MXFTree *tree);

int mxf_tree_insert(MXFTree *tree, void *data);
int mxf_tree_remove(MXFTree *tree, void *key);
void* mxf_tree_find(MXFTree *tree, void *key);
int mxf_tree_traverse(MXFTree *tree, mxf_tree_process_node_f process_f, void *process_data);



#ifdef __cplusplus
}
#endif


#endif

