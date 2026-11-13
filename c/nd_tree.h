/******************************************************************************

 nd_tree.h: ND-Tree Nondominated Archive.

 ---------------------------------------------------------------------
                       Copyright (C) 2026
          Jonathan Fieldsend <J.E.Fieldsend@exeter.ac.uk>
          Manuel Lopez-Ibanez  <manuel.lopez-ibanez@manchester.ac.uk>

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.

 ---------------------------------------------------------------------

 Literature:

 [1] Jaszkiewicz, A., Lust, T.: ND-tree-based update: A fast algorithm for the
     dynamic nondominance problem. IEEE Transactions on Evolutionary
     Computation 22(5), 778-791 (2018).
     https://doi.org/10.1109/TEVC.2018.2799684

 [2] Jaszkiewicz, A., Zielniewicz, P.: Improving the efficiency of the
     distance-based hypervolume estimation using ND-Tree. IEEE Transactions on
     Evolutionary Computation 29(3), 726-733 (2025).
     https://doi.org/10.1109/TEVC.2024.3391857

 [3] Lang, B.: Space-partitioned ND-Trees for the dynamic nondominance problem.
     IEEE Transactions on Evolutionary Computation 26(5), 1004-1014 (2022).
     https: //doi.org/10.1109/TEVC.2022.3145631

 [4] Fieldsend, J.E.: Data structures for non-dominated sets: implementations
     and empirical assessment of two decades of advances. In: Coello Coello,
     C.A. (ed.) Proceedings of the Genetic and Evolutionary Computation
     Conference, GECCO 2020, pp. 489–497. ACM Press, New York, NY (2020).
     https://doi.org/10.1145/3377930.3390150

 [5] Fieldsend, J.E.: On the active use of an ND-Tree-based archive for
     multi-objective optimisation. In: Legrand, P., et al. (eds.) EA 2023:
     Artificial Evolution, pp. 1-14. Lecture Notes in Computer Science,
     Springer (2023).  https://doi.org/10.1007/ 978-3-031-42616-2_1


 FIXME: The code below does not implement all the improvements described in the
 above papers.

 *****************************************************************************/
#ifndef ND_TREE_ARCHIVE_H
#define ND_TREE_ARCHIVE_H

#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h> // memcpy

#include "sort.h"
#include "maxminclamp.h"
#include "nondominated.h"
#include "archiving_priv.h"
#include "solutions_list.h"
#include "flex_bucket.h"

/***
    FIXME: The code would be faster if max_bucket_size and/or max_children were
    compile-time constants. We could reserve space for 20 max_children but only
    use dim+1.
*/

/**
   ND-tree node.  Leaves own buckets; internal nodes own children.  The
   ideal/nadir/midpoint arrays are conservative bounds used to prune dominance
   checks before descending into subtrees.
*/
typedef struct NDTreeNode {
    struct NDTreeNode *parent;
    // FIXME: Use an anonymous union
    SolutionsList *bucket; // List of solutions (each list tracks duplicates).
    struct NDTreeNode *children;
    // box is a pointer to a block of 3 * dim points (ideal, nadir, midpoint).
    // (ideal, nadir) define a hyper-rectangle that contains all points in the
    // subtree.  The hyper-rectangle is not necessarily tight.
    double *box;
    // This is total_size of subtree (including duplicates). Leaf nodes have
    // coverage == 0 and we use node->n.
    uint32_t coverage;
    uint8_t n;
    uint8_t cap;
} NDTreeNode;

typedef struct NDTreeConfig {
    uint8_t max_children;
    uint8_t max_bucket_size;
} NDTreeConfig;

typedef struct NDTreeArchive {
    /* It would be faster if this was not a pointer, but then we cannot
       relocate the tree in memory, which is needed for NDS-Forest.  */
    NDTreeNode * root;
    dimension_t dim;
    BaseArchive base;
    NDTreeConfig config;
} NDTreeArchive;


static inline double *
ndtree_get_ideal(const NDTreeNode * restrict node, dimension_t dim _attr_maybe_unused)
{
    return node->box;
}

static inline double *
ndtree_get_nadir(const NDTreeNode * restrict node, dimension_t dim)
{
    return node->box + dim;
}

static inline double *
ndtree_get_midpoint(const NDTreeNode * restrict node, dimension_t dim)
{
    return node->box + 2 * dim;
}

static inline SolutionsList *
ndtree_bucket_new(NDTreeNode *node, uint8_t cap)
{
    node->n = 0;
    node->cap = cap;
    node->bucket = malloc(sizeof(*node->bucket) * node->cap);
    return node->bucket;
}

static inline void
ndtree_bucket_free(NDTreeNode *node)
{
    for (int i = 0; i < node->n; i++)
        solutions_list_free(&node->bucket[i]);
    free(node->bucket);
    node->bucket = NULL;
    node->n = 0;
}

_attr_pure_func
static inline bool
ndtree_node_is_leaf(const NDTreeNode *node)
{
    return node->coverage == 0;
}

static void
ndtree_node_set_bounds(NDTreeNode * restrict node, const double * restrict z, dimension_t dim)
{
    ASSUME(dim >= 2);
    memcpy(ndtree_get_ideal(node, dim), z, dim * sizeof(*z));
    memcpy(ndtree_get_nadir(node, dim), z, dim * sizeof(*z));
    memcpy(ndtree_get_midpoint(node, dim), z, dim * sizeof(*z));
}

static inline bool
update_ideal(NDTreeNode * restrict node, const double * restrict z, dimension_t dim)
{
    ASSUME(dim > 1);
    bool any_changed = false;
    double * restrict ideal = ndtree_get_ideal(node, dim);
    for (dimension_t i = 0; i < dim; i++) {
        if (z[i] < ideal[i]) {
            ideal[i] = z[i];
            any_changed = true;
        }
    }
    return any_changed;
}

static inline bool
update_nadir(NDTreeNode * restrict node, const double * restrict z, dimension_t dim)
{
    ASSUME(dim > 1);
    bool any_changed = false;
    double * restrict nadir = ndtree_get_nadir(node, dim);
    for (dimension_t i = 0; i < dim; i++) {
        if (z[i] > nadir[i]) {
            nadir[i] = z[i];
            any_changed = true;
        }
    }
    return any_changed;
}

static inline void
update_midpoint(NDTreeNode * restrict node, dimension_t dim)
{
    ASSUME(dim > 1);
    const double * restrict ideal = ndtree_get_ideal(node, dim);
    const double * restrict nadir = ndtree_get_nadir(node, dim);
    double * restrict midpoint = ndtree_get_midpoint(node, dim);
    for (dimension_t i = 0; i < dim; i++) {
        // With doubles, we do not expect the sum to overflow and this is
        // faster than ideal + (nadir - ideal)/2.
        midpoint[i] = 0.5 * (ideal[i] + nadir[i]);
    }
}

static inline bool
ndtree_node_update_bounds(NDTreeNode * restrict node,
                          const double * restrict z, dimension_t dim,
                          bool * restrict ideal_changed, bool * restrict nadir_changed)
{
    if (*ideal_changed)
        *ideal_changed = update_ideal(node, z, dim);
    if (*nadir_changed)
        *nadir_changed = update_nadir(node, z, dim);

    if (!*ideal_changed && !*nadir_changed)
        return false;
    update_midpoint(node, dim);
    return true;
}

