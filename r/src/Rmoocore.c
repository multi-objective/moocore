#include "Rcommon.h"
#include "eaf.h"

#define DECLARE_CALL(RET_TYPE, NAME, ...)                                      \
    extern RET_TYPE NAME(__VA_ARGS__);
#include "init.h"
#undef DECLARE_CALL

static eaf_t **
compute_eaf_helper (SEXP DATA, int nobj, const int * cumsizes, int nruns,
                    const double *percentile, int nlevels)
{
    double *data = REAL(DATA);
    int *level = levels_from_percentiles(percentile, nlevels, nruns);

    DEBUG2(
        Rprintf ("attsurf ({(%f, %f", data[0], data[1]);
        for (int k = 2; k < nobj; k++) {
            Rprintf (", %f", data[k]);
        }
        Rprintf (")...}, %d, { %d", nobj, cumsizes[0]);
        for (int k = 1; k < nruns; k++) {
            Rprintf (", %d", cumsizes[k]);
        }
        Rprintf ("}, %d, { %d", nruns, level[0]);
        for (int k = 1; k < nlevels; k++) {
            Rprintf (", %d", level[k]);
        }
        Rprintf ("}, %d)\n", nlevels);
        );

    eaf_t **eaf = attsurf (data, nobj, cumsizes, nruns, level, nlevels);
    free (level);

    DEBUG2(
        Rprintf ("eaf computed\n");
        for (int k = 0; k < nlevels; k++) {
            Rprintf ("eaf[%d] = %lu\n", k, (unsigned long) eaf[k]->size);
        });

    return eaf;
}

SEXP
compute_eaf_C(SEXP DATA, SEXP CUMSIZES, SEXP PERCENTILE)
{
    const int nobj = Rf_nrows(DATA); /* We transpose the matrix before calling this function. */
    SEXP_2_INT_VECTOR(CUMSIZES, cumsizes, nruns);
    SEXP_2_DOUBLE_VECTOR_OR_NULL(PERCENTILE, percentile, nlevels);
    if (!percentile) nlevels = nruns;

    eaf_t **eaf = compute_eaf_helper(DATA, nobj, cumsizes, nruns, percentile, nlevels);
    int totalpoints = eaf_totalpoints (eaf, nlevels);

    SEXP mat;
    PROTECT(mat = Rf_allocMatrix(REALSXP, totalpoints, nobj + 1));
    eaf2matrix_R(REAL(mat), eaf, nobj, totalpoints, percentile, nlevels);
    eaf_free(eaf, nlevels);
    UNPROTECT(1);
    return mat;
}

SEXP
compute_eafdiff_C(SEXP DATA, SEXP CUMSIZES, SEXP INTERVALS)
{
    const int nobj = Rf_nrows(DATA); /* We transpose the matrix before calling this function. */
    SEXP_2_INT_VECTOR(CUMSIZES, cumsizes, nruns);
    SEXP_2_INT(INTERVALS, intervals);
    /* FIXME: This is similar to eafdiff_compute_matrix() */
    eaf_t **eaf = compute_eaf_helper(DATA, nobj, cumsizes, nruns, NULL, nruns);
    const int totalpoints = eaf_totalpoints (eaf, nruns);

    SEXP mat = PROTECT(Rf_allocMatrix(REALSXP, totalpoints, nobj + 1));
    double *rmat = REAL(mat);

    int pos = 0;
    for (int k = 0; k < nruns; k++) {
        int npoints = eaf[k]->size;
        DEBUG2(
            int totalsize = npoints * nobj;
            Rprintf ("totalpoints eaf[%d] = %d\n", k, totalsize)
            );
        // FIXME: Find the most efficient order of the loop.
        for (int i = 0; i < npoints; i++) {
            for (int j = 0; j < nobj; j++) {
                rmat[pos + j * totalpoints] = eaf[k]->data[j + i * nobj];
            }
            pos++;
        }
    }
    const int nsets1 = nruns / 2;
    pos += (nobj - 1) * totalpoints;
    for (int k = 0; k < nruns; k++) {
        int npoints = eaf[k]->size;
        for (int i = 0; i < npoints; i++) {
            rmat[pos] = eafdiff_percentile(eaf[k], i, nsets1, nruns, intervals);
            pos++;
        }
    }
    eaf_free(eaf, nruns);
    UNPROTECT (1);
    return mat;
}

