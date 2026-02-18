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
#include "hvc4d_priv.h"

#define STOP_DIMENSION 3 // stop on dimension 4.
#define MAX_ROWS_HV_INEX 15

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

    // Reserve space for main CDLL used by hv_recursive() and for the auxiliary
    // list used by onec4dplusU().  The main CDLL will store all points + 1
    // sentinel.  The auxiliary list will store at most n - 1 points + 1
    // sentinel.
    dlnode_t * head = malloc((n + 1 + n) * sizeof(*head));
    size_t i = 1;
    for (size_t j = 0; j < n; j++) {
        /* Filters those points that do not strictly dominate the reference
           point.  This is needed to assure that the points left are only those
           that are needed to calculate the hypervolume. */
        const double * restrict px = data + j * d;
        if (likely(strongly_dominates(px, ref, d))) {
            head[i].x = px; // this will be fixed a few lines below...
            i++;
        }
    }
    n = i - 1;
    if (unlikely(n <= MAX_ROWS_HV_INEX))
        goto finish;


    // Allocate single blocks of memory as much as possible.
    // We need space in r_next and r_prev for dimension 5 and above (d_stop - 1).
    head->r_next = malloc(2 * (d_stop - 1) * (n+1) * sizeof(head));
    head->r_prev = head->r_next + (d_stop - 1) * (n+1);
    // We only need space in area and vol for dimension 4 and above (d-stop).
    // Also reserve space for n-1 auxiliary 3D points used by onec4dplusU().
    head->area = malloc((2 * d_stop * n + 3 * (n-1)) * sizeof(*data));
    head->vol = head->area + d_stop * n;
    head->x = NULL; // head contains no data
    head->ignore = 0;  // should never get used

    // Reserve space for the sentinels.
    dlnode_t * list4d = new_cdllist(0, ref);
    // Link head and list4d; head is not used by HV4D, so next[0] and prev[0]
    // should remain untouched.
    head->next[0] = list4d;

    // Setup the auxiliary list used in onec4dplusU().
    dlnode_t * list_aux = head + n + 1;
    // Setup the auxiliary 3D points used in onec4dplusU().
    list_aux->vol = head->vol + d_stop * n;
    head->prev[0] = list_aux;
    list_aux->next[0] = list4d;

    for (i = 1; i <= n; i++) {
        // Shift x because qsort() cannot take the dimension to sort as an argument.
        head[i].x += d - 1;
        head[i].ignore = 0;
        head[i].r_next = head->r_next + i * (d_stop - 1);
        head[i].r_prev = head->r_prev + i * (d_stop - 1);
        head[i].area = head->area + (i - 1) * d_stop;
        head[i].vol = head->vol + (i - 1) * d_stop;
    }
    // Make sure they are not used.
    head->area = NULL;
    head->vol = NULL;

    dlnode_t ** scratch = malloc(n * sizeof(*scratch));
    for (i = 0; i < n; i++)
        scratch[i] = head + 1 + i;

    for (int j = d_stop - 2; j >= 0; j--) {
        /* FIXME: replace qsort() by something better:
           https://github.com/numpy/x86-simd-sort
           https://github.com/google/highway/tree/52a2d98d07852c5d69284e175666e5f8cc7d8285/hwy/contrib/sort
         */
        // Sort each dimension independently.
        qsort(scratch, n, sizeof(*scratch), compare_node);
        head->r_next[j] = scratch[0];
        scratch[0]->r_prev[j] = head;
        for (i = 1; i < n; i++) {
            scratch[i-1]->r_next[j] = scratch[i];
            scratch[i]->r_prev[j] = scratch[i-1];
        }
        scratch[n-1]->r_next[j] = head;
        head->r_prev[j] = scratch[n-1];
        // Consider next objective (in reverse order).
        for (i = 1; i <= n; i++)
            head[i].x--;
    }

    for (int j = 1; j >= 0; j--) {
        // Sort each dimension independently.
        qsort(scratch, n, sizeof(*scratch), compare_node);
        (list4d+1)->next[j] = scratch[0];
        scratch[0]->prev[j] = list4d+1;
        for (i = 1; i < n; i++) {
            scratch[i-1]->next[j] = scratch[i];
            scratch[i]->prev[j] = scratch[i-1];
        }
        scratch[n-1]->next[j] = list4d+2;
        (list4d+2)->prev[j] = scratch[n-1];
        if (j > 0) {
            // Consider next objective (in reverse order).
            for (i = 1; i <= n; i++)
                head[i].x--;
        } else {
            // Reset x to point to the first objective.
            for (i = 1; i <= n; i++)
                head[i].x -= STOP_DIMENSION - 1;
        }
    }
    free(scratch);

finish:
    *size = n;
    return head;
}

