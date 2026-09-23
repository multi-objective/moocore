#ifndef _TREAP_2D_DLNODE_H
#define _TREAP_2D_DLNODE_H

#include <stdlib.h>

typedef dlnode_t * TreapItem;
#include "treap2d.h"

typedef struct Treap2D {
    Treap treap;
    TreapNode * last_node;
    TreapNode node_pool[]; // Flexible array
} Treap2D;

static Treap2D *
treap2d_new(size_t n)
{
    Treap2D * tree = malloc(sizeof(Treap2D) + n * sizeof(TreapNode));
    tree->last_node = tree->node_pool;
    return tree;
}

static void
treap2d_free(Treap2D * tree)
{
    free(tree);
}

static inline void
treap2d_init_node(TreapNode * restrict node, dlnode_t * restrict p)
{
    // The treap is sorted in ascending p->x[1] and descending p->x[0].
    treap_node_init(node, p->x[1], p->x[0], p);
}

static inline dlnode_t *
treap2d_get_dlnode(TreapNode * node)
{
    return node->item;
}

static void
treap2d_init_with_sentinels(Treap2D *tree, dlnode_t * restrict p, dlnode_t * restrict s2, dlnode_t * restrict s1)
{
    TreapNode * node = tree->last_node;
    treap2d_init_node(node, p);
    treap2d_init_node(node + 1, s2);
    (node+1)->priority = 0; // Push to the bottom.
    treap2d_init_node(node + 2, s1);
    (node+2)->priority = 0; // Push to the bottom.
    treap_init_with_single_node(&tree->treap, node);
    node->left = node + 1;
    node->right = node + 2;
    tree->last_node += 3;
}

static TreapNode *
treap2d_find_le(const Treap2D *tree, double value)
{
    return treap_find_le(&tree->treap, value);
}

static inline void
treap2d_insert_and_displace_get_bounds(Treap2D *tree, dlnode_t * restrict p, TreapNode **prev, TreapNode **next)
{
    treap2d_init_node(tree->last_node, p);
    (void)treap_insert_and_displace_get_bounds(&tree->treap, tree->last_node, prev, next);
    tree->last_node++;
}

#endif // _TREAP_2D_DLNODE_H
