#ifndef _HV3D_PRIV_H
#define _HV3D_PRIV_H

typedef const double avl_item_t;
typedef struct avl_node_t {
    struct avl_node_t *next;
    struct avl_node_t *prev;
    struct avl_node_t *parent;
    struct avl_node_t *left;
    struct avl_node_t *right;
    avl_item_t * item;
    dlnode_t * dlnode;
    unsigned char depth;
} avl_node_t;

#include "avl_tiny.h"

static inline const double *
node_point(const avl_node_t *node)
{
    return node->item;
}

static inline avl_node_t *
new_avl_node(dlnode_t * p, avl_node_t * node)
{
    node->dlnode = p;
    node->item = p->x;
    return node;
}

/* Used by hvc3d.c and hv3dplus.c.

  This implements a variant of the 3D dimension-sweep algorithm by H. T. Kung,
  F. Luccio, and F. P. Preparata.  On Finding the Maxima of a Set of
  Vectors. Journal of the ACM, 22(4):469–476, 1975.

  The main difference is that the order of the points in 2D is tracked by p->cnext/p->closest.
*/
static inline void
hv3d_preprocessing(dlnode_t * list, size_t n)
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

    avl_tree_t tree;
    avl_init_tree(&tree, cmp_pdouble_asc_y_des_x_nonzero);
    avl_node_t * tnodes = malloc((n+2) * sizeof(*tnodes));

    // At the top we insert the first point, which is never dominated.
    dlnode_t * p = (list+1)->next[0];
    avl_node_t * nodeaux = new_avl_node(p, tnodes);
    avl_insert_top(&tree, nodeaux);
    set_delimiters(p, list+1, list);

    // After the top node, we insert sentinel 1 (-INF, ref[1])
    avl_node_t * node = new_avl_node(list, tnodes + 1);
    avl_insert_after(&tree, nodeaux, node);
    // Before the top node, we insert sentinel 2 (ref[0], -INF)
    node = new_avl_node(list + 1, tnodes + 2);
    avl_insert_before(&tree, nodeaux, node);
    set_delimiters(p, nodeaux->prev->dlnode, nodeaux->next->dlnode);

    const dlnode_t * stop = list+2;
    p = p->next[0];
    while (p != stop) {
        const double * px = p->x;
        const double * prev_x;
        // == 1 means that nodeaux goes before p, so move to the next one.
        if (avl_search_closest(&tree, px, &nodeaux) == 1) {
            prev_x = node_point(nodeaux);
            nodeaux = nodeaux->next;
        } else {
            prev_x = node_point(nodeaux->prev);
        }
        assert(node_point(nodeaux)[1] > px[1] // node_point(nodeaux) comes after px.
               || (node_point(nodeaux)[1] == px[1] && node_point(nodeaux)[0] < px[0]));
        assert(prev_x[1] <= px[1]);
        assert(prev_x[2] <= px[2]);
        if (prev_x[0] <= px[0]) { // px is dominated by a point in the tree.
#ifdef HVC_ONLY
            if (all_equal_double(prev_x, px, 3))
                nodeaux->prev->dlnode->ignore = true; // It will have zero hvc.
#endif
            remove_from_z(p);
        } else if (node_point(nodeaux)[1] == px[1]) { // px is dominated by a point in the tree.
            // FIXME: If the points were ordered by asc x we would only need the first condition.
            assert(node_point(nodeaux)[0] < px[0]);
            remove_from_z(p);
        } else {
            assert(node_point(nodeaux)[1] >= px[1]);
            // Delete everything in the tree that is dominated by p.
            while (node_point(nodeaux)[0] >= px[0]) {
                assert(node_point(nodeaux)[1] >= px[1]);
                nodeaux = nodeaux->next;
                /* FIXME: A possible speed up is to delete without rebalancing
                   the tree because avl_insert_before() will rebalance. */
                avl_unlink_node(&tree, nodeaux->prev);
            }
            node = new_avl_node(p, node + 1);
            avl_insert_before(&tree, nodeaux, node);
            set_delimiters(p, node->prev->dlnode, node->next->dlnode);
        }
        p = p->next[0];
    }
    free(tnodes);
#undef set_delimiters
}

#endif // _HV3D_PRIV_H
