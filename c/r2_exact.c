#include <stdlib.h>
#include <limits.h>
#include <float.h>
#include "common.h"
#include "sort.h"

// Computes (two times) the utility of axis-parallel segment between y1, y2, and y2p.
static double _utility(const double y1, const double y2, const double y2p)
{
   if (y1 == 0) {
        return 0;
    }

    double w = y2 / (y1 + y2);
    double wp;
    if (y2p == DBL_MAX) {
        wp = 1;
    } else {
        wp = y2p / (y1 + y2p);
    }

    return y1 * (wp * wp - w * w);
}

double r2_exact(const double * restrict data, size_t n, dimension_t dim,
                const double * restrict ref)
{
    assert(dim == 2);
    if (unlikely(n == 0)) return -1;

    // p is sorted by f1 (primarily), then f2 (secondarily)
    const double **p = generate_sorted_doublep_2d(data, n);
    if (unlikely(!p)) return -1;

    size_t j = 0;
    // skip points not dominated by "ideal" ref point
    while (j < n && p[j][0] < ref[0]) {
        j++;
    }

    // no points to evaluate
    if (unlikely(j == n)) {
        if (p[j - 1][1] <= ref[1]) return 0.0; // ideal ref. is dominated.
        return DBL_MAX; // ideal ref. is nondominated --> worst possible value
    }

    double prev_y1 = p[j][0] - ref[0];
    double prev_y2 = p[j][1] - ref[1];

    if (prev_y2 < 0) {
        // ideal ref. is dominated.
        return 0.0;
    }

    // first element
    double r2_exact = _utility(prev_y1, prev_y2, DBL_MAX);
    // printf("y2 segment: %f, %f, MAX: %f\n", p[j][0] - ref[0], p[j][1] - ref[1], _utility(p[j][0] - ref[0], p[j][1] - ref[1], DBL_MAX));

    while (j < n - 1) {
        j++;
        double y1 = p[j][0] - ref[0];
        double y2 = p[j][1] - ref[1];

        // skip anything that's not dominated by ref
        if (y2 < 0) continue;

        // printf("%f\n", y1);
        // printf("%f\n", y2);

        if (y2 < prev_y2) {
            // pj not dominated by previous non-dominated pj
            r2_exact += _utility(prev_y2, prev_y1, y1) + _utility(y1, y2, prev_y2);

            // printf("y1 segment: %f, %f, %f: %f\n", prev_y2, prev_y1, y1, _utility(prev_y2, prev_y1, y1));
            // printf("y2 segment: %f, %f, %f: %f\n", y1, y2, prev_y2, _utility(y1, y2, prev_y2));

            prev_y1 = y1;
            prev_y2 = y2;
        }
    }

    // last element
    r2_exact += _utility(prev_y2, prev_y1, DBL_MAX);
    // printf("y1 segment: %f, %f, MAX: %f\n", prev_y2, prev_y1, _utility(prev_y2, prev_y1, DBL_MAX));

    // we omitted a 1/2 in the computation thus far:
    r2_exact = 0.5 * r2_exact;

    free(p);
    return r2_exact;
}
