/*************************************************************************

 eaf.h: prototypes and shared types of eaf and eaf-test

 ---------------------------------------------------------------------

                    Copyright (c) 2006-2008, 2025
                  Carlos Fonseca <cmfonsec@ualg.pt>
          Manuel Lopez-Ibanez <manuel.lopez-ibanez@manchester.ac.uk>

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.

 ----------------------------------------------------------------------

*************************************************************************/
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <float.h>
#include <math.h>

#include "common.h"
#include "io.h"

/* If the input are always integers, adjusting this type will
   certainly improve performance.  */
#define OBJECTIVE_TYPE_INT 0
#define OBJECTIVE_TYPE_DOUBLE 1
#ifndef OBJECTIVE_TYPE
# define OBJECTIVE_TYPE OBJECTIVE_TYPE_DOUBLE
#endif
#if OBJECTIVE_TYPE == OBJECTIVE_TYPE_DOUBLE
# define objective_t double
# define objective_MAX INFINITY
# define objective_MIN -INFINITY
# define fread_objective_t fread_double
# define read_objective_t_data read_double_data
#elif OBJECTIVE_TYPE == OBJECTIVE_TYPE_INT
# define objective_t int
# define objective_MAX INT_MAX
# define objective_MIN INT_MIN
# define fread_objective_t fread_int
# define read_objective_t_data read_int_data
#else
# error "Unknown OBJECTIVE_TYPE= value"
#endif

#include "bit_array.h"

typedef struct {
    int nruns; /* FIXME: there is no point in storing this here.  */
    size_t size;
    size_t maxsize;
    unsigned int nreallocs;
    bit_array * bit_attained;
    objective_t * data;
} eaf_t;


void
eaf_print_attsurf(const eaf_t *, int nobj,
                  FILE * coord_file, /* output file (coordinates)           */
                  FILE * indic_file, /* output file (attainment indicators) */
                  FILE * diff_file); /* output file (difference nruns/2)    */

eaf_t * eaf_create(int nobj, int nruns, int npoints);
void eaf_delete(eaf_t * eaf);
void eaf_free(eaf_t ** eaf, int nruns);
objective_t * eaf_store_point_help(eaf_t * eaf, int nobj,
                                   const int * save_attained);

static inline int
eaf_totalpoints(eaf_t ** eaf, int n)
{
    int totalpoints = 0;
    for (int k = 0; k < n; k++) totalpoints += (int)eaf[k]->size;
    return totalpoints;
}

eaf_t ** eaf2d(const objective_t * data, /* the objective vectors            */
               const int * cumsize,      /* the cumulative sizes of the runs */
               int nruns,                /* the number of runs               */
               const int * attlevel,     /* the desired attainment levels    */
               int nlevels               /* the number of att levels         */
);

eaf_t ** eaf3d(objective_t * data, const int * cumsize, int nruns,
               const int * attlevel, const int nlevels);

static inline eaf_t **
attsurf(objective_t * data,   /* the objective vectors            */
        int nobj,             /* the number of objectives         */
        const int * cumsize,  /* the cumulative sizes of the runs */
        int nruns,            /* the number of runs               */
        const int * attlevel, /* the desired attainment levels    */
        int nlevels           /* the number of att levels         */
)
{
    switch (nobj) {
    case 2:
        return eaf2d(data, cumsize, nruns, attlevel, nlevels);
        break;
    case 3:
        return eaf3d(data, cumsize, nruns, attlevel, nlevels);
        break;
    default:
        fatal_error(
            "this implementation only supports two or three dimensions.\n");
    }
}

static inline void
attained_left_right(const bit_array * attained, int division, int total,
                    int * count_left, int * count_right)
{
    assert(division < total);
    int count_l = 0;
    int count_r = 0;
    int k;

    for (k = 0; k < division; k++)
        if (bit_array_get(attained, k)) count_l++;
    for (k = division; k < total; k++)
        if (bit_array_get(attained, k)) count_r++;

    *count_left = count_l;
    *count_right = count_r;
}

static inline double
level2percentile(int level, int n)
{
    if (level == n) return 100.;
    return ((double)level * 100.0) / (double)n;
}

static inline int
percentile2level(double p, int n)
{
    const double tolerance = sqrt(DBL_EPSILON);
    double x = (n * p) / 100.0;
    int level = (x - floor(x) <= tolerance) ? (int)floor(x) : (int)ceil(x);

    assert(level <= n);
    assert(level >= 0);
    if (level < 1) level = 1;
    return level;
}

_attr_maybe_unused static int *
levels_from_percentiles(const double * percentile, int nlevels, int nruns)
{
    int * level;
    if (percentile) {
        level = malloc(sizeof(int) * nlevels);
        for (int k = 0; k < nlevels; k++)
            level[k] = percentile2level(percentile[k], nruns);
    } else {
        assert(nlevels == nruns);
        level = malloc(sizeof(int) * nruns);
        for (int k = 0; k < nruns; k++) level[k] = k + 1;
    }
    return level;
}

_attr_maybe_unused static void
all_percentiles(double * percentiles, int n_sets)
{
    const double x = 100.0 / (double)n_sets;
    for (int i = 1; i <= n_sets; i++) percentiles[i - 1] = i * x;
}

void eaf2matrix_R(double * rmat, eaf_t * const * eaf, int nobj, int totalpoints,
                  const double * percentile, int nlevels);

void eaf2matrix(double * rmat, eaf_t * const * eaf, int nobj,
                _attr_maybe_unused int totalpoints, const double * percentile,
                int nlevels);

double * eaf_compute_matrix(int * eaf_npoints, double * data, int nobj,
                            const int * cumsizes, int nruns,
                            const double * percentile, int nlevels);

static inline double
eafdiff_percentile(const eaf_t * eaf, size_t i, int division, int nruns,
                   int n_intervals)
{
    const bit_array * bit_attained =
        bit_array_offset(eaf->bit_attained, i, nruns);
    int count_left, count_right;
    attained_left_right(bit_attained, division, nruns, &count_left,
                        &count_right);
    return n_intervals * (double)((count_left / (double)division) -
                                  (count_right / (double)(nruns - division)));
}

double * eafdiff_compute_rectangles(int * eaf_npoints, double * data, int nobj,
                                    const int * cumsizes, int nruns,
                                    int intervals);
double * eafdiff_compute_matrix(int * eaf_npoints, double * data, int nobj,
                                const int * cumsizes, int nruns, int intervals);

#define cvector_assert(X) assert(X)
#include "cvector.h"
vector_define(vector_objective, objective_t) vector_define(vector_int, int)

    typedef struct {
    vector_objective xy;
    vector_int col;
} eaf_polygon_t;

#define eaf_compute_area eaf_compute_polygon

eaf_polygon_t * eaf_compute_polygon(eaf_t ** eaf, int nobj, int nlevels);
eaf_polygon_t * eaf_compute_polygon_old(eaf_t ** eaf, int nobj, int nlevels);
void eaf_print_polygon(FILE * stream, eaf_t ** eaf, int nobj, int nlevels);
eaf_polygon_t * eaf_compute_rectangles(eaf_t ** eaf, int nobj, int nlevels);