static void fpli_free_cdllist(dlnode_t * head)
{
    assert(head->next[0] == head->prev[0]->next[0]);
    free_cdllist(head->next[0]); // free 4D sentinels
    free(head->r_next);
    free(head[1].area); // Free ->area, ->vol and list_aux->vol (x_aux used by 4D basecase).
    free(head); // Free main CDLL and list_aux (4D basecase).
}

static inline void
update_bound(double * restrict bound, const double * restrict x, dimension_t dim)
{
    ASSUME(dim > STOP_DIMENSION);
    const double * restrict y = x + STOP_DIMENSION;

    PRAGMA_ASSUME_NO_VECTOR_DEPENDENCY // We need this to avoid a wasteful alias check.
    for (dimension_t d = 0; d < dim - STOP_DIMENSION; d++) {
        bound[d] = MIN(bound[d], y[d]);
    }
}

static void
delete_4d(dlnode_t * restrict nodep)
{
    nodep->prev[1]->next[1] = nodep->next[1];
    nodep->next[1]->prev[1] = nodep->prev[1];
}

static void
delete_3d(dlnode_t * restrict nodep)
{
    nodep->prev[0]->next[0] = nodep->next[0];
    nodep->next[0]->prev[0] = nodep->prev[0];
}

static void
reinsert_4d(dlnode_t * restrict nodep)
{
    nodep->prev[1]->next[1] = nodep;
    nodep->next[1]->prev[1] = nodep;
}

static void
reinsert_3d(dlnode_t * restrict nodep)
{
    nodep->prev[0]->next[0] = nodep;
    nodep->next[0]->prev[0] = nodep;
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
    delete_4d(nodep);
    delete_3d(nodep);
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
    reinsert_4d(nodep);
    reinsert_3d(nodep);
}

static void
reinsert(dlnode_t * restrict nodep, dimension_t dim, double * restrict bound)
{
    reinsert_nobound(nodep, dim);
    update_bound(bound, nodep->x, dim);
}

static double
fpli_onec4d(dlnode_t * restrict list, size_t c _attr_maybe_unused, dlnode_t * restrict the_point)
{
    ASSUME(c > 1);
    assert(list->next[0] == list->prev[0]->next[0]);
    dlnode_t * restrict list4d = list->next[0];
    dlnode_t * restrict list_aux = list->prev[0];
    double contrib = onec4dplusU(list4d, list_aux, the_point);
    return contrib;
}

_attr_optimize_finite_and_associative_math // Required for auto-vectorization: https://gcc.gnu.org/PR122687
static double
one_point_hv(const double * restrict x, const double * restrict ref, dimension_t d)
{
    ASSUME(2 <= d && d <= MOOCORE_DIMENSION_MAX);
    double hv = 1.0;
    for (dimension_t i = 0; i < d; i++)
        hv *= (ref[i] - x[i]);
    return hv;
}

_attr_optimize_finite_and_associative_math
static double
hv_two_points(const double * restrict x1, const double * restrict x2,
              const double * restrict ref, dimension_t d)
{
    ASSUME(2 <= d && d <= MOOCORE_DIMENSION_MAX);
    double hv = one_point_hv(x1, ref, d) + one_point_hv(x2, ref, d);
    double bound[MOOCORE_DIMENSION_MAX+1];
    upper_bound(bound, x1, x2, d);
    hv -= one_point_hv(bound, ref, d);
    return hv;
}

