#ifndef EPSILON_H
#define EPSILON_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#ifndef INFINITY
#define INFINITY (HUGE_VAL)
#endif
#include "common.h"


typedef uint_fast8_t dimension_t;

/*
   IMPLEMENTATION NOTE: Given objective vectors a and b,
   I_epsilon(a,b) is computed in the case of minimization as a/b for
   the multiplicative case (respectively, a - b for the additive
   case), whereas in the case of maximization it is computed as b/a
   for the multiplicative case (respectively, b - a for the additive
   case). This allows computing a single value for mixed optimization
   problems, where some objectives are to be maximized while others
   are to be minimized. Moreover, a lower value corresponds to a
   better approximation set, independently of the type of problem
   (minimization, maximization or mixed). However, the meaning of the
   value is different for each objective type. For example, imagine
   that f1 is to be minimized and f2 is to be maximized, and the
   multiplicative epsilon computed here for I_epsilon(A,B) = 3. This
   means that A needs to be multiplied by 1/3 for all f1 values and by
   3 for all f2 values in order to weakly dominate B.

   This also means that the computation of the multiplicative version
   for negative values doesn't make sense.
 */
static inline double
epsilon_mult_minmax (int dim, const signed char * restrict minmax,
                     const double * restrict points_a, int size_a,
                     const double * restrict points_b, int size_b)
{
    ASSUME(dim >= 2);
    ASSUME(dim <= 32);
    ASSUME(size_a >= 0);
    ASSUME(size_b >= 0);

    int a, b;
    int d;
    for (a = 0; a < size_a; a++)
        for (d = 0; d < dim; d++)
            if (points_a[a * dim + d] <= 0)
                fatal_error ("cannot calculate multiplicative epsilon indicator with values <= 0\n.");

    for (b = 0; b < size_b; b++)
        for (d = 0; d < dim; d++)
            if (points_b[b * dim + d] <= 0)
                fatal_error ("cannot calculate multiplicative epsilon indicator with values <= 0\n.");

    double epsilon = 0;
    for (b = 0; b < size_b; b++) {
        double epsilon_min = INFINITY;
        for (a = 0; a < size_a; a++) {
            double epsilon_max = 0;
            for (d = 0; d < dim; d++) {
                double epsilon_temp;
                if (minmax[d] < 0)
                    epsilon_temp =  points_a[a * dim + d] / points_b[b * dim + d];
                else if (minmax[d] > 0)
                    epsilon_temp =  points_b[b * dim + d] / points_a[a * dim + d];
                else
                    epsilon_temp =  1;

                ASSUME(epsilon_temp >= 0);
                /* This cannot happen:
                if (epsilon_temp < 0) {
                    fatal_error("cannot calculate multiplicative epsilon indicator with different signedness\n.");
                    } */
                epsilon_max = MAX (epsilon_max, epsilon_temp);
            }
            epsilon_min = MIN (epsilon_min, epsilon_max);
        }
        epsilon = MAX (epsilon, epsilon_min);
    }
    return epsilon;
}

static inline double
epsilon_additive_minimize(dimension_t dim,
                          const double * restrict points_a, size_t size_a,
                          const double * restrict points_b, size_t size_b)
{
    ASSUME(dim >= 2);
    ASSUME(dim <= 32);
    size_t a, b;
    dimension_t d;
    double epsilon = -INFINITY;
    for (b = 0; b < size_b; b++) {
        bool skip_max = false;
        double epsilon_min = INFINITY;
        for (a = 0; a < size_a; a++) {
            double epsilon_max = points_a[a * dim + 0] - points_b[b * dim + 0];
            if (epsilon_max >= epsilon_min)
                continue;
            for (d = 1; d < dim; d++) {
                double epsilon_temp = points_a[a * dim + d] - points_b[b * dim + d];
                epsilon_max = MAX(epsilon_max, epsilon_temp);
            }
            if (epsilon_max <= epsilon) {
                skip_max = true;
                break;
            }
            epsilon_min = MIN (epsilon_min, epsilon_max);
        }
        if (skip_max) continue;
        epsilon = MAX(epsilon, epsilon_min);
    }
    return epsilon;
}

