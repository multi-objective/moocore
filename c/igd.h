#ifndef IGD_H
#define IGD_H

/******************************************************************************

 GD was first proposed in [1] with p=2. IGD seems to have been mentioned first
 in [2], however, some people also used the name D-metric for the same thing
 with p=1 and later papers have often used IGD/GD with p=1.  GD_p and IGD_p
 were proposed in [4] and they change how the numerator is computed. This has a
 significant effect for GD and less so for IGD given a constant reference
 set. IGD+ was proposed in [5] and changes how to compute the distances. In
 general, norm=2 (Euclidean distance), but other norms are possible [4]. See
 [6] for a comparison.

 [1] D. A. Van Veldhuizen and G. B. Lamont. Evolutionary Computation and
     Convergence to a Pareto Front. In J. R. Koza, editor, Late Breaking Papers
     at the Genetic Programming 1998 Conference, pages 221–228, Stanford
     University, California, July 1998. Stanford University Bookstore.
     Keywords: generational distance.

 [2] Coello Coello, C.A., Reyes-Sierra, M.: A study of the parallelization of a
     coevolutionary multi-objective evolutionary algorithm.  In: Monroy, R., et
     al. (eds.) Proceedings of MICAI, LNAI, vol. 2972, pp. 688–697. Springer,
     Heidelberg, Germany (2004).

 [3] Q. Zhang and H. Li. MOEA/D: A Multiobjective Evolutionary Algorithm Based
     on Decomposition. IEEE Transactions on Evolutionary Computation,
     11(6):712–731, 2007. doi:10.1109/TEVC.2007.892759.

 [4] Schutze, O., Esquivel, X., Lara, A., Coello Coello, C.A.: Using the
     averaged Hausdorff distance as a performance measure in evolutionary
     multiobjective optimization. IEEE Trans. Evol. Comput. 16(4), 504–522 (2012)

 [5] H. Ishibuchi, H. Masuda, Y. Tanigaki, and Y. Nojima.  Modified Distance
     Calculation in Generational Distance and Inverted Generational Distance.
     In A. Gaspar-Cunha, C. H. Antunes, and C. A. Coello Coello, editors,
     Evolutionary Multi-criterion Optimization, EMO 2015 Part I, volume 9018 of
     Lecture Notes in Computer Science, pages 110–125. Springer, Heidelberg,
     Germany, 2015.

 [6] Leonardo C. T. Bezerra, Manuel López-Ibáñez, and Thomas Stützle. An
     empirical assessment of the properties of inverted generational distance
     indicators on multi- and many-objective optimization. In H. Trautmann,
     G. Rudolph, K. Klamroth, O. Schütze, M. M. Wiecek, Y. Jin, and C. Grimme,
     editors, Evolutionary Multi-criterion Optimization, EMO 2017, Lecture
     Notes in Computer Science, pages 31–45. Springer, 2017.

******************************************************************************/

#include <float.h>
#include <math.h>
#include "common.h"
#include "pow_int.h"
#include "nondominated.h" // minmax_from_bool

#ifndef INFINITY
#define INFINITY (HUGE_VAL)
#endif