static int polygon_len(const double *src, int nobj)
{
    const double *src_orig = src;
    while (*src != objective_MIN)
        src += nobj;
    src += nobj;
    return (src - src_orig) / nobj;
}

static int polygon_copy(double *dest, int start, int nrows, const double *src)
{
    int len = start;
    while (*src != objective_MIN) {
        dest[len] = *src;
        dest[len + nrows] = *(src + 1);
        len++;
        src += 2;
    }
    dest[len] = NA_REAL;
    dest[len + nrows] = NA_REAL;
    len++;
    return len - start;
}

SEXP
compute_eafdiff_rectangles_C(SEXP DATA, SEXP CUMSIZES, SEXP INTERVALS)
{
    int nprotected = 0;
    const int nobj = Rf_nrows(DATA); /* We transpose the matrix before calling this function. */
    SEXP_2_INT_VECTOR(CUMSIZES, cumsizes, nruns);
    SEXP_2_INT(INTERVALS, intervals);

    eaf_t **eaf = compute_eaf_helper(DATA, nobj, cumsizes, nruns, NULL, nruns);
    eaf_polygon_t * rects = eaf_compute_rectangles(eaf, nobj, nruns);
    eaf_free(eaf, nruns);

    const int division = nruns / 2;
    int nrow = vector_int_size(&rects->col);
    // Two points per row + color
    new_real_matrix (result, nrow, 2 * nobj + 1);
    const double * p_xy = vector_objective_begin(&rects->xy);
    int k;
    for (k = 0; k < nrow; ++k) {
        for (int i = 0; i < 2 * nobj; i++, p_xy++)
            result[k + nrow * i] = *p_xy;
    }
    vector_objective_dtor (&rects->xy);

    for (k = 0; k < nrow; ++k) {
        double color = vector_int_at(&rects->col, k);
        // Each color is within [0, nruns / 2] or [-nruns / 2, 0]
        result[k + nrow * 2 * nobj] = intervals * color / (double) division;
    }
    // FIXME: This may return duplicated rows, remove them.
    vector_int_dtor (&rects->col);
    free(rects);

    const char* const colnames[] = {"xmin", "ymin", "xmax", "ymax", "diff"};
    set_colnames(Rexp(result), colnames, 5);

    UNPROTECT (nprotected);
    return Rexp(result);
}

