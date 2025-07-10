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
#include <stdint.h>
#include "common.h"
#include "hv.h"
#define HV_DIMENSION 4
#include "hv_priv.h"

#define STOP_DIMENSION 3 /* default: stop on dimension 4 */

typedef struct fpli_dlnode {
    const double *x;              /* The data vector              */
    struct fpli_dlnode **next;         /* Next-node vector             */
    struct fpli_dlnode **prev;         /* Previous-node vector         */
    double *area;                 /* Area */
    double *vol;                  /* Volume */
    dimension_t ignore;           /* Restricts dim to be 255.  */
} fpli_dlnode_t;


static int compare_node(const void *p1, const void* p2)
{
    const double x1 = *((*(const fpli_dlnode_t **)p1)->x);
    const double x2 = *((*(const fpli_dlnode_t **)p2)->x);

    return (x1 < x2) ? -1 : (x1 > x2) ? 1 : 0;
}

/*
 * Setup circular double-linked list in each dimension
 */

static fpli_dlnode_t *
fpli_setup_cdllist(const double * restrict data, dimension_t d,
                   size_t * restrict size, const double * restrict ref)
{
    ASSUME(d > STOP_DIMENSION);
    dimension_t d_stop = d - STOP_DIMENSION;
    size_t n = *size;
    fpli_dlnode_t *head = malloc ((n+1) * sizeof(*head));
    head->next = malloc(2 * d_stop * (n+1) * sizeof(fpli_dlnode_t*));
    head->prev = head->next + d_stop * (n+1);
    head->area = malloc(2 * d_stop * (n+1) * sizeof(double));
    head->vol = head->area + d_stop * (n+1);
    head->x = NULL; /* head contains no data */
    head->ignore = 0;  /* should never get used */

    size_t i = 1;
    for (size_t j = 0; j < n; j++) {
        /* Filters those points that do not strictly dominate the reference
           point.  This is needed to assure that the points left are only those
           that are needed to calculate the hypervolume. */
        if (unlikely(strongly_dominates(data + j * d, ref, d))) {
            head[i].x = data + (j+1) * d; /* this will be fixed a few lines below... */
            head[i].ignore = 0;
            head[i].next = head->next + i * d_stop;
            head[i].prev = head->prev + i * d_stop;
            head[i].area = head->area + i * d_stop;
            head[i].vol = head->vol + i * d_stop;
            i++;
        }
    }
    n = i - 1;
    if (unlikely(n == 0))
        goto finish;

    fpli_dlnode_t **scratch = malloc(n * sizeof(fpli_dlnode_t*));
    for (i = 0; i < n; i++)
        scratch[i] = head + i + 1;

    for (int k = d-1; k >= 0; k--) {
        for (i = 0; i < n; i++)
            scratch[i]->x--;
        int j = k - STOP_DIMENSION;
        if (j < 0)
            continue;
        qsort(scratch, n, sizeof(fpli_dlnode_t*), compare_node);
        head->next[j] = scratch[0];
        scratch[0]->prev[j] = head;
        for (i = 1; i < n; i++) {
            scratch[i-1]->next[j] = scratch[i];
            scratch[i]->prev[j] = scratch[i-1];
        }
        scratch[n-1]->next[j] = head;
        head->prev[j] = scratch[n-1];
    }

    free(scratch);

    // FIXME: This should not be necessary.
    for (i = 0; i < d_stop; i++)
        head->area[i] = 0;

finish:
    *size = n;
    return head;
}

static void fpli_free_cdllist(fpli_dlnode_t * head)
{
    free(head->next);
    free(head->area);
    free(head);
}

static void
delete(fpli_dlnode_t * restrict nodep, dimension_t dim, double * restrict bound)
{
    ASSUME(dim > STOP_DIMENSION);
    for (dimension_t i = STOP_DIMENSION; i < dim; i++) {
        dimension_t d = i - STOP_DIMENSION;
        nodep->prev[d]->next[d] = nodep->next[d];
        nodep->next[d]->prev[d] = nodep->prev[d];
        if (bound[d] > nodep->x[i])
            bound[d] = nodep->x[i];
    }
}

