/*************************************************************************

 hypervolume computation

 ---------------------------------------------------------------------

                           Copyright (C) 2010, 2025
                    Carlos M. Fonseca <cmfonsec@dei.uc.pt>
          Manuel Lopez-Ibanez <manuel.lopez-ibanez@manchester.ac.uk>
                       Luis Paquete <paquete@dei.uc.pt>

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.

 ----------------------------------------------------------------------

 Relevant literature:

 [1]  C. M. Fonseca, L. Paquete, and M. Lopez-Ibanez. An
      improved dimension-sweep algorithm for the hypervolume
      indicator. In IEEE Congress on Evolutionary Computation,
      pages 1157-1163, Vancouver, Canada, July 2006.

 [2]  Nicola Beume, Carlos M. Fonseca, Manuel López-Ibáñez, Luís
      Paquete, and J. Vahrenhold. On the complexity of computing the
      hypervolume indicator. IEEE Transactions on Evolutionary
      Computation, 13(5):1075-1082, 2009.

*************************************************************************/

#include <stdlib.h>
#include <limits.h>
#include <float.h>
#include "common.h"
#include "hv.h"
#define HV_RECURSIVE
#include "hv4d_priv.h"

#define STOP_DIMENSION 3 // default: stop on dimension 4.

static int compare_node(const void * restrict p1, const void * restrict p2)
{
    const double * restrict x1 = (*(const dlnode_t **)p1)->x;
    const double * restrict x2 = (*(const dlnode_t **)p2)->x;
    return cmp_double_asc(*x1, *x2);
}

/** Setup circular double-linked list in each dimension.

    There are in fact two separate lists that are keep in sync:

     - A multi-dimensional list for dimensions 5 and above tracked by ->r_next[] and ->r_prev[], where head is the sentinel with head->x == NULL.

     - A list for dimensions 3 and 4 tracked by ->next[0 or 1] ->prev[0 or 1]. This list has 3 sentinels as required by hv4dplusU(). The first sentinel is saved in head->next[0].

 */