static void
ndtree_node_update_bounds_up(NDTreeNode * restrict node, const double * restrict z, dimension_t dim)
{
    ASSUME(dim >= 2);
    // Iterative instead of recursive because it is faster and helps inlining.
    bool ideal_changed = true, nadir_changed = true;
    do {
        if (!ndtree_node_update_bounds(node, z, dim, &ideal_changed, &nadir_changed))
            return;
        node = node->parent;
    } while (node);
}

static void
ndtree_node_propagate_bounds_up(NDTreeNode *node, dimension_t dim)
{
    ASSUME(dim >= 2);
    // Propagate the new bounds up the tree.
    bool ideal_changed = true, nadir_changed = true;
    while (node->parent) {
        if (ideal_changed)
            ideal_changed = update_ideal(node->parent, ndtree_get_ideal(node, dim), dim);
        if (nadir_changed)
            nadir_changed = update_nadir(node->parent, ndtree_get_nadir(node, dim), dim);
        if (!ideal_changed && !nadir_changed)
            return;
        update_midpoint(node->parent, dim);
        node = node->parent;
    }
}


/**
   Compute the bounds for a leaf node by looking at the z vectors of its buckets.
*/
static void
ndtree_node_compute_bounds_of_leaf(NDTreeNode *node, dimension_t dim)
{
    assert(ndtree_node_is_leaf(node));
    assert(node->n > 0);
    ndtree_node_set_bounds(node, node->bucket[0].z, dim);
    bool any_changed = false;
    for (uint8_t i = 1; i < node->n; i++) {
        any_changed |= update_ideal(node, node->bucket[i].z, dim);
        any_changed |= update_nadir(node, node->bucket[i].z, dim);
    }
    if (any_changed)
        update_midpoint(node, dim);
}

/**
   For a non-leaf node, whose children are all leaves, (re-)compute the bounds
   of each child.  This function does not propagate the bounds up the tree.
   See ndtree_node_propagate_bounds_up()
*/
static void
ndtree_node_compute_bounds_of_children(NDTreeNode *node, dimension_t dim)
{
    assert(!ndtree_node_is_leaf(node));
    // Compute the bounds of the children.
    for (uint8_t i = 0; i < node->n; i++) {
        NDTreeNode * child = node->children + i;
        ndtree_node_compute_bounds_of_leaf(child, dim);
    }
}

/**
   For a non-leaf node, whose children are all leaves with correct bounds,
   (re-)compute the bounds of the node.  This function does not
   propagate the bounds up the tree.  See ndtree_node_propagate_bounds_up()
*/
static void
ndtree_node_compute_bounds_from_children(NDTreeNode *node, dimension_t dim)
{
    assert(!ndtree_node_is_leaf(node));
    // Initialize with the bounds of the first child.
    NDTreeNode * child = node->children;
    memcpy(node->box, child->box, 3 * dim * sizeof(double));

    // Examine the bounds of the other children.
    bool any_changed = false;
    for (uint8_t i = 1; i < node->n; i++) {
        child = node->children + i;
        any_changed |= update_ideal(node, ndtree_get_ideal(child, dim), dim);
        any_changed |= update_nadir(node, ndtree_get_nadir(child, dim), dim);
    }
    if (any_changed)
        update_midpoint(node, dim);
}


/**
   Copy bucket to the next element in node->bucket. It does NOT update bounds.
*/
static inline void
bucket_append_move(NDTreeNode * restrict node, const SolutionsList * restrict bucket)
{
    assert(node->n < node->cap);
    solutions_list_copy(&node->bucket[node->n], bucket);
    node->n++;
}

/**
   bucket[] has max_bucket_size + 1 capacity, so we should never go over this
   capacity and there is no need to reallocate.

   return true if it succeeds, false if not enough memory.
 */
static inline bool
bucket_append(NDTreeNode * restrict node, const double * restrict z, const void * restrict x, dimension_t dim)
{
    assert(node->n < node->cap);
    if (!solutions_list_init(&node->bucket[node->n], z, x, dim))
        return false;
    node->n++;
    return true;
}

static size_t ndtree_assert_valid_down(const NDTreeNode * node, dimension_t dim);
static void ndtree_assert_valid_up(const NDTreeNode *node, dimension_t dim);
static void ndtree_assert_valid_tree(const NDTreeArchive * tree);

/**
   Initialize a new leaf node.
*/
static NDTreeNode *
ndtree_node_init(NDTreeNode *node, dimension_t dim, uint8_t max_bucket_size, NDTreeNode *parent)
{
    node->parent = parent;
    // Allocate a single block for ideal/nadir/midpoint.
    node->box = malloc(3U * dim * sizeof(double));
    if (node->box == NULL) {
        // PyErr_NoMemory();
        return NULL;
    }
    node->coverage = 0;
    // cap is max_bucket_size + 1 to be able to detect that the bucket is full.
    if (ndtree_bucket_new(node, max_bucket_size + 1) == NULL) {
        free(node->box);
        return NULL;
    }
    return node;
}

/**
   Allocate and initialize a new leaf node.
*/
static inline NDTreeNode *
ndtree_node_new(dimension_t dim, uint8_t max_bucket_size, NDTreeNode *parent)
{
    NDTreeNode *node = malloc(sizeof *node);
    if (node == NULL) {
        // PyErr_NoMemory();
        return NULL;
    }
    return ndtree_node_init(node, dim, max_bucket_size, parent);
}

static inline NDTreeNode *
ndtree_init(NDTreeArchive *tree, dimension_t dim,
            uint8_t max_children, uint8_t max_bucket_size,
            bool allow_duplicates, bool x_is_null)
{
    if (dim < 2 || dim >= MOOCORE_HV_DIMENSION_MAX) {
        // PyErr_SetString(PyExc_ValueError, "number_of_objectives must be at least 2 and less than 255");
        return NULL;
    }

    if (max_children == 0)
        max_children = dim + 1; // default value

    if (max_bucket_size < max_children)
        max_bucket_size = (uint8_t) MAX((int) max_children, 20); // default value

    tree->dim = dim;
    tree->config.max_children = max_children;
    tree->config.max_bucket_size = max_bucket_size;
    tree->base.allow_duplicates = allow_duplicates;
    tree->base.revision = 0;
    tree->base.x_is_null = x_is_null;
    tree->root = ndtree_node_new(dim, max_bucket_size, NULL);
    return tree->root;
}

static inline NDTreeArchive *
ndtree_new(dimension_t dim, uint8_t max_children, uint8_t max_bucket_size,
           bool allow_duplicates, bool x_is_null)
{
    NDTreeArchive * tree = malloc(sizeof(*tree));
    if (tree == NULL) {
        free(tree);
        return NULL;
    }
    if (!ndtree_init(tree, dim, max_children, max_bucket_size, allow_duplicates, x_is_null)) {
        free(tree);
        return NULL;
    }
    return tree;
}

static inline bool
ndtree_init_with_single_solution(NDTreeArchive *restrict archive,
                                 const double * restrict z, const void * restrict x,
                                 dimension_t dim, const NDTreeConfig * restrict config,
                                 bool allow_duplicates)
{
    ndtree_init(archive, dim, config->max_children, config->max_bucket_size, allow_duplicates, (x == NULL));
    if (!bucket_append(archive->root, z, x, dim))
        return false;
    assert(archive->root->n == 1);
    ndtree_node_set_bounds(archive->root, z, dim);
    DEBUG1(ndtree_assert_valid_tree(archive));
    return true;
}

static void
ndtree_node_free(NDTreeNode *node);

