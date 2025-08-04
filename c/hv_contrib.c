#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include "common.h"
#include "hv.h"
#include "nondominated.h"
#include "sort.h"

static inline double
hvc_1point_diff(const double * restrict points, dimension_t dim, size_t size,
                const double * restrict ref, const double hv_total)
{
    const double tolerance = sqrt(DBL_EPSILON);
    double hvc = hv_total - fpli_hv(points, dim, (int) size, ref);
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
hvc_1point_diffs(double * restrict hvc, double * restrict points, dimension_t dim, size_t size,
                 const double * restrict ref, const bool * uev, const double hv_total)
{
    ASSUME(size > 1);
    bool keep_uevs = uev != NULL;
    const bool * nondom = is_nondominated_minimise(points, dim, size,
                                                   /*keep_weakly=*/false);
    const double * const last = points + (size - 1) * dim;
    double * tmp = MOOCORE_MALLOC(dim, double);
    for (size_t i = 0; i < size - 1; i++) {
        if (unlikely(keep_uevs && uev[i])) {
            hvc[i] = hv_total;
        } else if (nondom[i] && strongly_dominates(points + i * dim, ref, dim)) {
            memcpy(tmp, points + i * dim, sizeof(double) * dim);
            memcpy(points + i * dim, last, sizeof(double) * dim);
            hvc[i] = hvc_1point_diff(points, dim, size - 1, ref, hv_total);
            memcpy(points + i * dim, tmp, sizeof(double) * dim);
        }
    }
    free(tmp);
    // Process the last point.
    if (unlikely(keep_uevs && uev[size - 1])) {
        hvc[size - 1] = hv_total;
    } else if (nondom[size - 1] && strongly_dominates(last, ref, dim)) {
        hvc[size - 1] = hvc_1point_diff(points, dim, size - 1, ref, hv_total);
    }
    free((void *)nondom);
}

/* Same as hvc_1point_diffs() but points that are dominated are ignored, i.e.,
   they do not influence the HVC of other points, except for duplicated points,
   which are still given assigned an HVC value of zero.
*/
static void
hvc_1point_diffs_nondom(double * restrict hvc, double * restrict points,
                        dimension_t dim, size_t size,
                        const double * restrict ref, const bool * uev,
                        const double hv_total)
{
    ASSUME(size > 1);
#define swap_points(A,B) do {                                                  \
        memcpy(tmp_point, points + (A) * dim, sizeof(double) * dim);           \
        memcpy(points + (A) * dim, points + (B) * dim, sizeof(double) * dim);  \
        memcpy(points + (B) * dim, tmp_point, sizeof(double) * dim);           \
    } while(0)

    bool keep_uevs = uev != NULL;
    bool * nondom = nondom_init(size);
    // Duplicated points will still contribute zero.
    size_t new_size = find_weak_nondominated_set_minimise(points, dim, size, nondom);
    size_t first = 0;
    double * tmp_point = MOOCORE_MALLOC(dim, double);
    if (new_size < size) {
        // Move all dominated points beyond new_size.
        while (nondom[first]) first++; // Find first dominated.
        size_t k = first;
        size_t last = size;
        while (k < new_size) {
            // There is a dominated point before new_size.
            do {
                last--;
            } while (!nondom[last]); // Find next nondominated.
            assert(!nondom[k]);
            assert(nondom[last]);
            swap_points(k, last);
            // Find next dominated.
            do {
                k++;
            } while (k < new_size && nondom[k]);
        }
    }
    const double * const last_point = points + (new_size - 1) * dim;
    for (size_t i = 0; i < new_size; i++) {
        if (unlikely(keep_uevs && uev[i])) {
            hvc[i] = hv_total;
        } else if (strongly_dominates(points + i * dim, ref, dim)) {
            memcpy(tmp_point, points + i * dim, sizeof(double) * dim);
            memcpy(points + i * dim, last_point, sizeof(double) * dim);
            hvc[i] = hvc_1point_diff(points, dim, new_size - 1, ref, hv_total);
            memcpy(points + i * dim, tmp_point, sizeof(double) * dim);
        }
    }
    if (new_size < size) {
        // Swap the dominated points and hvc back to their original position.
        size_t k = first;
        size_t last = size;
        while (k < new_size) {
            // There was a dominated point before new_size.
            do {
                last--;
            } while (!nondom[last]); // Find next nondominated.
            assert(!nondom[k]);
            assert(nondom[last]);
            swap_points(k, last);
            hvc[last] = hvc[k];
            hvc[k] = 0.0; // k was dominated so contributes zero.
            // Find next dominated.
            do {
                k++;
            } while (k < new_size && nondom[k]);
        }
    }
    free(tmp_point);
    free((void *)nondom);
#undef swap_points
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
    j++;
    while (j < n) {
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
            j++;
        } else if (prev[0] < p[j][0]) {
            // prev[1] <= p[j][1], thus pj contributes partially to hvc[prev].
            double new_h = p[j][1] - prev[1];
            if (new_h < height) {
                hvc[(prev - data) / 2] += (p[j][0] - prev[0]) * (height - new_h);
                DEBUG2_PRINT("hvc[%lld] += %g * %g = %g\n",
                             (long long) (prev - data) / 2,
                             p[j][0] - prev[0], height - new_h,
                             (p[j][0] - prev[0]) * (height - new_h));
                height = new_h;
            }
            j++;
        } else if (prev[1] == p[j][1]) { // && prev[0] == p[j][0]
            // Duplicates contribute zero.
            DEBUG2_PRINT("hvc[%lld] = %g\n", (long long)(prev - data) / 2,
                         hvc[(prev - data) / 2]);
            assert(hvc[(prev - data) / 2] == 0);
            /* We set height=0 here so that we set hvc[prev] = 0 when we find the
               next non-duplicate.  */
            height = 0;
            prev = p[j];
            // Everything above prev is weakly dominated by prev.
            do {
                j++;
            } while (j < n && prev[1] <= p[j][1]);
        } else { // prev[0] == p[j][0] && prev[1] < p[j][1]
            /* height == 0 means that the prev was a duplicate or
               dominated, so it doesn't contribute.  */
            height = MIN(height, p[j][1] - prev[1]);
            /* All points with same 0-coordinate are strictly above
               prev, so they can be ignored.  */
            do {
                assert(prev[1] < p[j][1]);
                j++;
            } while (j < n && prev[0] == p[j][0]);
        }
    }

    hvc[(prev - data) / 2] += (ref[0] - prev[0]) * height;
    DEBUG2_PRINT("hvc[%lld] = %g * %g = %g\n", (long long)(prev - data) / 2,
                 ref[0] - prev[0], height,  (ref[0] - prev[0]) * height);
    free(p);
    return hyperv;
}

