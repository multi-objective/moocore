/******************************************************************************
 HVC3D+ algorithm.
 ------------------------------------------------------------------------------

                        Copyright (C) 2013, 2016, 2017
                     Andreia P. Guerreiro <apg@dei.uc.pt>

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.

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
#include "common.h"
#define HV_DIMENSION 3
#define HVC_ONLY 1
#include "hv_priv.h"


static int compare_tree_asc_y( const void *p1, const void *p2)
{
    const double x1= *((const double *)p1+1);
    const double x2= *((const double *)p2+1);

    if (x1 < x2)
        return -1;
    else if (x1 > x2)
        return 1;
    else return 0;
}

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
print_x(const dlnode_t * p)
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

/* FIXME: There is a similar preprocessing() in hv3dplus.c. Why the differences? */
static inline void
preprocessing(dlnode_t * list, size_t n)
{
    avl_tree_t tree;
    avl_init_tree(&tree, compare_tree_asc_y); // FIXME: cmp_double_asc_y_des_x
    avl_node_t * tnodes = malloc((n+2) * sizeof(*tnodes));
    // FIXME: See hv3dplus preprocessing. We should insert the first point at the top.
    dlnode_t * p = list;
    avl_node_t * nodeaux = new_avl_node(p, tnodes);
    avl_insert_top(&tree, nodeaux);
    /* p->cnext[0] = list+1; */
    /* p->cnext[1] = list; */
    assert(list+1 == list->next[0]);
    p = p->next[0];
    avl_node_t * node = new_avl_node(p, tnodes + 1);
    avl_insert_before(&tree, nodeaux, node);
    p = p->next[0];

    assert(list+2 == list->prev[0]);
    const dlnode_t * stop = list+2;

    while (p != stop) {
        const double * px = p->x;
        // == 1 means that nodeaux goes before p, so move to the next one.
        if (avl_search_closest(&tree, px, &nodeaux) == 1)
            nodeaux = nodeaux->next;
        const double * prev_x = node_point(nodeaux);

        // FIXME: Can we detect duplicates here? If two points are duplicates,
        // both should contribute zero!
        if (prev_x[1] == px[1] && prev_x[0] <= px[0]) {
            nodeaux = nodeaux->next;
        }
        avl_node_t * prev = nodeaux->prev;
        prev_x = node_point(prev);

        assert(prev_x[1] <= px[1]);
        assert(prev_x[2] <= px[2]);
        if (prev_x[0] <= px[0] && prev_x[1] <= px[1] && prev_x[2] <= px[2]) {
            if (all_equal(prev_x, px, 3)) {
                p->ndomr = 2;
                remove_from_z(p);
                prev->dlnode->ndomr = 2; // So we will skip it later.
            } else if (prev->dlnode->ndomr >= 1 || node_point(prev->prev)[0] <= px[0]) {
                // prev_x was already dominated or there is another point that dominates it.
                p->ndomr = 2;
                remove_from_z(p);
            } else {
                p->ndomr = 1;
                assert(prev->dlnode->x == prev_x);
                p->domr = prev->dlnode;
                // FIXME: Do we need to insert this dominated point in the tree?
                // FIXME: Dont we need to setup p->closest?
            }
        } else {
            // Delete everything in the tree that is dominated by p.
            while (node_point(nodeaux)[0] >= px[0]) {
                nodeaux = nodeaux->next;
                /* FIXME: A possible speed up is to delete without rebalancing
                   the tree because avl_insert_before() will rebalance. */
                avl_unlink_node(&tree, nodeaux->prev);
            }
            node = new_avl_node(p, node + 1);
            avl_insert_before(&tree, nodeaux, node);
            // FIXME: p->cnext ?
            p->closest[0] = node->prev->dlnode;
            p->closest[1] = node->next->dlnode;
        }
        p = p->next[0];
    }
    free(tnodes);
}

