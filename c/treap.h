/*****************************************************************************

 treap.h: Randomised binary search tree (Treap) for a 2D mutually nondominated
          frontier.

 ---------------------------------------------------------------------
                       Copyright (C) 2026
          Manuel Lopez-Ibanez  <manuel.lopez-ibanez@manchester.ac.uk>

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.

 ---------------------------------------------------------------------

 [1] R. Seidel and C. R. Aragon. Randomized search trees.  Algorithmica,
     16:464-497, 1996

******************************************************************************/
#ifndef TREAP_H
#define TREAP_H

/*

   Randomized binary search tree for a 2-D mutually non-dominated
   minimization frontier.

   key is ordered by increasing item[0]

   Frontier invariant:

       x (item[0]) increases strictly from left to right
       y (item[1]) decreases strictly from left to right

   Consequently, for a new point (x,y), the old frontier points
   dominated by the new point form a contiguous range in key order.
   This allows the whole dominated range to be detached with
   split/merge operations instead of deleting its nodes individually.

   Nodes are externally owned. The treap does not allocate or free them.

   Expected complexity:

       search             O(log n)
       split              O(log n)
       merge              O(log n)
       insert             O(log n)
       insert+displace    O(log n)

   Worst-case complexity of a treap operation is O(n), as for any
   randomized BST.
*/
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef struct TreapNode TreapNode;
struct TreapNode {
    double key;
    uint64_t priority; // Parents should have a higher priority than children.
    TreapNode *left;
    TreapNode *right;
    TreapItem item;
};

typedef struct {
    TreapNode *root;
    uint64_t rng_state;
} Treap;


/**
   PRNG is xorshift64*, which is not cryptographically safe but it is fast.
*/
static inline uint64_t
treap_random(Treap *tree)
{
    uint64_t x = tree->rng_state;
    assert(x != 0); // State must never be zero.

    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;

    tree->rng_state = x;
    return x * UINT64_C(0x2545F4914F6CDD1D);
}


/**
   Treap Seed generation

   Use the address of the Treap object as process-dependent entropy, then
   thoroughly mix it with SplitMix64's mixing function.

   This is not intended as cryptographic randomness. Its purpose is simply
   to give separately allocated treaps normally different PRNG streams,
   without requiring global mutable state or an OS RNG call.
*/
static inline uint64_t
treap_seed(const Treap *tree)
{
    // Cast to uintptr_t to use the pointer value as an integer.
    uint64_t x = (uint64_t)(uintptr_t)tree;
    /* Mix the address. The addition is useful even though a fixed constant
       would ultimately be mixed anyway.  */
    x += UINT64_C(0x9e3779b97f4a7c15);

    x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
    x ^= x >> 31;
    // xorshift64* cannot leave the zero state.
    return x ? x : UINT64_C(0x9e3779b97f4a7c15);
}


/**
   (Re)-Initializes the tree structure. Nothing is free()d.

   Complexity: O(1)
*/
static inline void
treap_init(Treap *tree)
{
    tree->root = NULL;
    tree->rng_state = treap_seed(tree);
}

/*
   Initialize a node before inserting it.

   The node does not receive a priority until insertion because the
   priority should come from the destination treap's RNG.
*/
static inline void
treap_node_init(TreapNode *node, double key, TreapItem item)
{
    assert(node != NULL);
    node->key = key;
    node->left = NULL;
    node->right = NULL;
    node->item = item;
}

static inline void
treap_init_with_single_node(Treap *tree, TreapNode * node)
{
    assert(node->left == NULL);
    assert(node->right == NULL);
    treap_init(tree);
    tree->root = node;
    node->priority = treap_random(tree);
}