static dlnode_t *
fpli_setup_cdllist(const double * restrict data, dimension_t d,
                   size_t * restrict size, const double * restrict ref)
{
    ASSUME(d > STOP_DIMENSION + 1);
    dimension_t d_stop = d - STOP_DIMENSION;
    size_t n = *size;

    dlnode_t * head = malloc((n+1) * sizeof(*head));
    // Allocate single blocks of memory as much as possible.
    // We need space in r_next and r_prev for dimension 5 and above (d_stop - 1).
    head->r_next = malloc(2 * (d_stop - 1) * (n+1) * sizeof(head));
    head->r_prev = head->r_next + (d_stop - 1) * (n+1);
    // We only need space in area and vol for dimension 4 and above.
    head->area = malloc(2 * d_stop * (n+1) * sizeof(*data));
    head->vol = head->area + d_stop * (n+1);
    head->x = NULL; // head contains no data
    head->ignore = 0;  // should never get used

    // Reserve space for the sentinels.
    dlnode_t * list4d = new_cdllist(0, ref);
    // Link head and list4d; head is not used by HV4D, so next[0] and prev[0]
    // should remain untouched.
    head->next[0] = list4d;
    head->prev[0] = list4d; // Save it twice so we can use assert() later.

    size_t i = 1;
    for (size_t j = 0; j < n; j++) {
        /* Filters those points that do not strictly dominate the reference
           point.  This is needed to assure that the points left are only those
           that are needed to calculate the hypervolume. */
        if (likely(strongly_dominates(data + j * d, ref, d))) {
            head[i].x = data + (j+1) * d; // this will be fixed a few lines below...
            head[i].ignore = 0;
            head[i].r_next = head->r_next + i * (d_stop - 1);
            head[i].r_prev = head->r_prev + i * (d_stop - 1);
            head[i].area = head->area + i * d_stop;
            head[i].vol = head->vol + i * d_stop;
            i++;
        }
    }
    n = i - 1;
    if (unlikely(n == 0))
        goto finish;

    dlnode_t **scratch = malloc(n * sizeof(*scratch));
    for (i = 0; i < n; i++)
        scratch[i] = head + i + 1;

    for (int j = d_stop - 2; j >= -1; j--) {
        /* FIXME: replace qsort() by something better:
           https://github.com/numpy/x86-simd-sort
           https://github.com/google/highway/tree/52a2d98d07852c5d69284e175666e5f8cc7d8285/hwy/contrib/sort
         */
        // We shift x because qsort() cannot take the dimension to sort as an argument.
        for (i = 0; i < n; i++)
            scratch[i]->x--;
        // Sort each dimension independently.
        qsort(scratch, n, sizeof(*scratch), compare_node);
        if (j == -1) {
            (list4d+1)->next[1] = scratch[0];
            scratch[0]->prev[1] = list4d+1;
            for (i = 1; i < n; i++) {
                scratch[i-1]->next[1] = scratch[i];
                scratch[i]->prev[1] = scratch[i-1];
            }
            scratch[n-1]->next[1] = list4d+2;
            (list4d+2)->prev[1] = scratch[n-1];
        } else {
            head->r_next[j] = scratch[0];
            scratch[0]->r_prev[j] = head;
            for (i = 1; i < n; i++) {
                scratch[i-1]->r_next[j] = scratch[i];
                scratch[i]->r_prev[j] = scratch[i-1];
            }
            scratch[n-1]->r_next[j] = head;
            head->r_prev[j] = scratch[n-1];
        }
    }
    // Reset x to point to the first objective.
    for (i = 0; i < n; i++){
        scratch[i]->x -= STOP_DIMENSION;
        scratch[i]->is_bounded = false;
    }

    free(scratch);

    // Make sure it is not used.
    ASAN_POISON_MEMORY_REGION(head->area, sizeof(*data) * d_stop);
    ASAN_POISON_MEMORY_REGION(head->vol, sizeof(*data) * d_stop);

finish:
    *size = n;
    return head;
}

static void fpli_free_cdllist(dlnode_t * head)
{
    assert(head->next[0] == head->prev[0]);
    free_cdllist(head->next[0]); // free 4D sentinels
    free(head->r_next);
    free(head->area);
    free(head);
}

static inline void
update_bound(double * restrict bound, const double * restrict x, dimension_t dim)
{
    ASSUME(dim > STOP_DIMENSION);
    const double * restrict y = x + STOP_DIMENSION;

    PRAGMA_ASSUME_NO_VECTOR_DEPENDENCY // We need this to avoid a wasteful alias check.
    for (dimension_t d = 0; d < dim - STOP_DIMENSION; d++) {
        if (bound[d] > y[d])
            bound[d] = y[d];
    }
}

static void
delete_dom(dlnode_t * restrict nodep, dimension_t dim)
{
    ASSUME(dim > STOP_DIMENSION);
    assert(nodep->x);
    // d=0 is dimension 5.
    for (dimension_t d = 0; d < dim - 1 - STOP_DIMENSION; d++) {
        nodep->r_prev[d]->r_next[d] = nodep->r_next[d];
        nodep->r_next[d]->r_prev[d] = nodep->r_prev[d];
    }
    // Dimension 4.
    nodep->prev[1]->next[1] = nodep->next[1];
    nodep->next[1]->prev[1] = nodep->prev[1];
}

static void
delete(dlnode_t * restrict nodep, dimension_t dim, double * restrict bound)
{
    delete_dom(nodep, dim);
    update_bound(bound, nodep->x, dim);
}


static void
reinsert_nobound(dlnode_t * restrict nodep, dimension_t dim)
{
    ASSUME(dim > STOP_DIMENSION);
    assert(nodep->x);
    // d=0 is dimension 5.
    for (dimension_t d = 0; d < dim - 1 - STOP_DIMENSION; d++) {
        nodep->r_prev[d]->r_next[d] = nodep;
        nodep->r_next[d]->r_prev[d] = nodep;
    }
    // Dimension 4.
    nodep->prev[1]->next[1] = nodep;
    nodep->next[1]->prev[1] = nodep;
}

