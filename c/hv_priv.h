
#include "sort.h"

static inline const double **
new_sorted_scratch(const double * restrict data, size_t * n_p, const dimension_t d,
                   const double * restrict ref, cmp_fun_t cmp)
{
    size_t n = *n_p;
    const double **scratchd = malloc(n * sizeof(*scratchd));
    size_t i, j;
    for (i = 0, j = 0; j < n; j++) {
        /* Filters those points that do not strictly dominate the reference
           point.  This is needed to ensure that the points left are only those
           that are needed to calculate the hypervolume. */
        if (unlikely(strongly_dominates(data + j * d, ref, d))) {
            scratchd[i] = data + j * d;
            i++;
        }
    }
    if (i > 1)
        qsort(scratchd, i, sizeof(*scratchd), cmp);

    *n_p = i; // Update number of points.
    return scratchd;
}
