#ifndef NONDOMINATED_H
#define NONDOMINATED_H

#include <string.h> // memcpy
#include "common.h"

static inline bool *
nondom_init (size_t size)
{
    bool * nondom = malloc (sizeof(bool) * size);
    for (size_t n = 0; n <  size; n++)
        nondom[n] = true;
    return nondom;
}

static inline const double *
force_agree_minimize (const double *points, int dim, int size,
                      const signed char *minmax, const signed char agree _no_warn_unused)
{
    eaf_assert(agree != AGREE_MINIMISE);

    bool no_copy = true;
    for (int d = 0; d < dim; d++) {
        if (minmax[d] > 0) {
            no_copy = false;
            break;
        }
    }
    if (no_copy)
        return points;

    double *pnew = malloc(dim * size * sizeof(double));
    memcpy(pnew, points, dim * size * sizeof(double));

    for (int d = 0; d < dim; d++) {
        eaf_assert(minmax[d] != 0);
        if (minmax[d] > 0)
            for (int k = 0; k < size; k++)
                pnew[k * dim + d] = -pnew[k * dim + d];
    }
    return pnew;
}

static inline int compare_x_asc_y_asc (const void *p1, const void *p2)
{
    const double x1 = **(const double **)p1;
    const double x2 = **(const double **)p2;
    const double y1 = *(*(const double **)p1 + 1);
    const double y2 = *(*(const double **)p2 + 1);
    return (x1 < x2) ? -1 : ((x1 > x2) ? 1 :
                             ((y1 < y2) ? -1 : ((y1 > y2) ? 1 : 0)));
}

/* When find_dominated_p == true, then stop as soon as one dominated point is
   found and return its position.

   When find_dominated_p == false, store which points are nondominated in nondom
   and return the number of nondominated points.

*/
static inline int
find_nondominated_set_2d_(const double *points, int size,
                          const signed char *minmax, const signed char agree,
                          bool *nondom, bool find_dominated_p, bool keep_weakly)
{
    if (size < 2)
        return size;

    // FIXME: We should do this before reaching this function and remove minmax
    // and agree from this function.
    const double *pp = force_agree_minimize (points, 2, size, minmax, agree);

    const double **p = malloc (size * sizeof(double *));
    for (int k = 0; k < size; k++)
        p[k] = pp + 2 * k;

    qsort(p, size, sizeof(*p), &compare_x_asc_y_asc);
    int n_dominated = 0, k = 0, j = 1;
    do {
        while (j < size && p[j][1] >= p[k][1]) {
            if (!keep_weakly || p[j][0] != p[k][0] || p[j][1] != p[k][1]) {
                if (find_dominated_p) {
                    // In this context, it means "position of the first dominated solution found".
                    n_dominated = (int)((p[j] - pp) / 2);
                    goto early_end;
                }
                nondom[j] = false;
                n_dominated++;
            }
            j++;
        }
        k = j;
        j++;
    } while (j < size);

    if (find_dominated_p) {
        // In this context, it means "no dominated solution found".
        n_dominated = -1;
        goto early_end;
    }

    bool * nondom_new = malloc(size * sizeof(bool));
    for (k = 0; k < size; k++)
        nondom_new[(p[k] - pp) / 2] = nondom[k];
    memcpy(nondom, nondom_new, size * sizeof(bool));
    free (nondom_new);
early_end:
    free(p);
    if (pp != points)
        free((void*)pp);
    return n_dominated;
}

/* When find_dominated_p == true, then stop as soon as one dominated point is
   found and return its position.

   When find_dominated_p == false, store which points are nondominated in nondom
   and return the number of nondominated points.

*/
static inline int
find_nondominated_set_ (const double *points, int dim, int size,
                        const signed char *minmax, const signed char agree,
                        bool *nondom, bool find_dominated_p, bool keep_weakly)
{
    if (dim == 2)
        return find_nondominated_set_2d_(points, size, minmax, agree,
                                       nondom, find_dominated_p, keep_weakly);
    int j, k, d;

    for (k = 0; k < size - 1; k++) {
        for (j = k + 1; j < size; j++) {

            if (!nondom[k]) break;
            if (!nondom[j]) continue;

            bool k_leq_j = true;
            bool j_leq_k = true;

            const double *pk = points + k * dim;
            const double *pj = points + j * dim;

            /* FIXME: As soon as j_leq_k and k_leq_j become false,
               neither k or j will be removed, so break.  */
            /* FIXME: Do not handle agree here, assume that objectives
               have been fixed already to agree on
               minimization/maximization.  */
            if (agree < 0) {
                for (d = 0; d < dim; d++) {
                    j_leq_k = j_leq_k && (pj[d] <= pk[d]);
                    k_leq_j = k_leq_j && (pk[d] <= pj[d]);
                }
            } else if (agree > 0) {
                for (d = 0; d < dim; d++) {
                    j_leq_k = j_leq_k && (pj[d] >= pk[d]);
                    k_leq_j = k_leq_j && (pk[d] >= pj[d]);
                }
            } else {
                for (d = 0; d < dim; d++) {
                    if (minmax[d] < 0) {
                        j_leq_k = j_leq_k && (pj[d] <= pk[d]);
                        k_leq_j = k_leq_j && (pk[d] <= pj[d]);
                    } else if (minmax[d] > 0) {
                        j_leq_k = j_leq_k && (pj[d] >= pk[d]);
                        k_leq_j = k_leq_j && (pk[d] >= pj[d]);
                    }
                }
            }

            // k is removed if it is weakly dominated by j (unless keep_weakly == FALSE).
            nondom[k] = !j_leq_k || (keep_weakly && k_leq_j);
            // j is removed if it is dominated by k.
            nondom[j] = (!k_leq_j || j_leq_k);

            eaf_assert(nondom[k] || nondom[j]); /* both cannot be removed.  */

            if (find_dominated_p && (!nondom[k] || !nondom[j])) {
                return nondom[k] ? j : k;
            }
        }
    }

    if (find_dominated_p) return -1;

    int new_size = 0;
    for (k = 0; k < size; k++)
        if (nondom[k]) new_size++;
    return new_size;
}

