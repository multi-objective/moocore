#include "whv.h"
#undef DEBUG
#define DEBUG 1
#include "common.h"
#include <float.h>

static int cmp_data_y_desc (const void *p1, const void *p2)
{
    const double *x1 = (const double *)p1;
    const double *x2 = (const double *)p2;

    return (x1[1] > x2[1]) ? -1 : (x1[1] < x2[1]) ? 1 :
        (x1[0] < x2[0]) ? -1 : (x1[0] > x2[0]) ? 1
        : 0;
}

static int cmp_rectangles_y_desc (const void *p1, const void *p2)
{
    const double *x1 = (const double *)p1;
    const double *x2 = (const double *)p2;

    // Order by upper (top-right) corner.
    return (x1[3] > x2[3]) ? -1 : (x1[3] < x2[3]) ? 1 :
        (x1[2] < x2[2]) ? -1 : (x1[2] > x2[2]) ? 1
        : 0;
}

static double *
whv_preprocess_rectangles(double * rectangles, int nrow,
                          const double *reference, int *new_nrow_p)
{
    const int ncol = 5;
    for (int k = 0; k < nrow; k++) {
        rectangles[k * ncol + 0] = MIN(rectangles[k * ncol + 0], reference[0]);
        rectangles[k * ncol + 1] = MIN(rectangles[k * ncol + 1], reference[1]);
        rectangles[k * ncol + 2] = MIN(rectangles[k * ncol + 2], reference[0]);
        rectangles[k * ncol + 3] = MIN(rectangles[k * ncol + 3], reference[1]);
    }
    int * skip = malloc(nrow * sizeof(int));
    int skip_nrow = 0;
    for (int k = 0; k < nrow; k++) {
        if (rectangles[k * ncol + 0] == rectangles[k * ncol + 2]
            || rectangles[k * ncol + 1] == rectangles[k * ncol + 3])
            skip[skip_nrow++] = k;
    }

    if (skip_nrow == 0) {
        free(skip);
        *new_nrow_p = nrow;
        return rectangles;
    }
    int new_nrow = nrow - skip_nrow;
    if (new_nrow == 0) {
        free(skip);
        *new_nrow_p = 0;
        return NULL;
    }
    double *dest = malloc(sizeof(double) * 5 * new_nrow);
    skip[skip_nrow] = nrow;
    int j = 0, k = 0;
    for (int s = 0; s <= skip_nrow; s++) {
        while (k < skip[s]) {
            for (int c = 0; c < 5; c++)
                dest[j * ncol + c] = rectangles[k * ncol + c];
            k++, j++;
        }
        k = skip[s] + 1;
    }
    free(skip);
    *new_nrow_p = new_nrow;
    return dest;
}

double
rect_weighted_hv2d(double *data, int n, double * rectangles,
                   int rectangles_nrow, const double * reference)
{
#define print_point(k, p, r, rect)                                             \
    DEBUG2_PRINT("%d: p[%lu] = (%16.15g, %16.15g)"                                 \
                 "\trectangle[%lu] = (%16.15g, %16.15g, %16.15g, %16.15g)\n",  \
                 __LINE__, (unsigned long) k, p[0], p[1], (unsigned long) r, rect[0], rect[1], rect[2], rect[3])

#define print_rect(r, rect)                                                    \
    DEBUG2_PRINT("%d: rectangle[%lu] = (%16.15g, %16.15g, %16.15g, %16.15g, %16.15g)\n", \
                 __LINE__, (unsigned long) r, rect[0], rect[1], rect[2], rect[3], rect[4])

// rectangles: Two points per row + color
// FIXME: Should we really allow color == 0
#define get_rectangle(ROW) do {                                                \
        rect = rectangles + (ROW) * (nobj * 2 + 1);                            \
        lower0= rect[0]; lower1= rect[1]; upper0= rect[2]; upper1= rect[3];    \
        color = rect[4];                                                       \
        print_rect(ROW, rect);                                                 \
        eaf_assert(lower0 < upper0);                                           \
        eaf_assert(lower1 < upper1);                                           \
        eaf_assert(color >= 0);                                                \
    } while(0)

#define next_point() do {                                                      \
        top = p[1];                                                            \
        pk++;                                                                  \
        if (pk >= n || top == last_top || p[0] >= last_right)                  \
            goto return_whv;                                                   \
        p += nobj;                                                             \
        print_point(pk, p, r, rect);                                           \
    } while(0)
    // We cannot use %zu for size_t because of MingW compiler.
    DEBUG2_PRINT("n = %lu\trectangles = %lu\n", (unsigned long)n, (unsigned long)rectangles_nrow);
    if (rectangles_nrow <= 0 || n <= 0) return 0;

    int old_rectangles_nrow = rectangles_nrow;
    rectangles =
        whv_preprocess_rectangles(rectangles, old_rectangles_nrow, reference, &rectangles_nrow);
    if (rectangles_nrow == 0)
        return 0;

    const int nobj = 2;
    qsort (data, n, 2 * sizeof(*data), &cmp_data_y_desc);
    qsort (rectangles, rectangles_nrow, (nobj * 2 + 1) * sizeof(*rectangles),
           &cmp_rectangles_y_desc);

    double whv = 0.0;
    int r = 0;
    const double *rect;
    // rectangles: Two points per row + color
    double lower0, lower1, upper0, upper1, color;
    get_rectangle(r);

    const double *p = data;
    int pk = 0;
    print_point(pk, p, r, rect);
    double top = upper1;
    // lowest_upper1;
    const double last_top = rectangles[rectangles_nrow * (nobj * 2 + 1) - 2];
    // largest upper0;
    double last_right = -DBL_MAX;
    for (r = 0; r < rectangles_nrow; r++) {
        last_right = MAX (last_right, rectangles[r * (nobj * 2 + 1) + 2]);
    }
    // Find first useful point.
    while (p[1] >= upper1) {
        // FIXME: We should delete repeated/dominated points when
        // sorting, skip for now.
        // Case #1: p is above the remaining rectangles: Next point
        next_point();
    }

    r = 0;
    while (true) {
        eaf_assert(p[1] < upper1);
        do {
            if (p[0] < upper0 && lower1 < top) {
                // Case #4: p strictly dominates u and not completed
                eaf_assert(p[0] < upper0 && p[1] < upper1);
                eaf_assert(top > lower1);
                eaf_assert(top > p[1]);
                // min(top, upper1) because the whole rect may be below top.
                whv += (upper0 - MAX(p[0],lower0)) * (MIN(top, upper1) - MAX(p[1], lower1)) * color;
                DEBUG2_PRINT("whv: %16.15g\n", whv);
            } // else case #3: not dominated or already completed, skip
            // Next rectangle
            r++;
            if (r >= rectangles_nrow) break; // goto next_point;
            get_rectangle(r);
        } while (p[1] < upper1);
        // Also restart rectangles
        r = 0;
        get_rectangle(r);
        do  {
            // FIXME: we need to loop because of repeated/dominated points when
            // sorting. We should delete them, skip for now.
            next_point();
        } while (top == p[1] && p[1] >= upper1);
    }
return_whv:
    if (old_rectangles_nrow != rectangles_nrow)
        free (rectangles);

    DEBUG2_PRINT("whv: %16.15g\n", whv);
    return whv;
}