static void
ndtree_node_free_children(NDTreeNode *node)
{
    assert(node->children != NULL);
    for (size_t i = 0; i < node->n; i++)
        ndtree_node_free(&node->children[i]);
    free(node->children);
    node->children = NULL;
}
/**
   Free the memory consumed by node and all its children.

*/
static void
ndtree_node_free(NDTreeNode *node)
{
    if (node == NULL)
        return;

    free(node->box);
    node->box = NULL;
    node->cap = 0;
    if (ndtree_node_is_leaf(node)) {
        ndtree_bucket_free(node);
        return;
    }
    ndtree_node_free_children(node);
}

static void
ndtree_free_nodes(NDTreeArchive *tree)
{
    ndtree_node_free(tree->root);
    free(tree->root);
}

static inline bool
ndtree_node_is_empty(const NDTreeNode *node)
{
    assert(node != NULL);
    return node->n == 0;
}

static size_t
ndtree_node_coverage(const NDTreeNode *node)
{
    if (!ndtree_node_is_leaf(node))
        return node->coverage;

    size_t total = 0;
    for (size_t i = 0; i < node->n; i++)
        total += node->bucket[i].x.n;
    return total;
}

static size_t
ndtree_total_size(const NDTreeArchive *tree)
{
    return ndtree_node_coverage(tree->root);
}

// Should we store in the tree the unique counts as well?
static size_t
ndtree_node_unique_count(const NDTreeNode *node)
{
    if (!ndtree_node_is_leaf(node)) {
        size_t total = 0;
        for (size_t i = 0; i < node->n; i++)
            total += ndtree_node_unique_count(&node->children[i]);
        return total;
    }
    return node->n;
}

static size_t
ndtree_unique_size(const NDTreeArchive *tree)
{
    if (!tree->base.allow_duplicates)
        return ndtree_total_size(tree);
    return ndtree_node_unique_count(tree->root);
}


static void
ndtree_node_count_leaves(const NDTreeNode * restrict node,
                         size_t * restrict total, size_t * restrict count)
{
    if (ndtree_node_is_leaf(node)) {
        *total += node->n;
        (*count)++;
        return;
    }

    for (size_t i = 0; i < node->n; i++)
        ndtree_node_count_leaves(node->children + i, total, count);
}

static void
ndtree_node_count_internal(const NDTreeNode * restrict node,
                         size_t * restrict total, size_t * restrict count)
{
    if (!ndtree_node_is_leaf(node)) {
        *total += node->n;
        (*count)++;
        for (size_t i = 0; i < node->n; i++)
            ndtree_node_count_internal(node->children + i, total, count);
    }
}

static size_t
ndtree_node_max_height(const NDTreeNode *node)
{
    if (!ndtree_node_is_leaf(node)) {
        size_t max_h = 0;
        assert(node->n > 0);
        for (size_t i = 0; i < node->n; i++)
            max_h = MAX(max_h, ndtree_node_max_height(&node->children[i]));
        return max_h + 1;
    }
    return 1;
}

static size_t
ndtree_node_min_height(const NDTreeNode *node)
{
    if (!ndtree_node_is_leaf(node)) {
        size_t min_h = SIZE_MAX;
        assert(node->n > 0);
        for (size_t i = 0; i < node->n; i++)
            min_h = MIN(min_h, ndtree_node_min_height(&node->children[i]));
        return min_h + 1;
    }
    return 1;
}

static inline void
ndtree_print_stats(FILE * stream, const NDTreeArchive *tree)
{
    size_t total_size = ndtree_total_size(tree);
    size_t unique_size = ndtree_unique_size(tree);
    fprintf(stream, "# Min height = %zu\n", ndtree_node_min_height(tree->root));
    fprintf(stream, "# Max height = %zu\n", ndtree_node_max_height(tree->root));
    fprintf(stream, "# Total size = %zu\n", total_size);
    fprintf(stream, "# Unique size = %zu\n", unique_size);
    size_t total = 0, count = 0;
    ndtree_node_count_leaves(tree->root, &total, &count);
    assert(total == unique_size);
    fprintf(stream, "# Number of leaves = %zu\n", count);
    fprintf(stream, "# Occupancy = %g%%\n",
            count > 0 ? (100. * (double)total / (double)(count * tree->config.max_bucket_size)) : 0);
    total = 0;
    count = 0;
    ndtree_node_count_internal(tree->root, &total, &count);
    fprintf(stream, "# Total children = %zu\n", total);
    fprintf(stream, "# Number of internal nodes = %zu\n", count);
    fprintf(stream, "# Occupancy = %g%%\n",
            count > 0 ? (100. * (double)total / (double)(count * tree->config.max_children)) : 0);
}

static inline void
ndtree_free(NDTreeArchive *tree)
{
    if (tree == NULL)
        return;
    ndtree_free_nodes(tree);
    free(tree);
}

static inline bool
ndtree_has_x_values(const NDTreeArchive *archive)
{
    return !archive->base.x_is_null;
}

static void
ndtree_go_up_reducing_size(NDTreeNode *node, uint32_t removed)
{
    // Iterative instead of recursive because it is faster and helps inlining.
    node = node->parent;
    while (node) {
        ASSUME(node->coverage >= removed);
        node->coverage -= removed;
        node = node->parent;
    }
}

static void
ndtree_go_up_increasing_size(NDTreeNode *node, uint32_t added)
{
    // Iterative instead of recursive because it is faster and helps inlining.
    node = node->parent;
    while (node) {
        node->coverage += added;
        node = node->parent;
    }
}


static bool
ndtree_z_is_inside_box(const NDTreeNode * restrict node, const double * restrict z, dimension_t dim)
{
    ASSUME(dim >= 2);
    // FIXME: splitting the loop may help vectorization.
    const double * restrict ideal = ndtree_get_ideal(node, dim);
    const double * restrict nadir = ndtree_get_nadir(node, dim);
    for (dimension_t i = 0; i < dim; i++) {
        if (z[i] < ideal[i] || z[i] > nadir[i])
            return false;
    }
    return true;
}

/**
   If z is already in one of the buckets, append to the corresponding solution
   list. Otherwise, reject it.
 */
static inline int
ndtree_node_bucket_append_if_present(NDTreeNode * restrict node,
                                     const double * restrict z, const void * restrict x,
                                     dimension_t dim)
{
    assert(ndtree_node_is_leaf(node));
    for (int i = 0; i < node->n; i++) {
        if (all_equal_double(node->bucket[i].z, z, dim)) {
            if (!solutions_list_append(&node->bucket[i], x))
                return ARCHIVE_INSERT_MEMORY_ERROR;
            return ARCHIVE_INSERT_DUPLICATED;
        }
    }
    return ARCHIVE_INSERT_REJECTED;
}

/**
   Append a new bucket (z, x) to node. The caller must test wrong_x_is_null().

   This function does NOT update the bounds.  The caller must handle that.
*/
static int
ndtree_node_bucket_append(NDTreeNode * restrict node,
                          const double * restrict z, const void * restrict x,
                          dimension_t dim)
{
    assert(ndtree_node_is_leaf(node));
    assert(ndtree_node_bucket_append_if_present(node, z, x, dim) == ARCHIVE_INSERT_REJECTED);
    if (!bucket_append(node, z, x, dim))
        return ARCHIVE_INSERT_MEMORY_ERROR;
    return ARCHIVE_INSERT_ACCEPTED;
}

