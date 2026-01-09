#ifndef _HVC4D_PRIV_H
#define _HVC4D_PRIV_H
/******************************************************************************
 Compute hypervolume contributions in 4D. Currently only used by hv.c
 ------------------------------------------------------------------------------

                        Copyright (C) 2025--2026
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

******************************************************************************/

#include "hv4d_priv.h"

/**
   Assumes that the HV3D+ data structure is reconstructed up to z <= newp->x[2].
   Sweeps the points in the data structure in ascending order of y-coordinate, includes newp
   in the z list, sets up  the "closest" data of newp and, if equalz=true (i.e., all points in
   the data structure have x[2] == newp->x[2]), it also updates the "closest" data of the point
   that follows newp according to the lexicographic order (z,y,x).
*/
static inline bool
continue_base_update_z_closest(dlnode_t * restrict list, dlnode_t * restrict newp, bool equalz)
{
    const double newx[] = { newp->x[0], newp->x[1], newp->x[2] };
    assert(list+1 == list->next[0]);
    dlnode_t * p = (list+1)->cnext[1];

    // find newp->closest[0]
    while (p->x[1] < newx[1]){
        assert(p->x[2] <= newx[2]);
        p = p->cnext[1];
    }

    if (p->x[1] != newx[1] || p->x[0] > newx[0])
        p = p->cnext[0];

    assert(lexicographic_less_2d(p->x, newx));
    assert(lexicographic_less_2d(newx, p->cnext[1]->x));

    // Check if newp is dominated.
    if (weakly_dominates(p->x, newx, 3)) {
        return false;
    }
    dlnode_t * lex_prev = p; // newp->closest[0] = lex_prev
    p = lex_prev->cnext[1];

    // if all points in the list have z-coordinate equal to px[2]
    if (equalz) {
        assert(p->cnext[0]->x[1] < newx[1]);
        assert(p->x[1] >= newx[1]);
        // Check if newp dominates points in the list.
        while (p->x[0] >= newx[0]) {
            remove_from_z(p);
            p = p->cnext[1];
        }

        assert(p->x[0] < newx[0]);
        // update the closest of points in the list (max. 1), if needed
        if (p != list)
            p->closest[0] = newp;

        //setup z list
        assert(p == list || lex_prev->next[0] == p);
        newp->prev[0] = lex_prev;
        newp->next[0] = lex_prev->next[0];

        newp->closest[1] = list;

    } else {
        //setup z list
        newp->prev[0] = list->prev[0]->prev[0];
        newp->next[0] = list->prev[0];

        // find newp->closest[1]
        while (p->x[0] >= newx[0]){
            assert(p->x[2] <= newx[2]);
            assert(!weakly_dominates(newx, p->x, 3));
            p = p->cnext[1];
        }
        newp->closest[1] = p;
    }

    newp->closest[0] = lex_prev;
    // update cnext
    lex_prev->cnext[1] = newp;
    newp->cnext[0] = lex_prev;
    newp->cnext[1] = p;
    p->cnext[0] = newp;

    //update z list
    newp->next[0]->prev[0] = newp;
    newp->prev[0]->next[0] = newp;

    return true;
}

static inline void
lex_sort_equal_z_and_setup_nodes(dlnode_t * newp_aux, double * x_aux, size_t n)
{
    const double ** scratch = malloc(n * sizeof(*scratch));
    double * x = x_aux;
    size_t i;
    for (i = 0; i < n; i++){
        scratch[i] = x;
        x += 3;
    }

    qsort(scratch, n, sizeof(*scratch), cmp_ppdouble_asc_rev_2d);

    for (i = 0; i < n; i++) {
        newp_aux->x = scratch[i];
        assert(i == 0 || lexicographic_less_2d(scratch[i-1], scratch[i]));
        newp_aux++;
    }

    free(scratch);
}

/**
   Compute the hv contribution of "the_point" in d=4 by iteratively computing
   the one contribution problem in d=3. */
