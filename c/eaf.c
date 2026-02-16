/*************************************************************************

 eaf: Computes the empirical attainment function (EAF) from a number
 of approximation sets.

 ---------------------------------------------------------------------

    Copyright (C) 2005
            Carlos M. Fonseca <cmfonsec@ualg.pt>
    Copyright (C) 2006-2008, 2015, 2025
            Carlos M. Fonseca <cmfonsec@dei.uc.pt>
            Manuel Lopez-Ibanez <manuel.lopez-ibanez@manchester.ac.uk>

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.

 ----------------------------------------------------------------------


*************************************************************************/
#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "eaf.h"

#ifndef DEBUG
#define DEBUG 0
#endif

#ifndef DEBUG_POLYGONS
#define DEBUG_POLYGONS 0
#endif

static int compare_x_asc(const void * p1, const void * p2)
{
    objective_t x1 = **(objective_t **)p1;
    objective_t x2 = **(objective_t **)p2;

    return (x1 < x2) ? -1 : ((x1 > x2) ? 1 : 0);
}

static int compare_y_desc(const void * p1, const void * p2)
{
    objective_t y1 = *(*(objective_t **)p1+1);
    objective_t y2 = *(*(objective_t **)p2+1);

    return (y1 > y2) ? -1 : ((y1 < y2) ? 1 : 0);
}

static void
point2d_printf(FILE * stream, const objective_t x, const objective_t y)
{
    fprintf(stream, point_printf_format point_printf_sep point_printf_format, x, y);
}

static void
point_printf(FILE * stream, const objective_t * restrict p, dimension_t nobj)
{
    point2d_printf(stream, p[0], p[1]);
    for (dimension_t k = 2; k < nobj; k++)
        fprintf(stream, point_printf_sep point_printf_format, p[k]);
}

eaf_t * eaf_create(int nobj, int nruns, int npoints)
{
    ASSUME(nruns >= 0);
    ASSUME(npoints >= 0);

    eaf_t * eaf = MOOCORE_MALLOC(1, eaf_t);
    eaf->nruns = nruns;
    eaf->size = 0;
    eaf->nreallocs = 0;
    /* Maximum is npoints, but normally it will be smaller, so at most
       log2(2 * nruns) realloc will occur.  */
    eaf->maxsize = 256 + npoints / (2 * nruns);
    /* fprintf(stderr,"maxsize %ld = %d npoints, %d nruns\n", */
    /*         eaf->maxsize, npoints, nruns); */
    eaf->data = MOOCORE_MALLOC(nobj * eaf->maxsize, objective_t);
    eaf->bit_attained = malloc(bit_array_bytesize(nruns) * eaf->maxsize);
    return eaf;
}

void eaf_delete(eaf_t * eaf)
{
    free(eaf->data);
    free(eaf->bit_attained);
    free(eaf);
}

void eaf_free(eaf_t ** eaf, int nruns)
{
    for (int k = 0; k < nruns; k++)
        eaf_delete (eaf[k]);
    free(eaf);
}

static void
eaf_realloc(eaf_t * eaf, dimension_t nobj)
{
    const int nruns = eaf->nruns;
    eaf->data = realloc(eaf->data,
                        sizeof(objective_t) * nobj * eaf->maxsize);
    assert(eaf->data);
    eaf->bit_attained = realloc(eaf->bit_attained,
                                bit_array_bytesize(nruns) * eaf->maxsize);
    assert(eaf->bit_attained);
}

objective_t *
eaf_store_point_help(eaf_t * restrict eaf, dimension_t nobj,
                     const int * restrict save_attained)
{
    const int nruns = eaf->nruns;

    if (eaf->size == eaf->maxsize) {
        assert (eaf->size < INT_MAX / 2);
        //size_t old_maxsize = eaf->maxsize;
        eaf->maxsize = (size_t) ((double) eaf->maxsize * (1.0 + 1.0 / pow(2, eaf->nreallocs / 4.0)));
        eaf->maxsize += 100; // At least we increase it by 100 points
        /* fprintf(stderr,"maxsize (%d): %ld -> %ld\n", eaf->nreallocs, */
        /*         old_maxsize, eaf->maxsize); */
        eaf->nreallocs++;
        // FIXME: We could save memory by only storing eaf->attained per point if requested.
        eaf_realloc(eaf, nobj);
    }
    // FIXME: provide a bit_array function to do this.
    for (int k = 0; k < nruns; k++) {
        bit_array_set(bit_array_offset(eaf->bit_attained, eaf->size, nruns), k, (bool) save_attained[k]);
    }
    return eaf->data + nobj * eaf->size;
}