static void
reinsert(dlnode_t * restrict nodep, dimension_t dim, double * restrict bound)
{
    reinsert_nobound(nodep, dim);
    update_bound(bound, nodep->x, dim);
}

static double
fpli_hv4d(dlnode_t * restrict list, size_t c _attr_maybe_unused)
{
    ASSUME(c > 1);
    assert(list->next[0] == list->prev[0]);
    dlnode_t * restrict list4d = list->next[0];
    // hv4dplusU() will change the sentinels for 3D, so we need to reset them.
    reset_sentinels_3d(list4d);
    double hv = hv4dplusU(list4d);
    return hv;
}

static inline void
restore_points(dlnode_t * list, dlnode_t * last)
{
    dlnode_t * newp = (list+1)->next[1];
    while (newp != last) {
        // MANUEL: We only modify the points that are not ignored.
        if (newp->ignore < 3)
            newp->is_bounded = false;
        newp = newp->next[1];
    }
}

static inline void
update_bound_3d(dlnode_t * newp, const double * bound)
{
    for (int i = 0; i < 3; i++) {
        if (newp->x[i] < bound[i]) {
            newp->is_bounded = true;
            for (int k = 0; k < i; k++)
                newp->bound[k] = newp->x[k];
            for (; i < 3; i++)
                newp->bound[i] = MAX(newp->x[i], bound[i]);
            DEBUG1(newp->bound[3] = newp->x[3]);
            return;
        }
    }
}

/* Compute the hv contribution of "the_point" in d=4 by iteratively computing the one contribution problem in d=3. */
static inline double
onec4dplusU(dlnode_t * list, dlnode_t * the_point)
{
    // MANUEL: It would be better to move this check to hv.c to avoid calling reset_sentinels_3d. You can add an assert here.
    if (the_point->ignore >= 3) {
        return 0;
    }

    assert(list+2 == list->prev[0]);
    assert(list+2 == list->prev[1]);
    assert(list+1 == list->next[1]);

    dlnode_t * newp = (list+1)->next[1];
    const dlnode_t * last = list+2;

    the_point->closest[0] = list+1;
    the_point->closest[1] = list;

    const double * the_point_x = the_point->x;
    // MANUEL: I added this because I think we cannot (and should not) call this function with an empty list.
    assert(newp != last);
    // PART 1: Setup 3D base (TODO: improve)
    while (newp != last && newp->x[3] <= the_point_x[3]) {
        // MANUEL: When can newp be equal to the_point?
        if (newp != the_point && newp->ignore < 3){

            if (weakly_dominates(newp->x, the_point_x, 3)) {
                the_point->ignore = 3;
                restore_points(list, newp);
                return 0;
            }

            // MANUEL: This modifies ->x[], why?
            update_bound_3d(newp, the_point_x);

            if (restart_base_setup_z_and_closest(list, newp)) {
                add_to_z(newp);
                update_links(list, newp);
            }
        }
        newp = newp->next[1];
    }

    restart_base_setup_z_and_closest(list, the_point);
    double volume = one_contribution_3d(the_point);
    assert(volume > 0);
    double height = newp->x[3] - the_point_x[3];
    // It cannot be zero because we exited the loop above.
    assert(height > 0);
    double hv = volume * height;

    // PART 2: Update the 3D contribution
    while (newp != last &&
            (newp->x[0] > the_point_x[0] || newp->x[1] > the_point_x[1] || newp->x[2] > the_point_x[2])) {

        // MANUEL: I think newp cannot be equal to the_point here. If it was
        // equal, we would have exited the loop.
        assert(newp != the_point); //
        if (newp != the_point && newp->ignore < 3) {
            // MANUEL: This modifies ->x[], why?
            update_bound_3d(newp, the_point_x);

            if (restart_base_setup_z_and_closest(list, newp)) {

                // newp was not dominated by something else.
                double newp_v = one_contribution_3d(newp);
                assert(newp_v > 0);
                volume -= newp_v;

                add_to_z(newp);
                update_links(list, newp);
            }
        }
        // FIXME: It newp was dominated, can we accumulate the height and update
        // hv later?
        height = newp->next[1]->x[3] - newp->x[3];
        assert(height >= 0);
        hv += volume * height;

        newp = newp->next[1];
    }

    restore_points(list, newp);
    return hv;
}

