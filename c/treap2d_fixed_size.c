#include "treap2d_fixed_size.h"

Treap2D *
treap2d_new(size_t n)
{
    Treap2D * tree = malloc(sizeof(Treap2D) + n * sizeof(TreapNode));
    tree->last_node = tree->node_pool;
    return tree;
}

void
treap2d_free(Treap2D * tree)
{
    free(tree);
}

void
treap2d_init_with_single_node(Treap2D *tree, double x, double y)
{
    treap_node_init(tree->last_node, x, y);
    treap_init_with_single_node(&tree->treap, tree->last_node);
    tree->last_node++;
}

TreapNode *
treap2d_find_le(const Treap2D *tree, double value)
{
    return treap_find_le(&tree->treap, value);
}

TreapNode **
treap2d_find_le_link(Treap2D *tree, double value)
{
    return treap_find_le_link(&tree->treap, value);
}

double
treap2d_node_get_x(const TreapNode *node)
{
    return node->x;
}

double
treap2d_node_get_y(const TreapNode *node)
{
    return node->y;
}

void
treap2d_validate_tree(Treap2D * tree)
{
    treap_validate_tree(tree->treap.root);
}

TreapNode *
treap2d_erase_at(TreapNode **link)
{
    return treap_erase_at(link);
}

void
treap2d_insert(Treap2D *tree, double x, double y)
{
    treap_node_init(tree->last_node, x, y);
    treap_insert_and_displace(&tree->treap, tree->last_node);
    tree->last_node++;
}
