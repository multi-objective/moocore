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
#include <stdio.h>
#include <limits.h>
#include <float.h>
#include <string.h>
#include "common.h"
#include "hv_priv.h"

/* ---------------------------------- Data Structure ---------------------------------------*/

typedef struct dlnode {
    const double *x;            // point coordinates (objective vector).
    struct dlnode * closest[2]; // closest[0] == cx, closest[1] == cy
    struct dlnode * cnext[2]; //current next
    struct dlnode * next[2]; /* keeps the points sorted according to coordinates 2,3 and 4
                                (in the case of 2 and 3, only the points swept by 4 are kept) */
    struct dlnode * prev[2]; //keeps the points sorted according to coordinates 2 and 3 (except the sentinel 3)
    int ndomr;    //number of dominators
} dlnode_t;



/* ---------------------------------- Data Structures Functions ---------------------------------------*/


static void
init_sentinels(dlnode_t * list, const double * ref, dimension_t dim)
{
    // Allocate the 3 sentinels of dimension dim.
    const double z4[] = {
        -DBL_MAX, ref[1], -DBL_MAX, -DBL_MAX, // Sentinel 1
        ref[0], -DBL_MAX, -DBL_MAX, -DBL_MAX, // Sentinel 2
        -DBL_MAX, -DBL_MAX, ref[2], ref[3]    // Sentinel 2
    };

    double * x = malloc(sizeof(z4));
    memcpy(x, z4, sizeof(z4));

    /* The list that keeps the points sorted according to the 3rd-coordinate
       does not really need the 3 sentinels, just one to represent (-inf, -inf,
       ref[2]).  But we need the other two to maintain a list of nondominated
       projections in the (x,y)-plane of points that is kept sorted according
       to the 1st and 2nd coordinates, and for that list we need two sentinels
       to represent (-inf, ref[1]) and (ref[0], -inf). */
    dlnode_t * s1 = list;
    dlnode_t * s2 = list + 1;
    dlnode_t * s3 = list + 2;

    // Sentinel 1
    s1->x = x;
    s1->closest[0] = s2;
    s1->closest[1] = s1;
    s1->cnext[1] = NULL;
    s1->cnext[0] = NULL;
    s1->next[0] = s2;
    s1->next[1] = s2;
    s1->prev[0] = s3;
    s1->prev[1] = s3;
    s1->ndomr = 0;

    x += dim;
    // Sentinel 2
    s2->x = x;
    s2->closest[0] = s2;
    s2->closest[1] = s1;
    s2->cnext[1] = NULL;
    s2->cnext[0] = NULL;
    s2->next[0] = s3;
    s2->next[1] = s3;
    s2->prev[0] = s1;
    s2->prev[1] = s1;
    s2->ndomr = 0;

    x += dim;
    // Sentinel 3
    s3->x = x;
    s3->closest[0] = s2;
    s3->closest[1] = s1;
    s3->cnext[1] = NULL;
    s3->cnext[0] = NULL;
    s3->next[0] = s1;
    s3->next[1] = s1;
    s3->prev[0] = s2;
    s3->prev[1] = s2;
    s3->ndomr = 0;
}





/* ---------------------------------- Update data structure ---------------------------------------*/




static inline void
add_to_z(dlnode_t * new)
{
    new->next[0] = new->prev[0]->next[0]; //in case new->next[0] was removed for being dominated
    new->next[0]->prev[0] = new;
    new->prev[0]->next[0] = new;
}

