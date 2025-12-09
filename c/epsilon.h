#ifndef EPSILON_H
#define EPSILON_H

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#ifndef INFINITY
#define INFINITY (HUGE_VAL)
#endif
#include "common.h"
#include "nondominated.h" // minmax_from_bool

static inline bool
all_positive(const double * restrict points, size_t size, dimension_t dim)
{
    ASSUME(size > 0);
    ASSUME(1 <= dim && dim <= 32);
    const size_t len = size * dim;
    for (size_t a = 0; a < len; a++)
        if (points[a] <= 0)
            return false;

    return true;
}
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


_attr_optimize_finite_math
static inline double
epsilon_helper_(bool do_mult, const enum objs_agree_t agree,
                const int * restrict minmax, dimension_t dim,
                const double * restrict points_a, size_t size_a,
                const double * restrict points_b, size_t size_b)
{
    ASSUME(2 <= dim && dim <= 32);
    ASSUME(size_a > 0 && size_b > 0);
    ASSUME(agree == AGREE_MINIMISE || agree == AGREE_MAXIMISE || agree == AGREE_NONE);
    assert((agree == AGREE_NONE) == (minmax != NULL));

// Converting these macros to an inline function hinders vectorization.
#define eps_value_(X, Y)   (do_mult ? ((X) / (Y)) : ((X) - (Y)))
#define eps_value_minmax_(M, X, Y)                                             \
    ((M < 0) ? eps_value_(X,Y) : ((M > 0) ? eps_value_(Y,X) : 0))
#define eps_value_agree_(DIM)                                                  \
    (minmax ? eps_value_minmax_(minmax[DIM], pa[DIM], pb[DIM])                 \
     : ((agree == AGREE_MINIMISE)                                              \
        ? eps_value_(pa[DIM], pb[DIM]) : eps_value_(pb[DIM], pa[DIM])))

    double epsilon = do_mult ? 0 : -INFINITY;
    for (size_t b = 0; b < size_b; b++) {
        double epsilon_min = INFINITY;
        const double * restrict pb = points_b + b * dim;
        for (size_t a = 0; a < size_a; a++) {
            const double * restrict pa = points_a + a * dim;
            double epsilon_max = MAX(eps_value_agree_(0), eps_value_agree_(1));

            if (epsilon_max >= epsilon_min)
                continue;

            for (dimension_t d = 2; d < dim; d++) {
                double epsilon_temp = eps_value_agree_(d);
                epsilon_max = MAX(epsilon_max, epsilon_temp);
            }

            if (epsilon_max <= epsilon)
                goto skip_max;
            epsilon_min = MIN(epsilon_min, epsilon_max);
        }
        epsilon = MAX(epsilon, epsilon_min);
    skip_max:
        (void)0;
    }
    return epsilon;
}

#undef eps_value_
#undef eps_value_minmax_
#undef eps_value_agree_

_attr_optimize_finite_math
static inline double
epsilon_mult_agree_none(const int * restrict minmax, dimension_t dim,
                        const double * restrict points_a, size_t size_a,
                        const double * restrict points_b, size_t size_b)
{
    return epsilon_helper_(/* do_mult=*/true, AGREE_NONE, minmax, dim, points_a, size_a, points_b, size_b);
}

_attr_optimize_finite_math
static inline double
epsilon_mult_agree_min(dimension_t dim,
                       const double * restrict points_a, size_t size_a,
                       const double * restrict points_b, size_t size_b)
{
    return epsilon_helper_(/* do_mult=*/true, AGREE_MINIMISE, /*minmax=*/NULL, dim, points_a, size_a, points_b, size_b);
}

_attr_optimize_finite_math
static inline double
epsilon_mult_agree_max(dimension_t dim,
                       const double * restrict points_a, size_t size_a,
                       const double * restrict points_b, size_t size_b)
{
    return epsilon_helper_(/* do_mult=*/true, AGREE_MAXIMISE, /*minmax=*/NULL, dim, points_a, size_a, points_b, size_b);
}


_attr_optimize_finite_math
static inline double
epsilon_addi_agree_none(const int * restrict minmax, dimension_t dim,
                        const double * restrict points_a, size_t size_a,
                        const double * restrict points_b, size_t size_b)
{
    return epsilon_helper_(/* do_mult=*/false, AGREE_NONE, minmax, dim, points_a, size_a, points_b, size_b);
}

