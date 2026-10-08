#ifndef _HV3D_PRIV_H
#define _HV3D_PRIV_H

#include "treap2d_dlnode.h"

/* Used by hvc3d.c and hv3dplus.c.

  This implements a variant of the 3D dimension-sweep algorithm by H. T. Kung,
  F. Luccio, and F. P. Preparata.  On Finding the Maxima of a Set of
  Vectors. Journal of the ACM, 22(4):469–476, 1975.

  The main difference is that the order of the points in 2D is tracked by p->cnext/p->closest.

  See also find_nondominated_set_3d_impl().
*/
static inline void
hv3d_preprocessing(dlnode_t * restrict list, size_t n)
{
    // FIXME: Can we unify these two paths to always use either ->cnext or ->closest?
#ifdef HVC_ONLY
# define set_delimiters(DLNODE, DEM0, DEM1) do {                             \
        (DLNODE)->closest[0] = (DEM0);                                       \
        (DLNODE)->closest[1] = (DEM1);                                       \
    } while(false)
#else
    // In HV3D+ closest is ONLY used to initialize cnext, so closest is not
    // needed in every node.
# define set_delimiters(DLNODE, DEM0, DEM1) do {                             \
        (DLNODE)->cnext[0] = (DEM0);                                         \
        (DLNODE)->cnext[1] = (DEM1);                                         \
    } while(false)
#endif

    ASSUME(n >= 1);
    assert(list+1 == list->next[0]);
    assert(list+2 == list->prev[0]);

    Treap2D * tree = treap2d_new(n+2);
    assert(tree != NULL);
    // At the top we insert the first point, which is never dominated.
    dlnode_t * p = (list+1)->next[0];
    // Before the top node, we insert list + 1, sentinel 2 (ref[0], -INF),
    // After the top node, we insert list, sentinel 1 (-INF, ref[1])
    treap2d_init_with_sentinels(tree, p, list+1, list);
    set_delimiters(p, list+1, list);

    _attr_maybe_unused dlnode_t * prev_p = p;
    double pk0 = p->x[0], pk1 = p->x[1], _attr_maybe_unused pk2 = p->x[2];
    const dlnode_t * stop = list+2;
    for (p = p->next[0]; p != stop; p = p->next[0]) {
        const double pj0 = p->x[0], pj1 = p->x[1], pj2 = p->x[2];
        if ((pk0 > pj0) | (pk1 > pj1)) {
            TreapNode *pred = treap2d_find_le(tree, pj1);
            assert(pred != NULL);
            const double * prev_x = treap2d_get_dlnode(pred)->x;
            if (prev_x[0] <= pj0) {
                // pj is dominated by a point in the tree.
#ifdef HVC_ONLY
                if (all_equal_double(prev_x, p->x, 3))
                    treap2d_get_dlnode(pred)->ignore = true; // It will have zero hvc.
#endif
                remove_from_z(p);
                continue;
            }
            // pj is NOT dominated
            TreapNode * prev, *next;
            treap2d_insert_and_displace_get_bounds(tree, p, &prev, &next);
            // Check that the data structure is properly setup.
            assert(treap2d_get_dlnode(prev)->x[0] > pj0 && treap2d_get_dlnode(prev)->x[1] < pj1);
            assert(treap2d_get_dlnode(next)->x[0] < pj0 && treap2d_get_dlnode(next)->x[1] > pj1);
            set_delimiters(p, treap2d_get_dlnode(prev), treap2d_get_dlnode(next));

            pk0 = pj0; pk1 = pj1; pk2 = pj2;
            prev_p = p;
        } else {
            // pj is dominated by a previous point.
#ifdef HVC_ONLY
            if (pk0 == pj0 && pk1 == pj1 && pk2 == pj2)
                prev_p->ignore = true; // It will have zero hvc.
#endif
            remove_from_z(p);
        }
    }
    treap2d_free(tree);
#undef set_delimiters
}

#endif // _HV3D_PRIV_H
