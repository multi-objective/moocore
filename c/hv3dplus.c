/******************************************************************************
 HV3D+ algorithm.
 ------------------------------------------------------------------------------

                        Copyright (c) 2013, 2016, 2017
                Andreia P. Guerreiro <apg@dei.uc.pt>

 This program is free software (software libre); you can redistribute
 it and/or modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 3 of the
 License.

 This program is distributed in the hope that it will be useful, but
 WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, you can obtain a copy of the GNU
 General Public License at:
                 http://www.gnu.org/copyleft/gpl.html
 or by writing to:
           Free Software Foundation, Inc., 59 Temple Place,
                 Suite 330, Boston, MA 02111-1307 USA

 ------------------------------------------------------------------------------

 Reference:

 [1] Andreia P. Guerreiro and Carlos M. Fonseca. Computing and Updating
     Hypervolume Contributions in Up to Four Dimensions. IEEE Transactions on
     Evolutionary Computation, 22(3):449–463, June 2018.

 ------------------------------------------------------------------------------

 Modified by Manuel López-Ibáñez (2025):
  - Integration within moocore.
  - Correct handling of weakly dominated points and repeated coordinates during
    preprocessing().
  - More efficient setup_cdllist() and preprocessing() in terms of time and memory.

******************************************************************************/

#include <float.h>
#include <string.h>
#include "common.h"
#define HV_DIMENSION 3
#include "hv_priv.h"

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

/* ---------------- Data Structures Functions --------------------------------*/

static inline void
print_x(dlnode_t * p)
{
    assert(p != NULL);
    const double * x = p->x;
    fprintf(stderr, "x: %g %g %g\n", x[0], x[1], x[2]);
}

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


/* -------------------- Preprocessing ---------------------------------------*/

/*
  This implements a variant of the 3D dimension-sweep algorithm by H. T. Kung,
  F. Luccio, and F. P. Preparata.  On Finding the Maxima of a Set of
  Vectors. Journal of the ACM, 22(4):469–476, 1975.

  The main difference is that the order of the points in 2D is tracked by p->cnext.
*/
static inline void
preprocessing(dlnode_t * list, size_t n)
{
    ASSUME(n >= 1);
    assert(list+1 == list->next[0]);
    assert(list+2 == list->prev[0]);

    avl_tree_t tree;
    avl_init_tree(&tree, cmp_double_asc_y_des_x);
    avl_node_t * tnodes = malloc((n+2) * sizeof(*tnodes));

    // At the top we insert the first point, which is never dominated.
    dlnode_t * p = (list+1)->next[0];
    avl_node_t * nodeaux = new_avl_node(p, tnodes);
    avl_insert_top(&tree, nodeaux);
    p->cnext[0] = list+1;
    p->cnext[1] = list;

    // After the top node, we insert sentinel 1 (-INF, ref[1])
    avl_node_t * node = new_avl_node(list, tnodes + 1);
    avl_insert_after(&tree, nodeaux, node);
    // Before the top node, we insert sentinel 2 (ref[0], -INF)
    node = new_avl_node(list + 1, tnodes + 2);
    avl_insert_before(&tree, nodeaux, node);
    assert(p->cnext[0] == nodeaux->prev->dlnode);
    assert(p->cnext[1] == nodeaux->next->dlnode);

    const dlnode_t * stop = list + 2;
    p = p->next[0];
    while (p != stop) {
        const double * px = p->x;
        const double * prev_x;
        // == 1 means that nodeaux goes before pj, so move to the next one.
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
            remove_from_z(p);
        } else if (node_point(nodeaux)[1] == px[1]) { // px is dominated by a point in the tree.
            // FIXME: If the points were ordered by asc x we would only need the first condition.
            assert(node_point(nodeaux)[0] < px[0]);
            remove_from_z(p);
        } else {
            assert(node_point(nodeaux)[1] >= px[1]);
            // Delete everything in the tree that is dominated by pj.
            while (node_point(nodeaux)[0] >= px[0]) {
                assert(node_point(nodeaux)[1] >= px[1]);
                nodeaux = nodeaux->next;
                /* FIXME: A possible speed up is to delete without rebalancing
                   the tree because avl_insert_before() will rebalance. */
                avl_unlink_node(&tree, nodeaux->prev);
            }
            node = new_avl_node(p, node + 1);
            avl_insert_before(&tree, nodeaux, node);
            p->cnext[0] = node->prev->dlnode;
            p->cnext[1] = node->next->dlnode;
        }
        p = p->next[0];
    }
    free(tnodes);
}

static inline double
compute_area3d_simple(const double * px, const dlnode_t * q)
{
    return compute_area_simple(px, q, 1);
}

static double
hv3dplus(dlnode_t * list)
{
    restart_list_y(list);
    assert(list+2 == list->prev[0]);

    double area = 0, volume = 0;
    dlnode_t * p = (list+1)->next[0];
    const dlnode_t * stop = list+2;
    while (p != stop) {
        area += compute_area3d_simple(p->x, p->cnext[0]);
        p->cnext[0]->cnext[1] = p;
        p->cnext[1]->cnext[0] = p;
        ASSERT_OR_DO(
            (p->next[0]->x[2] > p->x[2]) || (p->next[0]->x[0] < p->x[0]) || (p->next[0]->x[1] < p->x[1]),
            print_x(p);
            print_x(p->next[0]);
            );
        assert(area > 0);
        /* It is possible to have two points with the same z-value, e.g.,
           (1,2,3) and (2,1,3). */
        volume += area * (p->next[0]->x[2] - p->x[2]);
        p = p->next[0];
    }
    return volume;
}

double
hv3d_plus(const double * restrict data, size_t n, const double * restrict ref)
{
    dlnode_t * list = setup_cdllist(data, n, ref);
    double hv = hv3dplus(list);
    free_cdllist(list);
    return hv;
}