static void
reinsert(fpli_dlnode_t * restrict nodep, dimension_t dim, double * restrict bound)
{
    ASSUME(dim > STOP_DIMENSION);
    for (dimension_t i = STOP_DIMENSION; i < dim; i++) {
        dimension_t d = i - STOP_DIMENSION;
        nodep->prev[d]->next[d] = nodep;
        nodep->next[d]->prev[d] = nodep;
        if (bound[d] > nodep->x[i])
            bound[d] = nodep->x[i];
    }
}

static void
fpli_hv4d_setup_cdllist(const fpli_dlnode_t * restrict pp,
                        dlnode_t * restrict list, size_t n _attr_maybe_unused)
{
    ASSUME(n > 1);
    reset_sentinels(list);

    const dimension_t d = HV_DIMENSION - 3; // index within the list.
    dlnode_t * q = list+1;
    dlnode_t * list3 = list+3;
    assert(list->next[d] == list + 1);
    assert(q->next[d] == list + 2);
    for (size_t i = 0; pp->x != NULL; pp = pp->next[0]) {
        dlnode_t * p = list3 + i;
        p->x = pp->x;
        // Initialize it when debugging so it will crash if uninitialized.
        DEBUG1(
            p->closest[0] = NULL;
            p->closest[1] = NULL;
            p->cnext[0] = NULL;
            p->cnext[1] = NULL;);
        // FIXME: Can we use pp->ignore to initialize p->ndomr?
        p->ndomr = 0;
        // Link the list in order.
        q->next[d] = p;
        p->prev[d] = q;
        q = p;
        i++;
    }
    assert((list3 + n - 1) == q);
    assert(list+2 == list->prev[d]);
    // q = last point, q->next = s3, s3->prev = last point
    q->next[d] = list+2;
    (list+2)->prev[d] = q;
}

static double
one_point_hv(const double * restrict x, const double * restrict ref, dimension_t d)
{
    double hv = ref[0] - x[0];
    for (dimension_t i = 1; i < d; i++)
        hv *= (ref[i] - x[i]);
    return hv;
}

double hv4dplusU(dlnode_t * list);

static double
fpli_hv4d(fpli_dlnode_t *list, dlnode_t * restrict list4d, size_t c)
{
    ASSUME(c > 1);
    fpli_hv4d_setup_cdllist(list->next[0], list4d, c);
    double hv = hv4dplusU(list4d);
    return hv;
}