static double
hvc2d_nondom(double * restrict hvc, const double * restrict data, size_t n,
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
    j++;
    while (j < n) {
        DEBUG2_PRINT("[%lld]=(%g, %g) -> [%lld]=(%g,%g) (height=%g)\n",
                     (long long)(prev - data) / 2, prev[0], prev[1],
                     (long long)(p[j] - data) / 2, p[j][0], p[j][1], height);
        // likely() because most points will be non-dominated.
        if (likely(prev[1] > p[j][1])) {
            assert(prev[0] < p[j][0]);
            /* Compute the contribution of prev.  */
            hvc[(prev - data) / 2] = (p[j][0] - prev[0]) * height;
            DEBUG2_PRINT("hvc[%lld] = %g * %g = %g\n",
                         (long long) (prev - data) / 2,
                         p[j][0] - prev[0], height,
                         (p[j][0] - prev[0]) * height);
            height = prev[1] - p[j][1];
            // Compute the hypervolume of p[j]
            hyperv += (ref[0] - p[j][0]) * height;
            prev = p[j];
            j++;
        } else if (prev[0] == p[j][0]) { // && prev[1] <= p[j][1]
            if (prev[1] == p[j][1]) {
                // Duplicates contribute zero.
                DEBUG2_PRINT("hvc[%lld] = %g\n", (long long)(prev - data) / 2,
                             hvc[(prev - data) / 2]);
                assert(hvc[(prev - data) / 2] == 0);
                /* We set this here so that we set hvc[prev] = 0 when we find the next
                   non-duplicate.  */
                height = 0;
                prev = p[j];
            }
            /* All points with same 0-coordinate are weakly dominated by
               prev, so they can be ignored.  */
            do {
                j++;
            } while (j < n && prev[0] == p[j][0]);
        } else { // prev[0] < p[j][0] && prev[1] <= p[j][1]
            // Skip everything that is dominated by prev.
            do {
                j++;
            } while (j < n && prev[1] <= p[j][1]);
        }
    }

    hvc[(prev - data) / 2] = (ref[0] - prev[0]) * height;
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
          dimension_t dim, size_t size, const double * restrict ref,
          bool ignore_dominated)
{
    const double tolerance = sqrt(DBL_EPSILON);
    double hv_total_true = fpli_hv(points, dim, (int) size, ref);
    if (fabs(hv_total_true - hv_total) > tolerance) {
        fatal_error("hv_total = %-22.15g != hv_total_true = %-22.15g !", hv_total, hv_total_true);
    }
    double * hvc_true = MOOCORE_MALLOC(size, double);
    /* The functions below will skip points that do not dominate the reference point.  */
    memset(hvc_true, 0, size * sizeof(double));

    if (ignore_dominated)
        hvc_1point_diffs_nondom(hvc_true, points, dim, size, ref, NULL, hv_total);
    else
        hvc_1point_diffs(hvc_true, points, dim, size, ref, NULL, hv_total);
    for (size_t i = 0; i < size; i++) {
        if (fabs(hvc[i] - hvc_true[i]) > tolerance) {
            fprintf(stderr, "%-22.15g", points[i * dim]);
            for (dimension_t d = 1; d < dim; d++) {
                fprintf(stderr, " %-22.15g", points[i * dim + d]);
            }
            fprintf(stderr, "\n");
            fatal_error("hvc[%zu] = %-22.15g != hvc_true[%zu] = %-22.15g !", i, hvc[i], i, hvc_true[i]);
        }
    }
    free(hvc_true);
}

