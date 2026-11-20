/*************************************************************************

 treap_archive.c: 2D Archive based on Randomised Binary Tree (Treap)

 ---------------------------------------------------------------------
                       Copyright (C) 2026
          Jonathan Fieldsend <J.E.Fieldsend@exeter.ac.uk>
          Manuel Lopez-Ibanez  <manuel.lopez-ibanez@manchester.ac.uk>

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.

 ---------------------------------------------------------------------

*************************************************************************/
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include "sort.h"
#include "maxminclamp.h"

#define TREAP_ITEM_COUNTS_DUPLICATES 1
#include "treap_archive.h"
#include "archiving_priv.h"

#include "treap_iterator.h"


//---------------------------------------------------------------------------

bool
treap_archive_move(TreapArchive * restrict dst, TreapArchive * restrict src)
{
    memcpy(dst, src, sizeof(*src));
    memset(src, 0, sizeof(*src));
    free(src);
    return true;
}

void
treap_archive_init(TreapArchive *arch, bool keep_duplicates, bool x_is_null)
{
    treap_init(&arch->tree);
    arch->total_size = 0;
    arch->unique_size = 0;
    base_archive_init(&arch->base, keep_duplicates, x_is_null);
}


TreapArchive *
treap_archive_new(bool keep_duplicates, bool x_is_null)
{
    TreapArchive * arch = malloc(sizeof(*arch));
    if (arch != NULL)
        treap_archive_init(arch, keep_duplicates, x_is_null);

    return arch;
}

static inline TreapNode *
treap_archive_node_new(const double * restrict z, const void * restrict x)
{
    TreapNode * node = malloc(sizeof(*node));
    if (!node)
        return NULL;
    if (!void_list_init(&node->item, x)) {
        free(node);
        return NULL;
    }
    treap_node_init(node, z[0], z[1], node->item);
    return node;
}

bool
treap_archive_init_with_single_solution(TreapArchive * restrict arch,
                                        const double * restrict z, const void * restrict x,
                                        bool keep_duplicates)
{
    // FIXME: Use a pool of nodes.
    TreapNode *node = treap_archive_node_new(z, x);
    if (node == NULL)
        return false;

    assert(node->left == NULL);
    assert(node->right == NULL);
    treap_init_with_single_node(&arch->tree, node);
    arch->unique_size = 1;
    arch->total_size = node->item.n;
    base_archive_init(&arch->base, keep_duplicates, (x == NULL));
    return true;
}

void
treap_archive_free_nodes(TreapArchive * arch)
{
    treap_free_nodes(&arch->tree);
}

void
treap_archive_free(TreapArchive * arch)
{
    treap_archive_free_nodes(arch);
    free(arch);
}

size_t
treap_archive_total_size(const TreapArchive *arch)
{
    return arch->total_size;
}

size_t
treap_archive_unique_size(const TreapArchive *arch)
{
    return arch->unique_size;
}

bool
treap_archive_find_exact_vector(const TreapArchive * arch, const double * z)
{
    TreapNode *node = treap_find(&arch->tree, z[0]);
    return (node && node->y == z[1]);
}

VoidList *
treap_archive_find_solutions_of_vector(const TreapArchive * arch, const double * z)
{
    TreapNode *node = treap_find(&arch->tree, z[0]);
    if (node && node->y == z[1])
        return &node->item;
    return NULL;
}


/**
   Returns true if z dominates every point in the tree, false otherwise or if z
   exists in the tree or the tree is empty.

   Complexity: O(2 log n)
*/
bool
treap_archive_dominated_by(const TreapArchive * arch, const double * z)
{
    const TreapNode * root =  arch->tree.root;
    if (arch->unique_size == 0) {
        assert(root == NULL);
        return false;
    }
    if (arch->unique_size == 1) {
        return dominates_2d(z, (double [2]){ root->x, root->y });
    }

    const TreapNode * first = treap_minimum(root);
    const TreapNode * last = treap_maximum(root);
    assert(first != last);
    // Even if they are both equal, first and last are not the same point.
    return z[0] <= first->x && z[1] <= last->y;
}