static inline int
find_dominated_point (const double *points, int dim, int size,
                      const signed char *minmax)
{
    bool *nondom = nondom_init (size);

    int pos = find_nondominated_set_ (points, dim, size, minmax,
                                      AGREE_NONE, nondom,
                                      /* find_dominated_p = */true,
                                      /* keep_weakly = */false);
    free (nondom);
    return pos;
}

static inline int
find_nondominated_set_agree (const double *points, int dim, int size,
                             const signed char *minmax, const signed char agree,
                             bool *nondom)
{
    return find_nondominated_set_ (points, dim, size, minmax, agree, nondom,
                                   /* find_dominated_p = */false,
                                   /* keep_weakly = */false);
}

static inline int
find_nondominated_set (const double *points, int dim, int size,
                       const signed char *minmax, bool *nondom)
{
    return find_nondominated_set_ (points, dim, size, minmax, AGREE_NONE, nondom,
                                   /* find_dominated_p = */false,
                                   /* keep_weakly = */false);
}

static inline int
find_weak_nondominated_set (const double *points, int dim, int size,
                            const signed char *minmax, bool *nondom)
{
    return find_nondominated_set_ (points, dim, size, minmax, AGREE_NONE, nondom,
                                   /* find_dominated_p = */false,
                                   /* keep_weakly = */true);
}

static inline int
get_nondominated_set (double **pareto_set_p,
                      const double *points, int dim, int size,
                      const signed char *minmax)
{
    bool *nondom  = nondom_init(size);
    int new_size = find_nondominated_set (points, dim, size, minmax, nondom);

    DEBUG2 (
        fprintf (stderr, "# size\tnondom\tdom\n");
        fprintf (stderr, "  %d\t%d\t%d\n",
                 size, new_size, size - new_size);
        );

    if (new_size > size) {/* This can't happen.  */
        fatal_error ("%s:%d: a bug happened: new_size > old_size!\n",
                     __FILE__, __LINE__);
    }

    double *pareto_set = malloc (sizeof (double) * new_size * dim);
    int n, k;
    // FIXME: We could use new_size to stop earlier.
    for (n = 0, k = 0; n < size; n++) {
        if (!nondom[n]) continue;
        memcpy(&pareto_set[dim * k], &points[dim * n], sizeof(points[0]) * dim);
        k++;
    }
    eaf_assert (k == new_size);
    free (nondom);
    *pareto_set_p = pareto_set;
    return new_size;
}

_no_warn_unused static bool *
is_nondominated (const double * data, int nobj, int npoint, const bool * maximise, bool keep_weakly)
{
    bool * nondom = nondom_init(npoint);
    const signed char * minmax = minmax_from_bool(nobj, maximise);
    find_nondominated_set_ (data, nobj, npoint, minmax, AGREE_NONE, nondom,
                            /* find_dominated_p = */false,
                            /* keep_weakly = */keep_weakly);
    free((void *)minmax);
    return nondom;
}

static inline void
agree_objectives (double *points, int dim, int size,
                  const signed char *minmax, const signed char agree)
{
    for (int d = 0; d < dim; d++)
        if ((agree > 0 && minmax[d] < 0)
            || (agree < 0 && minmax[d] > 0))
            for (int k = 0; k < size; k++)
                points[k * dim + d] = -(points[k * dim + d]);
}


static inline void
normalise (double *points, int dim, int size,
           const signed char *minmax, signed char agree,
           const double lower_range, const double upper_range,
           const double *lbound, const double *ubound)
{
    int k, d;
    const double range = upper_range - lower_range;

    double *diff = malloc (dim * sizeof(double));
    for (d = 0; d < dim; d++) {
        diff[d] = ubound[d] - lbound[d];
        if (diff[d] == 0.0) // FIXME: Should we use approximate equality?
            diff[d] = 1; // FIXME: Do we need to handle agree?
    }

    for (k = 0; k < size; k++) {
        double *p = points + k * dim;
        for (d = 0; d < dim; d++)
            if ((agree > 0 && minmax[d] < 0)
                || (agree < 0 && minmax[d] > 0))
                p[d] = lower_range + range * (ubound[d] + p[d]) / diff[d];
            else
                p[d] = lower_range + range * (p[d] - lbound[d]) / diff[d];
    }

    free (diff);
}

_no_warn_unused static void
agree_normalise (double *data, int nobj, int npoint,
                 const bool * maximise,
                 const double lower_range, const double upper_range,
                 const double *lbound, const double *ubound)
{
    const signed char * minmax = minmax_from_bool(nobj, maximise);
    // We have to make the objectives agree before normalisation.
    // FIXME: Do normalisation and agree in one step.
    agree_objectives (data, nobj, npoint, minmax, AGREE_MINIMISE);
    normalise(data, nobj, npoint, minmax, AGREE_MINIMISE,
              lower_range, upper_range, lbound, ubound);
    free ((void *)minmax);
}

int * pareto_rank (const double *points, int dim, int size);


#endif /* NONDOMINATED_H */
