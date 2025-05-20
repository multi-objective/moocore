/*************************************************************************

 hv-plus.c

 ---------------------------------------------------------------------

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

 ----------------------------------------------------------------------

 Reference:

 [1] A. P. Guerreiro, C. M. Fonseca, “Computing and Updating Hypervolume Contributions in Up to Four Dimensions”, CISUC Technical Report TR-2017-001, University of Coimbra, 2017

*************************************************************************/


#include <stdlib.h>
#include <limits.h>
#include <float.h>
#include <string.h>
#include "common.h"
#include "sort.h"

typedef struct dlnode {
    const double *x;            // point coordinates (objective vector).
    struct dlnode * closest[2]; // closest[0] == cx, closest[1] == cy
    struct dlnode * cnext[2];   // current next
    struct dlnode * next; // keeps the points sorted according to coordinate 3.
    struct dlnode * prev; // keeps the points sorted according to coordinate 3 (except the sentinel 3).
} dlnode_t;

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

static void initSentinels(dlnode_t * list, const double * ref)
{
    /* The list that keeps the points sorted according to the 3rd-coordinate
       does not really need the 3 sentinels, just one to represent (-inf, -inf,
       ref[2]).  But we need the other two to maintain a list of nondominated
       projections in the (x,y)-plane of points that is kept sorted according
       to the 1st and 2nd coordinates, and for that list we need two sentinels
       to represent (-inf, ref[1]) and (ref[0], -inf). */
    dlnode_t * s1 = list;
    dlnode_t * s2 = list + 1;
    dlnode_t * s3 = list + 2;

    // Allocate the 3 sentinels of dimension 3.
    double * z = malloc(3 * 3 * sizeof(double));
    // Sentinel 1 (-INF, ref[1], -INF)
    z[0] = -DBL_MAX;
    z[1] = ref[1];
    z[2] = -DBL_MAX;
    s1->x = z;
    s1->closest[0] = s2;
    s1->closest[1] = s1;

    s1->next = s2;
    s1->cnext[0] = NULL;
    s1->cnext[1] = NULL;
    s1->prev = s3;

    // Sentinel 2 (ref[0], -INF, -INF)
    z += 3;
    z[0] = ref[0];
    z[1] = -DBL_MAX;
    z[2] = -DBL_MAX;
    s2->x = z;
    s2->closest[0] = s2;
    s2->closest[1] = s1;

    s2->next = s3;
    s2->cnext[0] = NULL;
    s2->cnext[1] = NULL;
    s2->prev = s1;

    // Sentinel 3 (-INF, -INF, ref[2])
    z += 3;
    z[0] = -DBL_MAX;
    z[1] = -DBL_MAX;
    z[2] = ref[2];
    s3->x = z;
    s3->closest[0] = s2;
    s3->closest[1] = s1;

    s3->next = s1;
    s3->cnext[0] = NULL;
    s3->cnext[1] = NULL;
    s3->prev = s2;
}



/* ---------------------------------- Sort ---------------------------------------*/

static int
compare_point3d(const void * restrict p1, const void * restrict p2)
{
    const double *x1 = *((const double **)p1);
    const double *x2 = *((const double **)p2);
    for (int i = 2; i >= 0; i--) {
        if (x1[i] < x2[i])
            return -1;
        if (x1[i] > x2[i])
            return 1;
    }
    return 0;
}

static void print_x(dlnode_t * p)
{
    assert(p != NULL);
    const double * x = p->x;
    fprintf(stderr, "x: %g %g %g\n", x[0], x[1], x[2]);
}


/* ---------------------------------- Preprocessing ---------------------------------------*/


static int
cmp_double_asc_y_des_x(const void * restrict p1, const void * restrict p2)
{
    const double x1 = *(const double *)p1;
    const double x2 = *(const double *)p2;
    const double y1 = *((const double *)p1+1);
    const double y2 = *((const double *)p2+1);
    return (y1 < y2) ? -1: ((y1 > y2) ? 1 : (x1 > x2 ? -1 : 1));
}