static const NDTreeNode *
ndtree_find_node_of_vector(const NDTreeNode * restrict node,
                           const double * restrict z, dimension_t dim,
                           uint8_t * restrict bucket_index)
{
    ASSUME(dim >= 2);
    if (ndtree_node_is_leaf(node)) {
        for (uint8_t i = 0; i < node->n; i++) {
            if (all_equal_double(node->bucket[i].z, z, dim)) {
                *bucket_index = i;
                return node;
            }
        }
        return NULL;
    }
    for (size_t i = 0; i < node->n; i++) {
        if (ndtree_z_is_inside_box(&node->children[i], z, dim)) {
            const NDTreeNode * res = ndtree_find_node_of_vector(&node->children[i], z, dim, bucket_index);
            if (res)
                return res;
        }
    }
    return NULL;
}

static inline VoidList *
ndtree_find_solutions_of_vector(const NDTreeArchive * restrict tree,
                                const double * restrict z)
{
    uint8_t bucket_index;
    const NDTreeNode *node = ndtree_find_node_of_vector(
        tree->root, z, tree->dim, &bucket_index);
    if (!node)
        return NULL;
    return &node->bucket[bucket_index].x;
}

static inline bool
ndtree_find_exact_vector(const NDTreeArchive * restrict tree,
                         const double * restrict z)
{
    uint8_t bucket_index;
    return ndtree_find_node_of_vector(tree->root, z, tree->dim, &bucket_index) != NULL;
}

static bool
ndtree_node_dominates(const NDTreeNode * restrict node,
                      const double * restrict z, dimension_t dim)
{
    if (dominates(ndtree_get_nadir(node, dim), z, dim))
        return true;
    if (weakly_dominates(z, ndtree_get_ideal(node, dim), dim))
        return false;

    if (dominates(ndtree_get_ideal(node, dim), z, dim) || dominates(z, ndtree_get_nadir(node, dim), dim)) {
        if (ndtree_node_is_leaf(node)) {
            for (size_t i = 0; i < node->n; i++) {
                const double *bucket_z = node->bucket[i].z;
                if (dominates(bucket_z, z, dim))
                    return true;
                if (weakly_dominates(z, bucket_z, dim))
                    return false;
            }
        } else {
            for (size_t i = 0; i < node->n; i++) {
                if (ndtree_node_dominates(&node->children[i], z, dim))
                    return true;
            }
        }
    }
    return false;
}

static inline bool
ndtree_dominates(const NDTreeArchive * restrict tree, const double * restrict z)
{
    return !ndtree_node_is_empty(tree->root) && ndtree_node_dominates(tree->root, z, tree->dim);
}

/**
   Returns true if z dominates every point in the tree, false otherwise or if z
   exists in the tree or the tree is empty.

*/
static inline bool
ndtree_dominated_by(const NDTreeArchive * restrict tree, const double * restrict z)
{
    if (ndtree_node_is_empty(tree->root))
        return false;

    const dimension_t dim = tree->dim;
    const double * ideal = ndtree_get_ideal(tree->root, dim);

    /* The ideal at the root is tight as long as we never erase a nondominated
       solution. Further down the tree a node's ideal may need tightening, as
       an element leading to the ideal bound value may be dominated and
       removed, but the dominator put in a different subtree, so the node with
       the deletion could have its ideal now over-estimated. The root does not
       have such a risk.  */
    if (!weakly_dominates(z, ideal, dim))
        return false;

    return !tree->base.allow_duplicates
        || !all_equal_double(z, ideal, dim)
        || !ndtree_find_exact_vector(tree, z);
}

static void
ndtree_remove_child_at(NDTreeNode * restrict parent, uint8_t index)
{
    NDTreeNode * removed _attr_maybe_unused = &parent->children[index];
    assert(ndtree_node_is_leaf(removed));
    // The child must be already empty.
    assert(removed->bucket == NULL && removed->children == NULL && removed->box == NULL);

    parent->n--;
#if 0
    for (uint8_t i = index; i < parent->n; i++) {
        parent->children[i] = parent->children[i+1];
        NDTreeNode * child = &parent->children[i];
        // We have to relink the grand children to their new parent.
        if (!ndtree_node_is_leaf(child)) {
            for (uint8_t k = 0; k < child->n; k++) {
                child->children[k].parent = child;
            }
        }
    }
#else
    if (index < parent->n) {
        // The order of the children is irrelevant, so overwrite with the last one.
        parent->children[index] = parent->children[parent->n];
        NDTreeNode * child = &parent->children[index];
        // We have to relink the grand children to their new parent.
        if (!ndtree_node_is_leaf(child)) {
            for (uint8_t k = 0; k < child->n; k++) {
                child->children[k].parent = child;
            }
        }
    }
#endif
}

/**
   Changes the node type after prunning some children.
*/
static NDTreeNode *
ndtree_remove_node_after_pruning(NDTreeNode *node, uint8_t max_bucket_size)
{
    if (node->n == 1) { // Replace tree with the only child.
        NDTreeNode *child = node->children;
        free(node->box);
        child->parent = node->parent;
        assert(node->bucket == NULL);
        memcpy(node, child, sizeof(*node));
        free(child);
        // Update the children's parents.
        if (!ndtree_node_is_leaf(node)) {
            for (uint8_t i = 0; i < node->n; i++)
                node->children[i].parent = node;
        }
        assert(node->bucket == NULL || node->children == NULL);
    } else if (node->n == 0) { // Becomes an empty leaf node.
        node->coverage = 0;
        ndtree_node_free_children(node);
        assert(node->bucket == NULL);
        assert(node->box != NULL);
        if (node->parent != NULL) {
            free(node->box);
            node->box = NULL;
        }
        // If this is the root, we allocate an empty bucket.  Otherwise, this
        // node will be removed by ndtree_remove_child_at() later.
        if (node->parent == NULL
            && !ndtree_bucket_new(node, max_bucket_size + 1))
            return NULL;
    }
    return node;
}

static NDTreeNode *
ndtree_closest_child(NDTreeNode *children, uint8_t n, const double * z, dimension_t dim)
{
    ASSUME(n > 0);
    ASSUME(n < 255);

    NDTreeNode *closest_child = NULL, *first_closest_child = NULL;
    double closest_distance = HUGE_VAL;
    // Find the closest child whose bounding box contains z.
    for (uint8_t i = 0; i < n; i++) {
        NDTreeNode *child = &children[i];
        assert(child->n > 0);
        if (!ndtree_z_is_inside_box(child, z, dim))
            continue;

        if (first_closest_child == NULL) {
            first_closest_child = child;
        } else {
            double distance = squared_distance(ndtree_get_midpoint(child, dim), z, dim);
            if (distance < closest_distance) {
                closest_child = child;
                closest_distance = distance;
            }
        }
    }
    // Prefer children whose bounding boxes contain z.
    if (first_closest_child != NULL) {
        if (closest_child == NULL)
            return first_closest_child;
        // Calculate first_closest_child distance if we found more than one.
        double distance = squared_distance(ndtree_get_midpoint(first_closest_child, dim), z, dim);
        return (distance <= closest_distance) ? first_closest_child : closest_child;
    }

    // z is not inside any box, so we have to calculate all distances.
    closest_child = &children[0];
    closest_distance = squared_distance(ndtree_get_midpoint(closest_child, dim), z, dim);
    for (uint8_t i = 1; i < n; i++) {
        NDTreeNode *child = &children[i];
        double distance = squared_distance(ndtree_get_midpoint(child, dim), z, dim);
        if (distance < closest_distance) {
            closest_distance = distance;
            closest_child = child;
        }
    }
    return closest_child;
}