_attr_optimize_finite_math
static inline double
gd_common_helper_(const enum objs_agree_t agree,
                  const int * restrict minmax, dimension_t dim,
                  const double * restrict points_a, size_t size_a,
                  const double * restrict points_r, size_t size_r,
                  bool plus, bool psize, uint_fast8_t p)
{
    ASSUME(2 <= dim && dim <= MOOCORE_DIMENSION_MAX);
    if (size_a == 0) return INFINITY;
    ASSUME(size_a > 0);
    ASSUME(size_r > 0);
    ASSUME(agree == AGREE_MINIMISE || agree == AGREE_MAXIMISE || agree == AGREE_NONE);
    assert((agree == AGREE_NONE) == (minmax != NULL));

    double gd = 0;
    for (size_t a = 0; a < size_a; a++) {
        double min_dist = INFINITY;
        const double * restrict pa = points_a + a * dim;
        for (size_t r = 0; r < size_r; r++) {
            double diff[MOOCORE_DIMENSION_MAX + 1];
            const double * restrict pr = points_r + r * dim;
            // TODO: Implement taxicab and infinity norms
            for (dimension_t d = 0; d < dim; d++) {
                double a_d = pa[d];
                double r_d = pr[d];
                diff[d] = minmax
                    ? (plus
                       ? ((minmax[d] < 0) ? MAX(r_d - a_d, 0.) : (minmax[d] > 0 ? MAX(a_d - r_d, 0.) : 0))
                       : (likely(minmax[d] != 0) ? (a_d - r_d) : 0))
                    : (plus
                       ? ((agree == AGREE_MINIMISE) ? MAX(r_d - a_d, 0.) : MAX(a_d - r_d, 0.))
                       : (a_d - r_d));
            }
            double dist = 0.0;
            for (dimension_t d = 0; d < dim; d++)
                dist += diff[d] * diff[d];

            if (unlikely(dist == 0))
                goto zero_skip;
            // We calculate the sqrt() of the Euclidean outside the loop, which
            // is faster and does not change the minimum.
            min_dist = MIN(min_dist, dist);
        }
        ASSUME(min_dist > 0);
        // Here we calculate the actual Euclidean distance.
        if (p == 1)
            min_dist = sqrt(min_dist);
        else if (p % 2 == 0)
            min_dist = pow_uint(min_dist, p/2);
        else
            min_dist = pow_uint(sqrt(min_dist), p);
        gd += min_dist;
    zero_skip:
        (void)0;
    }
    ASSUME(gd >= 0);

    if (p == 1)
        return gd / (double) size_a;
    else if (psize)
        return (double) powl(gd / (double) size_a, 1.0 / p);
    else
        return (double) powl(gd, 1.0 / p) / (double) size_a;
}

_attr_optimize_finite_math
static inline double
gd_common_agree_none(const int * restrict minmax, dimension_t dim,
                    const double * restrict points_a, size_t size_a,
                    const double * restrict points_r, size_t size_r,
                    bool plus, bool psize, uint_fast8_t p)
{
    return gd_common_helper_(AGREE_NONE, minmax, dim, points_a, size_a, points_r, size_r, plus, psize, p);
}

_attr_optimize_finite_math
static inline double
gd_common_agree_min(dimension_t dim,
                    const double * restrict points_a, size_t size_a,
                    const double * restrict points_r, size_t size_r,
                    bool plus, bool psize, uint_fast8_t p)
{
    return gd_common_helper_(AGREE_MINIMISE, /*minmax=*/NULL, dim, points_a, size_a, points_r, size_r, plus, psize, p);
}

_attr_optimize_finite_math
static inline double
gd_common_agree_max(dimension_t dim,
                    const double * restrict points_a, size_t size_a,
                    const double * restrict points_r, size_t size_r,
                    bool plus, bool psize, uint_fast8_t p)
{
    return gd_common_helper_(AGREE_MAXIMISE, /*minmax=*/NULL, dim, points_a, size_a, points_r, size_r, plus, psize, p);
}

_attr_optimize_finite_math
static inline double
gd_common(const int * restrict minmax, dimension_t dim,
          const double * restrict points_a, size_t size_a,
          const double * restrict points_r, size_t size_r,
          bool plus, bool psize, uint_fast8_t p)
{
    // This forces the compiler to generate three specialized versions of the function.
    switch (check_all_minimize_maximize(minmax, dim)) {
      case AGREE_MINIMISE:
          return gd_common_agree_min(dim, points_a, size_a, points_r, size_r, plus, psize, p);
      case AGREE_MAXIMISE:
          return gd_common_agree_max(dim, points_a, size_a, points_r, size_r, plus, psize, p);
      default:
          return gd_common_agree_none(minmax, dim, points_a, size_a, points_r, size_r, plus, psize, p);
    }
}

