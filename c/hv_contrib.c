#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include "common.h"
#include "hv.h"

/* Given a list of points, compute the hypervolume of each set that can be
   obtained by removing just one point.

   It does not actually compute the contribution but HV_total - HV_i,
   where HV_total is the total HV and HV_i is the contribution of the
   point, that is, it actually computes the HV minus the point i.
*/
static double *
hv_1point_diffs (double *hvc, double *points, int dim, int size, const double * ref,
                 const bool * uev)
{
    bool keep_uevs = uev != NULL;

    if (hvc == NULL)
        hvc = malloc (sizeof(double) * size);

    double * tmp = malloc (sizeof(double) * dim);
    for (int i = 0; i < size; i++) {
        memcpy (tmp, points + i * dim, sizeof(double) * dim);
        memcpy (points + i * dim, ref, sizeof(double) * dim);
        hvc[i] = (keep_uevs && uev[i])
            ? 0.0
            : fpli_hv(points, dim, size, ref);
        memcpy (points + i * dim, tmp, sizeof(double) * dim);
    }
    free(tmp);
    return hvc;
}

void
hv_contributions (double *hvc, double *points, int dim, int size, const double * ref)
{
    const double tolerance = sqrt(DBL_EPSILON);
    double hv_total = fpli_hv(points, dim, size, ref);
    hv_1point_diffs(hvc, points, dim, size, ref, NULL);
    for (int i = 0; i < size; i++) {
        hvc[i] = hv_total - hvc[i];
        // Handle very small values.
        hvc[i] = (fabs(hvc[i]) >= tolerance) ? hvc[i] : 0.0;
        eaf_assert(hvc[i] >= 0);
    }
}