/**
   Splits a leaf node by converting it to an internal node and distributing the
   bucket elements among newly created children.

   Complexity O(max(n_children * n_children, max_bucket_size) * max_bucket_size + ?)
 */
static int
ndtree_node_split(NDTreeNode *node, dimension_t dim,
                  uint8_t max_bucket_size, uint8_t max_children)
{
    ASSUME(node->n < 255);
    /* At this point, the bounds may be wrong because we inserted a new z and
       it triggered the split, but we did not update the bounds.  */
    size_t bucket_n = node->n;
    ASSUME(bucket_n >= max_children);
    assert(ndtree_node_is_leaf(node));
    ASSUME(node->coverage == 0);
    for (unsigned i = 0; i < bucket_n; i++)
        node->coverage += node->bucket[i].x.n;
    ASSUME(node->coverage > 0);

    // distance[i]: sum of distances of solution i to all other solutions.
    double *distance_sum = malloc((bucket_n + bucket_n * bucket_n) * sizeof(*distance_sum));
    NDTreeNode * children = calloc(max_children, sizeof *children);
    if (distance_sum == NULL || children == NULL) {
        free(distance_sum);
        free(children);
        return ARCHIVE_INSERT_MEMORY_ERROR;

    }
    // Only distance_sum needs initialisation.
    memset(distance_sum, 0, bucket_n * sizeof(*distance_sum));
    double *distance_matrix = distance_sum + bucket_n;
    const SolutionsList *bucket = node->bucket;
    /* FIXME: We could probably vectorize this loop by copying the midpoints to
       a temporary buffer, then computing all distances at once.  But is the
       copy worth the potential speed-up?  */
    for (unsigned i = 0; i < bucket_n; i++) {
        for (unsigned j = i + 1; j < bucket_n; j++) {
            double d = squared_distance(bucket[i].z, bucket[j].z, dim);
            distance_matrix[i * bucket_n + j] = d;
            distance_matrix[j * bucket_n + i] = d;
            distance_sum[i] += d;
            distance_sum[j] += d;
        }
    }

    NDTreeNode * child = &children[0];
    if (!ndtree_node_init(child, dim, max_bucket_size, node)) {
        free(distance_sum);
        free(children);
        return ARCHIVE_INSERT_MEMORY_ERROR;
    }

    uint8_t selected_bucket = (uint8_t) which_max(distance_sum, bucket_n);
    ASSUME(selected_bucket < bucket_n);
    // We do not update bounds until the end of this function, when we know the
    // child assigned to each bucket.
    bucket_append_move(child, &bucket[selected_bucket]);

    // Store the buckets assigned to children and the ones not yet assigned.
    uint8_t bucket_of_child[255];
    for (uint8_t i = 0; i < bucket_n; i++)
        bucket_of_child[i] = i;

    bucket_of_child[0] = selected_bucket;
    bucket_of_child[selected_bucket] = 0;
    /* Select the remaining seeds and reuse distance_sum as an accumulator.

       distance_sum[b] = sum of distances from bucket b to all currently selected seeds.
    */
    memset(distance_sum, 0, bucket_n * sizeof(*distance_sum));
    for (unsigned i = 1; i < max_children; i++) {
        double max_distance = -1;
        size_t selected_pos = SIZE_MAX;
        /* Update the accumulators of the remaining unassigned buckets and find
           the unassigned bucket with the largest accumulated distance.  The
           newly selected seed is now at bucket_of_child[i-1].  Unassigned
           buckets start at i.  */
        for (unsigned k = i; k < bucket_n; k++) {
            uint8_t bucket_k = bucket_of_child[k];
            distance_sum[bucket_k] += distance_matrix[selected_bucket * bucket_n + bucket_k];
            double accumulator = distance_sum[bucket_k];
            if (accumulator > max_distance) {
                max_distance = accumulator;
                selected_pos = k;
            }
        }
        ASSUME(selected_pos < SIZE_MAX);  // There must be one not added yet.
        // Remember the original bucket index before rearranging the array.
        selected_bucket = bucket_of_child[selected_pos];
        child = &children[i];
        if (!ndtree_node_init(child, dim, max_bucket_size, node)) {
            free(distance_sum);
            free(children);
            return ARCHIVE_INSERT_MEMORY_ERROR;
        }
        bucket_append_move(child, &bucket[selected_bucket]);
        // Move the selected bucket into the assigned portion of the array.
        if (i != selected_pos)
            SWAP(bucket_of_child[i], bucket_of_child[selected_pos]);
    }

    /* At this point all children have one bucket, given by
       bucket_of_child[i] when i < max_children, so we can re-use the
       previously calculated distances. */
    if (max_children < bucket_n) {
        unsigned bucket_k = bucket_of_child[max_children];
        unsigned min_child = 0;
        double min_dist = distance_matrix[bucket_k * bucket_n + bucket_of_child[0]];
        for (unsigned j = 1; j < max_children; j++) {
            double dist_j = distance_matrix[bucket_k * bucket_n + bucket_of_child[j]];
            if (dist_j < min_dist) {
                min_child = j;
                min_dist = dist_j;
            }
        }
        child = &children[min_child];
        bucket_append_move(child, &bucket[bucket_k]);
        // We do not need to SWAP.
    }

    free(distance_sum);
    node->children = children;
    node->n = max_children;
    node->cap = max_children;
    ndtree_node_compute_bounds_of_children(node, dim);
    if (max_children < bucket_n - 1) {
        /* We are not done yet! The are still buckets not assigned to children,
           but now we need to update bounds every time we add a new bucket.  */

        /* Now some children have two buckets, so we have to rely on
           ndtree_closest_child(), which is quite wasteful for children that
           have only one.  */
        for (unsigned k = max_children + 1; k < bucket_n; k++) {
            unsigned bucket_k = bucket_of_child[k];
            const double *z = bucket[bucket_k].z;
            NDTreeNode *target = ndtree_closest_child(children, max_children, z, dim);
            bucket_append_move(target, &bucket[bucket_k]);
            bool ideal_changed = true, nadir_changed = true;
            ndtree_node_update_bounds(target, z, dim, &ideal_changed, &nadir_changed);
        }
    }
    ndtree_node_compute_bounds_from_children(node, dim);
    ndtree_node_propagate_bounds_up(node, dim);
    // FIXME: Ideally, we would reuse the memory.
    free(node->bucket);
    node->bucket = NULL;
    DEBUG1(ndtree_assert_valid_up(node, dim));
    DEBUG1(ndtree_assert_valid_down(node, dim));
    return ARCHIVE_INSERT_ACCEPTED;
}