static inline const double *node_point(const avl_node_t *node)
{
    return node->item;
}


static avl_node_t * new_avl_node(dlnode_t * p, avl_node_t * node)
{
    node->dlnode = p;
    node->item = p->x;
    return node;
}

/* ---------------------------------- Update data structure ---------------------------------------*/

static inline void removeFromz(dlnode_t * old)
{
    old->prev->next = old->next;
    old->next->prev = old->prev;
}


/*
  This implements a variant of the 3D dimension-sweep algorithm by H. T. Kung,
   F. Luccio, and F. P. Preparata.  On Finding the Maxima of a Set of
   Vectors. Journal of the ACM, 22(4):469–476, 1975.

   The main different is that the order of the points is 2D tracked by p->closest.
*/

static void preprocessing(dlnode_t * list, size_t n)
{
    ASSUME(n >= 1);

    avl_tree_t tree;
    avl_init_tree(&tree, cmp_double_asc_y_des_x);
    avl_node_t * tnodes = malloc((n+2) * sizeof(*tnodes));

    // At the top we insert the first point, which is never dominated.
    dlnode_t * p = (list+1)->next;
    avl_node_t * nodeaux = new_avl_node(p, tnodes);
    avl_insert_top(&tree, nodeaux);
    p->closest[0] = list+1;
    p->closest[1] = list;

    // Before the top node, we insert sentinel 2 (ref[0], -INF)
    avl_node_t * node = new_avl_node(list + 1, tnodes + 1);
    avl_insert_before(&tree, nodeaux, node);

    // After the top node, we insert sentinel 1 (-INF, ref[1])
    assert(list+1 == list->next);
    node = new_avl_node(list, tnodes + 2);
    avl_insert_after(&tree, nodeaux, node);
    assert(p->closest[0] == nodeaux->prev->dlnode);
    assert(p->closest[1] == nodeaux->next->dlnode);

    const dlnode_t * stop = list + 2;
    assert(stop == list->prev);
    p = p->next;
    while (p != stop) {
        const double * prev_x;
        // == 1 means that nodeaux goes before pj, so move to the next one.
        if (avl_search_closest(&tree, p->x, &nodeaux) == 1) {
            prev_x = node_point(nodeaux);
            nodeaux = nodeaux->next;
        } else {
            prev_x = node_point(nodeaux->prev);
        }
        assert(node_point(nodeaux)[1] > p->x[1] // node_point(nodeaux) comes after p->x.
               || (node_point(nodeaux)[1] == p->x[1] && node_point(nodeaux)[0] < p->x[0]));
        assert(prev_x[1] <= p->x[1]);
        assert(prev_x[2] <= p->x[2]);
        if (prev_x[0] <= p->x[0]) { // p->x is dominated by a point in the tree.
            removeFromz(p);
        } else if (node_point(nodeaux)[1] == p->x[1]) { // p->x is dominated by a point in the tree.
            // FIXME: If the points were ordered by asc x we would only need the first condition.
            assert(node_point(nodeaux)[0] < p->x[0]);
            removeFromz(p);
        } else {
            assert(node_point(nodeaux)[1] >= p->x[1]);
            // Delete everything in the tree that is dominated by pj.
            while (node_point(nodeaux)[0] >= p->x[0]) {
                assert(node_point(nodeaux)[1] >= p->x[1]);
                nodeaux = nodeaux->next;
                /* FIXME: A possible speed up is to delete without rebalancing
                   the tree because avl_insert_before() will rebalance. */
                avl_unlink_node(&tree, nodeaux->prev);
            }
            node = new_avl_node(p, node + 1);
            avl_insert_before(&tree, nodeaux, node);
            p->closest[0] = node->prev->dlnode;
            p->closest[1] = node->next->dlnode;
        }
        p = p->next;
    }
    free(tnodes);
}

