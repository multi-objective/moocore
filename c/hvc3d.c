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
    // update 'head's of neighbour of 'p'
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

    if (p->cnext[0]->cnext[1]->x[1] > p->x[1]
        || (p->cnext[0]->cnext[1]->x[1] == p->x[1] && p->cnext[0]->cnext[1]->x[0] > p->x[0]))
        p->cnext[0]->cnext[1] = p;

    if (p->cnext[1]->cnext[0]->x[0] > p->x[0]
        || (p->cnext[1]->cnext[0]->x[0] == p->x[0] && p->cnext[1]->cnext[0]->x[1] > p->x[1]))
        p->cnext[1]->cnext[0] = p;
}


static void
update_volume(dlnode_t * q, double z)
{
    // FIXME: Sometimes q is a sentinel and this step is useless. How to avoid updating sentinels?
    //assert(q->area > 0);
    q->volume += q->area * (z - q->last_slice_z);
    q->last_slice_z = z;
}

static void
update_volume_simple(const double * px, dlnode_t * q, uint_fast8_t i)
{
    ASSUME(i == 0 || i == 1);
    const uint_fast8_t j = 1 - i;
    update_volume(q->cnext[j], px[2]);
    while (px[j] < q->x[j]) {
        update_volume(q, px[2]);
        q = q->cnext[i];
    }
    update_volume(q, px[2]);
}

/*
 * Compute all contributions.
 *
 * This code corresponds to HVC3D algorithm as described in the paper.
 * The main difference is that, althought each p maintains a list of its delimiters in 2D (p.L)
 * through 'head', this list does not contain the outer delimiters (these are accessible
 * through 'cnext') nor does it copy points to its list. Only one copy of each point exist in
 * the whole program.
 */
static double
hvc3d_list(dlnode_t * list)
{
    assert(list->next[0] == list+1);
    assert(list->prev[0] == list+2);
    dlnode_t * p = (list+1)->next[0];
    const dlnode_t * stop = list+2;
    if (p == stop)
        return 0;

    restart_list_y(list);
    // Process the first point.
    p->volume = 0;
    p->last_slice_z = p->x[2];
    setup_nondominated_point(p);
    p->area = compute_area_simple(p->x, p->cnext[0], p->head[1], 1);
    double area = p->area;
    add_nondominated_point(p);
    assert(area > 0);
    double volume = area * (p->next[0]->x[2] - p->x[2]);
    p = p->next[0];

    while (p != stop) {
        p->volume = 0;
        p->last_slice_z = p->x[2];

        setup_nondominated_point(p);
        assert(p->head[1] == p->cnext[0]->cnext[1]);
        // FIXME: Sometimes p->head[1]->cnext[0] == list+1, so we update a sentinel.
        update_volume_simple(p->x, p->head[1], 1);
        assert(p->head[1] == p->cnext[0]->cnext[1]);
        p->area = compute_area_simple(p->x, p->cnext[0], p->head[1], 1);
        area += p->area;

        dlnode_t * q = p->cnext[0];
        double x[] = { q->x[0], p->x[1] }; // join(p,q) - (x[2] is not important)
        //x[0] = q->x[0]; x[1] = p->x[1]; x[2] = p->x[2];
        q->area -= compute_area_simple(x, p->head[1], q->head[0], 0);

        q = p->cnext[1];
        x[0] = p->x[0]; x[1] = q->x[1];
        q->area -= compute_area_simple(x, p->head[0], q->head[1], 1);

        add_nondominated_point(p);
        assert(area > 0);
        /* FIXME: It is possible to have two points with the same z-value,
           e.g., (1,2,3) and (2,1,3). In that case, we should just update the
           area and skip most of the steps above. */
        volume += area * (p->next[0]->x[2] - p->x[2]);
        p = p->next[0];
    }
    setup_nondominated_point(p);
    // FIXME: p->head[1]->cnext[0] is always a sentinel, which is pointless to update.
    assert(p->head[1]->cnext[0] == list+1);
    update_volume_simple(p->x, p->head[1], 1);
    return volume;
}

static void
save_contributions(double * hvc, const dlnode_t * list, const double * restrict data)
{
    // FIXME: We could just loop over list+3 n times?
    // - We need to mark dominated points also as ignored.
    // - and we need to keep track of n
    /*
    for (size_t i = 0; i < n; i++) {
        const dlnode_t * p = list+3+i;
        if (!p->ignore)
            hvc[(p->x - data)/3] = p->volume;
    }
    */
    assert(list+1 == list->next[0]);
    const dlnode_t * p = (list+1)->next[0];
    const dlnode_t * stop = list+2;
    while (p != stop) {
        /* print_x(p); */
        /* fprintf(stderr, "hvc = %g\n", p->volume); */
        if (!p->ignore)
            hvc[(p->x - data)/3] = p->volume;
        p = p->next[0];
    }
}

/*  The caller must have initialized hvc to zero.  */
double
hvc3d(double * restrict hvc, const double * restrict data, size_t n, const double * restrict ref)
{
    dlnode_t * list = setup_cdllist(data, n, ref);
    double hv = hvc3d_list(list);
    save_contributions(hvc, list, data);
    free_cdllist(list);
    return hv;
}