static void
eaf_adjust_memory(eaf_t * restrict eaf, dimension_t nobj)
{
    if (eaf->size < eaf->maxsize) {
        //fprintf(stderr,"reduce size: %ld -> %ld\n", eaf->maxsize, eaf->size);
        eaf->maxsize = eaf->size;
        eaf_realloc(eaf, nobj);
    }
}

static void
eaf_store_point_2d(eaf_t * restrict eaf, objective_t x, objective_t y,
                   const int * restrict save_attained)
{
    const dimension_t nobj = 2;
    objective_t * pos = eaf_store_point_help(eaf, nobj, save_attained);
    pos[0] = x;
    pos[1] = y;
    eaf->size++;
}

static void
eaf_print_line(FILE * coord_file, FILE * indic_file, FILE * diff_file,
                const objective_t * restrict x, dimension_t nobj,
                const bit_array * restrict attained, int nruns)
{
    if (coord_file) {
        point_printf(coord_file, x, nobj);
        fprintf(coord_file,
                (coord_file == indic_file) || (coord_file == diff_file)
                ? "\t" : "\n");
    }

    int k, count1 = 0, count2 = 0;
    if (indic_file) {
        fprintf(indic_file, "%d",
                bit_array_get(attained, 0) ? (count1++,1) : 0);
        for (k = 1; k < nruns/2; k++)
            fprintf(indic_file, " %d",
                    bit_array_get(attained, k) ? (count1++,1) : 0);
        for (k = nruns/2; k < nruns; k++)
            fprintf(indic_file, " %d",
                    bit_array_get(attained, k) ? (count2++,1) : 0);

        fprintf(indic_file, (indic_file == diff_file) ? "\t" : "\n");
    } else if (diff_file) {
        attained_left_right(attained, nruns/2, nruns, &count1, &count2);
    }

    if (diff_file)
        fprintf(diff_file,"%d %d\n", count1, count2);
}

/* Print one attainment surface of the EAF.  */
void
eaf_print_attsurf(const eaf_t * restrict eaf, dimension_t nobj,
                  FILE * coord_file,  FILE * indic_file, FILE * diff_file)
{
    for (size_t i = 0; i < eaf->size; i++) {
        const objective_t * p = eaf->data + i * nobj;
        /* bit_array_fprintf(stderr, eaf->bit_attained, eaf->nruns * eaf->size); */
        eaf_print_line(coord_file, indic_file, diff_file,
                       p, nobj,
                       bit_array_offset(eaf->bit_attained, i, eaf->nruns),
                       eaf->nruns);
    }
}

_attr_maybe_unused static void
fprint_set2d (FILE * stream, const objective_t * const * data, int ntotal)
{
    for (int k = 0; k < ntotal; k++)
        fprintf (stream, "%6d: " point_printf_format " " point_printf_format "\n", k,
                 data[k][0], data[k][1]);
}

void
eaf2matrix_R(double * restrict rmat, eaf_t * const * eaf, int nobj, int totalpoints,
             const double * restrict percentile, int nlevels)
{
    int pos = 0;
    for (int k = 0; k < nlevels; k++) {
        size_t npoints = eaf[k]->size;
        double p = percentile ? percentile[k] : level2percentile(k+1, nlevels);
        for (size_t i = 0; i < npoints; i++) {
            for (int j = 0; j < nobj; j++) {
                rmat[pos + j * totalpoints] = eaf[k]->data[j + i * nobj];
            }
            rmat[pos + nobj * totalpoints] = p;
            pos++;
        }
    }
}

void
eaf2matrix(double * restrict rmat, eaf_t * const * eaf, int nobj,
           _attr_maybe_unused int totalpoints,
           const double * restrict percentile, int nlevels)
{
    const dimension_t ncol = (dimension_t)nobj + 1;
    int pos = 0;
    for (int k = 0; k < nlevels; k++) {
        size_t npoints = eaf[k]->size;
        double p = percentile ? percentile[k] : level2percentile(k+1, nlevels);
        for (size_t i = 0; i < npoints; i++) {
            for (dimension_t j = 0; j < nobj; j++) {
                rmat[j + pos * ncol] = eaf[k]->data[j + i * nobj];
            }
            rmat[nobj + pos * ncol] = p;
            pos++;
        }
    }
}

double *
eaf_compute_matrix(int * restrict eaf_npoints, double * restrict data, int nobj,
                   const int * restrict cumsizes, int nruns,
                   const double * restrict percentile, int nlevels)
{
    int * level = levels_from_percentiles(percentile, nlevels, nruns);
    eaf_t ** eaf = attsurf(data, nobj, cumsizes, nruns, level, nlevels);
    free (level);

    int totalpoints = eaf_totalpoints(eaf, nlevels);
    double * mat = malloc(sizeof(*mat) * totalpoints * (nobj + 1));
    eaf2matrix(mat, eaf, nobj, totalpoints, percentile, nlevels);
    eaf_free(eaf, nlevels);
    *eaf_npoints = totalpoints;
    return mat;
}