static inline double
onec4dplusU(dlnode_t * restrict list, dlnode_t * restrict list_aux,
            dlnode_t * restrict the_point)
{
    // FIXME: This is never triggered.
    assert(the_point->ignore < 3);
    if (the_point->ignore >= 3) {
        return 0;
    }

    assert(list+2 == list->prev[0]);
    assert(list+2 == list->prev[1]);
    assert(list+1 == list->next[1]);

    const dlnode_t * const last = list+2;
    dlnode_t * const z_first = (list+1)->next[0];
    dlnode_t * const z_last = last->prev[0];

    double * x_aux = list_aux->vol;
    dlnode_t * newp_aux = list_aux+1; // list_aux is a sentinel

    reset_sentinels_3d(list);
    restart_list_y(list);

    const double * the_point_x = the_point->x;
    // Setup the 3D base only if there are any points leq than the_point_x[3].
    if ((list+1)->next[1] != the_point || the_point_x[3] == the_point->next[1]->x[3]) {
        bool done_once = false;
        // Set the_point->ignore=3 so the loop will skip it, but restore its
        // value after the loop.
        dimension_t the_point_ignore = the_point->ignore;
        the_point->ignore = 3;
        dlnode_t * newp = z_first;
        assert(newp != last);

        // PART 1: Setup 2D base of the 3D base
        while (newp->x[2] <= the_point_x[2]) {
            const double * newpx = newp->x;
            if (newpx[3] <= the_point_x[3] && newp->ignore < 3) {
                if (weakly_dominates(newpx, the_point_x, 3)) {
                    assert(the_point->ignore == 3);
                    // Restore modified links (z list).
                    (list+1)->next[0] = z_first;
                    (list+2)->prev[0] = z_last;
                    return 0;
                }
                // x_aux is the coordinate-wise maximum between newpx and the_point_x.
                upper_bound(x_aux, newpx, the_point_x, 3);
                newp_aux->x = x_aux;
                x_aux += 3;

                if (continue_base_update_z_closest(list, newp_aux, done_once)) {
                    newp_aux++;
                    done_once = true;
                }
            }
            newp = newp->next[0];
        }
        the_point->ignore = the_point_ignore;

        // PART 2: Setup the remainder of the 3D base
        int c = 0;
        while (newp != last) {
            const double * newpx = newp->x;
            if (newpx[3] <= the_point_x[3] && newp->ignore < 3) {
                // x_aux is the coordinate-wise maximum between newpx and the_point_x.
                upper_bound(x_aux, newpx, the_point_x, 3);
                x_aux += 3;
                c++;
            }

            if (c > 0 && newp->next[0]->x[2] > newpx[2]) {
                if (c == 1) {
                    newp_aux->x = x_aux - 3;
                    continue_base_update_z_closest(list, newp_aux, false);
                    newp_aux++;
                } else {
                    // all points with equal z-coordinate will be added to the data structure in lex order
                    lex_sort_equal_z_and_setup_nodes(newp_aux, x_aux - 3*c, c);
                    continue_base_update_z_closest(list, newp_aux, false);
                    const double * prevp_aux_x = newp_aux->x;
                    newp_aux++;

                    for (int i = 1; i < c; i++) {
                        assert(newpx[2] == newp_aux->x[2]); // all c points have equal z
                        assert(prevp_aux_x[1] <= newp_aux->x[1]); // due to lexsort
                        if (newp_aux->x[0] < prevp_aux_x[0]){
                            // if newp_aux is not dominated by prevp
                            continue_base_update_z_closest(list, newp_aux, false);
                        }
                        prevp_aux_x = newp_aux->x;
                        newp_aux++;
                    }
                }
                c = 0;
            }
            newp = newp->next[0];
        }
    }

    dlnode_t * newp = the_point->next[1];
    while (newp->x[3] <= the_point_x[3])
        newp = newp->next[1];

    dlnode_t * tp_prev_z = the_point->prev[0];
    dlnode_t * tp_next_z = the_point->next[0];
    // This call should always return true.
#if DEBUG >= 1
    assert(restart_base_setup_z_and_closest(list, the_point));
#else
    restart_base_setup_z_and_closest(list, the_point);
#endif
    double volume = one_contribution_3d(the_point);
    the_point->prev[0] = tp_prev_z;
    the_point->next[0] = tp_next_z;

    assert(volume > 0);
    double height = newp->x[3] - the_point_x[3];
    // It cannot be zero because we exited the loop above.
    assert(height > 0);
    double hv = volume * height;

    // PART 3: Update the 3D contribution.
    while (newp != last) {
        const double * newpx = newp->x;
        if (weakly_dominates(newpx, the_point_x, 3))
            break;
        if (newp->ignore < 3) {
            // x_aux is the coordinate-wise maximum between newpx and the_point_x.
            upper_bound(x_aux, newpx, the_point_x, 3);
            newp_aux->x = x_aux;
            x_aux += 3;
            if (restart_base_setup_z_and_closest(list, newp_aux)) {
                // newp was not dominated by something else.
                double newp_v = one_contribution_3d(newp_aux);
                assert(newp_v > 0);
                volume -= newp_v;

                add_to_z(newp_aux);
                update_links(list, newp_aux);
                newp_aux++;
            }

            if (weakly_dominates(the_point_x, newpx, 3)){
                newp->ignore = 3;
            }
        }

        // FIXME: If newp was dominated, can we accumulate the height and update
        // hv later? AG: Yes
        height = newp->next[1]->x[3] - newpx[3];
        assert(height >= 0);
        hv += volume * height;

        newp = newp->next[1];
    }

    // Restore z list
    (list+1)->next[0] = z_first;
    (list+2)->prev[0] = z_last;

    return hv;
}

#endif // _HVC4D_PRIV_H