_attr_optimize_finite_math
static inline double
epsilon_addi_agree_min(dimension_t dim,
                       const double * restrict points_a, size_t size_a,
                       const double * restrict points_b, size_t size_b)
{
    return epsilon_helper_(/* do_mult=*/false, AGREE_MINIMISE, /*minmax=*/NULL, dim, points_a, size_a, points_b, size_b);
}

_attr_optimize_finite_math
static inline double
epsilon_addi_agree_max(dimension_t dim,
                       const double * restrict points_a, size_t size_a,
                       const double * restrict points_b, size_t size_b)
{
    return epsilon_helper_(/* do_mult=*/false, AGREE_MAXIMISE, /*minmax=*/NULL, dim, points_a, size_a, points_b, size_b);
}


_attr_optimize_finite_math
static inline double
epsilon_mult_minmax(const int * restrict minmax, dimension_t dim,
                    const double * restrict points_a, size_t size_a,
                    const double * restrict points_b, size_t size_b)
{
#if DEBUG >= 1
    if (!all_positive(points_a, size_a, dim) || !all_positive(points_b, size_b, dim)) {
        errprintf("cannot calculate multiplicative epsilon indicator with values <= 0.");
        return INFINITY;
    }
#endif
    // This forces the compiler to generate three specialized versions of the function.
    switch (check_all_minimize_maximize(minmax, dim)) {
      case AGREE_MINIMISE:
          return epsilon_mult_agree_min(dim, points_a, size_a, points_b, size_b);
      case AGREE_MAXIMISE:
          return epsilon_mult_agree_max(dim, points_a, size_a, points_b, size_b);
      default:
          return epsilon_mult_agree_none(minmax, dim, points_a, size_a, points_b, size_b);
    }
}

_attr_optimize_finite_math
static inline double
epsilon_additive_minmax(const int * restrict minmax, dimension_t dim,
                        const double * restrict points_a, size_t size_a,
                        const double * restrict points_b, size_t size_b)
{
    // This forces the compiler to generate three specialized versions of the function.
    switch (check_all_minimize_maximize(minmax, dim)) {
      case AGREE_MINIMISE:
          return epsilon_addi_agree_min(dim, points_a, size_a, points_b, size_b);
      case AGREE_MAXIMISE:
          return epsilon_addi_agree_max(dim, points_a, size_a, points_b, size_b);
      default:
          return epsilon_addi_agree_none(minmax, dim, points_a, size_a, points_b, size_b);
    }
}

_attr_maybe_unused static double
epsilon_additive(const double * restrict data, size_t n, dimension_t dim,
                 const double * restrict ref, size_t ref_size,
                 const bool * restrict maximise)
{
    ASSUME(dim >= 2);
    const int * minmax = minmax_from_bool(maximise, dim);
    double value = epsilon_additive_minmax(minmax, dim, data, n, ref, ref_size);
    free ((void *)minmax);
    return value;
}

_attr_maybe_unused static double
epsilon_mult(const double * restrict data, size_t n, dimension_t dim,
             const double * restrict ref, size_t ref_size,
             const bool * restrict maximise)
{
    ASSUME(dim >= 2);
    const int * minmax = minmax_from_bool(maximise, dim);
    double value = epsilon_mult_minmax(minmax, dim, data, n, ref, ref_size);
    free ((void *)minmax);
    return value;
}

/* FIXME: this can be done much faster. For example, the diff needs to
   be calculated just once and stored on a temporary array diff[].  */
static inline int
epsilon_additive_ind(const int * restrict minmax, dimension_t dim,
                     const double * restrict points_a, size_t size_a,
                     const double * restrict points_b, size_t size_b)
{
    double eps_ab = epsilon_additive_minmax(
        minmax, dim, points_a, size_a, points_b, size_b);
    double eps_ba = epsilon_additive_minmax(
        minmax, dim, points_b, size_b, points_a, size_a);

    DEBUG2(printf ("eps_ab = %g, eps_ba = %g\n", eps_ab, eps_ba));

    if (eps_ab <= 0 && eps_ba > 0)
        return -1;
    else if (eps_ab > 0 && eps_ba <= 0)
        return 1;
    else
        return 0;
}

#endif /* EPSILON_H */