static double
fpli_onec4d(dlnode_t * restrict list, size_t c _attr_maybe_unused, dlnode_t *the_point)
{
    ASSUME(c > 1);
    assert(list->next[0] == list->prev[0]);
    dlnode_t * restrict list4d = list->next[0];
    // hv4dplusU() will change the sentinels for 3D, so we need to reset them.
    reset_sentinels_3d(list4d);
    double contrib = onec4dplusU(list4d, the_point);
    return contrib;
}

_attr_optimize_finite_and_associative_math // Required for auto-vectorization: https://gcc.gnu.org/PR122687
static double
one_point_hv(const double * restrict x, const double * restrict ref, dimension_t d)
{
    double hv = ref[0] - x[0];
    for (dimension_t i = 1; i < d; i++)
        hv *= (ref[i] - x[i]);
    return hv;
}

static inline void
update_area(double * restrict area, const double * restrict x,
            const double * restrict ref, dimension_t dim)
{
    ASSUME(dim > STOP_DIMENSION);
    area[0] = one_point_hv(x, ref, STOP_DIMENSION);
    const double * restrict ref_d = ref + STOP_DIMENSION;
    const double * restrict x_d = x + STOP_DIMENSION;
    // Split into two loops to help the vectorizer.
    for (dimension_t d = 0; d < dim - STOP_DIMENSION; d++)
        area[d + 1] = (ref_d[d] - x_d[d]);
    for (dimension_t d = 0; d < dim - STOP_DIMENSION; d++)
        area[d + 1] *= area[d];
}

//#define HV_COUNTERS
_attr_maybe_unused static size_t debug_counter[6] = { 0 };