bool
treap_archive_dominates(const TreapArchive * arch, const double * z)
{
    return treap_dominates(&arch->tree, z);
}

// FIXME: This visits every node, so it is O(n). Is there a more efficient way?
static inline size_t
treap_archive_subtree_unique_size(const TreapNode *root, size_t * total_size)
{
    if (!root)
        return 0;
    *total_size += root->item.n;
    return 1 + treap_archive_subtree_unique_size(root->left, total_size)
        + treap_archive_subtree_unique_size(root->right, total_size);
}

static inline void
treap_archive_update_size(TreapArchive *arch)
{
    arch->total_size = 0;
    arch->unique_size = treap_archive_subtree_unique_size(arch->tree.root, &arch->total_size);
}

static inline void
treap_update_sizes_after_insert(TreapArchive * restrict arch, const TreapNode * restrict node,
                                size_t removed_unique, size_t removed_total)
{
    arch->unique_size++;
    arch->unique_size -= removed_unique;
    arch->total_size += node->item.n;
    arch->total_size -= removed_total;
    arch->base.revision++;
}
/**
   Insert new_node into self and return the nodes it displaces.

   Preconditions:

     - new_node is initialized and has a valid priority.
     - new_node->left == NULL
     - new_node->right == NULL
     - new_node->z[0] is its key.
     - its key does not already occur in self.
     - no node in self weakly dominates new_node.
     - self is a valid mutually-non-dominated minimization frontier.

   Postconditions:

     - new_node belongs to self.
     - every node returned is dominated by new_node.
     - no other node is removed from self.
     - the returned nodes form a valid treap/frontier.
*/
static inline TreapArchive *
treap_archive_insert_node_and_displace(TreapArchive *self, TreapNode *node)
{
    assert(node != NULL);

    TreapArchive * displaced = treap_archive_new(self->base.keep_duplicates, self->base.x_is_null);
    if (!displaced)
        return NULL;

    displaced->base.x_is_null = self->base.x_is_null;
    displaced->tree.root = treap_insert_and_displace(&self->tree, node);
    treap_archive_update_size(displaced); // O(n) ! Is there a more efficient way?
    treap_update_sizes_after_insert(self, node, displaced->unique_size, displaced->total_size);
    return displaced;
}

static inline void
treap_archive_insert_node_and_discard(TreapArchive *self, TreapNode *node)
{
    TreapNode * displaced = treap_insert_and_displace(&self->tree, node);
    size_t removed_total = 0;
    size_t removed_unique = treap_count_and_free(displaced, &removed_total);
    treap_update_sizes_after_insert(self, node, removed_unique, removed_total);
}


/**
   Insert new solution (z,x) and get the displaced treap nodes if displaced != NULL.

   When tree_can_dominate_z, the function assumes that no point in the tree can
   weakly dominate z. Violating this assumption is a user error.

   Returns one of the values of archive_insert_result_t.
*/
// FIXME: Split into tree_can_dominate_z/tree_cannot_dominate_z variants.
int
treap_archive_add(TreapArchive * restrict self, const double * restrict z,
                  const void * restrict x, bool tree_can_dominate_z,
                  TreapArchive ** restrict displaced)
{
    // FIXME: How to add to an already built tree of displaced?
    if (displaced) {
        assert(*displaced == NULL);
        *displaced = NULL;
    }
    VoidList * duplicated_item = NULL;
    if (tree_can_dominate_z) {
        TreapNode * leq = treap_find_weak_dominator(&self->tree, z[0], z[1]);
        if (leq) {
            if (!self->base.keep_duplicates || leq->x != z[0] || leq->y != z[1])
                return ARCHIVE_INSERT_REJECTED;
            duplicated_item = &leq->item;
        }
    } else {
        duplicated_item = treap_archive_find_solutions_of_vector(self, z);
    }

    int res = wrong_x_is_null(self->base.x_is_null, x);
    if (res)
        return res;

    if (duplicated_item) {
        assert(self->base.keep_duplicates);
        /* Exact duplicates cannot change the front geometry; append them
           to the existing treap node and skip all dominance work. */
        if (!void_list_append(duplicated_item, x))
            return ARCHIVE_INSERT_MEMORY_ERROR;
        self->base.revision++;
        self->total_size++;
        return ARCHIVE_INSERT_DUPLICATED; // Accepted
    }

    TreapNode *new_node = treap_archive_node_new(z, x);
    if (new_node == NULL)
        return ARCHIVE_INSERT_MEMORY_ERROR; // ERROR

    if (displaced) {
        *displaced = treap_archive_insert_node_and_displace(self, new_node);
        if (*displaced == NULL) {
            treap_node_free(new_node);
            return ARCHIVE_INSERT_MEMORY_ERROR;
        }
    } else
        treap_archive_insert_node_and_discard(self, new_node);

    return ARCHIVE_INSERT_ACCEPTED; // Accepted
}

