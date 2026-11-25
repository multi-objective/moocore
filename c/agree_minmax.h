#ifndef MINMAX_H
#define MINMAX_H
/****************************************************************************

 Support for mixed minimization/maximization of objectives.

 FIXME: The current approach is far from ideal!

 - Most of the code assumes minimization, so AGREE_MAXIMISE requires (typically
   but not always) to multiply by -1. However, AGREE_MAXIMISE=1, which is not
   what we need.
 - The ability to ignore some columns is not consistently implemented and it is not accessible from R/Python.
   It is probably better to remove the columns explicitly before doing any computations
   and not handle ignored columns in each individual function.
 - The agree argument is (always?) redundant. It may be faster to specialize for
   minimization and convert all inputs to minimization. See force_agree_minimize().
 - If we first remove ignored columns, then flipping sign-bits can be done
   very fast using (minmax[i]=-1.0 for maximization, 1.0 otherwise):

   static inline void
   apply_minmax(double * restrict dst, const double * restrict src,
                dimension_t dim, const double * restrict minmax)
   {
       // GCC generates vectorized AND+XOR without multiplication.
       for (dimension_t d = 0; d < dim; d++) {
           dst[d] = src[d] * copysign(1.0, minmax[i]);
       }
   }

****************************************************************************/
#include "config.h"

enum objs_agree_t { AGREE_MINIMISE = -1, AGREE_NONE = 0, AGREE_MAXIMISE = 1 };

static inline enum objs_agree_t
check_all_minimize_maximize(const int * restrict minmax, dimension_t dim)
{
    bool all_minimize = (minmax[0] < 0);
    bool all_maximize = (minmax[0] > 0);
    assert(!all_maximize || !all_minimize);
    if (all_minimize) {
        for (dimension_t d = 1; d < dim; d++) {
            if (minmax[d] >= 0)
                return AGREE_NONE;
        }
        return AGREE_MINIMISE;
    }
    if (all_maximize) {
        for (dimension_t d = 1; d < dim; d++) {
            if (minmax[d] <= 0)
                return AGREE_NONE;
        }
        return AGREE_MAXIMISE;
    }
    return AGREE_NONE;
}

/** Convert from bool vector to minmax vector.

    minmax needs to be an int (int64 in 64 bits) so that GCC can vectorize
    loops that are conditional on minmax[d].  Using 'signed char' does not work
    in some targets, e.g, -march=tigerlake.
*/
static inline int *
minmax_from_boolvec(const boolvec * restrict maximise, dimension_t nobj)
{
    ASSUME(nobj > 0);
    int * minmax = malloc(nobj * sizeof(*minmax));
    for (dimension_t k = 0; k < nobj; k++) {
        minmax[k] = maximise[k] ? AGREE_MAXIMISE : AGREE_MINIMISE;
    }
    return minmax;
}

static inline boolvec *
new_boolvec_maximise(dimension_t nobj, bool maximise_all)
{
    ASSUME(nobj > 0);
    boolvec * maximise = malloc(nobj * sizeof(*maximise));
    for (dimension_t k = 0; k < nobj; k++)
        maximise[k] = maximise_all;
    return maximise;
}

static inline const int *
default_minmax(dimension_t nobj, int default_value)
{
    ASSUME(nobj > 0);
    ASSUME(default_value == AGREE_MINIMISE || default_value == AGREE_MAXIMISE);
    int * minmax = malloc(nobj * sizeof(*minmax));
    for (dimension_t i = 0; i < nobj; i++)
        minmax[i] = default_value;
    return minmax;
}

static inline const int *
minmax_minimise(dimension_t nobj)
{
    return default_minmax(nobj, AGREE_MINIMISE);
}

static inline const int *
minmax_maximise(dimension_t nobj)
{
    return default_minmax(nobj, AGREE_MAXIMISE);
}

static inline bool
minmax_alloc(const int * restrict * minmax, bool maximise_all_flag,
             dimension_t d)
{
    if (*minmax == NULL) {
        *minmax = maximise_all_flag ? minmax_maximise(d) : minmax_minimise(d);
        return true;
    }
    return false;
}


/**
   Maybe create a copy of points (with lower dimensionality) depending on the
   value of minmax.
*/
static inline const double *
force_agree_minimize(const double * restrict points, size_t size,
                     dimension_t * restrict dim_p,
                     _attr_maybe_unused enum objs_agree_t agree,
                     const int * restrict minmax)
{
    assert(agree != AGREE_MINIMISE);
    bool copy = false;
    dimension_t true_dim = *dim_p;
    for (dimension_t d = 0; d < true_dim; d++) {
        if (minmax[d] >= 0) {
            copy = true;
            break;
        }
    }
    if (!copy)
        return points;

    dimension_t prefix = 0;
    while (prefix < true_dim && (minmax[prefix] != 0))
        prefix++;

    dimension_t new_dim = prefix;
    for (dimension_t d = prefix + 1; d < true_dim; d++)
        new_dim = (dimension_t) (new_dim + (minmax[d] != 0));

    assert(new_dim > 1);
    double * pnew = malloc(new_dim * size * sizeof(*pnew));

    if (new_dim < true_dim) { // Remove columns
        uint8_t idx[MOOCORE_DIMENSION_MAX + 1];
        for (dimension_t j = prefix, t = 0; j < true_dim; j++)
            if (minmax[j] != 0)
                idx[t++] = (uint8_t)j;

        size_t tail_active = new_dim - prefix;
        for (size_t i = 0; i < size; i++) {
            const double * restrict src = points + i * true_dim;
            double * restrict dst = pnew + i * new_dim;
            memcpy(dst, src, prefix * sizeof(*dst));
            for (dimension_t t = 0; t < tail_active; t++) {
                uint8_t j = idx[t];
                dst[prefix+t] = src[j];
            }
        }
        for (dimension_t t = 0, d = 0; d < true_dim; d++) {
            if (minmax[d] == 0) continue;
            if (minmax[d] > 0) {
                for (size_t k = 0; k < size; k++)
                    pnew[k * new_dim + t] = -pnew[k * new_dim + t];
            }
            t++;
        }
        *dim_p = new_dim;
    } else {
        memcpy(pnew, points, true_dim * size * sizeof(*pnew));
        for (dimension_t d = 0; d < true_dim; d++) {
            assert(minmax[d] != 0);
            if (minmax[d] > 0)
                for (size_t k = 0; k < size; k++)
                    pnew[k * true_dim + d] = -pnew[k * true_dim + d];
        }
    }
    return pnew;
}


#endif // MINMAX_H