/*
   eaf2d: compute attainment surfaces from points in objective space,
          using dimension sweeping.

   Input arguments:
        data : a pointer to the data matrix, stored as a linear array of
               objective_t in row major order.
        cumsize : an array containing the cumulative number of rows in each
                  non-dominated front (must be non-decreasing).
        nruns :	the number of independent non-dominated fronts.
        attlevel : an array containing the attainment levels to compute.
        nlevel : number of attainment levels to compute.
        coord_file  : stream to write the resulting attainment surfaces.
        indic_file  : stream to write the resulting attainment indices.
        diff_file   : stream to write the difference between the
                      first half and the second half of nruns.
*/

eaf_t **
eaf2d(const objective_t * restrict data, const int * restrict cumsize, int nruns,
      const int * restrict attlevel, const int nlevels)
{
    const dimension_t nobj = 2;
    const int ntotal = cumsize[nruns - 1]; /* total number of points in data */
    /* Access to the data is made via two arrays of pointers: ix, iy
       These are sorted, to allow for dimension sweeping */

    const objective_t ** datax = malloc(2 * ntotal * sizeof(*datax));
    const objective_t ** datay = datax + ntotal;

    for (int k = 0; k < ntotal; k++)
        datax[k] = datay[k] = data + nobj * k;

    DEBUG1(/* Sanity check. */
        for (int k = 1; k < nruns ; k++)
            assert(cumsize[k-1] < cumsize[k]);
        );
    DEBUG2(
        fprintf(stderr, "Original data:\n");
        fprint_set2d(stderr, datax, ntotal);
        );

    qsort(datax, ntotal, sizeof(*datax), &compare_x_asc);
    qsort(datay, ntotal, sizeof(*datay), &compare_y_desc);

    DEBUG2(
        fprintf(stderr, "Sorted data (x):\n");
        fprint_set2d(stderr, datax, ntotal);
        fprintf(stderr, "Sorted data (y):\n");
        fprint_set2d (stderr, datay, ntotal);
        );

    /* Setup a lookup table to go from a point to the approximation set (run)
       to which it belongs.  */

    int * runtab = malloc(ntotal * sizeof(*runtab));
    for (int k = 0, j = 0; k < ntotal; j++) {
        for (; k < cumsize[j]; k++)
            runtab[k] = j;
    }

    DEBUG2(
        fprintf(stderr, "Runtab:\n");
        for (int k = 0; k < ntotal; k++)
            fprintf(stderr, "%6d: %6d\n", k, runtab[k]);
        );

    /* Setup tables to keep attainment statistics. In particular,
       save_attained is needed to cope with repeated values on the same
       axis. */
    int * attained = malloc(2 * nruns * sizeof(*attained));
    int * save_attained = attained + nruns;
    eaf_t ** eaf = malloc(nlevels * sizeof(*eaf));

    for (int l = 0; l < nlevels; l++) {
        eaf[l] = eaf_create(nobj, nruns, ntotal);
        int level = attlevel[l];
        int x = 0;
        int y = 0;
        int nattained = 0;
        for (int k = 0; k < nruns; k++) attained[k] = 0;

        /* Start at upper-left corner */
        int run = runtab[(datax[x] - data) / nobj];
        attained[run]++;
        nattained++;
        do {
            /* Move right until desired attainment level is reached */
            while (x < ntotal - 1 &&
                   (nattained < level || datax[x][0] == datax[x+1][0])) {
                x++;
                if (datax[x][1] <= datay[y][1]) {
                    run = runtab[(datax[x] - data)/nobj];
                    if (!attained[run])
                        nattained++;
                    attained[run]++;
                }
            }
            DEBUG2(
                for (int k = 0; k < nruns; k++)
                    fprintf (stderr, "%d ", attained[k]);
                fprintf (stderr, "\n")
                );

            if (nattained < level)
                continue;

            /* Now move down until desired attainment level is no
               longer reached.  */
            do {
                /* If there are repeated values along the y axis,
                   we need to remember where we are.  */
                memcpy (save_attained, attained, nruns * sizeof(*attained));

                do {
                    if (datay[y][0] <= datax[x][0]) {
                        run = runtab[(datay[y] - data)/nobj];
                        attained[run]--;
                        if (!attained[run])
                            nattained--;
                    }
                    y++;
                    DEBUG2(
                        for (int k = 0; k < nruns; k++)
                            fprintf (stderr, "%d ", attained[k]);
                        fprintf (stderr, "\n")
                        );
                } while (y < ntotal && datay[y][1] == datay[y - 1][1]);
            } while (nattained >= level && y < ntotal);

            assert(nattained < level);
            eaf_store_point_2d(eaf[l], datax[x][0], datay[y - 1][1], save_attained);

        } while (x < ntotal - 1 && y < ntotal);
        eaf_adjust_memory(eaf[l], nobj);
    }
    free(attained);
    free(runtab);
    free(datax);

    return eaf;
}


