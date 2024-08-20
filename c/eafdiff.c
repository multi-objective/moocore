#include "eaf.h"

/* FIXME: Rmoocore.R contains another version of this function. */
/* FIXME: data cannot be const because eaf_compute_rectangles will sort it. */
double *
eafdiff_compute_rectangles(int *eaf_npoints, double * data, int nobj,
                           const int *cumsizes, int nruns, int intervals)
{
    /* This returns all levels. attsurf() should probably handle this case. */
    int * level = levels_from_percentiles(NULL, nruns, nruns);
    eaf_t **eaf = attsurf(data, nobj, cumsizes, nruns, level, nruns);
    free (level);

    eaf_polygon_t * rects = eaf_compute_rectangles(eaf, nobj, nruns);
    eaf_free(eaf, nruns);

    const int division = nruns / 2;
    int nrow = (int) vector_int_size(&rects->col);
    // Two points per row + color
    const int ncol = 2 * nobj + 1;
    double *result = malloc(sizeof(double) * nrow * ncol);
    const double * p_xy = vector_objective_begin(&rects->xy);
    for (int k = 0; k < nrow; k++) {
        for (int i = 0; i < ncol - 1; i++, p_xy++)
            result[k * ncol + i] = *p_xy;
        double color = vector_int_at(&rects->col, k);
        // Each color is within [0, nruns / 2] or [-nruns / 2, 0]
        result[k * ncol + ncol - 1] = intervals * color / (double) division;
    }
    vector_objective_dtor (&rects->xy);
    vector_int_dtor (&rects->col);
    free(rects);
    *eaf_npoints = nrow;
    // FIXME: This may return duplicated rows, remove them.
    return result;
}

double *
eafdiff_compute_matrix(int *eaf_npoints, double * data, int nobj,
                       const int *cumsizes, int nruns, int intervals)
{
    // FIXME: This assumes that half of the runs come from each side of the
    // difference but we could make this a parameter.
    const int nsets1 = nruns / 2;
    /* This returns all levels. attsurf() should probably handle this case. */
    int * level = levels_from_percentiles(NULL, nruns, nruns);
    eaf_t **eaf = attsurf(data, nobj, cumsizes, nruns, level, nruns);
    free (level);

    const int nrow = eaf_totalpoints (eaf, nruns);
    const int ncol = nobj + 1;
    double *result = malloc(sizeof(double) * nrow * ncol);
    int pos = 0;
    for (int k = 0; k < nruns; k++) {
        int npoints = (int) eaf[k]->size;
        // FIXME: Find the most efficient order of the loop.
        for (int i = 0; i < npoints; i++) {
            for (int j = 0; j < nobj; j++) {
                result[pos * ncol + j] = eaf[k]->data[i * nobj + j];
            }
            result[pos * ncol + nobj] = eafdiff_percentile(eaf[k], i, nsets1, nruns, intervals);
            pos++;
        }
    }
    eaf_free(eaf, nruns);
    *eaf_npoints = nrow;
    return result;
}
