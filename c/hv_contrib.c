#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include "common.h"
#include "hv.h"
#include "sort.h"

/* Given a list of points, compute the exclusive hypervolume contribution of
   point using the naive algorithm. That is, it removes one point at a time and
   calculates hv_total - hv_i, where hv_i is the hypervolume of the set minus
   the point i.

   With hv_total=0, it computes the negated hypervolume of each subset minus
   one point.
*/
static void
hv_1point_diffs (double *hvc, double *points, dimension_t dim, size_t size, const double * ref,
                 const bool * uev, const double hv_total)
{
    bool keep_uevs = uev != NULL;
    const double tolerance = sqrt(DBL_EPSILON);
    double * tmp = MOOCORE_MALLOC(dim, double);
    for (size_t i = 0; i < size; i++) {
        if (unlikely(keep_uevs && uev[i])) {
            hvc[i] = hv_total;
        } else if (unlikely(!strongly_dominates(points + i * dim, ref, dim))) {
            hvc[i] = 0.0;
        } else {
            memcpy(tmp, points + i * dim, sizeof(double) * dim);
            memcpy(points + i * dim, ref, sizeof(double) * dim);
            hvc[i] = hv_total - fpli_hv(points, dim, (int) size, ref);
            // Handle very small values.
            hvc[i] = fabs(hvc[i]) >= tolerance ? hvc[i] : 0.0;
            assert(hvc[i] >= 0);
            memcpy(points + i * dim, tmp, sizeof(double) * dim);
        }
    }
    free(tmp);
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
    ASSUME(n > 1);
    dimension_t dim = (dimension_t) d;
    size_t size = (size_t) n;
    double hv_total = fpli_hv(points, dim, (int) size, ref);
    hv_1point_diffs(hvc, points, dim, size, ref, NULL, hv_total);
    return hv_total;
}