#if DEBUG_POLYGONS > 0
#define PRINT_POINT(X,Y,C) do { \
    fprintf(stdout, "PRINT_POINT:"); point2d_printf(stdout, X, Y);             \
    if (C != INT_MIN) fprintf(stdout, "\t%d", C);                              \
    fprintf(stdout, "\n"); } while(0)
#else
#define PRINT_POINT(X,Y,C) (void)0
#endif

static size_t
eaf_max_size(eaf_t * const * eaf, int nlevels)
{
    size_t max_size = 0;
    for (int a = 0; a < nlevels; a++) {
        if (max_size < eaf[a]->size)
            max_size = eaf[a]->size;
    }
    return max_size;
}

static int
eaf_diff_color(const eaf_t * eaf, size_t k, int nruns)
{
    const bit_array * bit_attained = bit_array_offset(eaf->bit_attained, k, nruns);
    int count_left, count_right;
    attained_left_right(bit_attained, nruns/2, nruns, &count_left, &count_right);
    return count_left - count_right;
}

static void
init_colors(int * restrict color, const eaf_t * restrict eaf, size_t eaf_size, int nruns)
{
    for (size_t k = 0; k < eaf_size; k++) {
        color[k] = eaf_diff_color(eaf, k, nruns);
    }
}

static const objective_t *
next_polygon(const objective_t * src, int nobj, const objective_t * end)
{
    while (src < end && *src != objective_MIN)
        src += nobj;
    src += nobj;
    return src;
}

static void
min_max_in_objective(const objective_t * restrict v, dimension_t nobj,
                     objective_t * restrict min_ref, objective_t * restrict max_ref)
{
    assert(*v != objective_MIN); // Empty polygon?
    objective_t min = *v;
    objective_t max = *v;
    v += nobj;
    while (*v != objective_MIN) {
        if (min > *v) min = *v;
        if (max < *v) max = *v;
        v += nobj;
    }
    *min_ref = min;
    *max_ref = max;
}

static bool
polygon_dominates_point(const objective_t * p, const objective_t * x, int nobj)
{
    assert(x[0] != objective_MIN);
    while (p[0] != objective_MIN) {
        if (p[0] < x[0] && p[1] < x[1]) {
            /* point2d_printf(stdout, p[0], p[1]); */
            /* printf(" dominates "); */
            /* point2d_printf(stdout, x[0], x[1]); */
            /* printf("\n"); */
            return true;
        }
        p += nobj;
    }
    return false;
}

static bool
polygon_dominates_any_point(const objective_t * a, const objective_t * b, int nobj)
{
    while (b[0] != objective_MIN) {
        if (polygon_dominates_point(a, b, nobj)) return true;
        b += nobj;
    }
    return false;
}

static bool
polygons_intersect(const objective_t * a, const objective_t * b, dimension_t nobj)
{
    for (dimension_t k = 0; k < nobj; k++) {
        objective_t min_a, max_a, min_b, max_b;
        min_max_in_objective(a + k, nobj, &min_a, &max_a);
        min_max_in_objective(b + k, nobj, &min_b, &max_b);
        // If we can draw a line completely separating them in one axis, then
        // they don't intersect.
        if (max_a <= min_b || max_b <= min_a) return false;
    }
    // Two orthogonal polygons intersect if there is a corner of A that is
    // dominated by a corner of B and there is a corner of B that is dominated
    // by a corner of A
    return polygon_dominates_any_point(a, b, nobj) && polygon_dominates_any_point(b, a, nobj);
}


_attr_maybe_unused static void
polygon_print(const objective_t * p, dimension_t nobj)
{
    while (p[0] != objective_MIN) {
        point_printf(stderr, p, nobj);
        fprintf(stderr, "\n");
        p += nobj;
    }
    point_printf(stderr, p, nobj);
    fprintf(stderr, "\n");
}