//lexicographic order of coordinates (z,y,x)
static int compare_point3d(const void *p1, const void* p2)
{
    int i;
    for(i = 2; i >= 0; i--){
        double x1 = (*((const double **)p1))[i];
        double x2 = (*((const double **)p2))[i];

        if(x1 < x2)
            return -1;
        if(x1 > x2)
            return 1;
    }
    return 0;
}

static inline void
set_cnext_to_closest(dlnode_t * p)
{
    p->cnext[0] = p->closest[0];
    p->cnext[1] = p->closest[1];
    assert(p->cnext[0]);
    assert(p->cnext[1]);
}

/*
 * Setup circular double-linked list in each dimension
 */
static dlnode_t *
hvc_setup_cdllist(const double * data, size_t n, const double *ref)
{
    const dimension_t dim = HV_DIMENSION;
    const double **scratch = malloc(n * sizeof(*scratch));
    size_t i, j;
    for (i = 0, j = 0; j < n; j++) {
        /* Filter those points that do not strictly dominate the reference
           point.  This is needed to ensure that the points left are only those
           that are needed to calculate the hypervolume. */
        if (likely(strongly_dominates(data + j * dim, ref, dim))) {
            scratch[i] = data + j * dim;
            i++;
        }
    }
    n = i; // Update number of points.
    if (likely(n > 1))
        qsort(scratch, n, sizeof(*scratch), compare_point3d); // FIXME: cmp_double_asc_only_3d

    dlnode_t * list = new_cdllist(n, ref);
    if (unlikely(n == 0)) {
        free(scratch);
        return list;
    }

    const dimension_t d = HV_DIMENSION - 3; // index within the list.
    dlnode_t * q = list+1;
    dlnode_t * list3 = list+3;
    assert(list->next[d] == list + 1);
    assert(q->next[d] == list + 2);
    for (i = 0, j = 0; j < n; j++) {
        dlnode_t * p = list3 + i;
        p->x = scratch[j];
#if HV_DIMENSION == 4 || HVC_ONLY
        p->ndomr = 0;
#endif
#if HV_DIMENSION == 4
        // Initialize it when debugging so it will crash if uninitialized.
        DEBUG1(p->closest[0] = p->closest[1] = NULL);
#endif
        DEBUG1(p->cnext[0] = p->cnext[1] = NULL);
#ifdef HVC_ONLY
        assert(list->next[0] == list+1);
        p->closest[0] = list+1;
        p->closest[1] = list;
        set_cnext_to_closest(p);

        // FIXME: Is all this really needed? Test without it.
        p->head[0] = p->cnext[0]; //HVC-ONLY
        p->head[1] = p->cnext[1]; //HVC-ONLY

        p->area = 0; //HVC-ONLY
        p->volume = 0; //HVC-ONLY
        // FIXME: Is this needed? We set it again in hvc3d_list()
        p->lastSlicez = p->x[2]; //HVC-ONLY
        p->domr = NULL; //HVC-ONLY
#endif
         // Link the list in order.
        q->next[d] = p;
        p->prev[d] = q;
        q = p;
        i++;
    }
    n = i;
    free(scratch);
    assert((list3 + n - 1) == q);
    assert(list+2 == list->prev[d]);
    // q = last point, q->next = s3, s3->prev = last point
    q->next[d] = list+2;
    (list+2)->prev[d] = q;
#if HV_DIMENSION == 3
    preprocessing(list, n);
#endif
    return list;
}



static void
setup_nondominated_point(dlnode_t * p)
{
    set_cnext_to_closest(p);
    p->head[1] = p->cnext[0]->cnext[1];
    p->head[0] = p->cnext[1]->cnext[0];
}

