/*****************************************************************************

 Functions that use data-structures that need to be contained within a separate
 compilation unit.

*****************************************************************************/
#include "nondominated.h"

typedef const double avl_item_t;
typedef struct avl_node_t {
    struct avl_node_t *next;
    struct avl_node_t *prev;
    struct avl_node_t *parent;
    struct avl_node_t *left;
    struct avl_node_t *right;
    avl_item_t *item;
    unsigned char depth;
} avl_node_t;

#include "avl_tiny.h"

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
    /* FIXME: The AVL-tree is the bottleneck of this algorithm. A Treap
       [R. Seidel and C. R. Aragon. Randomized search trees.  Algorithmica,
       16:464–497, 1996] may be far more efficient by allowing to remove a
       range of items without rebalancing.  See
       https://alexdremov.me/treap-algorithm-explained/
       Instead of using randomized priorities, hash the node pointer or the node index.
    */
    avl_tree_t tree;
    avl_init_tree(&tree, qsort_cmp_pdouble_asc_x_nonzero);
    avl_node_t * tnodes = malloc((size+1) * sizeof(*tnodes));
    avl_node_t * node = tnodes;
    node->item = rows[0];
    avl_insert_top(&tree, node);

    const double sentinel[] = { INFINITY, -INFINITY };
    (++node)->item = sentinel;
    avl_insert_after(&tree, node - 1, node);

    // In this context, size means "no dominated solution found".
    size_t new_size = size;
    size_t prev_dominated = false;
    double pk0 = rows[0][0], pk1 = rows[0][1], pk2 = rows[0][2];
    for (size_t j = 1; j < size; j++) {
        const double * restrict pj = rows[j];
        DEBUG2(printf_point("pj = [ ", pj, 3, " ], "));
        const double pj0 = pj[0], pj1 = pj[1], pj2 = pj[2];
        if ((pk0 > pj0) | (pk1 > pj1)) {
            // Check if pj is dominated by a point in the tree.
            avl_node_t * nodeaux;
            int res = avl_search_closest(&tree, pj, &nodeaux);
            assert(res != 0);
            if (res > 0 || nodeaux->prev) {
                const double * restrict prev;
                if (res > 0) { // nodeaux goes before pj
                    prev = nodeaux->item;
                    nodeaux = nodeaux->next;
                    DEBUG2(printf_point("res > 0: prev: ", prev, 3, "\n"));
                } else { // nodeaux goes after pj, so move to the previous one.
                    prev = nodeaux->prev->item;
                    DEBUG2(printf_point("res < 0: prev: ", prev, 3, "\n"));
                }
                assert(prev[0] != sentinel[0]);
                assert(prev[0] <= pj0);
                if (prev[1] <= pj1)
                    goto j_is_dominated;
            }

            // pj is NOT dominated by a point in the tree.
            const double * restrict point = nodeaux->item;
            assert(pj0 <= point[0]);
            // Delete everything in the tree that is dominated by pj.
            while (pj1 <= point[1]) {
                DEBUG2(printf_point("delete point: ", point, 3, "\n"));
                assert(pj0 <= point[0]);
                nodeaux = nodeaux->next;
                point = nodeaux->item;
                /* FIXME: A possible speed up is to delete without rebalancing
                   the tree because avl_insert_before() will rebalance, but we
                   need to know which is the highest node that needs
                   rebalancing. */
                avl_unlink_node(&tree, nodeaux->prev);
            }
            DEBUG2((point == sentinel)
                   ? printf_point("insert before sentinel: ", sentinel, 2, "\n")
                   : printf_point("insert before point: ", point, 3, "\n"));
            (++node)->item = pj;
            avl_insert_before(&tree, nodeaux, node);
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
    free(tnodes);
    return new_size;
}


/**
   Returns NULL if point is dominated by a different point in the tree.
*/
static avl_node_t *
dominated_by_tree_3d(const avl_tree_t * restrict tree,
                     const double * restrict point)
{
    avl_node_t * nodeaux;
    int res = avl_search_closest(tree, point, &nodeaux);
    if (res >= 0) { // nodeaux goes before point
        const double * restrict prev = nodeaux->item;
        DEBUG2(printf_point("res > 0: prev = [ ", prev, 3, " ], "));
        DEBUG2(printf_point("point = [ ", point, 3, " ]\n"));
        assert(prev[0] != INFINITY);
        assert(prev[0] <= point[0]);
        assert(prev[1] <= point[1]);
        return prev[2] <= point[2] ? NULL : nodeaux->next;
    } else if (nodeaux->prev) { // nodeaux goes after point, so move to the next one.
        const double * restrict prev = nodeaux->prev->item;
        DEBUG2(printf_point("res <= 0: prev = [ ", prev, 3, " ], "));
        DEBUG2(printf_point("point = [ ", point, 3, " ]\n"));
        assert(prev[0] != INFINITY);
        assert(prev[0] <= point[0]);
        assert(prev[1] <= point[1]);
        return prev[2] <= point[2] ? NULL : nodeaux;
    }
    assert(nodeaux != NULL);
    return nodeaux; // point is not dominated
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

    avl_tree_t tree;
    avl_init_tree(&tree, qsort_cmp_pdouble_asc_y_asc_z);
    // FIXME: Use a workspace to allocate this once and re-alloc only if a larger number is needed.
    avl_node_t * tnodes = malloc((r_size + 1) * sizeof(*tnodes));
    avl_node_t * node = tnodes;
    node->item = r[0];
    avl_insert_top(&tree, node);

    const double sentinel[] = { INFINITY, INFINITY, -INFINITY};
    (++node)->item = sentinel;
    avl_insert_after(&tree, node - 1, node);

    size_t i = 1, new_size = s_size;
    do {
        const double * restrict v = s[k];
        DEBUG2_PRINT("i = %zu, j = %zu, s_size = %zu", i, k, s_size);
        DEBUG2(printf_point("v = [ ", v, 3, " ]\n"));

        while (i < r_size) { // Add to the tree all points in R that could dominate v.
            const double * restrict u = r[i];
            if (u[0] > v[0]) {
                break;
            }
            avl_node_t * nodeaux = dominated_by_tree_3d(&tree, u);
            if (nodeaux != NULL) { // u is NOT dominated by a point in the tree.
                const double * restrict point = nodeaux->item;
                assert(u[1] <= point[1]);
                // Delete everything in the tree that is dominated by u.
                while (u[2] <= point[2]) {
                    // printf("delete point: "); print_point(point); printf("\n");
                    assert(u[1] <= point[1]);
                    nodeaux = nodeaux->next;
                    point = nodeaux->item;
                    /* FIXME: A possible speed up is to delete without
                       rebalancing the tree because avl_insert_before() will
                       rebalance, but we need to know which is the highest node
                       that needs rebalancing. */
                    avl_unlink_node(&tree, nodeaux->prev);
                }
                // printf("insert before point: "); print_point(point); printf("\n");
                (++node)->item = u;
                avl_insert_before(&tree, nodeaux, node);
            }
            i++;
        }
        if (dominated_by_tree_3d(&tree, v) == NULL) {
            DEBUG2(printf_point("dominated_by_tree_3d: v=[ ", v, 3, " ]\n"));
            s[k] = NULL; // dominated
            new_size--;
        }
        k++;
    } while (k < s_size);

    free(tnodes);
    return new_size;
}
