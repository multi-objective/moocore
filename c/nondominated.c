/*****************************************************************************

 Functions that use data-structures that need to be contained within a separate
 compilation unit.

*****************************************************************************/
#include "nondominated.h"

#include "treap2d_fixed_size.h"

/**
   3D dimension-sweep algorithm by H. T. Kung, F. Luccio, and F. P. Preparata.
   On Finding the Maxima of a Set of Vectors. Journal of the ACM,
   22(4):469–476, 1975.

   A different implementation is available from Duarte M. Dias, Alexandre
   D. Jesus, Luís Paquete, A software library for archiving nondominated
   points, GECCO 2021. https://github.com/TLDart/nondLib/blob/main/nondlib.hpp

   rows should be already sorted by cmp_pdouble_asc_rev_3d().

   When find_dominated, return as soon as it finds one dominated point.
*/
size_t
find_nondominated_3d_impl_sorted(const double ** restrict rows, size_t size,
                                 const bool keep_weakly,
                                 const bool find_dominated)
{
    ASSUME(size > 1);
    Treap2D * tree = treap2d_new(size);
    assert(tree != NULL);
    double pk0 = rows[0][0], pk1 = rows[0][1], pk2 = rows[0][2];
    treap2d_init_with_single_node(tree, pk0, pk1);

    // In this context, size means "no dominated solution found".
    size_t new_size = size;
    size_t prev_dominated = false;
    for (size_t j = 1; j < size; j++) {
        const double * restrict pj = rows[j];
        DEBUG2(printf_point("pj = [ ", pj, 3, " ], "));
        const double pj0 = pj[0], pj1 = pj[1], pj2 = pj[2];
        if ((pk0 > pj0) | (pk1 > pj1)) {
            // Check if pj is dominated by a point in the tree.
            /* In a valid 2-D frontier, x increases and y decreases.  Therefore
               the only existing point that can dominate pj is the frontier
               predecessor with the largest key <= pj0.  */
            TreapNode *pred = treap2d_find_le(tree, pj0);
            if (pred != NULL && treap2d_node_get_y(pred) <= pj1)
                goto j_is_dominated;

            /* pj is not dominated by an existing frontier point.

               Insert it and detach every existing point dominated by it.

               The returned treap contains exactly those displaced nodes, but
               we do not need to traverse it here because those nodes will
               never again participate in dominance queries.  */
            treap2d_insert(tree, pj0, pj1);
            // Fall-through to j_is_NOT_dominated.
        } // Handle duplicates and points that are dominated by the immediate previous one.
        else if (!keep_weakly // Don't keep duplicates.
                 // or the previous was dominated, then this one is also dominated.
                 || prev_dominated
                 // or it is not a duplicate, so it is dominated.
                 || pk0 != pj0 || pk1 != pj1 || pk2 != pj2) {
            // The sorting function must be stable so that we keep only the
            // first duplicated point.
            DEBUG2(printf_point("weakly dominated by pk: ",
                                (double[3]){pk0, pk1, pk2}, 3, "\n"));
            goto j_is_dominated;
        }
        // j_is_NOT_dominated
        pk0 = pj0; pk1 = pj1; pk2 = pj2;
        prev_dominated = false;
        continue;

    j_is_dominated: // pj is dominated by a point in the tree or by pk.
        if (find_dominated) {
            // In this context, it means "position of the first dominated solution found".
            new_size = j;
            goto early_end;
        }
        prev_dominated = true;
        rows[j] = NULL;
        new_size--;
    }

early_end:
    treap2d_free(tree);
    return new_size;
}

/**
   Return true if point is dominated by a different point in the tree.
*/
static inline bool
dominated_by_tree_3d(Treap2D *tree, const double *point)
{
    DEBUG1(treap2d_validate_tree(tree));
    point++; // Ignore dim [0]
    // Find the rightmost point with prev->key <= point[0].
    TreapNode *prev_node = treap2d_find_le(tree, point[0]);
    if (prev_node != NULL && treap2d_node_get_y(prev_node) <= point[1]) {
        assert(treap2d_node_get_x(prev_node) <= point[0]);
        return true; // p is dominated
    }
    return false; // p is not dominated
}

// FIXME: How to merge this function and the one above?
static inline bool
insert_if_not_dominated_by_tree_3d(Treap2D *tree, const double *point)
{
    DEBUG1(treap2d_validate_tree(tree));
    point++; // Ignore dim [0]
    // Find the rightmost point with prev->key <= point[0].
    TreapNode **prev_link = treap2d_find_le_link(tree, point[0]);
    if (prev_link != NULL) {
        const double prev[2] = { treap2d_node_get_x(*prev_link), treap2d_node_get_y(*prev_link) };
        if (prev[1] <= point[1]) {
            assert(prev[0] <= point[0]);
            return false; // p is dominated
        } else if (prev[0] == point[0]) {
            // p dominates prev. Remove the already found node.
            treap2d_erase_at(prev_link);
            DEBUG1(treap2d_validate_tree(tree));
        }
    }
    // FIXME: This will call split_lt, but we just called find_le above, so can
    // we avoid one of them?
    treap2d_insert(tree, point[0], point[1]);
    return true; // p was inserted.
}

/**
   ALGORITHM 5.1. This algorithm accepts two sets R and S of 3-dimensional
   vectors with r_size and s_size elements, respectively, and finds all the
   elements of S that are not dominated by any element of R.
*/
size_t
kung_merge_dim3(const double ** restrict r, size_t r_size,
                const double ** restrict s, size_t s_size)
{
    // r and s should be already sorted.
    // Everything in s that is below the first element in r is nondominated.
    // FIXME: Use binary search to find k.
    const double r0 = r[0][0];
    if (r0 > s[s_size - 1][0])
        return s_size;
    size_t k = 0;
    while (r0 > s[k][0])
        k++;

    DEBUG1(for (size_t j = 0; j < k; j++) assert(s[j][0] < r0));
    DEBUG1(for (size_t j = k; j < s_size; j++) assert(r0 <= s[j][0]));

    Treap2D * tree = treap2d_new(r_size);
    assert(tree != NULL);
    treap2d_init_with_single_node(tree, r[0][1], r[0][2]);
    DEBUG2(printf_point("insert in tree: r=[ ", r[0], 3, " ]\n"));

    size_t i = 1, new_size = s_size;
    do {
        const double * restrict v = s[k];
        DEBUG2_PRINT("i = %zu, j = %zu, s_size = %zu", i, k, s_size);
        DEBUG2(printf_point("v = [ ", v, 3, " ]\n"));

        while (i < r_size) { // Add to the tree all points in R that could dominate v.
            const double * restrict u = r[i];
            if (u[0] > v[0])
                break;
            if (insert_if_not_dominated_by_tree_3d(tree, u)) {
                // u is NOT dominated by a point in the tree.
                DEBUG2(printf_point("!dominated_by_tree_3d: u=[ ", u, 3, " ]\n"));
            }
            i++;
        }
        if (dominated_by_tree_3d(tree, v)) {
            DEBUG2(printf_point("dominated_by_tree_3d: v=[ ", v, 3, " ]\n"));
            s[k] = NULL; // dominated
            new_size--;
        }
        k++;
    } while (k < s_size);
    treap2d_free(tree);
    return new_size;
}