static inline void
treap_split_lt_with_max(TreapNode *root, double value,
                        TreapNode **left, TreapNode **right,
                        TreapNode **left_max)
{
    if (root == NULL) {
        *left = NULL;
        *right = NULL;
        if (left_max)
            *left_max = NULL;
        return;
    }

    if (root->key < value) {
        *left = root;
        if (left_max) {
            TreapNode *sub_left_max;
            treap_split_lt_with_max(root->right, value, &root->right, right,
                                    &sub_left_max);
            /* If anything remained in root->right, its maximum is the maximum of
               the whole left result. Otherwise root is the max.  */
            *left_max = sub_left_max != NULL ? sub_left_max : root;
        } else {
            treap_split_lt_with_max(root->right, value, &root->right, right, NULL);
        }
    } else {
        *right = root;
        treap_split_lt_with_max(root->left, value, left, &root->left, left_max);
    }
}

/**
   Split by key.

   On return:

       *left  contains every node with key < value
       *right contains every node with key >= value

   The frontier and treap invariants are preserved.

   Expected complexity: O(log n).
*/
static inline void
treap_split_lt(TreapNode *root, double value,
               TreapNode **left, TreapNode **right)
{
    treap_split_lt_with_max(root, value, left, right, NULL);
}


/**
   Split by value.

   On return:

       *left  contains every node with key <= value
       *right contains every node with key > value

   The frontier and treap invariants are preserved.

   Expected complexity: O(log n).
*/
static inline void
treap_split_le(TreapNode *root, double value,
               TreapNode **left, TreapNode **right)
{
    if (root == NULL) {
        *left = NULL;
        *right = NULL;
        return;
    }

    if (root->key <= value) {
        // root belongs to the left result.
        *left = root;
        treap_split_le(root->right, value, &root->right, right);
    } else {
        // root belongs to the right result.
        *right = root;
        treap_split_le(root->left, value, left, &root->left);
    }
}

static inline void
treap_split_z1_ge_with_min(TreapNode *root, double y,
                           TreapNode **high_y, TreapNode **low_y,
                           TreapNode **low_y_min)
{
    if (root == NULL) {
        *high_y = NULL;
        *low_y = NULL;
        if (low_y_min)
            *low_y_min = NULL;
        return;
    }

    if (treap_item_get_z(root->item)[1] >= y) {
        *high_y = root;
        treap_split_z1_ge_with_min(root->right, y, &root->right, low_y, low_y_min);
    } else {
        *low_y = root;
        if (low_y_min) {
            TreapNode *sub_low_min;
            treap_split_z1_ge_with_min(root->left, y, high_y, &root->left, &sub_low_min);
            /* If root has a left part remaining, its minimum is the minimum of
               that part. Otherwise root is the minimum.  */
            if (low_y_min)
                *low_y_min = sub_low_min != NULL ? sub_low_min : root;
        } else {
            treap_split_z1_ge_with_min(root->left, y, high_y, &root->left, NULL);
        }
    }
}

/**
   Split a frontier by y.

   PRECONDITION:

       root is a valid mutually non-dominated frontier.

   Since y strictly decreases as x increases:

       *high_y contains every node with item[1] >= y
       *low_y  contains every node with item[1] <  y

   Therefore *high_y is a prefix in key order.

   This is the operation used to detach all nodes dominated by a new
   frontier point.

   Expected complexity: O(log n).
*/
static inline void
treap_split_z1_ge(TreapNode *root, double y,
                  TreapNode **high_y, TreapNode **low_y)
{
    treap_split_z1_ge_with_min(root, y, high_y, low_y, NULL);
}

static TreapNode *
treap_rotate_left(TreapNode *node)
{
    TreapNode *right = node->right;
    node->right = right->left;
    right->left = node;
    return right;
}

static TreapNode *
treap_rotate_right(TreapNode *node)
{
    TreapNode *left = node->left;
    node->left = left->right;
    left->right = node;
    return left;
}