/**
   Insert entry in node. If node is not a leaf node, try again with the closest
   child and so on.  Once it is inserted in a leaf node, maybe split it if it
   has grown too large.

*/
static int
ndtree_node_insert(NDTreeArchive * restrict tree, NDTreeNode * restrict node,
                   const double * restrict z, const void * restrict x)
{
    const dimension_t dim = tree->dim;
    const uint8_t max_bucket_size = tree->config.max_bucket_size;
    while (!ndtree_node_is_leaf(node)) {
        node->coverage++;
        node = ndtree_closest_child(node->children, node->n, z, dim);
    }
    int res = wrong_x_is_null(tree->base.x_is_null, x);
    if (res < 0)
        return res;

    res = ndtree_node_bucket_append(node, z, x, dim);
    assert(res == ARCHIVE_INSERT_ACCEPTED);
    ASSUME(ndtree_node_is_leaf(node));
    // Only update the bounds if we do not split the node.
    if (node->n == 1) {
        // The node was empty before z
        // This cannot happen in the current code, because we never add to an empty child.
        assert(false);
        ndtree_node_set_bounds(node, z, dim);
        ndtree_node_propagate_bounds_up(node, dim);
        DEBUG2(ndtree_assert_valid_tree(tree));
        return res;
    } else if (node->n <= max_bucket_size) {
        ndtree_node_update_bounds_up(node, z, dim);
        DEBUG2(ndtree_assert_valid_tree(tree));
        return res;
    }
    return ndtree_node_split(node, dim, max_bucket_size, tree->config.max_children);
}

/**
   Clear the subtree that has node as a parent and convert node to an empty
   leaf node. If node is the root of the tree, the tree is reset to its empty
   state.

   This does not free node nor its bucket, but it does free all solutions
   lists.
*/
static bool
ndtree_node_clear_to_empty_leaf(NDTreeNode *node, uint8_t max_bucket_size)
{
    // FIXME: This function should record what is being removed, so we can
    // return it.
    if (ndtree_node_is_leaf(node)) {
        /* An empty leaf has ->n=0, but ->bucket is already initialized to
           ->cap size and each ->bucket[i].x.n = 0 with ->bucket[i].x.list == NULL.   */
        assert(node->bucket != NULL);
        /* FIXME: The solutions lists may be empty if we moved these buckets
           elsewhere. How can we detect that?  */
        for (int i = 0; i < node->n; i++)
            solutions_list_free(&node->bucket[i]);

    } else {
        ndtree_node_free_children(node);
        if (!ndtree_bucket_new(node, max_bucket_size + 1))
            return false;
    }
    node->n = 0;
    node->coverage = 0;
    return true;
}

static uint32_t
ndtree_append_subtree_to_list(FlexBucket ** restrict list, NDTreeNode * restrict node)
{
    if (ndtree_node_is_leaf(node))
        return (uint32_t) flex_bucket_append_move(list, node->bucket, node->n);

    uint32_t total = 0;
    for (unsigned i = 0; i < node->n && list; i++)
        total += ndtree_append_subtree_to_list(list, &node->children[i]);
    return total;
}

static int
ndtree_add_duplicate(bool x_is_null, NDTreeNode * restrict node,
                     uint8_t bucket_index, const void * restrict x)
{
    int res = wrong_x_is_null(x_is_null, x);
    if (res < 0)
        return res;
    if (!solutions_list_append(&node->bucket[bucket_index], x))
        return ARCHIVE_INSERT_MEMORY_ERROR;
    ndtree_go_up_increasing_size(node, 1);
    return ARCHIVE_INSERT_DUPLICATED;
}

static int
ndtree_update_node(NDTreeArchive * restrict tree, NDTreeNode * restrict node,
                   const double * restrict z, const void * restrict x,
                   bool check, bool allow_duplicates, FlexBucket **displaced)
{
    const dimension_t dim = tree->dim;
    // FIXME: check=false is an optimization. With DEBUG=1 we should check that
    // it is correct by making sure that we never receive a weakly dominated point.
    if (check && weakly_dominates(ndtree_get_nadir(node, dim), z, dim)) {
        // Maybe there is one solution and it is a duplicate.
        if (allow_duplicates && ndtree_node_is_leaf(node) && node->n == 1
            && all_equal_double(z, node->bucket[0].z, dim)) {
            return ndtree_add_duplicate(tree->base.x_is_null, node, 0, x);
        }
        return ARCHIVE_INSERT_REJECTED;
    }

    bool clears_subtree = allow_duplicates
        ? dominates(z, ndtree_get_ideal(node, dim), dim)
        : weakly_dominates(z, ndtree_get_ideal(node, dim), dim);
    if (clears_subtree) {
        uint32_t removed = ndtree_append_subtree_to_list(displaced, node);
        if (!*displaced)
            return ARCHIVE_INSERT_MEMORY_ERROR;

        if (node->parent != NULL) {
            ndtree_go_up_reducing_size(node, removed);
            // Free bucket/children and subtrees. Mark the node as
            // uninitialized.  The caller will call ndtree_remove_child_at() to
            // remove it from node->parent->children[]
            ndtree_node_free(node);
            node->coverage = 0;
            node->n = 0;
            DEBUG2(ndtree_assert_valid_tree(tree));
        } else {
            // This completely clears the tree up to an empty root leaf.
            /* FIXME: Why not reuse the subtree for inserting the new entry OR
               detach the node from the tree and add it to displaced?  */
            if (!ndtree_node_clear_to_empty_leaf(node, tree->config.max_bucket_size)) {
                return ARCHIVE_INSERT_MEMORY_ERROR;
            }
            DEBUG2(ndtree_assert_valid_tree(tree));
        }
        return ARCHIVE_INSERT_ACCEPTED;
    }

    DEBUG2(ndtree_assert_valid_tree(tree));
    bool z_wdom_nadir = weakly_dominates(z, ndtree_get_nadir(node, dim), dim);
    if (check || !z_wdom_nadir) {
        bool ideal_wdom_z = weakly_dominates(ndtree_get_ideal(node, dim), z, dim);
        if (!ideal_wdom_z && !z_wdom_nadir)
            return ARCHIVE_INSERT_ACCEPTED;
        // If !ideal_wdom_z, then nothing in this node can weakly dominate z,
        // so do not check.
        check = check && ideal_wdom_z;
    }

    if (ndtree_node_is_leaf(node)) {
        if (z_wdom_nadir) {
            uint32_t removed = 0;
            for (uint8_t i = 0; i < node->n; ) {
                SolutionsList *bucket = &node->bucket[i];
                if (weakly_dominates(z, bucket->z, dim)) {
                    if (all_equal_double(z, bucket->z, dim)) {
                        if (allow_duplicates)
                            return ndtree_add_duplicate(tree->base.x_is_null, node, i, x);
                        // equal not allowed.
                        return ARCHIVE_INSERT_REJECTED;
                    }
                    // z dominates bucket_z
                    check = false; // No other bucket can dominate z.
                    removed += bucket->x.n;
                    flex_bucket_append_move(displaced, bucket, 1);
                    if (!*displaced)
                        return ARCHIVE_INSERT_MEMORY_ERROR;
                    node->n--;
                    if (i != node->n)
                        SWAP(node->bucket[i], node->bucket[node->n]);
                    // Do not increment i.
                } else if (check && weakly_dominates(bucket->z, z, dim)) {
                    return ARCHIVE_INSERT_REJECTED;
                } else {
                    i++;
                }
            }
            if (removed > 0) {
                ndtree_go_up_reducing_size(node, removed);
                if (node->n == 0 && node->parent != NULL) {
                    /* Since node->n == 0, the ndtree_node_free() will not call
                       solutions_list_free(), which is what we want because they
                       are now in *displaced.  */
                    ndtree_node_free(node);
                }
            }
        } else if (check) {
            // If !z_wdom_nadir then z cannot dominated anything in this leaf,
            // but it could be dominated.
            for (uint8_t i = 0; i < node->n; i++) {
                if (weakly_dominates(node->bucket[i].z, z, dim))
                    return ARCHIVE_INSERT_REJECTED;
            }
        }
    } else { // NOT a leaf node.
        uint8_t index = 0;
        while (index < node->n) {
            int res = ndtree_update_node(tree, &node->children[index], z, x,
                                         check, allow_duplicates, displaced);
            /* We may have a slightly invalid tree here because we may have
               removed all children, so coverage = 0 but n = 1.  That is, this
               looks like a leaf but it is just empty. */
            if (res != ARCHIVE_INSERT_ACCEPTED)
                return res;

            if (ndtree_node_is_empty(&node->children[index])) {
                ndtree_remove_child_at(node, index);
                DEBUG2(ndtree_assert_valid_tree(tree));
            } else {
                index++;
                assert(index < 255);
                DEBUG2(ndtree_assert_valid_tree(tree));
            }
        }
        if (ndtree_remove_node_after_pruning(node, tree->config.max_bucket_size) == NULL)
            return ARCHIVE_INSERT_MEMORY_ERROR;
        /* At this point, we may have have converted node to a (possibly empty)
           leaf node.  */
        DEBUG2(ndtree_assert_valid_tree(tree));
    }
    return ARCHIVE_INSERT_ACCEPTED;
}