/**
   Computation of the hypervolume via inclusion–exclusion.
*/
_attr_optimize_finite_and_associative_math
static double
hv_inex_list(const dlnode_t * restrict list, int n, dimension_t dim,
             const double * restrict ref)
{
    ASSUME(3 <= n && n <= MAX_ROWS_HV_INEX);
    ASSUME(2 <= dim && dim <= MOOCORE_DIMENSION_MAX);
    // Accumulate positive and negative values separately to improve accuracy.
    // If more accuracy is needed, we could use Neumaier compensated
    // accumulators.
    double hv[] = {0.0, 0.0}; // 0 is negative, 1 is positive.

    // Process individual points.
    for (int i = 0; i < n; ++i) {
        const double * restrict px = list[i].x;
        hv[1] += one_point_hv(px, ref, dim);
    }

    // Depth-first-search state.
    int start_stack[MAX_ROWS_HV_INEX - 1];
    double * buffer = malloc((n-1) * dim * sizeof(*buffer));
    if (!buffer)
        return -1;
    // Pre-compute to speed-up access.
    double * subset_max[MAX_ROWS_HV_INEX - 1];
    for (int i = 0; i < n - 1; i++) {
        subset_max[i] = buffer + i * dim;
    }

    // Build all possible subsets starting from each possible pair.
    for (int i = 0; i < n - 1; ++i) {
        const double * restrict pi = list[i].x;
        for (int j = i + 1; j < n; ++j) {
            const double * restrict pj = list[j].x;
            double * restrict child = subset_max[0];
            upper_bound(child, pi, pj, dim);
            hv[0] += one_point_hv(child, ref, dim);

            int top = 0;
            int idx = j + 1;
            while (true) {
                if (idx < n) {
                    start_stack[top] = idx + 1;
                    const double * restrict parent = subset_max[top];
                    ++top;
                    // At this point, subset size == top + 2.
                    child = subset_max[top];
                    upper_bound(child, list[idx].x, parent, dim);
                    // Inclusion–exclusion accumulation.
                    hv[top & 1] += one_point_hv(child, ref, dim);
                    idx++;
                } else if (top > 0) {
                    --top;
                    idx = start_stack[top];
                } else {
                    break;
                }
            }
        }
    }
    free(buffer);
    return hv[1] - hv[0];
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
             const double * restrict ref, double * restrict bound)
{
    ASSUME(c > 1);
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
            ASSUME(dim - 1 >= STOP_DIMENSION);
            if (dim - 1 == STOP_DIMENSION) {
                // base case of dimension 4.
                hypera = fpli_onec4d(list, c, p1);
                // hypera only has the contribution of p1.
                hypera += p1_prev->area[d_stop];
            } else {
                hypera = hv_recursive(list, dim - 1, c, ref, bound);
            }
            /* At this point, p1 is the point with the highest value in
               dimension dim in the list: If it is dominated in dimension
               dim-1, then it is also dominated in dimension dim. */
            if (p1->ignore == dim - 1) {
                DEBUG1(debug_counter[2]++);
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
    ASSUME(0 < d_stop && d_stop < MOOCORE_DIMENSION_MAX); // Silence -Walloc-size-larger-than= warning
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

    while (true) {
        dlnode_t * p1_prev = p0->r_prev[d_stop - 1];
        p1 = p0;
        p0 = p0->r_next[d_stop - 1];
        p1->vol[d_stop] = hyperv;
        assert(p1->ignore == 0);
        c++;
        double hypera;
        ASSUME(dim - 1 >= STOP_DIMENSION);
        if (dim - 1 == STOP_DIMENSION) {
            // base case of dimension 4.
            hypera = fpli_onec4d(list, c, p1);
            // hypera only has the contribution of p1.
            hypera += p1_prev->area[d_stop];
        } else {
            hypera = hv_recursive(list, dim - 1, c, ref, bound);
        }
        /* At this point, p1 is the point with the highest value in
           dimension dim in the list: If it is dominated in dimension
           dim-1, then it is also dominated in dimension dim. */
        if (p1->ignore == dim - 1) {
            DEBUG1(debug_counter[4]++);
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
    }
}


static double
hv2d(const double * restrict data, size_t n, const double * restrict ref)
{
    const double **p = generate_sorted_doublep_2d_filter_by_ref(data, &n, ref[0]);
    if (unlikely(n == 0)) return 0;
    if (unlikely(!p)) return -1;

    double hyperv = 0;
    double prev_j = ref[1];
    size_t j = 0;
    do {
        // Filter everything that may be above the ref point.
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

static double
hv1d(const double * restrict data, size_t n, const double * restrict ref)
{
    double min_val = data[0];
    for (size_t k = 1; k < n; k++) {
        min_val = MIN(min_val, data[k]);
    }
    return MAX(0.0, ref[0] - min_val);
}

double hv3d(const double * restrict data, size_t n, const double * restrict ref);
double hv4d(const double * restrict data, size_t n, const double * restrict ref);

/**
   Returns 0 if no point strictly dominates ref.
   Returns -1 if out of memory.
*/
double fpli_hv(const double * restrict data, size_t n, dimension_t dim,
               const double * restrict ref)
{
    if (unlikely(n == 0)) return 0.0;
    ASSUME(dim > 0);
    if (dim == 4) return hv4d(data, n, ref);
    if (dim == 3) return hv3d(data, n, ref);
    if (dim == 2) return hv2d(data, n, ref);
    if (unlikely(dim == 1)) return hv1d(data, n, ref);

    dlnode_t * list = fpli_setup_cdllist(data, dim, &n, ref);
    double hyperv;
    if (likely(n > MAX_ROWS_HV_INEX)) {
        hyperv = fpli_hv_ge5d(list, dim - 1, n, ref);
        fpli_free_cdllist(list);
        return hyperv;
    }
    if (unlikely(n > 2)) {
        hyperv = hv_inex_list(list+1, (int) n, dim, ref);
    } else if (unlikely(n == 2)) {
        hyperv = hv_two_points(list[1].x, list[2].x, ref, dim);
    } else if (unlikely(n == 1)) {
        hyperv = one_point_hv(list[1].x, ref, dim);
    } else {
        assert(n == 0);
        hyperv = 0.0; // Returning here would leak memory.
    }
    // Clean up.
    free(list);
    return hyperv;
}
