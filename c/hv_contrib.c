#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include "common.h"
#include "hv.h"
#include "nondominated.h"
#include "sort.h"

static inline double
hvc_1point_diff(const double * points, dimension_t dim, size_t size,
                const double * ref, const double hv_total)
{
    const double tolerance = sqrt(DBL_EPSILON);
    double hvc = hv_total - fpli_hv(points, dim, (int) size - 1, ref);
    // Handle very small values.
    hvc = (hvc >= tolerance) ? hvc : 0.0;
    return hvc;
}

/* Given a list of points, compute the exclusive hypervolume contribution of
   point using the naive algorithm. That is, it removes one point at a time and
   calculates hv_total - hv_i, where hv_i is the hypervolume of the set minus
   the point i.

   With hv_total=0, it computes the negated hypervolume of each subset minus
   one point.
*/
static void
hvc_1point_diffs(double *hvc, double *points, dimension_t dim, size_t size,
                 const double * ref, const bool * uev, const double hv_total)
{
    ASSUME(size > 1);
    bool keep_uevs = uev != NULL;
    double * tmp = MOOCORE_MALLOC(dim, double);
    const bool * maximise = new_bool_maximise(dim, /*maximise_all=*/false);
    const bool * nondom = is_nondominated(points, dim, size, maximise,
                                          /*keep_weakly=*/false);
    free((void *) maximise);
    const double * const last = points + (size - 1) * dim;
    for (size_t i = 0; i < size - 1; i++) {
        if (unlikely(keep_uevs && uev[i])) {
            hvc[i] = hv_total;
        } else if (nondom[i] && strongly_dominates(points + i * dim, ref, dim)) {
            memcpy(tmp, points + i * dim, sizeof(double) * dim);
            memcpy(points + i * dim, last, sizeof(double) * dim);
            hvc[i] = hvc_1point_diff(points, dim, size, ref, hv_total);
            memcpy(points + i * dim, tmp, sizeof(double) * dim);
        }
    }
    free(tmp);
    // Process the last point.
    if (unlikely(keep_uevs && uev[size - 1])) {
        hvc[size - 1] = hv_total;
    } else if (nondom[size - 1] && strongly_dominates(last, ref, dim)) {
        hvc[size - 1] = hvc_1point_diff(points, dim, size, ref, hv_total);
    }
    free((void *)nondom);
}

/* O(n log n) dimension-sweep algorithm.

   hvc[] must be already allocated with size n.  Points that are duplicated
   have zero exclusive contribution.  Thus, the sum of contributions may
   increase if one removes any duplicates.

   Returns -1 if something fails, >= 0 on success.
*/
static double
hvc2d(double * restrict hvc, const double * restrict data, size_t n,
      const double * restrict ref)
{
    ASSUME(n > 0);
    const double **p = generate_sorted_doublep_2d(data, &n, ref[0]);
    if (unlikely(n == 0)) return 0;
    if (unlikely(!p)) return -1;

    size_t j = 0;
    // Find first point below the reference point.
    while (j < n && p[j][1] >= ref[1])
        j++;
    if (unlikely(j == n)) {
        free(p);
        return 0;
    }
    double height = ref[1] - p[j][1];
    double hyperv = (ref[0] - p[j][0]) * height;
    const double * prev = p[j];
    while (j + 1 < n) {
        j++;
        DEBUG2_PRINT("[%lld]=(%g, %g) -> [%lld]=(%g,%g) (height=%g)\n",
                     (long long)(prev - data) / 2, prev[0], prev[1],
                     (long long)(p[j] - data) / 2, p[j][0], p[j][1], height);
        // likely() because most points will be non-dominated.
        if (likely(prev[1] > p[j][1])) {
            assert(prev[0] < p[j][0]);
            /* Compute the contribution of prev.  We have to accumulate because
               we may have computed partial contributions.  */
            hvc[(prev - data) / 2] += (p[j][0] - prev[0]) * height;
            DEBUG2_PRINT("hvc[%lld] += %g * %g = %g\n",
                         (long long) (prev - data) / 2,
                         p[j][0] - prev[0], height,
                         (p[j][0] - prev[0]) * height);
            height = prev[1] - p[j][1];
            // Compute the hypervolume of p[j]
            hyperv += (ref[0] - p[j][0]) * height;
            prev = p[j];
        } else if (prev[0] < p[j][0]) {
            // If p[j][1] >= prev[1], this contributes partially to hvc[prev].
            double new_h = p[j][1] - prev[1];
            if (new_h < height) {
                hvc[(prev - data) / 2] += (p[j][0] - prev[0]) * (height - new_h);
                DEBUG2_PRINT("hvc[%lld] += %g * %g = %g\n",
                             (long long) (prev - data) / 2,
                             p[j][0] - prev[0], height - new_h,
                             (p[j][0] - prev[0]) * (height - new_h));
                height = new_h;
            }
        } else if (prev[1] == p[j][1]) {
            // Duplicates contribute zero.
            DEBUG2_PRINT("hvc[%lld] = %g\n", (long long)(prev - data) / 2,
                         hvc[(prev - data) / 2]);
            assert(hvc[(prev - data) / 2] == 0);
            /* We set this here so that we set hvc[j] = 0 when we find the
               next non-duplicate.  */
            height = 0;
            prev = p[j];
        } else {
            /* height == 0 means that the prev was a duplicate or
               dominated, so it doesn't contribute.  */
            height = MIN(height, p[j][1] - prev[1]);
            /* All points with same 0-coordinate are strictly above
               prev, so they can be ignored.  */
            do {
                assert(prev[1] < p[j][1]);
                j++;
            } while (j < n && prev[0] == p[j][0]);
            if (j < n)
                j--; // p[j] is not a duplicate. We need to process it above.
            else
                break; // All points ignored
        }
    }

    hvc[(prev - data) / 2] += (ref[0] - prev[0]) * height;
    DEBUG2_PRINT("hvc[%lld] = %g * %g = %g\n", (long long)(prev - data) / 2,
                 ref[0] - prev[0], height,  (ref[0] - prev[0]) * height);
    free(p);
    return hyperv;
}