_attr_maybe_unused static void
eaf_check_polygons(eaf_polygon_t * p, dimension_t nobj)
{
    // This only works for 2 objectives.
    assert(nobj == 2);

    // Check #1: Polygons don't intersect
    // Last point of last polygon
    const objective_t * end = vector_objective_end(&p->xy);
    const objective_t * pi = vector_objective_begin(&p->xy);

    while (pi < end) {
        const objective_t * pj = next_polygon(pi, nobj, end);
        const objective_t * next = pj;
        while (pj < end) {
            if (polygons_intersect(pi, pj, nobj)) {
#if DEBUG_POLYGONS > 0
                fprintf(stderr, "ERROR: Polygons intersect!\n");
                polygon_print(pi, nobj);
                polygon_print(pj, nobj);
#endif
                assert(false);
            }
            pj = next_polygon(pj, nobj, end);
        }
        pi = next;
    }
    // Check #2: Every point in the EAF is a corner of a polygon and it has the
    // same color as the polygon.

    //TODO


}

/* Produce a polygon suitable to be plotted by the polygon function in R.  */
eaf_polygon_t *
eaf_compute_polygon(eaf_t ** eaf, int nobj, int nlevels)
{
/* FIXME: Don't add anything if color_0 == 0 */

#define POLY_SIZE_CHECK()                                                      \
    do { _poly_size_check--; assert(_poly_size_check >= 4);                    \
        assert(_poly_size_check % 2 == 0); _poly_size_check = 0; } while(0)
#define eaf_point(A,K) (eaf[(A)]->data + (K) * nobj)
#define push_point_color(X, Y, C)                                              \
        do {  vector_objective_push_back (&polygon->xy, (X));                  \
            vector_objective_push_back (&polygon->xy, (Y));                    \
            _poly_size_check++; PRINT_POINT(X,Y, C);                           \
    } while(0)

#define push_point(X, Y) push_point_color(X,Y, INT_MIN)

#if DEBUG_POLYGONS > 0
#define EXPENSIVE_CHECK_POLYGONS() eaf_check_polygons(polygon, (dimension_t)nobj)
#else
#define EXPENSIVE_CHECK_POLYGONS() (void)0
#endif

#define polygon_close(COLOR) do {                                              \
                vector_int_push_back (&polygon->col, COLOR);                   \
                push_point_color(objective_MIN, objective_MIN, COLOR);         \
                POLY_SIZE_CHECK(); EXPENSIVE_CHECK_POLYGONS();                 \
            } while(0)

    _attr_maybe_unused int _poly_size_check = 0;
    int nruns = eaf[0]->nruns;

    assert(nruns % 2 == 0);

    size_t max_size = eaf_max_size(eaf, nlevels);
    int * color = MOOCORE_MALLOC(max_size, int);
    eaf_polygon_t * polygon = MOOCORE_MALLOC(1, eaf_polygon_t);
    vector_objective_ctor (&polygon->xy, max_size);
    vector_int_ctor (&polygon->col, max_size);

    for (int b = 1; b < nlevels; b++) {
        const int a = b - 1;
        const int eaf_a_size = (int) eaf[a]->size;
        const int eaf_b_size = (int) eaf[b]->size;
        init_colors(color, eaf[a], eaf_a_size, nruns);

        /* Find color transitions along the EAF level set.  */
        objective_t topleft_y = objective_MAX;
        int last_b = -1;
        int ka = 0;
        while (true) {
            const objective_t * pka = NULL;
            const objective_t * pkb = NULL;
            int kb = last_b + 1;
            // Find a point in a that does not overlap with a point in b
            while (ka < eaf_a_size && kb < eaf_b_size) {
                pka = eaf_point (a, ka);
                pkb = eaf_point (b, kb);
                if (pkb[0] != pka[0])
                    break;
                // They overlap in x, so we will skip kb, remember it.
                topleft_y = pkb[1];
                last_b = kb;
                if (pkb[1] == pka[1]) {
                    /* Ignore points that exactly overlap.  */
                    // FIXME: This should not happen, but it does. We should remove these points.
                    // assert(false);
                    ka++; kb++;
                } else {
                    /* b intersects a above pka. */
                    assert(pkb[1] > pka[1]);
                    kb++;
                    break;
                }
            }

            /* Everything in A was overlapping. */
            if (ka == eaf_a_size)
                break;

            objective_t prev_pka_y = topleft_y;
            int color_0 = color[ka];
            /* Print points and corners until we reach a different color. */
            do {
                pka = eaf_point (a, ka);
                /* Find the point in B not above the current point in A. */
                while (kb < eaf_b_size) {
                    pkb = eaf_point (b, kb);
                    assert(pkb[0] > pka[0]);
                    if (pkb[1] <= pka[1])
                        break;
                    kb++;
                }
                assert(pka[1] < prev_pka_y);
                push_point (pka[0], prev_pka_y);
                push_point (pka[0], pka[1]);
                prev_pka_y = pka[1];
                ka++;

                if (kb < eaf_b_size && ka < eaf_a_size) {
                    const objective_t * pka_next = eaf_point (a, ka);
                    assert (pkb[0] > pka[0]);
                    assert (pkb[1] <= pka[1]);
                    if (pkb[0] <= pka_next[0]) {
                        /* If B intersects with A, stop here.  */
                        assert(pkb[1] == pka[1] || pkb[0] == pka_next[0]);
                        assert (prev_pka_y >= pkb[1]);
                        break;
                    }
                }
                /* FIXME: When is color_0 != color[ka] if they are in the same eaf level ? */
            } while (ka < eaf_a_size && color_0 == color[ka]);

            /* pka is the point before changing color, but ka is the
               position after.  */
            if (ka == eaf_a_size) {/* We reached the end of eaf_a */
                /* We don't have to go down the other side since eaf_a
                   completely dominates eaf_b, so just start by the
                   end. */
                if (last_b == eaf_b_size - 1) {
                    /* The last point we skipped was the last point, so there
                       is nothing on the other side, just create two points in
                       the infinity. */
                    push_point (objective_MAX, pka[1]);
                    push_point (objective_MAX, topleft_y);
                    assert(topleft_y > pka[1]);
                } else {
                    kb = eaf_b_size - 1;
                    pkb = eaf_point(b, kb);
                    assert (pkb[1] >= pka[1]);
                    if (pkb[1] > pka[1]) {
                        // Create two points in the infinity to jump from a to b (turn the corner).
                        /* If pkb is above pkda, then it may happen that pkb[0]
                           <= pka[0] if pkb is dominated by a previous pka. */
                        push_point (objective_MAX, pka[1]);
                        push_point (objective_MAX, pkb[1]);
                        assert (pkb[1] <= topleft_y);
                    } else {
                        // If they are at the same y-level, then pka must be to
                        // the left, otherwise we should have found the
                        // intersection earlier and something is wrong.
                        assert(pkb[0] > pka[0]);
                    }
                    /* Now print in reverse.  */
                    objective_t prev_pkb_x = pkb[0];
                    push_point (pkb[0], pkb[1]);
                    kb--;
                    while (kb > last_b) {
                        pkb = eaf_point(b, kb);
                        assert (pkb[1] > pka[1]); // pkb cannot be below pka
                        push_point (prev_pkb_x, pkb[1]);
                        push_point (pkb[0], pkb[1]);
                        prev_pkb_x = pkb[0];
                        kb--;
                    }
                    push_point (pkb[0], topleft_y);
                    assert (topleft_y > pkb[1]);
                }
                /* last_b = eaf_b_size - 1; */
                polygon_close(color_0); /* DONE */
                break; /* Really done! */
            } else {
                if (kb == eaf_b_size) {
                    assert (pka[1] < topleft_y);
                    /* There is nothing on the other side, just create
                       two points in the infinity. */
                    push_point (objective_MAX, pka[1]);
                    push_point (objective_MAX, topleft_y);
                    last_b = eaf_b_size - 1;
                } else {
                    // This polygon is bounded above by eaf_b from last_b up to kb.
                    assert (kb < eaf_b_size);
                    pkb = eaf_point (b, kb);
                    // If we have not finished eaf_b is because pkb is not above pka
                    assert (pkb[1] <= pka[1]);
                    assert (pkb[0] != pka[0]);
                    /* If pkb and pka are in the same horizontal, pkb
                       does not affect the next polygon. Otherwise, it
                       does. */
                    int save_last_b = (pkb[1] == pka[1]) ? kb : kb - 1;
                    /* Now print eaf_b in reverse.  */
                    objective_t prev_pkb_x = pkb[0];
                    push_point (pkb[0], pka[1]);
                    kb--;
                    while (kb > last_b) {
                        pkb = eaf_point (b, kb);
                        // pkb must be above pka or we would have found the
                        // intersection earlier.
                        assert (pkb[1] > pka[1]);
                        push_point (prev_pkb_x, pkb[1]);
                        push_point (pkb[0], pkb[1]);
                        prev_pkb_x = pkb[0];
                        kb--;
                    }
                    push_point (pkb[0], topleft_y);
                    assert (topleft_y > pkb[1]);
                    last_b = save_last_b;
                }
                polygon_close(color_0); /* DONE */
                assert(topleft_y >= pka[1]);
                topleft_y = pka[1];
            }
        }
    }
    free (color);
    DEBUG1(eaf_check_polygons(polygon, (dimension_t)nobj)); // This is slow with lots of polygons
    return polygon;
}