static int
ndtree_add_impl(NDTreeArchive * restrict tree,
                const double * restrict z, const void * restrict x,
                bool check, bool allow_duplicates, FlexBucket **displaced)
{
    const dimension_t dim = tree->dim;
    DEBUG1(ndtree_assert_valid_tree(tree));

    if (ndtree_node_is_empty(tree->root)) {
        int res = wrong_x_is_null(tree->base.x_is_null, x);
        if (res < 0)
            return res;
        res = ndtree_node_bucket_append(tree->root, z, x, dim);
        assert(res < 0 || res == ARCHIVE_INSERT_ACCEPTED);
        assert(tree->root->n == 1);
        ndtree_node_set_bounds(tree->root, z, dim);
        DEBUG1(ndtree_assert_valid_tree(tree));
        // At this point we have modified the tree.
        tree->base.revision++;
        return res;
    }

    int res = ndtree_update_node(tree, tree->root, z, x, check, allow_duplicates, displaced);
    if (res == ARCHIVE_INSERT_DUPLICATED) // At this point we have modified the tree.
        tree->base.revision++;
    if (res != ARCHIVE_INSERT_ACCEPTED)
        return res;
    // At this point we have modified the tree.
    tree->base.revision++;
    // It is empty only if we have removed everything.
    if (ndtree_node_is_empty(tree->root)) {
        res = wrong_x_is_null(tree->base.x_is_null, x);
        if (res < 0)
            return res;
        res = ndtree_node_bucket_append(tree->root, z, x, dim);
        assert(tree->root->n == 1);
        ndtree_node_set_bounds(tree->root, z, dim);
        DEBUG1(ndtree_assert_valid_tree(tree));
        return res;
    }
    // FIXME: Now we have to traverse the tree again to see where to insert
    // the node. Why not try to insert it in update_node() ? Or at least
    // return which subtree is the most promising and start from that.
    DEBUG1(ndtree_assert_valid_tree(tree));
    res = ndtree_node_insert(tree, tree->root, z, x);
    DEBUG1(ndtree_assert_valid_tree(tree));
    return res;
}

/**
   Insert new solution (z,x) and get the displaced solutions.

   *displaced must be NULL or already properly initialized.

   Returns one of the values of archive_insert_result_t.
*/
static inline int
ndtree_add(NDTreeArchive * restrict tree,
           const double * restrict z, const void * restrict x,
           bool check, FlexBucket **displaced)
{
    return ndtree_add_impl(tree, z, x, check, tree->base.allow_duplicates, displaced);
}

static inline int
ndtree_add_discard(NDTreeArchive * restrict tree,
                   const double * restrict z, const void * restrict x)
{
    // FIXME: Avoid using displaced at all.
    FlexBucket *displaced = NULL;
    int res = ndtree_add_impl(tree, z, x, /*check=*/true, tree->base.allow_duplicates, &displaced);
    if (displaced != NULL)
        flex_bucket_free(displaced);
    return res;
}


static int
ndtree_add_displaced_helper(NDTreeArchive * restrict tree,
                            const double * restrict z, FlexBucket **displaced)
{
    const void * x = NULL;
    int res = ndtree_add_impl(tree, z, x, /*check=*/false, /*allow_duplicates=*/false, displaced);
    assert(res == ARCHIVE_INSERT_ACCEPTED);
    return res;
}

/**
   Insert incoming solutions into tree and return displaced nodes in subtree.
   It does not allocate any new memory!
*/
static inline int
ndtree_insert_displaced(NDTreeArchive *tree, FlexBucket **displaced_p, bool *all_displaced)
{
    assert(*displaced_p != NULL);
    if (*displaced_p == NULL)
        return ARCHIVE_INSERT_MEMORY_ERROR;

    FlexBucket * incoming = *displaced_p;
    FlexBucket * displaced = NULL;
    // FIXME: There must be a better way than adding the points one by one!
    const bool tree_was_empty = ndtree_node_is_empty(tree->root);
    _attr_maybe_unused size_t prev_tree_unique;
    DEBUG1(prev_tree_unique = ndtree_unique_size(tree));

    const bool incoming_x_is_null = (incoming->solutions[0].x.list == NULL);
    // Failing this assert is a programming error and should never happen.
    assert(tree->base.x_is_null == incoming_x_is_null);
    tree->base.x_is_null = true; // Disable temporarily
    for (size_t i = 0; i < incoming->n_unique; i++) {
        // We should not have empty solutions.
        assert(incoming->solutions[i].x.n > 0);
        const double * z = incoming->solutions[i].z;
        // FIXME: We should return the node where z is inserted and the bucket
        // index, so we can avoid the ndtree_find_node_of_vector() call below.
        // FIXME: We should also not allocate a copy of z in this case, so we
        // do not need to free it below.
        int res = ndtree_add_displaced_helper(tree, z, &displaced);
        if (res < 0)
            return res;
        assert(res == ARCHIVE_INSERT_ACCEPTED);

        if (incoming_x_is_null && incoming->solutions[i].x.n == 1) {
            free((void*)incoming->solutions[i].z);
            continue; // We are done with this solutions[i].
        }

        /* Instead of inserting duplicates, find the just created SolutionsList
           in the tree and modify it.  */
        uint8_t bucket_index = -1;
        // const_cast so we can update z_node.
        NDTreeNode * z_node = (NDTreeNode *) ndtree_find_node_of_vector(
            tree->root, z, tree->dim, &bucket_index);
        SolutionsList * z_bucket = &z_node->bucket[bucket_index];
        free((void*)z_bucket->z); // allocated by ndtree_add_displaced_helper()
        assert(z_bucket->x.list == NULL); // Did we allocate this list by mistake?
        if (incoming_x_is_null) {
            // We have duplicates but no x values, just copy z and the counter.
            z_bucket->z = incoming->solutions[i].z;
            z_bucket->x.n = incoming->solutions[i].x.n;
        } else {
            // We have duplicates and x values, just copy the whole thing.
            solutions_list_copy(z_bucket, &incoming->solutions[i]);
        }
        // n - 1 because we already counted 1 when inserting z.
        ndtree_go_up_increasing_size(z_node, z_bucket->x.n - 1);
    }
    tree->base.x_is_null = incoming_x_is_null; // Reset.
    /* All displaced points will be inserted, so if the size of the tree is the
       same as the number of displaced, all previous points in the tree have
       been displaced.  */
    assert(ndtree_unique_size(tree) >= incoming->n_unique);
    assert(ndtree_unique_size(tree) > 0);
    *all_displaced = !tree_was_empty && (ndtree_unique_size(tree) == incoming->n_unique);
    assert(!*all_displaced || (displaced && prev_tree_unique == displaced->n_unique));
    assert(tree_was_empty || !displaced || displaced->n_unique <= prev_tree_unique);

    free(incoming);
    *displaced_p = displaced;
    DEBUG1(ndtree_assert_valid_tree(tree));
    return ARCHIVE_INSERT_ACCEPTED;
}