/* This function is only called if DEBUG>=1, but MSVC is not smart enough to
   remove the call when DEBUG==0, so libutil.c is still required to get a
   definition of fatal_error().  */
static inline void
hvc_check(double hv_total, const double * restrict hvc,
          double * restrict points,
          dimension_t dim, size_t size, const double * restrict ref)
{
    const double tolerance = sqrt(DBL_EPSILON);
    double hv_total_tmp = fpli_hv(points, dim, (int) size, ref);
    if (fabs(hv_total_tmp - hv_total) > tolerance) {
        fatal_error("hv_total = %g != hv_total_tmp = %g !", hv_total, hv_total_tmp);
    }
    double * hvc_tmp = MOOCORE_MALLOC(size, double);
    hvc_1point_diffs(hvc_tmp, points, dim, size, ref, NULL, hv_total);
    for (size_t i = 0; i < size; i++) {
        if (fabs(hvc[i] - hvc_tmp[i]) > tolerance) {
            fatal_error("hvc[%zu] = %g != hvc_tmp[%zu] = %g !", i, hvc[i], i, hvc_tmp[i]);
        }
    }
    free (hvc_tmp);
}

/* Store the exclusive hypervolume contribution of each input point in hvc[],
   which is allocated by the caller.

   Return the total hypervolume. A negative value indicates insufficient
   memory. A value of zero indicates that no input point strictly dominates the
   reference point.
*/
double
hv_contributions(double * restrict hvc, double * restrict points, int d, int n,
                 const double * restrict ref)
{
    assert(hvc != NULL);
    ASSUME(d > 1 && d <= 32);
    ASSUME(n >= 0);
    dimension_t dim = (dimension_t) d;
    size_t size = (size_t) n;
    if (n == 0) return 0;
    if (n == 1) {
        hvc[0] = fpli_hv(points, dim, (int) size, ref);
        return hvc[0];
    }
    /* We cannot rely on the caller and the functions below will skip points
       that do not dominate the reference point.  */
    memset(hvc, 0, n * sizeof(double));

    double hv_total;
    if (dim == 2) {
        hv_total = hvc2d(hvc, points, size, ref);
        DEBUG1(hvc_check(hv_total, hvc, points, dim, size, ref));
    } else {
        hv_total = fpli_hv(points, dim, (int) size, ref);
        hvc_1point_diffs(hvc, points, dim, size, ref, NULL, hv_total);
    }
    return hv_total;
}
