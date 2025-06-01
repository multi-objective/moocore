/******************************************************************************
 HV4D+ algorithm.
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
#define HV_DIMENSION 4
#include "hv_priv.h"

/* ---------- Data Structures Functions -------------------------------------*/

static inline void _attr_maybe_unused
print_point(const char *s, const double * x)
{
    fprintf(stderr, "%s: %g %g %g %g\n", s, x[0], x[1], x[2], x[4]);
}




/* ---------------------------------- Update data structure ---------------------------------------*/

static inline void
add_to_z(dlnode_t * new)
{
    new->next[0] = new->prev[0]->next[0]; //in case new->next[0] was removed for being dominated
    new->next[0]->prev[0] = new;
    new->prev[0]->next[0] = new;
}

static inline bool
lex_cmp_3d_102(const double * a, const double *b)
{
    return a[1] < b[1] || (a[1] == b[1] && (a[0] < b[0] || (a[0] == b[0] && a[2] < b[2])));
}

static inline bool
lex_cmp_3d_012(const double * a, const double *b)
{
    return a[0] < b[0] || (a[0] == b[0] && (a[1] < b[1] || (a[1] == b[1] && a[2] < b[2])));
}

/*
   Go through the points in the order of z and either remove points that are
   dominated by new with respect to x,y,z or update the cx and cy lists by
   adding new.
*/
static unsigned int
update_links(dlnode_t * list, dlnode_t * new)
{
    assert(list+2 == list->prev[0]);
    unsigned int ndom = 0;
    dlnode_t * p = new->next[0];
    const double * px = p->x;
    const double * newx = new->x;
    dlnode_t * stop = list+2;
    while (p != stop) {
        if (px[0] <= newx[0] && px[1] <= newx[1] && (px[0] < newx[0] || px[1] < newx[1]))
            break;

        if (newx[0] <= px[0]){
            //new <= p
            if (newx[1] <= px[1]){
                assert(weakly_dominates(newx, px, 3));
                p->ndomr++;
                // p->domr = new;
                ndom++;
                remove_from_z(p); //HV-ONLY (does not need dominated to compute HV)
            } else if (newx[0] < px[0] && lex_cmp_3d_102(newx, p->closest[1]->x)) { // newx[1] > px[1]
                p->closest[1] = new;
            }
        } else if (newx[1] < px[1] && lex_cmp_3d_012(newx, p->closest[0]->x)) {//newx[0] > px[0]
            p->closest[0] = new;
        }
        p = p->next[0];
        px = p->x;
    }
    return ndom;
}



// This does what setupZandClosest does while reconstructing L at z = new->x[2].
static void
restart_base_setup_z_and_closest(dlnode_t * list, dlnode_t * new)
{
    assert(list+1 == list->next[0]);
    dlnode_t * closest1 = list;
    dlnode_t * closest0 = list+1;
    const double * closest1x = closest1->x;
    const double * closest0x = closest0->x;
    dlnode_t * p = closest0->next[0];
    assert(p == list->next[0]->next[0]);
    const double * px =  p->x;
    const double * newx = new->x;
    restart_list_y(list);
    while (lexicographic_less_3d(px, newx)) {
        // reconstruct
        p->cnext[0] = p->closest[0];
        p->cnext[1] = p->closest[1];

        p->cnext[0]->cnext[1] = p;
        p->cnext[1]->cnext[0] = p;

        // setup_z_and_closest
        if (px[0] <= newx[0] && px[1] <= newx[1]) {
            new->ndomr++;
            assert(weakly_dominates(px, newx, 4));
            // FIXME: If it is dominated why update new->closest[0] and
            // new->closest[1] and the rest? Why not return here?
            return;
        } else if (px[1] < newx[1] && (px[0] < closest0x[0] || (px[0] == closest0x[0] && px[1] < closest0x[1]))) {
            closest0 = p;
            closest0x = closest0->x;
        } else if (px[0] < newx[0] && (px[1] < closest1x[1] || (px[1] == closest1x[1] && px[0] < closest1x[0]))) {
            closest1 = p;
            closest1x = closest1->x;
        }
        p = p->next[0];
        px = p->x;
    }

    new->closest[0] = closest0;
    new->closest[1] = closest1;

    new->prev[0] = p->prev[0];
    new->next[0] = p;
}