_attr_optimize_finite_math
static inline double
GD_minmax(const int * restrict minmax, dimension_t dim,
          const double * restrict points_a, size_t size_a,
          const double * restrict points_r, size_t size_r)
{
    return gd_common(minmax, dim,
                     points_a, size_a,
                     points_r, size_r,
                     /*plus=*/false, /*psize=*/false, /*p=*/1);
}

_attr_optimize_finite_math
static inline double
IGD_minmax(const int * restrict minmax, dimension_t dim,
           const double * restrict points_a, size_t size_a,
           const double * restrict points_r, size_t size_r)
{
    return gd_common(minmax, dim,
                     points_r, size_r,
                     points_a, size_a,
                     /*plus=*/false, /*psize=*/false, /*p=*/1);
}

_attr_maybe_unused static double
IGD(const double * restrict data, size_t npoints, dimension_t nobj,
    const double * restrict ref, size_t ref_size,
    const bool *  restrict maximise)
{
    const int * minmax = minmax_from_bool(maximise, nobj);
    double value = IGD_minmax(minmax, nobj, data, npoints, ref, ref_size);
    free ((void *)minmax);
    return value;
}

_attr_optimize_finite_math
static inline double
GD_p(const int * restrict minmax, dimension_t dim,
     const double * restrict points_a, size_t size_a,
     const double * restrict points_r, size_t size_r, unsigned int p)
{
    return gd_common(minmax, dim,
                     points_a, size_a,
                     points_r, size_r,
                     /*plus=*/false, /*psize=*/true, (uint_fast8_t)p);
}

_attr_optimize_finite_math
static inline double
IGD_p(const int * restrict minmax, dimension_t dim,
      const double * restrict points_a, size_t size_a,
      const double * restrict points_r, size_t size_r, unsigned int p)
{
    return gd_common(minmax, dim,
                     points_r, size_r,
                     points_a, size_a,
                     /*plus=*/false, /*psize=*/true, (uint_fast8_t) p);
}

_attr_optimize_finite_math
static inline double
IGD_plus_minmax(const int * restrict minmax, dimension_t dim,
                const double * restrict points_a, size_t size_a,
                const double * restrict points_r, size_t size_r)
{
    return gd_common(minmax, dim,
                     points_r, size_r,
                     points_a, size_a,
                     /*plus=*/true, /*psize=*/true, /*p=*/1);
}

_attr_maybe_unused static double
IGD_plus(const double * restrict data, size_t npoints, dimension_t nobj,
         const double * restrict ref, int ref_size,
         const bool * restrict  maximise)
{
    ASSUME(nobj > 0);
    const int * minmax = minmax_from_bool(maximise, nobj);
    double value = IGD_plus_minmax(minmax, nobj, data, npoints, ref, ref_size);
    free ((void *)minmax);
    return value;
}

_attr_optimize_finite_math
static inline double
avg_Hausdorff_dist_minmax(const int * restrict minmax, dimension_t dim,
                          const double * restrict points_a, size_t size_a,
                          const double * restrict points_r, size_t size_r,
                          unsigned int p)
{
    double gd_p = gd_common(minmax, dim,
                            points_a, size_a,
                            points_r, size_r,
                            /*plus=*/false, /*psize=*/true, (uint_fast8_t)p);

    double igd_p = gd_common(minmax, dim,
                             points_r, size_r,
                             points_a, size_a,
                             /*plus=*/false, /*psize=*/true, (uint_fast8_t)p);
    return MAX(gd_p, igd_p);
}
/* TODO: Implement p=INFINITY See [4] */

_attr_maybe_unused static double
avg_Hausdorff_dist(const double * restrict data, size_t npoints, dimension_t nobj,
                   const double * restrict ref, size_t ref_size,
                   const bool * restrict maximise, unsigned int p)
{
    ASSUME(nobj > 0);
    const int * minmax = minmax_from_bool(maximise, nobj);
    double value = avg_Hausdorff_dist_minmax(minmax, nobj, data, npoints, ref, ref_size, p);
    free ((void *)minmax);
    return value;
}

#endif /* IGD_H */