extern double
hvc3d(double * restrict hvc, const double * restrict data, size_t n, const double * restrict ref);

/* Store the exclusive hypervolume contribution of each input point in hvc[],
   which is allocated by the caller.

   Return the total hypervolume. A negative value indicates insufficient
   memory. A value of zero indicates that no input point strictly dominates the
   reference point.
*/
double
hv_contributions(double * restrict hvc, double * restrict points, int d, int n,
                 const double * restrict ref, bool ignore_dominated)
{
    assert(hvc != NULL);
    ASSUME(d > 1 && d <= 32);
    ASSUME(n >= 0);
    dimension_t dim = (dimension_t) d;
    size_t size = (size_t) n;
    if (size == 0) return 0;
    if (size == 1) {
        hvc[0] = fpli_hv(points, dim, (int) size, ref);
        return hvc[0];
    }
    /* We cannot rely on the caller and the functions below will skip points
       that do not dominate the reference point.  */
    memset(hvc, 0, size * sizeof(double));

    double hv_total;
    if (dim == 2) {
        hv_total = ignore_dominated
            ? hvc2d_nondom(hvc, points, size, ref)
            : hvc2d(hvc, points, size, ref);
        DEBUG1(hvc_check(hv_total, hvc, points, dim, size, ref, ignore_dominated));
    } else if (dim == 3 && ignore_dominated) {
        hv_total = hvc3d(hvc, points, size, ref);
        DEBUG1(hvc_check(hv_total, hvc, points, dim, size, ref,
                         /* ignore_dominated = */true));
    } else {
        hv_total = fpli_hv(points, dim, (int) size, ref);
        if (ignore_dominated)
            hvc_1point_diffs_nondom(hvc, points, dim, size, ref, NULL, hv_total);
        else
            hvc_1point_diffs(hvc, points, dim, size, ref, NULL, hv_total);
    }
    return hv_total;
}
