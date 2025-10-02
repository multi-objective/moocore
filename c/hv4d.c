/******************************************************************************
 HV4D+ algorithm.
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

#include <float.h>
#include <string.h>
#include "common.h"
#define HV_DIMENSION 4
#include "hv_priv.h"

// ---------- Data Structures Functions ---------------------------------------

static inline void _attr_maybe_unused
print_point(const char * s, const double * x)
{
    fprintf(stderr, "%s: %g %g %g %g\n", s, x[0], x[1], x[2], x[4]);
}


// ------------ Update data structure -----------------------------------------

static inline void
add_to_z(dlnode_t * new)
{
    new->next[0] =
        new->prev[0]
            ->next[0]; //in case new->next[0] was removed for being dominated
    new->next[0]->prev[0] = new;
    new->prev[0]->next[0] = new;
}

static inline bool
lex_cmp_3d_102(const double * restrict a, const double * restrict b)
{
    return a[1] < b[1] ||
           (a[1] == b[1] && (a[0] < b[0] || (a[0] == b[0] && a[2] < b[2])));
}

static inline bool
lex_cmp_3d_012(const double * restrict a, const double * restrict b)
{
    return a[0] < b[0] ||
           (a[0] == b[0] && (a[1] < b[1] || (a[1] == b[1] && a[2] < b[2])));
}

/*
   Go through the points in the order of z and either remove points that are
   dominated by new with respect to x,y,z or update the cx and cy lists by
   adding new.
*/
static void
update_links(dlnode_t * restrict list, dlnode_t * restrict new)
{
    assert(list + 2 == list->prev[0]);
    const double * newx = new->x;
    dlnode_t * p = new->next[0];
    const double * px = p->x;
    dlnode_t * stop = list + 2;
    while (p != stop) {
        // px dominates newx (but not equal)
        if (px[0] <= newx[0] && px[1] <= newx[1] &&
            (px[0] < newx[0] || px[1] < newx[1]))
            return;

        if (newx[0] <= px[0]) {
            //new <= p
            if (newx[1] <= px[1]) {
                assert(weakly_dominates(newx, px, 3));
                //p->ndomr++;
                remove_from_z(p);
            } else if (newx[0] < px[0] &&
                       lex_cmp_3d_102(newx,
                                      p->closest[1]->x)) { // newx[1] > px[1]
                p->closest[1] = new;
            }
        } else if (newx[1] < px[1] &&
                   lex_cmp_3d_012(newx, p->closest[0]->x)) { //newx[0] > px[0]
            p->closest[0] = new;
        }
        p = p->next[0];
        px = p->x;
    }
}


// This does what setupZandClosest does while reconstructing L at z = new->x[2].
__attribute__((hot)) static bool
restart_base_setup_z_and_closest(dlnode_t * restrict list,
                                 dlnode_t * restrict new)
{
    const double * restrict newx = new->x;
    // FIXME: This is the most expensive function in the HV4D+ algorithm.
    assert(list + 1 == list->next[0]);
    dlnode_t * closest1 = list;
    dlnode_t * closest0 = list + 1;
    const double * closest0x = closest0->x;
    const double * closest1x = closest1->x;
    dlnode_t * p = list + 1;
    assert(p->next[0] == list->next[0]->next[0]);
    restart_list_y(list);
    while (true) {
        p = p->next[0];
        const double * restrict px = p->x;
        // Help auto-vectorization.
        bool p_leq_new_0 = px[0] <= newx[0];
        bool p_leq_new_1 = px[1] <= newx[1];
        bool p_leq_new_2 = px[2] <= newx[2];

        if (p_leq_new_0 & p_leq_new_1 & p_leq_new_2) {
            //new->ndomr++;
            assert(weakly_dominates(px, newx, 4));
            return false;
        }

        if (!lexicographic_less_3d(px, newx)) break;

        // reconstruct
        set_cnext_to_closest(p);
        p->cnext[0]->cnext[1] = p;
        p->cnext[1]->cnext[0] = p;

        // setup_z_and_closest
        assert(px[0] > newx[0] || px[1] > newx[1]);
        if (px[1] < newx[1] &&
            (px[0] < closest0x[0] ||
             (px[0] == closest0x[0] && px[1] < closest0x[1]))) {
            closest0 = p;
            closest0x = px;
        } else if (px[0] < newx[0] &&
                   (px[1] < closest1x[1] ||
                    (px[1] == closest1x[1] && px[0] < closest1x[0]))) {
            closest1 = p;
            closest1x = px;
        }
    }

    new->closest[0] = closest0;
    new->closest[1] = closest1;

    new->prev[0] = p->prev[0];
    new->next[0] = p;
    return true;
}

