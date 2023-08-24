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

/*
   This implementation was inspired by 'tree' written by Ian Piumarta
   (http://piumarta.com/software/tree/).
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include <mxf/mxf_tree.h>
#include <mxf/mxf_macros.h>
#include <mxf/mxf_logging.h>


typedef struct
{
    MXFTree *tree;
    int op_status;
} MXFTreeWrap;



static MXFTreeNode* balance_tree(MXFTreeNode *root);

static void free_node(MXFTree *tree, MXFTreeNode **node)
{
    if(!(*node))
        return;

    if ((*node)->left)
        free_node(tree, &(*node)->left);
    if ((*node)->right)
        free_node(tree, &(*node)->right);

    if (tree->free_f)
        tree->free_f((*node)->data);
    SAFE_FREE(*node);
}

static MXFTreeNode* rotate_left(MXFTreeNode *root)
{
    MXFTreeNode *new_root = root->right;
    root->right = root->right->left;
    new_root->left = balance_tree(root);

    return balance_tree(new_root);
}

static MXFTreeNode* rotate_right(MXFTreeNode *root)
{
    MXFTreeNode *new_root = root->left;
    root->left = root->left->right;
    new_root->right = balance_tree(root);

    return balance_tree(new_root);
}

static MXFTreeNode* balance_tree(MXFTreeNode *root)
{
#define HEIGHT_DELTA(node) \
    ((node->left ? node->left->height : 0) - (node->right ? node->right->height : 0))

    int delta = HEIGHT_DELTA(root);

    if (delta < -1) {
        if (HEIGHT_DELTA(root->right) > 0)
            root->right = rotate_right(root->right);
        return rotate_left(root);
    } else if (delta > 1) {
        if (HEIGHT_DELTA(root->left) < 0)
            root->left = rotate_left(root->left);
        return rotate_right(root);
    }

    root->height = 0;
    if (root->left)
        root->height = root->left->height;
    if (root->right && root->right->height > root->height)
        root->height = root->right->height;
    root->height++;

    return root;
}

static MXFTreeNode* insert_node(MXFTreeWrap *tree_wrap, MXFTreeNode *root, MXFTreeNode *node)
{
    int cmp_result;

    if (!root)
        return node;

    cmp_result = tree_wrap->tree->compare_f(root->data, node->data);
    if (cmp_result < 0) {
        root->right = insert_node(tree_wrap, root->right, node);
    } else if (cmp_result > 0 || (cmp_result == 0 && tree_wrap->tree->allow_duplicates)) {
        root->left = insert_node(tree_wrap, root->left, node);
    } else {
        mxf_log_error("Node with same key already exists in tree\n");
        tree_wrap->op_status = 0;
    }

    if (tree_wrap->op_status == 0)
        return root;

    return balance_tree(root);
}

static MXFTreeNode* move_right(MXFTreeNode *root, MXFTreeNode *right)
{
    if (!root)
        return right;

    root->right = move_right(root->right, right);

    return balance_tree(root);
}

static MXFTreeNode* remove_node(MXFTreeWrap *tree_wrap, MXFTreeNode *root, MXFTreeNode *key_node)
{
    int cmp_result;

    if (!root)
        return NULL;

    cmp_result = tree_wrap->tree->compare_f(root->data, key_node->data);
    if (cmp_result == 0) {
        MXFTreeNode *new_root = move_right(root->left, root->right);
        root->left = NULL;
        root->right = NULL;
        free_node(tree_wrap->tree, &root);
        tree_wrap->op_status = 1;
        return new_root;
    }

    if (cmp_result < 0)
        root->right = remove_node(tree_wrap, root->right, key_node);
    else
        root->left = remove_node(tree_wrap, root->left, key_node);

    return balance_tree(root);
}

static MXFTreeNode* find_node(MXFTree *tree, MXFTreeNode *root, MXFTreeNode *key_node)
{
    int cmp_result;

    if (!root)
        return NULL;

    cmp_result = tree->compare_f(root->data, key_node->data);
    if (cmp_result == 0)
        return root;
    else if (cmp_result < 0)
        return find_node(tree, root->right, key_node);
    else
        return find_node(tree, root->left, key_node);
}

static int traverse_node(MXFTreeNode *root, mxf_tree_process_node_f process_f, void *process_data)
{
    if (!root)
        return 1;

    if (!process_f(root->data, process_data))
        return 0;

    return traverse_node(root->left, process_f, process_data) &&
           traverse_node(root->right, process_f, process_data);
}


int mxf_tree_create(MXFTree **tree, int allow_duplicates, mxf_tree_compare_node_f compare_f,
                    mxf_tree_free_node_f free_f)
{
    MXFTree *new_tree;

    CHK_MALLOC_ORET(new_tree, MXFTree);
    mxf_tree_init(new_tree, allow_duplicates, compare_f, free_f);

    *tree = new_tree;
    return 1;
}

void mxf_tree_free(MXFTree **tree)
{
    if (!(*tree))
        return;

    mxf_tree_clear(*tree);
    SAFE_FREE(*tree);
}

void mxf_tree_init(MXFTree *tree, int allow_duplicates, mxf_tree_compare_node_f compare_f,
                   mxf_tree_free_node_f free_f)
{
    tree->root = NULL;
    tree->allow_duplicates = allow_duplicates;
    tree->compare_f = compare_f;
    tree->free_f = free_f;
}

void mxf_tree_clear(MXFTree *tree)
{
    free_node(tree, &tree->root);
    /* leave compare_f and free_f */
}

int mxf_tree_insert(MXFTree *tree, void *data)
{
    MXFTreeWrap tree_wrap;
    MXFTreeNode *new_node;

    CHK_MALLOC_ORET(new_node, MXFTreeNode);
    new_node->left = NULL;
    new_node->right = NULL;
    new_node->height = 0;
    new_node->data = data;

    tree_wrap.tree = tree;
    tree_wrap.op_status = 1;
    tree->root = insert_node(&tree_wrap, tree->root, new_node);

    if (!tree_wrap.op_status)
        SAFE_FREE(new_node);

    return tree_wrap.op_status;
}

int mxf_tree_remove(MXFTree *tree, void *key)
{
    MXFTreeWrap tree_wrap;
    MXFTreeNode key_node;

    key_node.data = key;
    tree_wrap.tree = tree;
    tree_wrap.op_status = 0;
    tree->root = remove_node(&tree_wrap, tree->root, &key_node);

    return tree_wrap.op_status;
}

void* mxf_tree_find(MXFTree *tree, void *key)
{
    MXFTreeNode key_node;
    MXFTreeNode *result = NULL;

    key_node.data = (void*)key;

    result = find_node(tree, tree->root, &key_node);
    if (!result)
        return NULL;

    return result->data;
}

int mxf_tree_traverse(MXFTree *tree, mxf_tree_process_node_f process_f, void *process_data)
{
    return traverse_node(tree->root, process_f, process_data);
}