static TreapNode *
treap_insert_node_and_rotate(TreapNode * root, TreapNode *node)
{
    if (!root)
        return node;

    if (node->key < root->key) {
        root->left = treap_insert_node_and_rotate(root->left, node);
        assert(root->left != NULL);
        if (root->left->priority > root->priority)
            root = treap_rotate_right(root);
    } else {
        root->right = treap_insert_node_and_rotate(root->right, node);
        assert(root->right != NULL);
        if (root->right->priority > root->priority)
            root = treap_rotate_left(root);
    }
    return root;
}

/**
   Merge two treaps.

   PRECONDITION:

       Every key in left is less than every key in right.

   Returns the merged treap.

   Expected complexity: O(log n).
*/
static inline TreapNode *
treap_merge(TreapNode *left, TreapNode *right)
{
    if (!left)
        return right;

    if (!right)
        return left;

    if (left->priority > right->priority) {
        left->right = treap_merge(left->right, right);
        return left;
    }

    right->left = treap_merge(left, right->left);
    return right;
}

static inline void
treap_validate_tree(TreapNode * node)
{
    if (!node)
        return;

    if (node->left) {
        assert(node->left->key < node->key);
        assert(node->priority > node->left->priority);
        treap_validate_tree(node->left);
    }
    if (node->right) {
        assert(node->right->key > node->key);
        assert(node->priority > node->right->priority);
        treap_validate_tree(node->right);
    }
}
/**
   Find the node with the largest key such that key <= value.

   Returns NULL if no such node exists.

   Expected complexity: O(log n).
*/
static inline TreapNode *
treap_find_le(const Treap *tree, double value)
{
    TreapNode *node = tree->root, *best = NULL;

    while (node != NULL) {
        if (node->key <= value) {
            best = node;
            node = node->right;
        } else {
            node = node->left;
        }
    }
    return best;
}

static inline TreapNode **
treap_find_le_link(Treap *tree, double value)
{
    TreapNode **link = &tree->root, **best = NULL;

    while (*link != NULL) {
        TreapNode *node = *link;

        if (node->key <= value) {
            best = link;
            link = &node->right;
        } else {
            link = &node->left;
        }
    }

    return best;
}

/**
   Find the node with the largest key such that key < value.

   Returns NULL if no such node exists.

   Expected complexity: O(log n).
*/
static inline TreapNode *
treap_find_lt(const Treap *tree, double value)
{
    TreapNode *node = tree->root, *best = NULL;

    while (node != NULL) {
        if (node->key < value) {
            best = node;
            node = node->right;
        } else {
            node = node->left;
        }
    }
    return best;
}
/**
   Find the node with the smallest key such that key >= value.

   Returns NULL if no such node exists.

   Expected complexity: O(log n).
*/
static inline TreapNode *
treap_find_ge(const Treap *tree, double value)
{
    TreapNode *node = tree->root, *best = NULL;

    while (node != NULL) {
        if (node->key >= value) {
            best = node;
            node = node->left;
        } else {
            node = node->right;
        }
    }
    return best;
}

/**
   Find the node with the smallest key such that key > value.

   Returns NULL if no such node exists.

   Expected complexity: O(log n).
*/
static inline TreapNode *
treap_find_gt(const Treap *tree, double value)
{
    TreapNode *node = tree->root, *best = NULL;

    while (node != NULL) {
        if (node->key > value) {
            best = node;
            node = node->left;
        } else {
            node = node->right;
        }
    }
    return best;
}


/**
   Find a node with exactly key == value.

   Returns NULL if absent.

   Expected complexity: O(log n).
 */
static inline TreapNode *
treap_find(const Treap *tree, double value)
{
    TreapNode *node = tree->root;

    while (node) {
        if (value < node->key)
            node = node->left;
        else if (value > node->key)
            node = node->right;
        else
            return node;
    }
    return NULL;
}