static double
one_contribution_3d(dlnode_t * list, dlnode_t * new)
{
    restart_base_setup_z_and_closest(list, new);
    if (new->ndomr > 0)
        return 0;

    new->cnext[0] = new->closest[0];
    new->cnext[1] = new->closest[1];

    const double * newx = new->x;
    double area = compute_area_simple(newx, new->cnext[0], 1);
    dlnode_t * p = new->next[0];
    const double * px = p->x;
    assert(!weakly_dominates(px, newx, 4));

    double lastz = newx[2];
    double volume = area * (px[2] - lastz);
    while (px[0] > newx[0] || px[1] > newx[1]) {
        p->cnext[0] = p->closest[0];
        p->cnext[1] = p->closest[1];

        if (px[0] >= newx[0] && px[1] >= newx[1]) {
            area -= compute_area_simple(px, p->cnext[0], 1);
            p->cnext[1]->cnext[0] = p;
            p->cnext[0]->cnext[1] = p;

        } else if (px[0] >= newx[0]) {
            if (px[0] <= new->cnext[0]->x[0]) {
                const double tmpx[] = { px[0], newx[1] };
                // if px[0] == new->cnext[0]->x[0] then area starts at 0.
                area -= compute_area_simple(tmpx, new->cnext[0], 1);
                p->cnext[0] = new->cnext[0];
                p->cnext[1]->cnext[0] = p;
                new->cnext[0] = p;
            }
        } else if (px[1] <= new->cnext[1]->x[1]) {
            const double tmpx[] = { newx[0], px[1] };
            area -= compute_area_simple(tmpx, new->cnext[1], 0);
            p->cnext[1] = new->cnext[1];
            p->cnext[0]->cnext[1] = p;
            new->cnext[1] = p;
        }
        assert(!weakly_dominates(px, p->next[0]->x, 4));
        lastz = px[2];
        p = p->next[0];
        px = p->x;
        volume += area * (px[2] - lastz);
    }
    return volume;
}

/* Compute the hypervolume indicator in d=4 by iteratively computing the one
   contribution problem in d=3. */
static double
hv4dplusU(dlnode_t * list)
{
    assert(list+2 == list->prev[0]);
    assert(list+2 == list->prev[1]);
    assert(list+1 == list->next[1]);

    double volume = 0, hv = 0;
    dlnode_t * new = (list+1)->next[1];
    dlnode_t * last = list+2;
    while (new != last) {
        double new_v = one_contribution_3d(list, new);
        assert(new_v > 0 || (new_v == 0 && new->ndomr));
        // if new_v == 0, then new was dominated by something else.
        if (new_v > 0) {
            add_to_z(new);
            // FIXME update_links return ndom but that value is not used by the algorithm?
            update_links(list, new);
            volume += new_v;
        }
        assert(!weakly_dominates(new->x, new->next[1]->x, 4));
        double height = new->next[1]->x[3] - new->x[3];
        hv += volume * height;
        new = new->next[1];
    }
    return hv;
}

double hv4d(const double * restrict data, size_t n, const double * restrict ref)
{
    dlnode_t * list = setup_cdllist(data, n, ref);
    double hv = hv4dplusU(list);
    free_cdllist(list);
    return hv;
}

static dlnode_t *
setup_cdllist_recursive(const double **scratch, size_t n, const double * restrict ref)
{
    ASSUME(n > 1);
    const dimension_t dim = HV_DIMENSION;
    qsort(scratch, n, sizeof(*scratch), cmp_double_asc_rev_4d);
    dlnode_t * list = (dlnode_t *) malloc((n + 3) * sizeof(*list));
    init_sentinels(list, ref);

    const dimension_t d = HV_DIMENSION - 3; // index within the list.
    dlnode_t * q = list+1;
    dlnode_t * list3 = list+3;
    assert(list->next[d] == list + 1);
    assert(q->next[d] == list + 2);
    size_t i,j;
    for (i = 0, j = 0; j < n; j++) {
        if (weakly_dominates(q->x, scratch[j], dim)) {
            /* print_point("q", q->x); */
            /* print_point("i", scratch[j]); */
            continue;
        }
        dlnode_t * p = list3 + i;
        p->x = scratch[j];
        // Initialize it when debugging so it will crash if uninitialized.
        DEBUG1(
            p->closest[0] = NULL;
            p->closest[1] = NULL;
            p->cnext[0] = NULL;
            p->cnext[1] = NULL;);
        p->ndomr = 0;
        // Link the list in order.
        q->next[d] = p;
        p->prev[d] = q;
        q = p;
        i++;
    }
    n = i;
    assert((list3 + n - 1) == q);
    assert(list+2 == list->prev[d]);
    // q = last point, q->next = s3, s3->prev = last point
    q->next[d] = list+2;
    (list+2)->prev[d] = q;
    return list;
}

double hv4d_recursive(const double ** scratch, size_t n, const double * restrict ref)
{
    dlnode_t * list = setup_cdllist_recursive(scratch, n, ref);
    double hv = hv4dplusU(list);
    free_cdllist(list);
    return hv;
}