SEXP
compute_eafdiff_polygon_C(SEXP DATA, SEXP CUMSIZES, SEXP INTERVALS)
{
    int nprotected = 0;
    const int nobj = Rf_nrows(DATA); /* We transpose the matrix before calling this function. */
    SEXP_2_INT_VECTOR(CUMSIZES, cumsizes, nruns);
    SEXP_2_INT(INTERVALS, intervals);

    eaf_t **eaf = compute_eaf_helper(DATA, nobj, cumsizes, nruns, NULL, nruns);
    eaf_polygon_t *p = eaf_compute_area(eaf, nobj, nruns);
    eaf_free(eaf, nruns);

    const int division = nruns / 2;
    const int ncol = vector_int_size(&p->col);

    DEBUG2(Rprintf ("ncol: %d\n", ncol));

    int left_ncol = 0, right_ncol = 0;
    int left_len = 0, right_len = 0;

    /* First compute the adjusted colors, and how much space we need on each
       side. */
    const double *p_xy = vector_objective_begin(&p->xy);
    for (int k = 0; k < ncol; k++) {
        // Truncate colors to interval
        int color = vector_int_at(&p->col, k) * intervals / (double) division;
        int len = polygon_len (p_xy, nobj);
        p_xy += len * nobj;
        DEBUG2(Rprintf ("color: %d, len = %d\n", color, len));
        // First interval (-1, 1) is white
        if (color >= 1) {
            left_len += len;
            left_ncol++;
        } else if (color <= 1) {
            right_len += len;
            right_ncol++;
        }
        vector_int_set(&p->col, k, color);
    }

    DEBUG2(Rprintf ("left_len: %d, right_len: %d, left_ncol: %d, right_ncol: %d\n",
                    left_len, right_len, left_ncol, right_ncol));

    /* Now assign points to each side. */
    new_real_vector(left_col, left_ncol);
    new_real_vector(right_col, right_ncol);

    const int left_npoints = left_len;
    new_real_matrix (left, left_npoints, nobj);

    const int right_npoints = right_len;
    new_real_matrix (right, right_npoints, nobj);

    p_xy = vector_objective_begin(&p->xy);
    left_len = right_len = 0;
    left_ncol = right_ncol = 0;
    for (int k = 0; k < ncol; k++) {
        int len;
        int color = vector_int_at(&p->col, k);
        if (color >= 1) {
            len = polygon_copy (left, left_len, left_npoints, p_xy);
            left_len += len;
            left_col[left_ncol++] = color + 1;
        } else if (color <= 1) {
            len = polygon_copy (right, right_len, right_npoints, p_xy);
            right_len += len;
            right_col[right_ncol++] = 1 - color;
        } else {
            len = polygon_len (p_xy, nobj);
        }
        p_xy += nobj * len;
    }
    vector_int_dtor (&p->col);
    vector_objective_dtor (&p->xy);
    free(p);

    set_attribute(left, "col", left_col);
    set_attribute(right, "col", right_col);

    new_list(poly, 2);
    list_push_back (poly, left);
    list_push_back (poly, right);

    new_string_vector (names, list_len (poly));
    string_vector_push_back (names, "left");
    string_vector_push_back (names, "right");
    set_names (poly, names);

    UNPROTECT (nprotected);
    return Rexp(poly);
}

SEXP
R_read_datasets(SEXP FILENAME)
{
    SEXP_2_STRING(FILENAME, filename);
    /* Rprintf ("filename: %s\n", filename); */
    objective_t * data = NULL;
    int * cumsizes = NULL;
    int nobj = 0, nruns = 0;
    read_objective_t_data (filename, &data, &nobj, &cumsizes, &nruns);

    const int ntotal = cumsizes[nruns - 1];

    /* FIXME: Is this the fastest way to transfer a matrix from C to R ? */
    SEXP DATA;
    PROTECT(DATA = Rf_allocMatrix(REALSXP, ntotal, nobj + 1));
    double *rdata = REAL(DATA);
    matrix_transpose_double (rdata, data, ntotal, nobj);

    int k, j;
    size_t pos = ntotal * nobj;
    for (k = 0, j = 0; k < ntotal; k++, pos++) {
        if (k == cumsizes[j]) j++;
        rdata[pos] = j + 1;
    }
    free(data);
    free(cumsizes);
    UNPROTECT(1);
    return DATA;
}

#include "nondominated.h"

void
normalise_C(SEXP DATA, SEXP RANGE, SEXP LBOUND, SEXP UBOUND, SEXP MAXIMISE)
{
    int nprotected = 0;
    /* We transpose the matrix before calling this function. */
    SEXP_2_DOUBLE_MATRIX(DATA, data, nobj, npoint);
    SEXP_2_DOUBLE_VECTOR(RANGE, range, range_len);
    SEXP_2_DOUBLE_VECTOR(LBOUND, lbound, lbound_len);
    SEXP_2_DOUBLE_VECTOR(UBOUND, ubound, ubound_len);
    SEXP_2_LOGICAL_BOOL_VECTOR(MAXIMISE, maximise, maximise_len);

    assert(nobj == lbound_len);
    assert(nobj == ubound_len);
    assert (nobj == maximise_len);
    assert (range_len == 2);

    agree_normalise(data, nobj, npoint, maximise, range[0], range[1],
                    lbound, ubound);
    free (maximise);
    UNPROTECT(nprotected);
}