static double
hv_recursive(dlnode_t * restrict list, dimension_t dim, size_t c,
             const double * restrict ref, double * restrict bound, dlnode_t * the_point)
{
    ASSUME(c > 1);
    ASSUME(dim >= STOP_DIMENSION);
    if (dim == STOP_DIMENSION) {
        /*---------------------------------------
          base case of dimension 4
          --------------------------------------*/
        return fpli_onec4d(list, c, the_point);
    }
    ASSUME(dim > STOP_DIMENSION);
    /* ------------------------------------------------------
       General case for dimensions higher than 4D
       ------------------------------------------------------ */
    const dimension_t d_stop = dim - STOP_DIMENSION;
    assert(d_stop > 0);
    // d_stop - 1 is dimension 5 in r_prev and r_next.
    dlnode_t * p1 = list->r_prev[d_stop - 1];
    for (dlnode_t * pp = p1; pp->x; pp = pp->r_prev[d_stop - 1]) {
        if (pp->ignore < dim)
            pp->ignore = 0;
    }
    dlnode_t * p1_prev = p1->r_prev[d_stop - 1];
    dlnode_t * p0 = list;
    /* Delete all points x[dim] > bound[d_stop].  In case of repeated
       coordinates, delete also all points x[dim] == bound[d_stop] except
       one.  */
    while (p1->x[dim] > bound[d_stop]
           || p1_prev->x[dim] >= bound[d_stop]) {
        // FIXME: Instead of deleting each point, unlink the start and end
        // nodes after the loop.
        delete(p1, dim, bound);
        p0 = p1;
        p1 = p1->r_prev[d_stop - 1];
        p1_prev = p1->r_prev[d_stop - 1];
        c--;
        if (c == 1)
            break;
    }

    double hyperv;
    if (c == 1) {
        update_area(p1->area, p1->x, ref, dim);
        p1->vol[d_stop] = 0;
        assert(p0->x != NULL);
        /* if (p0->x == NULL)
            return p1->area[d_stop] * (ref[dim] - p1->x[dim]);
        */
        hyperv = p1->area[d_stop] * (p0->x[dim] - p1->x[dim]);
        // FIXME: This is never used?
        // bound[d_stop] = p0->x[dim];
        reinsert(p0, dim, bound);
        c++;
        p1 = p0;
        p1_prev = p0->r_prev[d_stop - 1];
        p0 = p0->r_next[d_stop - 1];
    } else {
        ASSUME(c > 1);
        DEBUG1(debug_counter[0]++);
        hyperv = p1_prev->vol[d_stop] + p1_prev->area[d_stop]
            * (p1->x[dim] - p1_prev->x[dim]);
        assert(p0 != p1_prev);
        assert(p0 == p1->r_next[d_stop - 1]);
        // p0->x may be NULL here and thus we may return below.
    }

    assert(c > 1);
    while (true) {
        // FIXME: This is not true in the first iteration if c > 1 previously.
        // assert(p0 == p1->r_prev[d_stop - 1]);
        assert(p1_prev == p1->r_prev[d_stop - 1]);
        p1->vol[d_stop] = hyperv;
        double hypera;
        if (p1->ignore >= dim) {
            DEBUG1(debug_counter[1]++);
            hypera = p1_prev->area[d_stop];
        } else {
            hypera = hv_recursive(list, dim - 1, c, ref, bound, p1);
            if(dim - 1 == STOP_DIMENSION){ //hypera only has the contribution of p1
                hypera += p1_prev->area[d_stop];
            }
            /* At this point, p1 is the point with the highest value in
               dimension dim in the list: If it is dominated in dimension
               dim-1, then it is also dominated in dimension dim. */
            if (p1->ignore == dim - 1) {
                DEBUG1(debug_counter[2]++);
                p1->ignore = dim;
            } else if (hypera <= p1_prev->area[d_stop]) {
                DEBUG1(debug_counter[3]++);
                p1->ignore = dim;
            }
        }
        p1->area[d_stop] = hypera;
        if (p0->x == NULL) {
            bound[d_stop] = p1->x[dim];
            hyperv += hypera * (ref[dim] - p1->x[dim]);
            return hyperv;
        }
        hyperv += hypera * (p0->x[dim] - p1->x[dim]);
        // FIXME: This is never used?
        // bound[d_stop] = p0->x[dim];
        reinsert(p0, dim, bound);
        c++;
        p1 = p0;
        p1_prev = p0->r_prev[d_stop - 1];
        p0 = p0->r_next[d_stop - 1];
    }
}