static inline bool
ndtree_init_with_displaced(NDTreeArchive *tree, FlexBucket *displaced,
                           dimension_t dim, const NDTreeConfig * config,
                           bool allow_duplicates, bool x_is_null)
{
    ndtree_init(tree, dim, config->max_children, config->max_bucket_size, allow_duplicates, x_is_null);
    bool all_displaced;
    int res = ndtree_insert_displaced(tree, &displaced, &all_displaced);
    DEBUG1(ndtree_assert_valid_tree(tree));
    assert(res != ARCHIVE_INSERT_REJECTED && res != ARCHIVE_INSERT_DUPLICATED);
    return res == ARCHIVE_INSERT_ACCEPTED;
}

static inline size_t
ndtree_node_get_unique_vectors(const NDTreeNode *node, const double *** z_list,
                               dimension_t dim)
{
    if (ndtree_node_is_leaf(node)) {
        for (size_t i = 0; i < node->n; i++) {
            assert(node->bucket[i].z != NULL);
            **z_list = node->bucket[i].z;
            (*z_list)++;
        }
        return node->n;
    }
    size_t total = 0;
    for (size_t i = 0; i < node->n; i++)
        total += ndtree_node_get_unique_vectors(&node->children[i], z_list, dim);
    return total;
}

/**
   z_list must be an array of pointers to 'const double' of length treap_unique_size(t).

   That is, z_list does not contain the actual data but just the pointers
   stored in the tree.  Since the tree does not stored the data either, the
   data may get deleted by something else if not used or copied immediately.
*/
static inline size_t
ndtree_get_unique_vectors(const NDTreeArchive *tree, const double ** z_list)
{
    return ndtree_node_get_unique_vectors(tree->root, &z_list, tree->dim);
}


static inline void
ndtree_node_get_x_values(const NDTreeNode *node, const void *** x_list)
{
    if (ndtree_node_is_leaf(node)) { // FIXME: Why check leafs first?
        for (size_t i = 0; i < node->n; i++)
            solutions_list_get_x_values(&node->bucket[i], x_list);
        return;
    }

    for (size_t i = 0; i < node->n; i++)
        ndtree_node_get_x_values(&node->children[i], x_list);

}

static inline void
ndtree_get_x_values(const NDTreeArchive *arch, const void ** x_list)
{
    if (!ndtree_has_x_values(arch))
        return;
    ndtree_node_get_x_values(arch->root, &x_list);
}

static inline void
ndtree_node_get_contents(const NDTreeNode *node,
                         double ** z_list, const void *** x_list,
                         dimension_t dim)
{
    if (ndtree_node_is_leaf(node)) { // FIXME: Why check leafs first?
        for (size_t i = 0; i < node->n; i++)
            solutions_list_get_contents(&node->bucket[i], z_list, x_list, dim);
        return;
    }

    for (size_t i = 0; i < node->n; i++)
        ndtree_node_get_contents(&node->children[i], z_list, x_list, dim);
}

/**
   z_list must be an array of double allocated with sufficient space. Vectors
   are copied to this array.

   x_list must be of array of void *. Only the pointers are copied.
*/
static inline void
ndtree_get_contents(const NDTreeArchive * restrict tree, double * z_list,
                    const void ** x_list)
{
    if (!ndtree_node_is_empty(tree->root)) {
        ndtree_node_get_contents(tree->root, &z_list,
                                 ndtree_has_x_values(tree) ? &x_list : NULL,
                                 tree->dim);
    }
}


static size_t
ndtree_assert_valid_down(const NDTreeNode * node, dimension_t dim)
{
    size_t total = 0;
    const double * restrict ideal _attr_maybe_unused = ndtree_get_ideal(node, dim);
    const double * restrict nadir _attr_maybe_unused = ndtree_get_nadir(node, dim);
    if (ndtree_node_is_leaf(node)) {
        for (size_t i = 0; i < node->n; i++) {
            total += node->bucket[i].x.n;
            for (dimension_t d = 0; d < dim; d++) {
                assert(ideal[d] <= node->bucket[i].z[d]);
                assert(nadir[d] >= node->bucket[i].z[d]);
            }
        }
        return total;
    }
    for (size_t i = 0; i < node->n; i++) {
        assert(node->children[i].parent == node);
        total += ndtree_assert_valid_down(&node->children[i], dim);
        const double * restrict child_ideal _attr_maybe_unused = ndtree_get_ideal(&node->children[i], dim);
        const double * restrict child_nadir _attr_maybe_unused = ndtree_get_nadir(&node->children[i], dim);
        for (dimension_t d = 0; d < dim; d++) {
            assert(ideal[d] <= child_ideal[d]);
            assert(nadir[d] >= child_nadir[d]);
        }
    }
    assert(total == node->coverage);
    return total;
}

static void
ndtree_assert_valid_tree(const NDTreeArchive * tree)
{
    size_t total = ndtree_assert_valid_down(tree->root, tree->dim);
    size_t unique_size = ndtree_unique_size(tree);
    assert (total >= unique_size);
    if (total == 0)
        return;

    double * points = malloc(tree->dim * unique_size * sizeof(*points));
    const double ** z_list = malloc(unique_size * sizeof(*z_list));
    _attr_maybe_unused size_t new_unique = ndtree_get_unique_vectors(tree, z_list);
    assert(unique_size == new_unique);
    for (size_t i = 0; i < unique_size; i++) {
        for (size_t d = 0; d < tree->dim; d++) {
            assert(z_list[i] != NULL);
            points[i * tree->dim + d] = z_list[i][d];
        }
    }
    const int * minmax = minmax_minimise(tree->dim);
    _attr_maybe_unused size_t pos =
        find_dominated_point_(points, unique_size, tree->dim,
                              /* keep_weakly=*/false, AGREE_NONE, minmax);
    free((void*)minmax);
    assert(pos >= unique_size);
    free(points);
    free(z_list);
}

static void
ndtree_assert_valid_up(const NDTreeNode *node, dimension_t dim)
{
    while (node->parent != NULL) {
        assert(!ndtree_node_is_leaf(node->parent));
        node = node->parent;
        const double * restrict ideal _attr_maybe_unused = ndtree_get_ideal(node, dim);
        const double * restrict nadir _attr_maybe_unused = ndtree_get_nadir(node, dim);
        for (size_t i = 0; i < node->n; i++) {
            assert(node->children[i].parent == node);
            const double * restrict child_ideal _attr_maybe_unused = ndtree_get_ideal(&node->children[i], dim);
            const double * restrict child_nadir _attr_maybe_unused = ndtree_get_nadir(&node->children[i], dim);
            for (dimension_t d = 0; d < dim; d++) {
                assert(ideal[d] <= child_ideal[d]);
                assert(nadir[d] >= child_nadir[d]);
            }
        }
    }
}

#include "nd_tree_iterator.h"

#endif // ND_TREE_ARCHIVE_H