static void
add_nondominated_point(dlnode_t * p)
{
    assert(p->ndomr == 0);
    // update 'head's of neighbour of 'p' only if 'p' is nondominated.
    if (p->ndomr == 0) {
        if (p->cnext[0]->head[1]->x[1] >= p->x[1]) {
            p->cnext[0]->head[1] = p;
            p->cnext[0]->head[0] = p->cnext[0]->cnext[0];
        } else {
            dlnode_t * q = p->cnext[0]->head[0];
            while (q->x[1] >= p->x[1]) {
                q = q->cnext[0];
            }
            p->cnext[0]->head[0] = q;
            q->cnext[1] = p;
        }

        if (p->cnext[1]->head[0]->x[0] >= p->x[0]) {
            p->cnext[1]->head[0] = p;
            p->cnext[1]->head[1] = p->cnext[1]->cnext[1];
        } else {
            dlnode_t * q = p->cnext[1]->head[1];
            while (q->x[0] >= p->x[0]){
                q = q->cnext[1];
            }

            p->cnext[1]->head[1] = q;
            q->cnext[0] = p;
        }
    }

    if (p->cnext[0]->cnext[1]->x[1] > p->x[1]
        || (p->cnext[0]->cnext[1]->x[1] == p->x[1] && p->cnext[0]->cnext[1]->x[0] > p->x[0]))
        p->cnext[0]->cnext[1] = p;

    if (p->cnext[1]->cnext[0]->x[0] > p->x[0]
        || (p->cnext[1]->cnext[0]->x[0] == p->x[0] && p->cnext[1]->cnext[0]->x[1] > p->x[1]))
        p->cnext[1]->cnext[0] = p;
}


static void
setup_dom_point(dlnode_t * p)
{
    set_cnext_to_closest(p);
    const dlnode_t * domr = p->domr;
    p->head[1] = (p->cnext[0]->cnext[1] == domr)
        ? domr->head[1] : p->cnext[0]->cnext[1];
    p->head[0] = (p->cnext[1]->cnext[0] == domr)
        ? domr->head[0] : p->cnext[1]->cnext[0];
}


static void add_dom_point(dlnode_t * p)
{
    p->head[0] = p->cnext[0];
    p->head[1] = p->cnext[1];

    dlnode_t * domr = p->domr;
    if (p->cnext[0]->cnext[1] == domr)
        domr->head[1] = p;
    else
        p->cnext[0]->cnext[1] = p;

    if (p->cnext[1]->cnext[0] == domr)
        domr->head[0] = p;
    else
        p->cnext[1]->cnext[0] = p;
}

static void
update_volume(dlnode_t * q, double z)
{
    q->volume += q->area * (z - q->lastSlicez);
    q->lastSlicez = z;
}

static void
update_volume_simple(const double * px, dlnode_t * q, int i)
{
    const int j = 1 - i;
    update_volume(q->cnext[j], px[2]);
    while (px[j] < q->x[j]) {
        update_volume(q, px[2]);
        q = q->cnext[i];
    }
    update_volume(q, px[2]);
}

/*
  Returns the area dominated by 'p', by sweeping points in ascending order of
  coordinate 'i' (which is either 0 or 1, i.e., x or y), starting from
  point 'q' and stopping when a point nondominated by 'p' and with coordinate
  i¡ higher than that of 'p' on the (x,y)-plane is reached.
       p  - the point whose contributions in 2D is to be computed
       i - dimension used for sweeping points (in ascending order)
       q  - outer delimiter of p (with lower 'i'-coordinate value than p) from
            which to start the sweep.
       u  - The delimiter of p with lowest 'i'-coordinate which is not q. If p has
            inner delimiters, then u is the inner delimiter of p with lowest
            'i'-coordinate, otherwise, u is the outer delimiter with higher
            'i'-coordinate than p. (Note: u is given because of the cases for
            (which p has inner delimiter(s). When p does not have any inner delimiters
            then u=q->cnext[i], but this is not true when inner delimiters exist)
*/
static double
computeAreaSimple(const double * p, const dlnode_t * q, const dlnode_t * u, int i)
{
    int j = 1 - i;
    double area = (q->x[j] - p[j]) * (u->x[i] - p[i]);
    while (p[j] < u->x[j]) {
        q = u;
        u = u->cnext[i];
        area += (q->x[j] - p[j]) * (u->x[i] - q->x[i]);
    }
    return area;
}