static double
fpli_hv_ge5d(dlnode_t * restrict list, dimension_t dim, size_t c,
             const double * restrict ref)
{
    ASSUME(c > 1);
    ASSUME(dim > STOP_DIMENSION);
    const dimension_t d_stop = dim - STOP_DIMENSION;
    ASSUME(0 < d_stop && d_stop < 255); // Silence -Walloc-size-larger-than= warning
    double * bound = malloc(d_stop * sizeof(*bound));
    for (dimension_t i = 0; i < d_stop; i++)
        bound[i] = -DBL_MAX;

    /* ------------------------------------------------------
       General case for dimensions higher than 4D
       ------------------------------------------------------ */
    dlnode_t * p1 = list->r_prev[d_stop - 1];
    // FIXME: This should be the initial state of the list when building it.
    // Delete all points in dimensions < dim.
    do {
        delete_dom(p1, dim);
        p1 = p1->r_prev[d_stop - 1];
        c--;
    } while (c > 1);

    update_area(p1->area, p1->x, ref, dim);
    p1->vol[d_stop] = 0;
    dlnode_t * p0 = p1->r_next[d_stop - 1];
    assert(p0->x != NULL);
    double hyperv = p1->area[d_stop] * (p0->x[dim] - p1->x[dim]);
    // FIXME: This is never used?
    // bound[d_stop] = p0->x[dim];
    reinsert_nobound(p0, dim);
    p1 = p0;
    dlnode_t * p1_prev = p0->r_prev[d_stop - 1];
    p0 = p0->r_next[d_stop - 1];
    c++;

    assert(c > 1);
    while (true) {
        // FIXME: This is not true in the first iteration if c > 1 previously.
        //assert(p0 == p1->r_prev[d_stop]);
        assert(p1_prev == p1->r_prev[d_stop - 1]);
        p1->vol[d_stop] = hyperv;
        assert(p1->ignore == 0);
        double hypera = hv_recursive(list, dim - 1, c, ref, bound, p1);
        if(dim - 1 == STOP_DIMENSION){ //hypera only has the contribution of p1
            hypera += p1_prev->area[d_stop];
        }
        /* At this point, p1 is the point with the highest value in
           dimension dim in the list: If it is dominated in dimension
           dim-1, then it is also dominated in dimension dim. */
        if (p1->ignore == dim - 1) {
            DEBUG1(debug_counter[4]++);
            p1->ignore = dim;
        } else if (hypera <= p1_prev->area[d_stop]) {
            DEBUG1(debug_counter[5]++);
            p1->ignore = dim;
        }
        p1->area[d_stop] = hypera;
        if (p0->x == NULL) {
            free(bound);
#if defined(HV_COUNTERS) && DEBUG >= 1
            for (size_t i = 0; i < sizeof(debug_counter)/sizeof(size_t); i++)
                fprintf(stderr, "debug_counter[%zu] = %zu\n", i, debug_counter[i]);
#endif
            hyperv += hypera * (ref[dim] - p1->x[dim]);
            return hyperv;
        }
        hyperv += hypera * (p0->x[dim] - p1->x[dim]);
        // FIXME: This is never used?
        // bound[d_stop] = p0->x[dim];
        // FIXME: Does updating the bound here matters?
        reinsert(p0, dim, bound);
        p1 = p0;
        p1_prev = p0->r_prev[d_stop - 1];
        p0 = p0->r_next[d_stop - 1];
        c++;
    }
}


static double
hv2d(const double * restrict data, size_t n, const double * restrict ref)
{
    const double **p = generate_sorted_doublep_2d(data, &n, ref[0]);
    if (unlikely(n == 0)) return 0;
    if (unlikely(!p)) return -1;

    double hyperv = 0;
    double prev_j = ref[1];
    size_t j = 0;
    do {
        /* Filter everything that may be above the ref point. */
        if (p[j][1] < prev_j) {
            // We found one point that dominates ref.
            hyperv += (ref[0] - p[j][0]) * (prev_j - p[j][1]);
            prev_j = p[j][1];
        }
        j++;
    } while (j < n);

    free(p);
    return hyperv;
}

double hv3d(const double * restrict data, size_t n, const double * restrict ref);
double hv4d(const double * restrict data, size_t n, const double * restrict ref);

/*
   Returns 0 if no point strictly dominates ref.
   Returns -1 if out of memory.
*/
double fpli_hv(const double * restrict data, int d, int npoints,
               const double * restrict ref)
{
    size_t n = (size_t) npoints;
    if (unlikely(n == 0)) return 0.0;
    ASSUME(d > 1 && d < 256);
    if (d == 4) return hv4d(data, n, ref);
    if (d == 3) return hv3d(data, n, ref);
    if (d == 2) return hv2d(data, n, ref);
    dimension_t dim = (dimension_t) d;
    dlnode_t * list = fpli_setup_cdllist(data, dim, &n, ref);
    double hyperv;
    if (likely(n > 1)) {
        hyperv = fpli_hv_ge5d(list, dim - 1, n, ref);
    } else if (unlikely(n == 1)) {
        hyperv = one_point_hv(list->r_next[0]->x, ref, dim);
    } else {
        assert(n == 0);
        hyperv = 0.0; // Returning here would leak memory.
    }
    // Clean up.
    fpli_free_cdllist(list);
    return hyperv;
}