/*
 * Setup circular double-linked list in each dimension
 */
static dlnode_t *
setup_cdllist(const double * restrict data, size_t n, const double * restrict ref)
{
    ASSUME(n > 1);
    const dimension_t d = 3;

    const double **scratchd = malloc(n * sizeof(const double*));
    size_t i, j;
    for (i = 0, j = 0; j < n; j++) {
        /* Filters those points that do not strictly dominate the reference
           point.  This is needed to ensure that the points left are only those
           that are needed to calculate the hypervolume. */
        if (strongly_dominates(data + j * d, ref, d)) { /* TODO: unlikely */
            scratchd[i] = data + j * d;
            i++;
        }
    }
    n = i; // Update number of points.

    dlnode_t * list = (dlnode_t *) malloc((n + 3) * sizeof(dlnode_t));
    dlnode_t * list3 = list+3;
    initSentinels(list, ref);
    if (n == 0) { /* TODO: unlikely */
        free(scratchd);
        return list;
    }

    qsort(scratchd, n, sizeof(const double*), compare_point3d);

    dlnode_t * q = list+1;
    assert(list->next == list + 1);
    assert(q->next == list + 2);
    for (size_t i = 0; i < n; i++) {
        dlnode_t * p = list3 + i;
        p->x = scratchd[i];
        // clearPoint:
        assert(list->next == list + 1);
        // FIXME: Do we need to initialize this here?
        p->closest[0] = list + 1;
        p->closest[1] = list;
        // FIXME: Do we need to initialize this here?
        p->cnext[0] = list + 1;
        p->cnext[1] = list;
        // Link the list in order.
        q->next = p;
        p->prev = q;
        q = p;
    }
    free(scratchd);
    q = list->prev;
    (list3 + n - 1)->next = q;
    q->prev = list3 + n - 1;
    preprocessing(list, n);
    return list;
}



static void free_cdllist(dlnode_t * list)
{
    free((void*) list->x); // Free sentinels.
    free(list);
}





/* ----------------------Hypervolume Indicator Algorithms ---------------------------------------*/



static double
compute_area3d_simple(const double * px, const dlnode_t * q)
{
    const dlnode_t * u = q->cnext[1];
    double area = (q->x[0] - px[0]) * (u->x[1] - px[1]);
    assert(area > 0);
    while (px[0] < u->x[0]) {
        q = u;
        u = u->cnext[1];
        assert((q->x[0] - px[0]) * (u->x[1] - q->x[1]) > 0);
        area += (q->x[0] - px[0]) * (u->x[1] - q->x[1]);
    }
    return area;
}

static double
hv3dplus(dlnode_t * list)
{
    // restartList:
    list->next->cnext[1] = list; // Link sentinels (-inf ref[1] -inf) and (ref[0] -inf -inf).
    list->cnext[0] = list+1;
    assert(list + 1 == list->next);

    double area = 0;
    double volume = 0;
    dlnode_t * p = (list+1)->next;
    dlnode_t * stop = list+2;
    assert(stop == list->prev);
    while (p != stop) {
        p->cnext[0] = p->closest[0];
        p->cnext[1] = p->closest[1];
        area += compute_area3d_simple(p->x, p->cnext[0]);
        p->cnext[0]->cnext[1] = p;
        p->cnext[1]->cnext[0] = p;
        ASSERT_OR_DO(
            (p->next->x[2] > p->x[2]) || (p->next->x[0] < p->x[0]) || (p->next->x[1] < p->x[1]),
            print_x(p);
            print_x(p->next);
            );
        assert(area > 0);
        volume += area * (p->next->x[2] - p->x[2]);
        p = p->next;
    }
    return volume;

}

double hv3d_plus(const double * restrict data, int n, const double * restrict ref)
{
    ASSUME(n >= 0);
    dlnode_t * list = setup_cdllist(data, (size_t) n, ref);
    double hv = hv3dplus(list);
    free_cdllist(list);
    return hv;
}