/*
 * Compute all contributions.
 *   list - list of points
 *   considerDominated - 1 indicates whether dominated points are admitted, i.e, they decrease
 *                      the contribution of the only point dominating them (domr).
 *                      0 indicates that dominated points should be ignored.
 *
 * This code corresponds to HVC3D algorithm as described in the paper.
 * The main difference is that, althought each p maintains a list of its delimiters in 2D (p.L)
 * through 'head', this list does not contain the outer delimiters (these are accessible
 * through 'cnext') nor does it copy points to its list. Only one copy of each point exist in
 * the whole program.
 */
static double
hvc3d_list(dlnode_t * list, bool considerDominated)
{
    restart_list_y(list);
    assert(list->next[0] == list+1);
    assert(list->prev[0] == list+2);

    double area = 0, volume = 0;
    dlnode_t * p = (list+1)->next[0];
    const dlnode_t * stop = list+2;
    while (p != stop) {
        // FIXME: We do not need most of this for the first point. We could
        // initialize area, p->area and p->volume directly.
        p->area = 0;
        p->volume = 0;
        p->lastSlicez = p->x[2];

        if (p->ndomr < 1) { // p->ndomr == 0
            setup_nondominated_point(p);
            assert(p->head[1] == p->cnext[0]->cnext[1]);
            update_volume_simple(p->x, p->head[1], 1);
            assert(p->head[1] == p->cnext[0]->cnext[1]);
            p->area = computeAreaSimple(p->x, p->cnext[0], p->head[1], 1);
            area += p->area;

            dlnode_t * q = p->cnext[0];
            double x[] = { q->x[0], p->x[1] }; // join(p,q) - (x[2] is not important)
            //x[0] = q->x[0]; x[1] = p->x[1]; x[2] = p->x[2];
            q->area -= computeAreaSimple(x, p->head[1], q->head[0], 0);

            q = p->cnext[1];
            x[0] = p->x[0]; x[1] = q->x[1];
            q->area -= computeAreaSimple(x, p->head[0], q->head[1], 1);

            add_nondominated_point(p);

        } else if (considerDominated && p->ndomr == 1) {
            // If dominated points must be taken into account, then remove the
            // area of their single dominating point that they dominate.
            assert(p->domr->ndomr == 0); // domr cannot be dominated.
            update_volume(p->domr, p->x[2]);
            setup_dom_point(p);
            p->domr->area -= computeAreaSimple(p->x, p->head[0], p->head[1], 1);
            add_dom_point(p);
        }
        assert(p->ndomr <= 1);
        assert(area > 0);
        /* It is possible to have two points with the same z-value, e.g.,
           (1,2,3) and (2,1,3). */
        volume += area * (p->next[0]->x[2] - p->x[2]);
        p = p->next[0];

    }
    setup_nondominated_point(p);
    // FIXME: p->head[1]->cnext[0] may be a sentinel, which is pointless to update.
    update_volume_simple(p->x, p->head[1], 1);
    return volume;
}

static void
save_contributions(double * hvc, const dlnode_t * list, const double * restrict data)
{
    // FIXME: Could we just loop over list++ n times?
    assert(list+1 == list->next[0]);
    const dlnode_t * p = (list+1)->next[0];
    const dlnode_t * stop = list+2;
    while (p != stop) {
        /* print_x(p); */
        /* fprintf(stderr, "hvc = %g\n", p->volume); */
        hvc[(p->x - data)/3] = p->volume;
        p = p->next[0];
    }
}

double
hvc3d(double * restrict hvc, const double * restrict data, size_t n, const double * restrict ref)
{
    // This function already calls preprocessing.
    dlnode_t * list = hvc_setup_cdllist(data, n, ref);
    bool considerDominated = true;
    /* fprintf(stderr, "hvc[%d] = [", n); */
    /* for(size_t i = 0; i < n; i++) */
    /*     fprintf(stderr, "%g ", hvc[i]); */
    /* fprintf(stderr, "]\n"); */
    double hv = hvc3d_list(list, considerDominated);
    save_contributions(hvc, list, data);
    free_cdllist(list);
    return hv;
}