/* FIXME: This version does not care about intersections, which is
   much simpler, but it produces artifacts when plotted with the
   polygon function in R.  */
eaf_polygon_t *
eaf_compute_polygon_old (eaf_t ** eaf, int nobj, int nlevels)
{
    _attr_maybe_unused int _poly_size_check = 0;
    size_t max_size = eaf_max_size(eaf, nlevels);
    int nruns = eaf[0]->nruns;
    assert(nruns % 2 == 0);

    int * color = MOOCORE_MALLOC(max_size, int);
    eaf_polygon_t * polygon = MOOCORE_MALLOC(1, eaf_polygon_t);
    vector_objective_ctor (&polygon->xy, max_size);
    vector_int_ctor (&polygon->col, max_size);

    for (int b = 1; b < nlevels; b++) {
        const int a = b - 1;
        const int eaf_a_size = (int) eaf[a]->size;
        const int eaf_b_size = (int) eaf[a + 1]->size;
        init_colors(color, eaf[a], eaf_a_size, nruns);
        /* Find color transitions along the EAF level set.  */
        int last_b = -1;
        objective_t topleft_y = objective_MAX;
        int ka = 0;
        while (ka < eaf_a_size) {
            objective_t prev_pka_y = topleft_y;
            const objective_t * pka;

            /* Print points and corners until we reach a different color. */
            int color_0 = color[ka];
            do {
                pka = eaf_point (a, ka);
                push_point (pka[0], prev_pka_y);
                push_point (pka[0], pka[1]);
                prev_pka_y = pka[1];
                ka++;
            } while (ka < eaf_a_size && color_0 == color[ka]);

            /* pka is the point before changing color, but ka is the
               position after.  */
            if (ka == eaf_a_size) {/* We reached the end of eaf_a */
                /* We don't have to go down the other side since eaf_a
                   completely dominates eaf_b, so just start by the
                   end. */
                int kb = eaf_b_size - 1;

                if (last_b == kb) {
                    /* There is nothing on the other side, just create
                       two points in the infinity. */
                    push_point (objective_MAX, pka[1]);
                    push_point (objective_MAX, topleft_y);
                } else {
                    const objective_t * pkb = eaf_point(a + 1, kb);
                    assert (pkb[1] >= pka[1]);
                    if (pkb[1] > pka[1]) {
                        /* Create two points in the infinity.  */
                        push_point (objective_MAX, pka[1]);
                        push_point (objective_MAX, pkb[1]);
                    }
                    objective_t prev_pkb_x = pkb[0];
                    push_point (pkb[0], pkb[1]);
                    kb--;
                    while (kb > last_b) {
                        pkb = eaf_point(a + 1, kb);
                        push_point (prev_pkb_x, pkb[1]);
                        push_point (pkb[0], pkb[1]);
                        prev_pkb_x = pkb[0];
                        kb--;
                    }
                    push_point (pkb[0], topleft_y);
                    last_b = eaf_b_size - 1;
                }
                polygon_close(color_0); /* DONE */
            } else {
                int kb = last_b + 1;
                /* Different color, go down by the other side until
                   reaching this point. */
                if (kb == eaf_b_size) {
                    /* There is nothing on the other side, just create
                       two points in the infinity. */
                    push_point (objective_MAX, pka[1]);
                    push_point (objective_MAX, topleft_y);
                    polygon_close(color_0); /* DONE */
                } else {
                    const objective_t * pkb;
                    do {
                        pkb = eaf_point (b, kb);
                        if (pkb[1] <= pka[1])
                            break;
                        kb++;
                    } while (kb < eaf_b_size);
                    int save_last_b = kb - 1;
                    if (kb == eaf_b_size) {
                        /* There is nothing on the other side, just create
                           two points in the infinity. */
                        push_point (objective_MAX, pka[1]);
                        push_point (objective_MAX, pkb[1]);
                    } else {/* pkb_y <= pka_y */
                        objective_t prev_pkb_x = pkb[0];
                        push_point (pkb[0], pka[1]);
                        /* Now print in reverse.  */
                        kb--;
                        while (kb > last_b) {
                            pkb = eaf_point (b, kb);
                            push_point (prev_pkb_x, pkb[1]);
                            push_point (pkb[0], pkb[1]);
                            prev_pkb_x = pkb[0];
                            kb--;
                        }
                        push_point (pkb[0], topleft_y);
                    }
                    polygon_close(color_0); /* DONE */
                    last_b = save_last_b;
                }
            }
            topleft_y = pka[1];
        }
    }

    free (color);
    return polygon;
}
#undef eaf_point
#undef push_point
#undef polygon_close
#undef PRINT_POINT
#undef POLY_SIZE_CHECK