SEXP
is_nondominated_C(SEXP DATA, SEXP MAXIMISE, SEXP KEEP_WEAKLY)
{
    int nprotected = 0;
    /* We transpose the matrix before calling this function. */
    SEXP_2_DOUBLE_MATRIX(DATA, data, nobj, npoint);
    SEXP_2_LOGICAL_BOOL_VECTOR(MAXIMISE, maximise, maximise_len);
    SEXP_2_LOGICAL(KEEP_WEAKLY, keep_weakly);
    assert (nobj == maximise_len);

    bool * bool_is_nondom = is_nondominated(data, nobj, npoint, maximise, keep_weakly);
    free (maximise);

    new_logical_vector (is_nondom, npoint);
    bool_2_logical_vector(is_nondom, bool_is_nondom, npoint);
    free (bool_is_nondom);
    UNPROTECT(nprotected);
    return Rexp(is_nondom);
}

SEXP
pareto_ranking_C(SEXP DATA)
{
    int nprotected = 0;
    /* We transpose the matrix before calling this function. */
    SEXP_2_DOUBLE_MATRIX(DATA, data, nobj, npoint);

    /* FIXME: How to assign directly? */
    new_int_vector (rank, npoint);
    int * rank2 = pareto_rank(data, nobj, npoint);
    for (int i = 0; i < npoint; i++) {
        rank[i] = rank2[i];
    }
    free (rank2);
    UNPROTECT(nprotected);
    return Rexp(rank);
}


#include "hv.h"

SEXP
hypervolume_C(SEXP DATA, SEXP REFERENCE)
{
    int nprotected = 0;
    /* We transpose the matrix before calling this function. */
    SEXP_2_DOUBLE_MATRIX(DATA, data, nobj, npoint);
    SEXP_2_DOUBLE_VECTOR(REFERENCE, reference, reference_len);
    assert (nobj == reference_len);
    new_real_vector(hv, 1);
    hv[0] = fpli_hv(data, nobj, npoint, reference);
    UNPROTECT (nprotected);
    return Rexp(hv);
}

SEXP
hv_contributions_C(SEXP DATA, SEXP REFERENCE)
{
    int nprotected = 0;
    SEXP_2_DOUBLE_MATRIX(DATA, data, nobj, npoint);
    SEXP_2_DOUBLE_VECTOR(REFERENCE, reference, reference_len);
    assert (nobj == reference_len);
    new_real_vector(hv, npoint);
    hv_contributions(hv, data, nobj, npoint, reference);
    UNPROTECT (nprotected);
    return Rexp(hv);
}

#include "whv.h"

SEXP
rect_weighted_hv2d_C(SEXP DATA, SEXP RECTANGLES)
{
    int nprotected = 0;
    /* We transpose the matrix before calling this function. */
    SEXP_2_DOUBLE_MATRIX(DATA, data, nobj, npoint);
    SEXP_2_DOUBLE_MATRIX(RECTANGLES, rectangles, unused, rectangles_nrow);
    new_real_vector(hv, 1);
    hv[0] = rect_weighted_hv2d(data, npoint, rectangles, rectangles_nrow);
    UNPROTECT (nprotected);
    return Rexp(hv);
}

#include "whv_hype.h"

hype_sample_dist *
Sexp_to_dist(SEXP DIST, SEXP SEED)
{
    int nprotected = 0;
    hype_sample_dist * dist = NULL;
    SEXP_2_INT(SEED, seed);

    const char * dist_type = CHAR(STRING_ELT(VECTOR_ELT(DIST, 0), 0));
    if (0 == strcmp(dist_type, "uniform")) {
        dist = hype_dist_unif_new(seed);
    } else if (0 == strcmp(dist_type, "exponential")) {
        const double * mu = REAL(VECTOR_ELT(DIST, 1));
        dist = hype_dist_exp_new(mu[0], seed);
    } else if (0 == strcmp(dist_type, "point")) {
        const double * mu = REAL(VECTOR_ELT(DIST, 1));
        dist = hype_dist_gaussian_new(mu, seed);
    } else {
        Rf_error("unknown dist_type: %s", dist_type);
    }
    UNPROTECT (nprotected);
    return dist;
}