static double
hv_recursive(fpli_dlnode_t * restrict list, dlnode_t * restrict list4d,
             dimension_t dim, size_t c,
             const double * restrict ref, double * restrict bound)
{
    ASSUME(dim > STOP_DIMENSION);
    ASSUME(c > 1);
    /* ------------------------------------------------------
       General case for dimensions higher than 4D
       ------------------------------------------------------ */
    const dimension_t d_stop = dim - STOP_DIMENSION;
    fpli_dlnode_t *p1 = list->prev[d_stop];
    for (fpli_dlnode_t *pp = p1; pp->x; pp = pp->prev[d_stop]) {
        if (pp->ignore < dim)
            pp->ignore = 0;
    }
    fpli_dlnode_t *p0 = list;
    /* Delete all points x[dim] > bound[d_stop].  In case of repeated
       coordinates, delete also all points x[dim] == bound[d_stop] except
       one.  */
    while (p1->x[dim] > bound[d_stop] || p1->prev[d_stop]->x[dim] >= bound[d_stop]) {
        // FIXME: Instead of deleting each point, unlink the start and end
        // nodes after the loop.
        delete(p1, dim, bound);
        p0 = p1;
        p1 = p1->prev[d_stop];
        c--;
        if (c == 1)
            break;
    }

    double hyperv = 0;
    if (c > 1) {
        hyperv = p1->prev[d_stop]->vol[d_stop] + p1->prev[d_stop]->area[d_stop]
            * (p1->x[dim] - p1->prev[d_stop]->x[dim]);
    } else {
        ASSUME(c == 1);
        p1->area[0] = one_point_hv(p1->x, ref, STOP_DIMENSION);
        for (dimension_t i = STOP_DIMENSION; i < dim; i++)
            p1->area[i + 1 - STOP_DIMENSION] = p1->area[i - STOP_DIMENSION] * (ref[i] - p1->x[i]);
        p1->vol[d_stop] = 0;
        if (p0->x == NULL) {
            return p1->area[d_stop] * (ref[dim] - p1->x[dim]);
        }
        hyperv = p1->area[d_stop] * (p0->x[dim] - p1->x[dim]);
        bound[d_stop] = p0->x[dim];
        reinsert(p0, dim, bound);
        c++;
        p1 = p0;
        p0 = p0->next[d_stop];
    }

    assert(c > 1);
    while (true) {
        p1->vol[d_stop] = hyperv;
        double hypera;
        if (p1->ignore >= dim) {
            hypera = p1->prev[d_stop]->area[d_stop];
        } else {
            if (dim - 1 == STOP_DIMENSION) {
                /*---------------------------------------
                  base case of dimension 4
                  --------------------------------------*/
                hypera = fpli_hv4d(list, list4d, c);
            } else {
                hypera = hv_recursive(list, list4d, dim-1, c, ref, bound);
            }
            if (hypera <= p1->prev[d_stop]->area[d_stop])
                p1->ignore = dim;
        }
        p1->area[d_stop] = hypera;
        if (p0->x == NULL) {
            hyperv += hypera * (ref[dim] - p1->x[dim]);
            return hyperv;
        }
        hyperv += hypera * (p0->x[dim] - p1->x[dim]);
        bound[d_stop] = p0->x[dim];
        reinsert(p0, dim, bound);
        c++;
        p1 = p0;
        p0 = p0->next[d_stop];
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
        while (j < n && p[j][1] >= prev_j)
            j++;
        if (unlikely(j == n))
            break; /* No other point dominates ref. */
        // We found one point that dominates ref.
        hyperv += (ref[0] - p[j][0]) * (prev_j - p[j][1]);
        prev_j = p[j][1];
        j++;
    } while (j < n);

    free(p);
    return hyperv;
}

double hv3d_plus(const double * restrict data, size_t n, const double * restrict ref);
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
    ASSUME(d < 256);
    ASSUME(d > 1);
    if (d == 4) return hv4d(data, n, ref);
    if (d == 3) return hv3d_plus(data, n, ref);
    if (d == 2) return hv2d(data, n, ref);
    dimension_t dim = (dimension_t) d;
    fpli_dlnode_t * list = fpli_setup_cdllist(data, dim, &n, ref);
    double hyperv;
    if (unlikely(n == 0)) {
        /* Returning here would leak memory.  */
        hyperv = 0.0;
    } else if (unlikely(n == 1)) {
        hyperv = one_point_hv(list->next[0]->x, ref, dim);
    } else {
        const dimension_t d_stop = dim - STOP_DIMENSION;
        ASSUME(d_stop > 1 && d_stop < 255); // Silence -Walloc-size-larger-than= warning
        double * bound = malloc(d_stop * sizeof(double));
        for (dimension_t i = 0; i < d_stop; i++)
            bound[i] = -DBL_MAX;
        dlnode_t * list4d = new_cdllist(n, ref);
        hyperv = hv_recursive(list, list4d, dim - 1, n, ref, bound);
        free_cdllist(list4d);
        free(bound);
    }
    /* Clean up.  */
    fpli_free_cdllist(list);
    return hyperv;
}