void
eaf_print_polygon (FILE *stream, eaf_t **eaf, int nobj, int nlevels)
{
    eaf_polygon_t *p = eaf_compute_area (eaf, nobj, nlevels);

    for(size_t i = 0; i < vector_objective_size(&p->xy); i += 2) {
        point2d_printf(stream, vector_objective_at(&p->xy, i),
                       vector_objective_at(&p->xy, i + 1));
        fprintf(stream, "\n");
    }

    fprintf (stream, "# col =");
    for (size_t k = 0; k < vector_int_size (&p->col); k++)
        fprintf (stream, " %d", vector_int_at(&p->col, k));
    fprintf (stream, "\n");

    vector_objective_dtor (&p->xy);
    vector_int_dtor (&p->col);
    free(p);
}

static size_t
rectangle_add(eaf_polygon_t * regions,
              objective_t lx, objective_t ly,
              objective_t ux, objective_t uy,
              int color)
{
#if 0
    printf("rectangle_add: (" point_printf_format ", " point_printf_format ", " point_printf_format ", " point_printf_format ")[%d]\n", lx, ly, ux, uy, color);
#endif
    assert(lx < ux);
    assert(ly < uy);
    vector_objective *rect = &regions->xy;
    vector_objective_push_back(rect, lx);
    vector_objective_push_back(rect, ly);
    vector_objective_push_back(rect, ux);
    vector_objective_push_back(rect, uy);

    vector_int_push_back(&regions->col, color);
    return vector_objective_size(rect);
}