int
treap_archive_add_discard(TreapArchive * restrict self,
                          const double * restrict z, const void * restrict x)
{
    return treap_archive_add(self, z, x, /*tree_can_dominate_z=*/true, /*displaced=*/NULL);
}

/**
   Insert the elements of incoming into arch and return displaced nodes in incoming.
   It does not allocate any new memory!

   This function assumes that:

   - no vector in arch can weakly dominated a vector in incoming.
   - no vector in incoming appears in arch.
*/
int
treap_archive_insert_displaced(TreapArchive *arch, TreapArchive *incoming, bool *all_displaced)
{
    // Failing this assert is a programming error and should never happen.
    assert(arch->base.x_is_null == incoming->base.x_is_null);
    assert(arch->base.keep_duplicates == incoming->base.keep_duplicates);

    arch->unique_size += incoming->unique_size;
    arch->total_size += incoming->total_size;
    TreapNode * displaced;
    // FIXME: Regenerate all priorities of incoming nodes using arch->tree so
    // that we do not need to pass arch->tree to the union.
    arch->tree.root = treap_frontier_union(&arch->tree, incoming->tree.root, arch->tree.root, &displaced, all_displaced);
    // Reuse incoming for displaced.
    incoming->tree.root = displaced;
    treap_archive_update_size(incoming);  // O(n) ! Is there a more efficient way?
    arch->unique_size -= incoming->unique_size;
    arch->total_size -= incoming->total_size;
    arch->base.revision++;
    return ARCHIVE_INSERT_ACCEPTED;
}

bool
treap_archive_has_x_values(const TreapArchive *arch)
{
    return !arch->base.x_is_null;
}

static inline void
treap_archive_node_get_x_values(const TreapNode *node, const void *** x_list)
{
    if (!node)
        return;

    treap_archive_node_get_x_values(node->left, x_list);
    void_list_get_x_values(&node->item, x_list);
    treap_archive_node_get_x_values(node->right, x_list);
}

void
treap_archive_get_x_values(const TreapArchive *arch, const void ** x_list)
{
    if (!treap_archive_has_x_values(arch))
        return;
    treap_archive_node_get_x_values(arch->tree.root, &x_list);
}

static inline void
treap_archive_node_get_contents(const TreapNode *node, double ** z_list, const void *** x_list)
{
    if (!node)
        return;

    treap_archive_node_get_contents(node->left, z_list, x_list);
    // Duplicate z for each x
    for (size_t i = 0; i < node->item.n; i++, *z_list += 2)
        memcpy(*z_list, (double [2]){ node->x, node->y}, 2 * sizeof **z_list);

    if (x_list != NULL)
        void_list_get_x_values(&node->item, x_list);
    treap_archive_node_get_contents(node->right, z_list, x_list);
}

/**
   z_list must be an array of double allocated with sufficient space. Vectors
   are copied to this array.

   x_list must be of (void *). Only the pointers are copied.
*/
void
treap_archive_get_contents(const TreapArchive *arch, double * z_list, const void ** x_list)
{
    treap_archive_node_get_contents(arch->tree.root, &z_list,
                                    treap_archive_has_x_values(arch) ? &x_list : NULL);
}

/**
   z_list must be an array of double allocated with sufficient space. Vectors
   are copied to this array.
*/
void
treap_archive_get_unique_vectors(const TreapArchive *arch, double * z)
{
    treap_get_unique_vectors(&arch->tree, z);
}