static inline void
remove_from_z(dlnode_t * old)
{
    old->prev[0]->next[0] = old->next[0];
    old->next[0]->prev[0] = old->prev[0];
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

static int
update_links(dlnode_t * list, dlnode_t * new)
{
    int ndom = 0;
    const double * newx = new->x;
    dlnode_t * p = new->next[0];
    const double * px = p->x;
    dlnode_t * stop = list->prev[0];
    assert(stop == list+2);
    while (p != stop) {
        if (px[0] <= newx[0] && px[1] <= newx[1] && (px[0] < newx[0] || px[1] < newx[1]))
            break;

        if (newx[0] <= px[0]){
            //new <= p
            if (newx[1] <= px[1]){
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

static inline bool
weakly_dominates(const double * a, const double * b, dimension_t dim)
{
    ASSUME(dim >= 1);
    for (dimension_t d = 0; d < dim; d++)
        if (a[d] > b[d])
            return false;
    return true;
}

static int
compare_point4d(const void * p1, const void * p2)
{
    const double * x1 = *((const double **)p1);
    const double * x2 = *((const double **)p2);

    for (int i = 3; i >= 0; i--) {
        if (x1[i] < x2[i])
            return -1;
        if (x1[i] > x2[i])
            return 1;
    }
    return 0;
}

static void print_point(const char *s, const double * x)
{
    fprintf(stderr, "%s: %g %g %g %g\n", s, x[0], x[1], x[2], x[4]);
}



/*
 * Setup circular double-linked list in each dimension
 */
static dlnode_t *
setup_cdllist(const double * restrict data, size_t n, const double * restrict ref, dimension_t dim)
{
    ASSUME(n >= 1);
    const double **scratch = new_sorted_scratch(data, &n, dim, ref, compare_point4d);
    dlnode_t * list = (dlnode_t *) malloc((n + 3) * sizeof(*list));
    init_sentinels(list, ref, dim);
    if (n == 0) {
        free(scratch);
        return list;
    }

    dlnode_t * q = list+1;
    dlnode_t * list3 = list+3;
    assert(list->next[1] == list + 1);
    assert(q->next[1] == list + 2);
    size_t j = 0;
    for (size_t i = 0; i < n; i++) {
        if (weakly_dominates(q->x, scratch[i], dim)) {
            /* print_point("q", q->x); */
            /* print_point("i", scratch[i]); */
            continue;
        }
        dlnode_t * p = list3 + j;
        p->x = scratch[i];
        assert(list->next[0] == list + 1);
        assert(list->next[1] == list + 1);
        // FIXME: Do we need to initialize this here?
        p->closest[0] = list + 1;
        p->closest[1] = list;
        // FIXME: Do we need to initialize this here?
        p->cnext[0] = list + 1;
        p->cnext[1] = list;
        p->ndomr = 0;
        // Link the list in order.
        q->next[1] = p;
        p->prev[1] = q;
        q = p;
        j++;
    }
    n = j;
    free(scratch);
    assert((list3 + n - 1) == q);
    assert(list+2 == list->prev[1]);
    q = list->prev[1];
    (list3 + n - 1)->next[1] = q;
    q->prev[1] = list3 + n - 1;
    return list;
}


static void
free_cdllist(dlnode_t * list)
{
    free((void*) list->x); // Free sentinels.
    free(list);
}





/* ----------------------Hypervolume Indicator Algorithms ---------------------------------------*/



static inline void
restart_list_y(dlnode_t * list)
{
    assert(list+1 == list->next[0]);
    list->next[0]->cnext[1] = list; //link sentinels sentinels ((-inf ref[1] -inf) and (ref[0] -inf -inf))
    list->cnext[0] = list->next[0];

}

static inline double
compute_area_simple(const double * px, int i, const dlnode_t * q)
{
    int j = 1 - i;
    const dlnode_t * u = q->cnext[i];
    double area = (q->x[j] - px[j]) * (u->x[i] - px[i]);
    while (px[j] < u->x[j]) {
        q = u;
        u = u->cnext[i];
        area += (q->x[j] - px[j]) * (u->x[i] - q->x[i]);
    }
    return area;
}

// This does what setupZandClosest does while reconstructing L at z = new->x[2].
static void
restart_base_setup_z_and_closest(dlnode_t * list, dlnode_t * new)
{
    dlnode_t * closest1 = list;
    dlnode_t * closest0 = list->next[0];
    assert(closest0 == list+1);
    const double * closest1x = closest1->x;
    const double * closest0x = closest0->x;
    dlnode_t * p = list->next[0]->next[0];
    assert(p == (list+1)->next[0]);
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
    double area = compute_area_simple(newx, 1, new->cnext[0]);
    dlnode_t * p = new->next[0];
    const double * px = p->x;
    double lastz = newx[2];
    double volume = area * (px[2] - lastz);
    while (px[0] > newx[0] || px[1] > newx[1]) {
        p->cnext[0] = p->closest[0];
        p->cnext[1] = p->closest[1];

        if (px[0] >= newx[0] && px[1] >= newx[1]) {
            area -= compute_area_simple(px, 1, p->cnext[0]);
            p->cnext[1]->cnext[0] = p;
            p->cnext[0]->cnext[1] = p;

        } else if (px[0] >= newx[0]) {
            if (px[0] <= new->cnext[0]->x[0]) {
                const double tmpx[] = { px[0], newx[1] };
                // if px[0] == new->cnext[0]->x[0] then area starts at 0.
                area -= compute_area_simple(tmpx, 1, new->cnext[0]);
                p->cnext[0] = new->cnext[0];
                p->cnext[1]->cnext[0] = p;
                new->cnext[0] = p;
            }
        } else if (px[1] <= new->cnext[1]->x[1]) {
            const double tmpx[] = { newx[0], px[1] };
            area -= compute_area_simple(tmpx, 0, new->cnext[1]);
            p->cnext[1] = new->cnext[1];
            p->cnext[0]->cnext[1] = p;
            new->cnext[1] = p;
        }
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
    double volume = 0, hv = 0;
    dlnode_t * last = list->prev[1];
    assert(last == list->prev[0]);
    assert(last == list+2);
    dlnode_t * new = list->next[1]->next[1];
    assert(new == (list+1)->next[1]);
    while (new != last) {
        volume += one_contribution_3d(list, new);
        add_to_z(new);
        update_links(list, new);
        double height = new->next[1]->x[3] - new->x[3];
        hv += volume * height;
        new = new->next[1];
    }
    return hv;
}




double hv4d(const double * restrict data, size_t n, const double * restrict ref)
{
    dlnode_t * list = setup_cdllist(data, n, ref, 4);
    double hv = hv4dplusU(list);
    free_cdllist(list);
    return hv;
}