static inline double
epsilon_additive_maximize(dimension_t dim,
                          const double * restrict points_a, size_t size_a,
                          const double * restrict points_b, size_t size_b)
{
    ASSUME(dim >= 2);
    ASSUME(dim <= 32);
    size_t a, b;
    dimension_t d;
    double epsilon = -INFINITY;
    for (b = 0; b < size_b; b++) {
        bool skip_max = false;
        double epsilon_min = INFINITY;
        for (a = 0; a < size_a; a++) {
            double epsilon_max = points_b[b * dim + 0] - points_a[a * dim + 0];
            if (epsilon_max >= epsilon_min)
                continue;
            for (d = 1; d < dim; d++) {
                double epsilon_temp = points_b[b * dim + d] - points_a[a * dim + d];
                epsilon_max = MAX (epsilon_max, epsilon_temp);
            }
            if (epsilon_max <= epsilon) {
                skip_max = true;
                break;
            }
            epsilon_min = MIN (epsilon_min, epsilon_max);
        }
        if (skip_max) continue;
        epsilon = MAX(epsilon, epsilon_min);
    }
    return epsilon;
}

static inline double
epsilon_additive_minmax(int dim, const signed char * restrict minmax,
                        const double * restrict points_a, int size_a,
                        const double * restrict points_b, int size_b)
{
    ASSUME(dim >= 2);
    ASSUME(dim <= 32);
    ASSUME(size_a >= 0);
    ASSUME(size_b >= 0);

    bool all_minimize = true, all_maximize = true;
    for (dimension_t d = 0; d < dim; d++) {
        if (minmax[d] < 0) {
            all_maximize = false;
        } else if (minmax[d] > 0) {
            all_minimize = false;
        } else {
            all_minimize = false;
            all_maximize = false;
            break;
        }
    }
    assert(!all_maximize || !all_minimize);
    if (all_minimize)
        return epsilon_additive_minimize((dimension_t) dim, points_a, (size_t) size_a, points_b, (size_t) size_b);
    if (all_maximize)
        return epsilon_additive_maximize((dimension_t) dim, points_a, (size_t) size_a, points_b, (size_t) size_b);

    int a, b;
    int d;
    double epsilon = -INFINITY;
    for (b = 0; b < size_b; b++) {
        bool skip_max = false;
        double epsilon_min = INFINITY;
        for (a = 0; a < size_a; a++) {
            double epsilon_max = (minmax[0] < 0)
                ? points_a[a * dim + 0] - points_b[b * dim + 0]
                : (minmax[0] > 0)
                ? points_b[b * dim + 0] - points_a[a * dim + 0]
                : 0;
            if (epsilon_max >= epsilon_min)
                continue;
            for (d = 1; d < dim; d++) {
                double epsilon_temp = points_a[a * dim + d] - points_b[b * dim + d];
                if (minmax[d] > 0)
                    epsilon_temp = -epsilon_temp;
                else if (minmax[d] == 0)
                    epsilon_temp = 0;
                epsilon_max = MAX (epsilon_max, epsilon_temp);
            }
            if (epsilon_max <= epsilon) {
                skip_max = true;
                break;
            }
            epsilon_min = MIN (epsilon_min, epsilon_max);
        }
        if (skip_max) continue;
        epsilon = MAX(epsilon, epsilon_min);
    }
    return epsilon;
}

_attr_maybe_unused static double
epsilon_additive (const double * restrict data, int nobj, int npoints,
                  const double * restrict ref, int ref_size,
                  const bool * restrict maximise)
{
    ASSUME(nobj >= 2);
    ASSUME(npoints >= 0);
    ASSUME(ref_size >= 0);

    const signed char *minmax = minmax_from_bool(nobj, maximise);
    double value = epsilon_additive_minmax (nobj, minmax, data, npoints, ref, ref_size);
    free ((void *)minmax);
    return(value);
}
_attr_maybe_unused static double
epsilon_mult (const double * restrict data, int nobj, int npoints,
              const double * restrict ref, int ref_size,
              const bool * restrict maximise)
{
    ASSUME(nobj >= 2);
    ASSUME(npoints >= 0);
    ASSUME(ref_size >= 0);

    const signed char *minmax = minmax_from_bool(nobj, maximise);
    double value = epsilon_mult_minmax (nobj, minmax, data, npoints, ref, ref_size);
    free ((void *)minmax);
    return(value);
}

/* FIXME: this can be done much faster. For example, the diff needs to
   be calculated just once and stored on a temporary array diff[].  */
static inline int
epsilon_additive_ind (int dim, const signed char * restrict minmax,
                      const double * restrict points_a, int size_a,
                      const double * restrict points_b, int size_b)
{
    double eps_ab = epsilon_additive_minmax(
        dim, minmax, points_a, size_a, points_b, size_b);
    double eps_ba = epsilon_additive_minmax(
        dim, minmax, points_b, size_b, points_a, size_a);

    DEBUG2(printf ("eps_ab = %g, eps_ba = %g\n", eps_ab, eps_ba));

    if (eps_ab <= 0 && eps_ba > 0)
        return -1;
    else if (eps_ab > 0 && eps_ba <= 0)
        return 1;
    else
        return 0;
}

#endif /* EPSILON_H */