SEXP
whv_hype_C(SEXP DATA, SEXP IDEAL, SEXP REFERENCE, SEXP DIST, SEXP SEED, SEXP NSAMPLES)
{
    int nprotected = 0;
    SEXP_2_DOUBLE_MATRIX(DATA, data, nobj, npoints);
    SEXP_2_INT(NSAMPLES, nsamples);
    SEXP_2_DOUBLE_VECTOR(REFERENCE, reference, reference_len);
    SEXP_2_DOUBLE_VECTOR(IDEAL, ideal, ideal_len);
    eaf_assert(reference_len == ideal_len);
    eaf_assert(reference_len == 2);
    new_real_vector(hv, 1);
    hype_sample_dist * dist = Sexp_to_dist(DIST, SEED);
    if (!dist) {
        Rf_error("Sexp_to_dist failed to create dist");
    } else {
        hv[0] = whv_hype_estimate(data, npoints, ideal, reference, dist, nsamples);
        hype_dist_free(dist);
    }
    UNPROTECT (nprotected);
    return Rexp(hv);
}

#include "epsilon.h"
#include "igd.h"
#include "nondominated.h"

enum unary_metric_t {
    EPSILON_ADD,
    EPSILON_MUL,
    INV_GD,
    INV_GDPLUS,
    AVG_HAUSDORFF
};

static inline SEXP
unary_metric_ref(SEXP DATA, SEXP REFERENCE, SEXP MAXIMISE,
                 enum unary_metric_t metric, SEXP EXTRA)
{
    int nprotected = 0;
    /* We transpose the matrix before calling this function. */
    SEXP_2_DOUBLE_MATRIX(DATA, data, nobj, npoint);
    double *ref = REAL(REFERENCE);
    /* We transpose the matrix before calling this function. */
    int ref_size = Rf_ncols(REFERENCE);
    SEXP_2_LOGICAL_BOOL_VECTOR(MAXIMISE, maximise, maximise_len);
    assert (nobj == maximise_len);

    new_real_vector(value, 1);
    switch (metric) {
      case EPSILON_ADD:
          value[0] = epsilon_additive (data, nobj, npoint, ref, ref_size, maximise);
          break;
      case EPSILON_MUL:
          value[0] = epsilon_mult (data, nobj, npoint, ref, ref_size, maximise);
          break;
      case INV_GD:
          value[0] = IGD (data, nobj, npoint, ref, ref_size, maximise);
          break;
      case INV_GDPLUS:
          value[0] = IGD_plus (data, nobj, npoint, ref, ref_size, maximise);
          break;
      case AVG_HAUSDORFF: {
          SEXP_2_INT(EXTRA, p);
          value[0] = avg_Hausdorff_dist (data, nobj, npoint, ref, ref_size, maximise, p);
          break;
      }
      default:
          Rf_error("unknown unary metric");
    }

    free (maximise);
    UNPROTECT (nprotected);
    return Rexp(value);
}

SEXP
epsilon_mul_C(SEXP DATA, SEXP REFERENCE, SEXP MAXIMISE)
{
    return(unary_metric_ref(DATA, REFERENCE, MAXIMISE, EPSILON_MUL, R_NilValue));
}

SEXP
epsilon_add_C(SEXP DATA, SEXP REFERENCE, SEXP MAXIMISE)
{
    return(unary_metric_ref(DATA, REFERENCE, MAXIMISE, EPSILON_ADD, R_NilValue));
}

SEXP
igd_C(SEXP DATA, SEXP REFERENCE, SEXP MAXIMISE)
{
    return(unary_metric_ref(DATA, REFERENCE, MAXIMISE, INV_GD, R_NilValue));
}

SEXP
igd_plus_C(SEXP DATA, SEXP REFERENCE, SEXP MAXIMISE)
{
    return(unary_metric_ref(DATA, REFERENCE, MAXIMISE, INV_GDPLUS, R_NilValue));
}

SEXP
avg_hausdorff_dist_C(SEXP DATA, SEXP REFERENCE, SEXP MAXIMISE, SEXP P)
{
    return(unary_metric_ref(DATA, REFERENCE, MAXIMISE, AVG_HAUSDORFF, P));
}