/**
   Insert a single node.

   PRECONDITIONS:

       - node is initialized with treap_node_init().
       - node->key does not already exist in the treap.
       - node->left == NULL.
       - node->right == NULL.
       - node->item[0] == node->key.

   This function does not check whether the resulting set is a valid
   Pareto frontier. Use treap_insert_and_displace() when inserting
   a new non-dominated point into an existing frontier.
*/
static inline void
treap_insert_node(Treap *tree, TreapNode *node)
{
    assert(tree != NULL);
    assert(node != NULL);
    assert(node->left == NULL);
    assert(node->right == NULL);
    // node->key is not already in the tree.
    assert(treap_find(tree, node->key) == NULL);
    DEBUG1(treap_validate_tree(tree->root));

    node->priority = treap_random(tree);

#if 0
    TreapNode *left;
    TreapNode *right;
    /* Since keys are guaranteed unique by the caller, there is no need to
       search for an existing key before doing the split.  */
    treap_split_lt(tree->root, node->key, &left, &right);
    tree->root = treap_merge(treap_merge(left, node), right);
#else
    tree->root = treap_insert_node_and_rotate(tree->root, node);
#endif
    DEBUG1(treap_validate_tree(tree->root));
}

static inline TreapNode *
treap_erase_at(TreapNode **link)
{
    assert(link != NULL);
    assert(*link != NULL);

    TreapNode *node = *link;
    *link = treap_merge(node->left, node->right);
    node->left = NULL;
    node->right = NULL;
    return node;
}

/**
   Inserts node into tree and detaches all nodes dominated by node.

   On return:

     *prev = node's predecessor in the resulting tree
     *next = node's successor in the resulting tree

   Returns the root of a separate treap containing the displaced nodes.

   Expected complexity: O(log n).

   FIXME: This function performs 4 tree traversals.  It may be possible to do
   it faster while rotating nodes as needed.
*/
static inline TreapNode *
treap_insert_and_displace_get_bounds(Treap *tree, TreapNode *node,
                                     TreapNode **prev, TreapNode **next)
{
    assert(tree != NULL);
    assert(node != NULL);
    assert(node->left == NULL);
    assert(node->right == NULL);

    const double x = node->key;
    const double y = treap_item_get_z(node->item)[1];
    TreapNode *left, *right;
    if (prev) {
        TreapNode *left_max;
        treap_split_lt_with_max(tree->root, x, &left, &right, &left_max);
        *prev = left_max;
    } else {
        treap_split_lt(tree->root, x, &left, &right);
    }

    /* On the frontier, z[1] decreases with increasing key.  Therefore the
       nodes in right with z[1] >= y are exactly the nodes dominated by
       node, and they form a contiguous prefix.  */
    TreapNode *displaced, *keep;
    if (next) {
        TreapNode *keep_min;
        treap_split_z1_ge_with_min(right, y, &displaced, &keep, &keep_min);
        *next = keep_min;
    } else {
        treap_split_z1_ge(right, y, &displaced, &keep);
    }

    // Insert the new node between left and keep.
    node->priority = treap_random(tree);
    node->left = NULL;
    node->right = NULL;
    // Key ordering is: left < node < keep
    tree->root = treap_merge(treap_merge(left, node), keep);
    DEBUG1(treap_validate_tree(tree->root));
    return displaced;
}

/**
   Insert a new point into a mutually non-dominated frontier and detach all
   existing nodes dominated by it.

   PRECONDITIONS:

       - tree is a valid mutually non-dominated frontier.
       - node is not already in tree.
       - node->key is unique in tree.
       - no node in tree weakly dominates node.
       - node->left == NULL.
       - node->right == NULL.
       - node->key == node->item[0].

   POSTCONDITIONS:

       - node belongs to tree.
       - the returned root is a separate treap containing exactly the nodes
         removed from tree.
       - every returned node is dominated by node.
       - no node other than those dominated by node is removed.

   Expected complexity: O(log n).
*/
static inline TreapNode *
treap_insert_and_displace(Treap *tree, TreapNode *node)
{
    return treap_insert_and_displace_get_bounds(tree, node, NULL, NULL);
}

#endif /* TREAP_H */
