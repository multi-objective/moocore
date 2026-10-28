#ifndef NDTREE_ITERATOR_H
#define NDTREE_ITERATOR_H

typedef struct NDTreeIterator {
    const NDTreeNode *current_node;
    size_t bucket_index;
    size_t solution_index;
    // To detect modifications of the tree while iterating.
    uint64_t revision;
    const NDTreeArchive *arch;
} NDTreeIterator;

static void
ndtree_iter_init(NDTreeIterator * it, const NDTreeArchive *tree)
{
    it->bucket_index = 0;
    it->solution_index = 0;
    it->revision = tree->revision;
    it->arch = tree;
    // Find first leaf.
    const NDTreeNode *node = tree->root;
    if (node != NULL) {
        while (!ndtree_node_is_leaf(node))
            node = &node->children[0];
    }
    it->current_node = node;
}

static inline NDTreeIterator *
ndtree_iter_new(const NDTreeArchive *tree)
{
    if (tree == NULL)
        return NULL;

    NDTreeIterator *it = malloc(sizeof(*it));
    if (it == NULL)
        return NULL;

    ndtree_iter_init(it, tree);
    return it;
}

static inline void
ndtree_iter_free(NDTreeIterator *it)
{
    free(it);
}

static const NDTreeNode *
ndtree_next_leaf(const NDTreeNode *node)
{
    while (node != NULL) {
        // Descend to the next subtree if there is a next sibling.
        const NDTreeNode *parent = node->parent;
        if (!parent)
            return NULL;

        assert(node - parent->children >= 0);
        assert(node - parent->children < 255);
        uint8_t i = 1 + (uint8_t)(node - parent->children);

        assert(parent->n > 0);
        if (i < parent->n) {
            // Move to the next sibling, then descend to its leftmost leaf.
            node = &parent->children[i];

            while (!ndtree_node_is_leaf(node))
                node = &node->children[0];

            return node;
        }
        // No more siblings here. Move up and look for one.
        node = parent;
    }
    return NULL;
}


/**
   1  node returned
   0  end of iteration
   -1  error
*/
static inline int
ndtree_iter_next(NDTreeIterator *it, const double **z, const void **x)
{
    if (it == NULL || z == NULL || it->revision != it->arch->revision)
        return -1;

    while (it->current_node != NULL) {
        const NDTreeNode *node = it->current_node;

        assert(ndtree_node_is_leaf(node));
        // Emit solutions from the current leaf.
        while (it->bucket_index < node->n) {
            const SolutionsList *bucket = &node->bucket[it->bucket_index];
            if (it->solution_index < bucket->x.n) {
                *z = bucket->z;
                if (x)
                    *x = bucket->x.list[it->solution_index];
                it->solution_index++;
                return 1;
            }
            it->bucket_index++;
            it->solution_index = 0;
        }

        // Current leaf exhausted.
        it->current_node = ndtree_next_leaf(node);
        it->bucket_index = 0;
        it->solution_index = 0;
    }
    *z = NULL;
    if (x)
        *x = NULL;
    return 0;
}

#endif // NDTREE_ITERATOR_H