// FIXME: This is very similar to the loop in hvc3d_list() but it doesn't use p->last_slice_z
static double
one_contribution_3d(dlnode_t * restrict new)
{
    set_cnext_to_closest(new);
    const double * newx = new->x;
    // if newx[0] == new->cnext[0]->x[0], the first area is zero
    double area = compute_area_no_inners(newx, new->cnext[0], 1);
    double volume = 0;
    dlnode_t * p = new;
    const double * px = p->x;
    assert(!weakly_dominates(p->next[0]->x, newx, 4));
    while (true) {
        double lastz = px[2];
        p = p->next[0];
        px = p->x;
        volume += area * (px[2] - lastz);

        if (px[0] <= newx[0] && px[1] <= newx[1]) return volume;

        assert(px[0] > newx[0] || px[1] > newx[1]);
        assert(!weakly_dominates(px, p->next[0]->x, 4));

        set_cnext_to_closest(p);

        if (px[0] < newx[0]) {
            if (px[1] <= new->cnext[1]->x[1]) {
                const double tmpx[] = {newx[0], px[1]};
                // if px[1] == new->cnext[1]->x[1] then area starts at 0.
                area -= compute_area_no_inners(tmpx, new->cnext[1], 0);
                p->cnext[1] = new->cnext[1];
                p->cnext[0]->cnext[1] = p;
                new->cnext[1] = p;
            }
        } else if (px[1] < newx[1]) {
            if (px[0] <= new->cnext[0]->x[0]) {
                const double tmpx[] = {px[0], newx[1]};
                // if px[0] == new->cnext[0]->x[0] then area starts at 0.
                area -= compute_area_no_inners(tmpx, new->cnext[0], 1);
                p->cnext[0] = new->cnext[0];
                p->cnext[1]->cnext[0] = p;
                new->cnext[0] = p;
            }
        } else {
            assert(px[0] >= newx[0] && px[1] >= newx[1]);
            // if px[0] == p->cnext[0]->x[0] then area starts at 0.
            area -= compute_area_no_inners(px, p->cnext[0], 1);
            p->cnext[1]->cnext[0] = p;
            p->cnext[0]->cnext[1] = p;
        }
    }
    return volume;
}

/* Compute the hypervolume indicator in d=4 by iteratively computing the one
   contribution problem in d=3. */
double
hv4dplusU(dlnode_t * list)
{
    assert(list + 2 == list->prev[0]);
    assert(list + 2 == list->prev[1]);
    assert(list + 1 == list->next[1]);

    double volume = 0, hv = 0;
    dlnode_t * new = (list + 1)->next[1];
    dlnode_t * last = list + 2;
    while (new != last) {
        if (restart_base_setup_z_and_closest(list, new)) {
            // new was not dominated by something else.
            double new_v = one_contribution_3d(new);
            assert(new_v > 0);
            volume += new_v;
            add_to_z(new);
            update_links(list, new);
        }
        double height = new->next[1]->x[3] - new->x[3];
        assert(height >= 0);
        hv += volume * height;
        new = new->next[1];
    }
    return hv;
}

double
hv4d(const double * restrict data, size_t n, const double * restrict ref)
{
    dlnode_t * list = setup_cdllist(data, n, ref);
    double hv = hv4dplusU(list);
    free_cdllist(list);
    return hv;
}