eaf_polygon_t *
eaf_compute_rectangles(eaf_t **eaf, int nobj, int nlevels)
{
#define eaf_point(A,K) (eaf[(A)]->data + (K) * nobj)
#if 0
#define printf_points(ka,kb,pka,pkb)                                           \
    printf("%4d: pa[%d]=(" point_printf_format ", " point_printf_format "), pb[%d] = (" point_printf_format ", " point_printf_format ")\n", \
           __LINE__, ka, pka[0], pka[1], kb, pkb[0], pkb[1])
#else
#define printf_points(ka,kb,pka,pkb)
#endif
    int nruns = eaf[0]->nruns;

    assert(nruns % 2 == 0);

    size_t max_size = eaf_max_size(eaf, nlevels);
    int * color = MOOCORE_MALLOC(max_size, int);
    eaf_polygon_t * regions = MOOCORE_MALLOC(1, eaf_polygon_t);
    vector_objective_ctor (&regions->xy, max_size);
    vector_int_ctor (&regions->col, max_size);

    for (int b = 1; b < nlevels; b++) {
        const int a = b - 1;
        const int eaf_a_size = (int) eaf[a]->size;
        const int eaf_b_size = (int) eaf[b]->size;
        if (eaf_a_size == 0 || eaf_b_size == 0) continue;

        // FIXME: Skip points with color 0?
        init_colors(color, eaf[a], eaf_a_size, nruns);
        objective_t top = objective_MAX;
        int ka = 0, kb = 0;
        const objective_t * pkb = eaf_point(b, kb);
        const objective_t * pka = eaf_point(a, ka);
        printf_points(ka, kb, pka, pkb);
        while (true) {

            while (pka[1] < pkb[1]) {
                if (pka[0] < pkb[0]) // pka strictly dominates pkb
                    rectangle_add(regions, pka[0], pkb[1], pkb[0], top, color[ka]);
                top = pkb[1];
                kb++;
                if (kb >= eaf_b_size) goto close_eaf;
                pkb = eaf_point (b, kb);
                printf_points(ka, kb, pka, pkb);
            }
            // pka_above_equal_pkb:
            if (pka[0] < pkb[0]) { // pka does not strictly dominate pkb
                rectangle_add(regions, pka[0], pka[1], pkb[0], top, color[ka]);
            } else {
                // Skip repeated points
                assert(pka[0] == pkb[0] && pka[1] == pkb[1]);
            }
            top = pka[1];
            ka++;
            if (ka >= eaf_a_size) goto next_eaf;
            pka = eaf_point (a, ka);
            printf_points(ka, kb, pka, pkb);

            if (pkb[1] == top) { // pkb was not above but equal to previous pka
                // Move to next pkb
                kb++;
                if (kb >= eaf_b_size) goto close_eaf;
                pkb = eaf_point (b, kb);
                printf_points(ka, kb, pka, pkb);
            }
        }
    close_eaf:
        // b is finished, add one rectangle for each pka point.
        while (true) {
            assert(pka[1] < pkb[1]);
            rectangle_add(regions, pka[0], pka[1], objective_MAX, top, color[ka]);
            top = pka[1];
            ka++;
            if (ka >= eaf_a_size) break;
            pka = eaf_point (a, ka);
            printf_points(ka, kb, pka, pkb);
        }
    next_eaf:
        continue;
    }
    return regions;
#undef eaf_point
}
